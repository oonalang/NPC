<?php

declare(strict_types=1);

$root = rtrim($argv[1] ?? '', '/');
if ($root === '' || !is_file($root . '/libstore.php')) {
    fwrite(STDERR, "usage: library_panel_test.php <panel-copy>\n");
    exit(2);
}

require $root . '/libstore.php';

$failures = 0;

function check(string $label, bool $ok): void
{
    global $failures;
    echo ($ok ? 'ok   ' : 'FAIL ') . $label . "\n";
    if (!$ok) {
        $failures++;
    }
}

function rejects(callable $work, string $label): void
{
    try {
        $work();
        check($label, false);
    } catch (RuntimeException $error) {
        check($label, true);
    }
}

function elf(int $machine, int $class = 2, int $size = 8192): string
{
    $body = str_repeat("\0", $size);
    $body = substr_replace($body, "\x7fELF", 0, 4);
    $body[4] = chr($class);
    $body[5] = "\x01";
    $body[6] = "\x01";
    $body = substr_replace($body, pack('v', $machine), 18, 2);
    for ($i = 64; $i < $size; ++$i) {
        $body[$i] = chr($i % 251);
    }
    return $body;
}

function http(string $url, ?string $post = null): array
{
    $options = ['http' => [
        'method' => $post === null ? 'GET' : 'POST',
        'ignore_errors' => true,
        'timeout' => 10,
        'header' => "Content-Type: application/x-www-form-urlencoded\r\n",
    ]];
    if ($post !== null) {
        $options['http']['content'] = $post;
    }
    $body = @file_get_contents($url, false, stream_context_create($options));
    $code = 0;
    $headers = function_exists('http_get_last_response_headers')
        ? (http_get_last_response_headers() ?? [])
        : ($http_response_header ?? []);
    if (isset($headers[0]) && preg_match('/\s(\d{3})\s/', $headers[0], $match)) {
        $code = (int)$match[1];
    }
    return [$code, (string)$body];
}

function curl(string $arguments): string
{
    $output = [];
    exec('curl -s -m 10 ' . $arguments . ' 2>&1', $output);
    return implode("\n", $output);
}

function panel(string $arguments, ?string &$sid, string $headers): string
{
    $cookie = ($sid === null || $sid === '') ? '' : ' -b ' . escapeshellarg('licencepanel=' . $sid);
    $body = curl($cookie . ' -D ' . escapeshellarg($headers) . ' ' . $arguments);
    $raw = (string)@file_get_contents($headers);
    if (preg_match('/Set-Cookie:\s*licencepanel=([0-9a-f]+)/i', $raw, $match)) {
        $sid = $match[1];
    }
    return $body;
}

@mkdir($root . '/private', 0770, true);

$key = openssl_pkey_new(['private_key_bits' => 2048, 'private_key_type' => OPENSSL_KEYTYPE_RSA]);
$pem = '';
openssl_pkey_export($key, $pem);
$details = openssl_pkey_get_details($key);
file_put_contents($root . '/private/test.pem', $pem);
@chmod($root . '/private/test.pem', 0600);

$config = [
    'admin_user' => 'tester',
    'admin_hash' => password_hash('secret', PASSWORD_DEFAULT),
    'private_key' => $root . '/private/test.pem',
    'store' => $root . '/data/keys.json',
    'library_product' => 'ETHNIR_LITE',
    'library_build' => '1',
];
file_put_contents($root . '/config.php', "<?php\nreturn " . var_export($config, true) . ";\n");

$source = $root . '/private/source.so';
file_put_contents($source, elf(0xB7));

$entry = lib_install($source, 'libZANEV1.so', 'lite', 'first build', $pem, 'ETHNIR_LITE', '1');
check('stores the arm64 library under its own name', ($entry['name'] ?? '') === 'libZANEV1.so');
check('records the digest', ($entry['sha256'] ?? '') === hash_file('sha256', lib_root() . '/' . $entry['name']));
check('marks the build signed', $entry['signed'] === true);

$manifestPath = lib_root() . '/' . $entry['name'] . '.manifest';
$raw = is_file($manifestPath) ? (string)file_get_contents($manifestPath) : '';
$lines = explode("\n", $raw);
$parts = explode('|', $lines[0] ?? '');
check('manifest frame is three lines', count($lines) === 3 && $lines[2] === '');
check('manifest names the published file', ($parts[3] ?? '') === $entry['name']);
check('manifest records size and digest', ($parts[4] ?? '') === (string)filesize(lib_root() . '/' . $entry['name']) &&
    ($parts[5] ?? '') === hash_file('sha256', lib_root() . '/' . $entry['name']));
check('manifest signature verifies', openssl_verify($lines[0] ?? '', base64_decode($lines[1] ?? '', true),
    $details['key'], OPENSSL_ALGO_SHA256) === 1);

file_put_contents($root . '/private/tag1.so', elf(0xB7, 2, 2048));
$tagged = lib_install($root . '/private/tag1.so', 'libnets.0a656191.so', 'lite', '', $pem, 'ETHNIR_LITE', '1');
check('drops a stale panel tag from the name', ($tagged['name'] ?? '') === 'libnets.so');

file_put_contents($root . '/private/tag2.so', elf(0xB7, 2, 4096));
$retagged = lib_install($root . '/private/tag2.so', 'libnets.so', 'lite', '', $pem, 'ETHNIR_LITE', '1');
check('a re-upload replaces the same name', ($retagged['sha256'] ?? '') === hash_file('sha256', lib_root() . '/libnets.so') &&
    count(lib_list()) === 2);
check('removes the replaced build', lib_remove('libnets.so') && count(lib_list()) === 1);

rejects(static function () use ($root, $pem): void {
    $path = $root . '/private/text.so';
    file_put_contents($path, 'not a library at all');
    lib_install($path, 'text.so', 'lite', '', $pem, 'ETHNIR_LITE', '1');
}, 'rejects a non-ELF file');

rejects(static function () use ($root, $pem): void {
    $path = $root . '/private/elf32.so';
    file_put_contents($path, elf(0xB7, 1));
    lib_install($path, 'elf32.so', 'lite', '', $pem, 'ETHNIR_LITE', '1');
}, 'rejects a 32-bit library');

rejects(static function () use ($root, $pem): void {
    $path = $root . '/private/x86.so';
    file_put_contents($path, elf(0x3E));
    lib_install($path, 'x86.so', 'lite', '', $pem, 'ETHNIR_LITE', '1');
}, 'rejects a non-arm64 library');

rejects(static function () use ($root, $pem): void {
    $path = $root . '/private/big.so';
    file_put_contents($path, elf(0xB7, 2, 64));
    $handle = fopen($path, 'r+');
    ftruncate($handle, LIB_MAX_BYTES + 1);
    fclose($handle);
    clearstatcache();
    lib_install($path, 'big.so', 'lite', '', $pem, 'ETHNIR_LITE', '1');
}, 'rejects an oversized library');

rejects(static function () use ($root, $pem): void {
    $path = $root . '/private/badkey.so';
    file_put_contents($path, elf(0xB7));
    lib_install($path, 'badkey.so', 'lite', '', 'not a key', 'ETHNIR_LITE', '1');
}, 'rejects an unusable signing key');

$expires = time() + 60;
$token = lib_download_token($entry['name'], $expires);
check('accepts a fresh ticket', lib_download_allowed($entry['name'], (string)$expires, $token));
check('rejects another file', !lib_download_allowed('other.so', (string)$expires, $token));
check('rejects an expired ticket', !lib_download_allowed($entry['name'], (string)(time() - 5),
    lib_download_token($entry['name'], time() - 5)));
check('rejects an oversized lifetime', !lib_download_allowed($entry['name'],
    (string)(time() + LIB_TOKEN_TTL + 3600), lib_download_token($entry['name'], time() + LIB_TOKEN_TTL + 3600)));
check('rejects a forged signature', !lib_download_allowed($entry['name'], (string)$expires, str_repeat('0', 64)));

$stamp = time();
$nonce = bin2hex(random_bytes(16));
check('request signature is deterministic',
    lib_request_token('latest', 'lite', $stamp, $nonce) === lib_request_token('latest', 'lite', $stamp, $nonce));
check('latest resolves on the channel', (lib_resolve('latest', 'lite')['name'] ?? '') === $entry['name']);
check('latest ignores other channels', lib_resolve('latest', 'beta') === null);

$socket = stream_socket_server('tcp://127.0.0.1:0', $number, $reason);
$address = stream_socket_get_name($socket, false);
fclose($socket);
$port = (int)substr((string)$address, strrpos((string)$address, ':') + 1);
$base = 'http://127.0.0.1:' . $port;

$log = $root . '/server.log';
$sessions = $root . '/sessions';
@mkdir($sessions, 0770, true);
$server = proc_open([PHP_BINARY, '-d', 'session.save_path=' . $sessions, '-S', '127.0.0.1:' . $port, '-t', $root],
    [0 => ['file', '/dev/null', 'r'], 1 => ['file', $log, 'a'], 2 => ['file', $log, 'a']], $pipes);
$ready = false;
for ($try = 0; $try < 60 && $server !== false; ++$try) {
    $probe = @fsockopen('127.0.0.1', $port, $errno, $errstr, 0.2);
    if ($probe !== false) {
        fclose($probe);
        $ready = true;
        break;
    }
    usleep(100000);
}
check('test server started', $ready);

if ($ready) {
    $library = (string)file_get_contents(lib_root() . '/' . $entry['name']);

    [$code, $body] = http($base . '/libdl.php?f=' . rawurlencode($entry['name']) . '&e=' . $expires . '&s=' . $token);
    check('downloads with a valid ticket', $code === 200 && $body === $library);

    [$code] = http($base . '/libdl.php?f=' . rawurlencode($entry['name']));
    check('refuses a link without a ticket', $code === 404);

    [$code] = http($base . '/libdl.php?f=' . rawurlencode($entry['name']) . '&e=' . $expires . '&s=' . str_repeat('0', 64));
    check('refuses a forged ticket', $code === 404);

    $manifestToken = lib_download_token($entry['name'] . '.manifest', $expires);
    [$code, $body] = http($base . '/libdl.php?f=' . rawurlencode($entry['name'] . '.manifest') . '&e=' . $expires . '&s=' . $manifestToken);
    check('downloads the signed manifest', $code === 200 && $body === $raw);

    $stamp = time();
    $nonce = bin2hex(random_bytes(16));
    $request = http_build_query([
        'file' => 'latest',
        'channel' => 'lite',
        'ts' => (string)$stamp,
        'nonce' => $nonce,
        'sig' => lib_request_token('latest', 'lite', $stamp, $nonce),
    ]);
    [$code, $body] = http($base . '/libtok.php', $request);
    $ticket = json_decode($body, true);
    check('issues a ticket', $code === 200 && ($ticket['status'] ?? false) === true);
    check('ticket points at the protected path', ($ticket['path'] ?? '') === '/lib/protected/' . $entry['name'] &&
        ($ticket['manifest_path'] ?? '') === '/lib/protected/' . $entry['name'] . '.manifest');

    if (is_array($ticket) && isset($ticket['e'], $ticket['s'])) {
        [$code, $body] = http($base . '/libdl.php?f=' . rawurlencode($entry['name']) . '&e=' . $ticket['e'] . '&s=' . $ticket['s']);
        check('ticket opens the library', $code === 200 && $body === $library);
    }

    [$code] = http($base . '/libtok.php', http_build_query([
        'file' => 'latest',
        'channel' => 'lite',
        'ts' => (string)time(),
        'nonce' => $nonce,
        'sig' => str_repeat('a', 64),
    ]));
    check('refuses an unsigned ticket request', $code === 403);

    [$code] = http($base . '/libtok.php', 'file=latest&channel=lite');
    check('refuses a bare ticket request', $code === 403);

    $stale = time() - 400;
    $staleNonce = bin2hex(random_bytes(16));
    [$code, $body] = http($base . '/libtok.php', http_build_query([
        'file' => 'latest',
        'channel' => 'lite',
        'ts' => (string)$stale,
        'nonce' => $staleNonce,
        'sig' => lib_request_token('latest', 'lite', $stale, $staleNonce),
    ]));
    check('names a stale clock separately', $code === 403 &&
        (json_decode($body, true)['reason'] ?? '') === 'Clock skew');

    $headers = $root . '/headers.txt';
    $panelSession = '';
    $page = panel(escapeshellarg($base . '/admin.php'), $panelSession, $headers);
    preg_match('/name="csrf" value="([0-9a-f]{32})"/', $page, $match);
    $csrf = $match[1] ?? '';
    check('panel serves a login form', $csrf !== '' && $panelSession !== '');
    panel('-d ' . escapeshellarg('action=login&csrf=' . $csrf . '&user=tester&pass=secret') . ' ' .
        escapeshellarg($base . '/admin.php'), $panelSession, $headers);
    $page = panel(escapeshellarg($base . '/libs.php'), $panelSession, $headers);
    check('libraries page opens for the owner', str_contains($page, 'Protected libraries'));

    $upload = $root . '/private/panel-upload.so';
    file_put_contents($upload, elf(0xB7, 2, 4096));
    $result = panel('-F ' . escapeshellarg('csrf=' . $csrf) .
        ' -F ' . escapeshellarg('action=upload') .
        ' -F ' . escapeshellarg('channel=lite') .
        ' -F ' . escapeshellarg('note=from curl') .
        ' -F ' . escapeshellarg('library=@' . $upload . ';filename=libZANEV1.so') .
        ' ' . escapeshellarg($base . '/libs.php'), $panelSession, $headers);
    check('panel uploads and signs a build', str_contains($result, 'Uploaded and signed'));
    check('panel prints the protected link', str_contains($result, '/lib/protected/'));

    $anonymous = curl('-F ' . escapeshellarg('action=upload') . ' ' . escapeshellarg($base . '/libs.php'));
    check('anonymous upload is refused', !str_contains($anonymous, 'Uploaded and signed'));

    [$code] = http($base . '/libs.php');
    check('libraries page needs a session', $code === 302);
}

if ($server !== false) {
    proc_terminate($server);
    proc_close($server);
}

foreach (lib_list() as $listed) {
    lib_remove($listed['name']);
}
check('removes every published build', lib_list() === [] && !is_file($manifestPath));

exit($failures === 0 ? 0 : 1);
