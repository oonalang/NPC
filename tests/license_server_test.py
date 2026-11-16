#!/usr/bin/env python3
import argparse
import base64
import concurrent.futures
import hashlib
import http.client
import json
import os
import resource
from pathlib import Path
import shutil
import signal
import socket
import subprocess
import tempfile
import time
import urllib.parse

p=argparse.ArgumentParser()
p.add_argument('--php',required=True)
p.add_argument('--verifier',required=True)
a=p.parse_args()
root=Path(__file__).resolve().parents[1]
with tempfile.TemporaryDirectory(prefix='shai-license-server-') as folder:
    temp=Path(folder)
    for name in ['connect.php','lib.php','SessionProtocol.php']:
        shutil.copyfile(root/'server/license'/name,temp/name)
    private=temp/'private.pem';public=temp/'public.pem';store=temp/'licenses.json'
    subprocess.run(['openssl','genpkey','-algorithm','RSA','-pkeyopt','rsa_keygen_bits:3072','-out',str(private)],check=True,stderr=subprocess.DEVNULL)
    public.write_bytes(subprocess.check_output(['openssl','pkey','-in',str(private),'-pubout']))
    der=subprocess.check_output(['openssl','pkey','-in',str(private),'-pubout','-outform','DER'])
    kid=hashlib.sha256(der).hexdigest()
    (temp/'config.php').write_text("<?php return ['store'=>'"+str(store)+"','private_key'=>'"+str(private)+"','build'=>1];")
    (temp/'revoke.php').write_text("<?php require __DIR__.'/lib.php'; $s=licence_store_open(); $v=licence_find($s,'race-key'); $v['revoked']=1; licence_update($s,'race-key',$v); echo 'OK';")
    def row(expiry=0,seats=1):return dict(expires=expiry,devices=[],seats=seats,revoked=0)
    licenses={'test-key':row(),'race-key':row(),'expired-key':row(int(time.time())-1),'never-key':row()}
    store.write_text(json.dumps(licenses))
    with socket.socket() as sock:
        sock.bind(('127.0.0.1',0));port=sock.getsockname()[1]
    env=os.environ.copy();env['PHP_CLI_SERVER_WORKERS']='4'
    log=(temp/'server.log').open('wb')
    server=subprocess.Popen([a.php,'-n','-S',f'127.0.0.1:{port}','-t',str(temp)],env=env,stdout=log,stderr=log,start_new_session=True)
    def send(values=None,path='/connect.php',method='POST'):
        body=urllib.parse.urlencode(values or {})
        connection=http.client.HTTPConnection('127.0.0.1',port,timeout=6)
        try:
            connection.request(method,path,body,{'Content-Type':'application/x-www-form-urlencoded'})
            response=connection.getresponse();data=response.read();return response.status,data,response.getheader('Cache-Control')
        finally:connection.close()
    try:
        for attempt in range(100):
            try:send(method='GET');break
            except OSError:time.sleep(.02)
        else:raise AssertionError('PHP test server failed to start')
        request=dict(protocol='2',game='CODMGR',build='1',user_key='test-key',serial='device-12345678',nonce='a'*64)
        code,data,cache=send(request);assert code==200 and cache=='no-store',(code,data)
        grant=json.loads(data)['data'];payload=grant['payload'];signature=grant['sig']
        parts=payload.split('|');assert len(parts)==11 and parts[5]==request['nonce'] and parts[-1]==kid
        assert int(parts[7])-int(parts[6])==180
        def verified(text=payload,sig=signature,nonce=request['nonce'],key='test-key',serial=request['serial']):
            result=subprocess.run([a.verifier,str(public),kid,key,serial,nonce,str(int(time.time()))],input=(text+'\n'+sig+'\n').encode(),stdout=subprocess.DEVNULL)
            return result.returncode==0
        assert verified()
        assert not verified(nonce='b'*64)
        assert not verified(key='other-key')
        assert not verified(sig=base64.b64encode(b'x'*384).decode())
        for changes in [{'protocol':'1'},{'game':'OTHER'},{'build':'2'},{'nonce':'0'*63},{'nonce':'Z'*64},{'user_key':'bad|key'},{'serial':'device-bad\n'},{'user_key':'expired-key'}]:
            code,data,_=send(dict(request,**changes));assert code>=400 and not json.loads(data)['status'],(changes,code,data)
        assert send(method='GET')[0]==405
        assert send(dict(request,padding='x'*5000))[0]==413
        assert send({'user_key':'test-key','serial':request['serial'],'game':'CODMGR'})[0]==426
        renewed=dict(request,nonce='b'*64)
        code,data,_=send(renewed);assert code==200
        second=json.loads(data)['data'];assert verified(second['payload'],second['sig'],nonce='b'*64)
        assert not verified(payload,signature,nonce='b'*64)
        def claim(index):
            values=dict(request,user_key='race-key',serial=f'device-{index:08d}',nonce=f'{index:064x}')
            for attempt in range(3):
                code,data,_=send(values)
                if code!=503:return code
                time.sleep(.02)
            return code
        with concurrent.futures.ThreadPoolExecutor(max_workers=8) as pool:
            results=list(pool.map(claim,range(8)))
        assert results.count(200)==1,results
        current=json.loads(store.read_text());assert len(current['race-key']['devices'])==1
        device=current['race-key']['devices'][0]
        with concurrent.futures.ThreadPoolExecutor(max_workers=5) as pool:
            futures=[pool.submit(send,dict(request,user_key='race-key',serial=device,nonce=f'{20+i:064x}')) for i in range(4)]
            futures.append(pool.submit(send,{},'/revoke.php'))
            for future in futures:future.result()
        assert json.loads(store.read_text())['race-key']['revoked']==1
        assert send(dict(request,user_key='race-key',serial=device))[0]==403
        current=json.loads(store.read_text());current['test-key']['revoked']=1;store.write_text(json.dumps(current))
        assert send(request)[0]==403
        original=store.read_text();store.write_text('corrupt')
        assert send(request)[0]==503 and store.read_text()=='corrupt'
        store.write_text(original)
        saved=store.read_bytes()
        partial="require '"+str(temp/'lib.php')+"'; exit(licence_save(licence_store_open(), ['partial'=>str_repeat('x',4096)]) ? 4 : 0);"
        def limit_file_size():
            resource.setrlimit(resource.RLIMIT_FSIZE,(512,512))
            signal.signal(signal.SIGXFSZ,signal.SIG_IGN)
        result=subprocess.run([a.php,'-n','-d','display_errors=0','-r',partial],preexec_fn=limit_file_size,stdout=subprocess.DEVNULL,stderr=subprocess.DEVNULL)
        assert result.returncode==0 and store.read_bytes()==saved
        print('PHP/C++ integration: signed nonce-bound leases, renewal/replay, strict v2, invalid inputs, revocation, concurrent final-seat allocation, admin mutation and corrupt-store rejection passed')
    finally:
        os.killpg(server.pid,signal.SIGTERM);server.wait(timeout=10);log.close()
