#!/usr/bin/env python3
import argparse
import concurrent.futures
import hashlib
import http.client
import json
import os
from pathlib import Path
import re
import signal
import socket
import subprocess
import tempfile
import time
import urllib.parse
import zipfile


p = argparse.ArgumentParser()
p.add_argument('--php', required=True)
p.add_argument('--verifier', required=True)
p.add_argument('--reference-panel', type=Path)
a = p.parse_args()
root = Path(__file__).resolve().parents[1]
if a.reference_panel:
    with zipfile.ZipFile(a.reference_panel) as original:
        for name in ('admin.php', 'get-key.php'):
            marker = b'?>\n<!doctype html>'
            assert original.read(name).split(marker, 1)[1] == (root / 'server/license' / name).read_bytes().split(marker, 1)[1], name
        assert original.read('index.php') == (root / 'server/license/index.php').read_bytes()

with tempfile.TemporaryDirectory(prefix='shai-current-panel-') as folder:
    temp = Path(folder)
    for source in (root / 'server/license').glob('*.php'):
        if source.name != 'config.example.php':
            (temp / source.name).write_bytes(source.read_bytes())
    (temp / 'data').mkdir()
    (temp / 'sessions').mkdir()
    private = temp / 'private.pem'
    public = temp / 'public.pem'
    store = temp / 'data/keys.json'
    subprocess.run(['openssl', 'genpkey', '-algorithm', 'RSA', '-pkeyopt', 'rsa_keygen_bits:3072', '-out', str(private)], check=True, stderr=subprocess.DEVNULL)
    public.write_bytes(subprocess.check_output(['openssl', 'pkey', '-in', str(private), '-pubout']))
    der = subprocess.check_output(['openssl', 'pkey', '-in', str(private), '-pubout', '-outform', 'DER'])
    kid = hashlib.sha256(der).hexdigest()
    modulus = subprocess.check_output(['openssl', 'rsa', '-pubin', '-in', str(public), '-modulus', '-noout']).decode().strip().split('=', 1)[1]
    password_hash = subprocess.check_output([a.php, '-n', '-r', "echo password_hash('fixture-password', PASSWORD_DEFAULT);"]).decode()
    config = dict(store=str(store), private_key=str(private), build=1, admin_user='fixture-admin', admin_hash=password_hash, app_modulus=modulus)
    php_config = '<?php return ' + '[' + ','.join(repr(k) + '=>' + repr(v) for k, v in config.items()) + '];'
    (temp / 'config.php').write_text(php_config)
    store.write_text(json.dumps({'legacy-premium': dict(expires=0, devices=[], seats=0, revoked=0)}))
    (temp / 'vendor-fixture.php').write_text('''<?php
foreach (['CURLOPT_POST','CURLOPT_POSTFIELDS','CURLOPT_RETURNTRANSFER','CURLOPT_CONNECTTIMEOUT','CURLOPT_TIMEOUT','CURLOPT_FOLLOWLOCATION','CURLOPT_SSL_VERIFYPEER','CURLOPT_SSL_VERIFYHOST','CURLINFO_RESPONSE_CODE'] as $i => $name) define($name, $i + 1);
function curl_init($url) { return (object)['url'=>$url,'status'=>200,'options'=>[]]; }
function curl_setopt_array($curl, $options) { $curl->options=$options; return true; }
function curl_exec($curl) {
    $url=parse_url($curl->url); parse_str($url['query'] ?? '', $q);
    if (($url['scheme'] ?? '')!=='https' || ($url['host'] ?? '')!=='publisher.linkvertise.com' || ($url['path'] ?? '')!=='/api/v1/anti_bypassing' || ($q['token'] ?? '')!==str_repeat('a',64) || ($curl->options[CURLOPT_SSL_VERIFYPEER] ?? false)!==true || ($curl->options[CURLOPT_SSL_VERIFYHOST] ?? 0)!==2 || ($curl->options[CURLOPT_FOLLOWLOCATION] ?? true)!==false || ($curl->options[CURLOPT_POST] ?? false)!==true) throw new RuntimeException('Invalid vendor request');
    if (($q['hash'] ?? '')===str_repeat('e',64)) { file_put_contents(__DIR__.'/vendor-entered','1'); usleep(1500000); }
    if (($q['hash'] ?? '')===str_repeat('f',64)) $curl->status=503;
    return ($q['hash'] ?? '')===str_repeat('0',64) ? 'FALSE' : 'TRUE';
}
function curl_getinfo($curl, $kind) { return $curl->status; }
function curl_close($curl) {}
''')
    with socket.socket() as sock:
        sock.bind(('127.0.0.1', 0))
        port = sock.getsockname()[1]
    env = os.environ.copy()
    env['PHP_CLI_SERVER_WORKERS'] = '4'
    log = (temp / 'server.log').open('wb')
    server = subprocess.Popen([a.php, '-n', '-d', 'session.save_path=' + str(temp / 'sessions'), '-d', 'auto_prepend_file=' + str(temp / 'vendor-fixture.php'), '-S', f'127.0.0.1:{port}', '-t', str(temp)], env=env, stdout=log, stderr=log, start_new_session=True)

    class Browser:
        def __init__(self):
            self.cookies = {}

        def request(self, path, values=None, follow=False):
            connection = http.client.HTTPConnection('127.0.0.1', port, timeout=8)
            try:
                connection.request('POST' if values is not None else 'GET', path, urllib.parse.urlencode(values or {}), {'Content-Type': 'application/x-www-form-urlencoded', 'Cookie': '; '.join(k + '=' + v for k, v in self.cookies.items())})
                response = connection.getresponse()
                data = response.read().decode()
                location = response.getheader('Location')
                status = response.status
                for name, value in response.getheaders():
                    if name.lower() == 'set-cookie':
                        key, val = value.split(';', 1)[0].split('=', 1)
                        self.cookies[key] = val
            finally:
                connection.close()
            if follow and status in (302, 303):
                assert location and not urllib.parse.urlparse(location).netloc
                return self.request('/' + location.lstrip('/'))
            return status, data, location

    def csrf(html):
        found = re.search(r'name="csrf" value="([a-f0-9]+)"', html)
        assert found, 'Missing CSRF form'
        return found.group(1)

    def progress(browser, expected):
        code, html, _ = browser.request('/get-key.php')
        assert code == 200 and f'aria-label="{expected} of 3 checkpoints complete"' in html
        return html

    def go(browser, step):
        html = progress(browser, step - 1)
        code, _, location = browser.request('/get-key.php', dict(action='go', csrf=csrf(html), step=step))
        assert code == 303 and location == f'https://linkvertise.com/12345/checkpoint-{step}'

    def callback(browser, step, proof):
        return browser.request('/get-key.php?' + urllib.parse.urlencode(dict(checkpoint=step, hash=proof)), follow=True)

    def grant(key, capabilities, tier):
        nonce = os.urandom(32).hex()
        serial = 'fixture-device-123'
        values = dict(protocol='2', game='CODMGR', build='1', user_key=key, serial=serial, nonce=nonce)
        code, raw, _ = Browser().request('/connect.php', values)
        assert code == 200
        result = json.loads(raw)['data']
        parts = result['payload'].split('|')
        assert parts[9] == str(capabilities) and result['tier'] == tier
        verified = subprocess.run([a.verifier, str(public), kid, key, serial, nonce, str(int(time.time()))], input=(result['payload'] + '\n' + result['sig'] + '\n').encode(), stdout=subprocess.DEVNULL)
        assert verified.returncode == 0
        return code

    try:
        for attempt in range(100):
            try:
                Browser().request('/index.php')
                break
            except OSError:
                time.sleep(.02)
        else:
            raise AssertionError('PHP panel server failed to start')
        admin = Browser()
        code, html, _ = admin.request('/admin.php')
        assert code == 200 and 'Sign in' in html
        token = csrf(html)
        assert admin.request('/admin.php', dict(action='login', csrf='bad', user='fixture-admin', pass_='unused'))[0] == 400
        code, html, _ = admin.request('/admin.php', {'action': 'login', 'csrf': token, 'user': 'fixture-admin', 'pass': 'fixture-password'}, follow=True)
        assert code == 200 and 'Sign out' in html
        token = csrf(html)
        assert admin.request('/admin.php', dict(action='create', csrf='bad', tier='premium'))[0] == 400
        for tier, count, days, seats in [('free', 3, 1, 1), ('premium', 2, 0, 0)]:
            code, html, _ = admin.request('/admin.php', dict(action='create', csrf=token, key='', count=count, tier=tier, days=days, seats=seats), follow=True)
            assert code == 200 and f'Created {count} {tier.title()} keys.' in html
            current = json.loads(store.read_text())
            records = {key: row for key, row in current.items() if row.get('tier') == tier}
            assert len(records) == count
            for key, row in records.items():
                assert re.fullmatch(r'ETHNIR-(FREE|PREM)-[A-F0-9]{32}', key)
                assert row['seats'] == seats
                assert (row['expires'] == 0) == (days == 0)
        premium = next(k for k, v in current.items() if v.get('tier') == 'premium')
        free = next(k for k, v in current.items() if v.get('tier') == 'free')
        grant(premium, 7, 'premium')
        grant(free, 3, 'free')
        grant('legacy-premium', 7, 'premium')
        settings = dict(action='save_giveaway', csrf=token, giveaway_enabled='on', giveaway_token='a' * 64)
        settings.update({f'giveaway_link_{i}': f'https://linkvertise.com/12345/checkpoint-{i}' for i in range(1, 4)})
        bad = dict(settings, giveaway_link_1='https://attacker@linkvertise.com/12345/one')
        code, html, _ = admin.request('/admin.php', bad, follow=True)
        assert code == 200 and 'three distinct HTTPS' in html
        code, html, _ = admin.request('/admin.php', settings, follow=True)
        assert code == 200 and '10-hour giveaway enabled.' in html
        public_browser = Browser()
        html = progress(public_browser, 0)
        assert public_browser.request('/get-key.php', dict(action='go', csrf='bad', step=1))[0] == 400
        code, html, _ = callback(public_browser, 3, '3' * 64)
        assert code == 200 and 'Unexpected checkpoint' in html
        assert public_browser.request('/get-key.php', dict(action='go', csrf=csrf(html), step=3))[2] == 'get-key.php'
        go(public_browser, 1)
        assert 'did not verify' in callback(public_browser, 1, '0' * 64)[1]
        progress(public_browser, 0)
        go(public_browser, 1)
        assert 'did not verify' in callback(public_browser, 1, 'f' * 64)[1]
        progress(public_browser, 0)
        go(public_browser, 1)
        assert 'Checkpoint 1 verified' in callback(public_browser, 1, '1' * 64)[1]
        go(public_browser, 2)
        assert 'already used' in callback(public_browser, 2, '1' * 64)[1]
        progress(public_browser, 1)
        assert 'Checkpoint 2 verified' in callback(public_browser, 2, '2' * 64)[1]
        go(public_browser, 3)
        code, html, _ = callback(public_browser, 3, '3' * 64)
        assert code == 200 and 'Your Free key is ready' in html
        current = json.loads(store.read_text())
        issued = [(k, v) for k, v in current.items() if v.get('origin') == 'giveaway']
        assert len(issued) == 1
        key, row = issued[0]
        assert row['expires'] - row['created'] == 36000 and row['seats'] == 1 and row['tier'] == 'free'
        assert row['claim_proofs'] == [hashlib.sha256((str(i) * 64).encode()).hexdigest() for i in range(1, 4)]
        assert key in html
        grant(key, 3, 'free')
        callback(public_browser, 3, '3' * 64)
        assert sum(v.get('origin') == 'giveaway' for v in json.loads(store.read_text()).values()) == 1
        session = temp / 'sessions' / ('sess_' + public_browser.cookies['ethnirfree'])
        session.write_bytes(re.sub(rb's:7:"started";i:\d+;', b's:7:"started";i:1;', session.read_bytes()))
        assert key in public_browser.request('/get-key.php')[1]
        replay = Browser()
        for step in range(1, 4):
            go(replay, step)
            code, html, _ = callback(replay, step, str(step) * 64)
        assert code == 200 and 'already claimed' in html
        progress(replay, 0)
        assert sum(v.get('origin') == 'giveaway' for v in json.loads(store.read_text()).values()) == 1
        claimants = [Browser(), Browser()]
        for claimant in claimants:
            for step in (1, 2):
                go(claimant, step)
                assert f'Checkpoint {step} verified' in callback(claimant, step, str(step + 3) * 64)[1]
            go(claimant, 3)
        with concurrent.futures.ThreadPoolExecutor(max_workers=2) as pool:
            results = list(pool.map(lambda claimant: callback(claimant, 3, '6' * 64), claimants))
        assert sum('Your Free key is ready' in result[1] for result in results) == 1
        assert sum('already claimed' in result[1] for result in results) == 1
        assert sum(v.get('origin') == 'giveaway' for v in json.loads(store.read_text()).values()) == 2
        slow = Browser()
        go(slow, 1)
        with concurrent.futures.ThreadPoolExecutor(max_workers=2) as pool:
            pending = pool.submit(callback, slow, 1, 'e' * 64)
            for attempt in range(100):
                if (temp / 'vendor-entered').exists():
                    break
                time.sleep(.01)
            else:
                raise AssertionError('Vendor fixture was not entered')
            started = time.monotonic()
            grant(premium, 7, 'premium')
            assert time.monotonic() - started < 1.2 and not pending.done(), 'Vendor call held licence store lock'
            assert 'Checkpoint 1 verified' in pending.result()[1]
        text = (temp / 'server.log').read_text()
        assert 'Fatal error' not in text and 'Warning:' not in text and 'Deprecated:' not in text
        print('Full panel HTTP: admin/CSRF, 128-bit Free/Premium batches, signed tiers and legacy records, 3 verified ads, exact 10-hour/one-seat issuance, vendor failure, duplicate/replayed/concurrent claims, earned-key retention and nonblocking vendor verification passed (test-only vendor mock)')
        if a.reference_panel:
            print('Uploaded panel: admin/get-key HTML, CSS and JS unchanged; index.php byte-identical')
    finally:
        os.killpg(server.pid, signal.SIGTERM)
        server.wait(timeout=10)
        log.close()
