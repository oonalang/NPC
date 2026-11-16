<?php
declare(strict_types=1);
const SHAI_LEASE_SECONDS = 180;

function shai_failure(string $reason, int $code = 403): void
{
    http_response_code($code);
    echo json_encode(['status' => false, 'reason' => $reason]);
    exit;
}

function shai_field(string $name, int $maximum): string
{
    $value = $_POST[$name] ?? '';
    if (!is_string($value) || strlen($value) > $maximum) shai_failure('Malformed request', 400);
    return $value;
}

function shai_issue(): array
{
    if (shai_field('protocol', 1) !== '2') shai_failure('Client update required', 426);
    if (shai_field('game', 16) !== 'CODMGR') shai_failure('Unknown client');
    $config = licence_config();
    $build = shai_field('build', 16);
    if ($build !== (string)$config['build']) shai_failure('Client update required', 426);
    $key = shai_field('user_key', 128);
    $serial = shai_field('serial', 64);
    $nonce = shai_field('nonce', 64);
    if ($key === '' || preg_match('/[\x00-\x1f\x7f|]/', $key) ||
        !preg_match('/^[A-Za-z0-9-]{8,64}$/D', $serial) ||
        !preg_match('/^[0-9a-f]{64}$/D', $nonce)) shai_failure('Malformed request', 400);
    $store = licence_store_open();
    licence_lock_store($store);
    $licence = licence_find($store, $key);
    if ($licence === null || !empty($licence['revoked'])) shai_failure('Access key rejected');
    $now = time();
    $expiry = licence_signable_expiry((int)$licence['expires']);
    if ($expiry <= $now) shai_failure('Access key expired');
    $devices = licence_devices($licence);
    $newDevice = !in_array($serial, $devices, true);
    if ($newDevice && !licence_has_room($licence)) shai_failure('Access key device limit reached');
    $private = openssl_pkey_get_private(licence_private_key());
    if ($private === false) throw new RuntimeException('Signing configuration');
    $details = openssl_pkey_get_details($private);
    if ($details === false || $details['type'] !== OPENSSL_KEYTYPE_RSA || $details['bits'] < 3072) {
        throw new RuntimeException('Signing configuration');
    }
    $der = base64_decode(preg_replace('/-----[^-]+-----|\s/', '', $details['key']), true);
    if ($der === false) throw new RuntimeException('Signing configuration');
    $keyId = hash('sha256', $der);
    $until = min($now + SHAI_LEASE_SECONDS, $expiry);
    $tier = licence_tier($licence);
    $capabilities = $tier === 'premium' ? 7 : 3;
    $payload = implode('|', ['v2','CODMGR',$build,$key,$serial,$nonce,
        (string)$now,(string)$until,(string)$expiry,(string)$capabilities,$keyId]);
    $signature = '';
    if (!openssl_sign($payload, $signature, $private, OPENSSL_ALGO_SHA256)) {
        throw new RuntimeException('Signing configuration');
    }
    if ($newDevice) {
        $devices[] = $serial;
        $licence = licence_apply_devices($licence, $devices);
        if (!licence_update($store, $key, $licence)) throw new RuntimeException('Storage unavailable');
    }
    return ['status' => true, 'data' => ['payload' => $payload, 'sig' => base64_encode($signature), 'tier' => $tier]];
}
