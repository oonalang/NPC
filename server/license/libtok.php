<?php

declare(strict_types=1);

ini_set('display_errors', '0');
header('Content-Type: application/json; charset=utf-8');
header('Cache-Control: no-store');
header('X-Content-Type-Options: nosniff');
require __DIR__ . '/libstore.php';

function libtok_fail(int $status, string $reason): void
{
    http_response_code($status);
    echo json_encode(['status' => false, 'reason' => $reason]);
    exit;
}

if (($_SERVER['REQUEST_METHOD'] ?? '') !== 'POST') {
    libtok_fail(405, 'POST required');
}
if ((int)($_SERVER['CONTENT_LENGTH'] ?? 0) > 2048) {
    libtok_fail(413, 'Request too large');
}

$file = (string)($_POST['file'] ?? '');
$channel = (string)($_POST['channel'] ?? '');
$timestamp = (string)($_POST['ts'] ?? '');
$nonce = (string)($_POST['nonce'] ?? '');
$signature = (string)($_POST['sig'] ?? '');

if (!(bool)preg_match('/^[0-9]{1,10}$/', $timestamp) || !(bool)preg_match('/^[0-9a-f]{32}$/', $nonce)) {
    libtok_fail(403, 'Rejected');
}
if (!(bool)preg_match('/^[0-9a-f]{64}$/', $signature)) {
    libtok_fail(403, 'Rejected');
}
if ($channel !== '' && !(bool)preg_match('/^[a-z0-9-]{1,32}$/', $channel)) {
    libtok_fail(403, 'Rejected');
}
if ($file !== '' && $file !== 'latest' && !lib_name_ok($file)) {
    libtok_fail(403, 'Rejected');
}

try {
    if (!hash_equals(lib_request_token($file, $channel, (int)$timestamp, $nonce), $signature)) {
        libtok_fail(403, 'Rejected');
    }
} catch (Throwable $error) {
    libtok_fail(503, 'Library service unavailable');
}

if (abs(time() - (int)$timestamp) > LIB_CLOCK_SKEW) {
    libtok_fail(403, 'Clock skew');
}

try {
    $entry = lib_resolve($file, $channel);
} catch (Throwable $error) {
    libtok_fail(503, 'Library service unavailable');
}
if ($entry === null) {
    libtok_fail(404, 'No library published');
}

$expires = time() + LIB_TOKEN_TTL;
$manifest = $entry['name'] . '.manifest';
if (!is_file(lib_root() . '/' . $manifest)) {
    libtok_fail(404, 'No library published');
}

echo json_encode([
    'status' => true,
    'file' => $entry['name'],
    'length' => $entry['size'],
    'sha256' => $entry['sha256'],
    'path' => '/lib/protected/' . $entry['name'],
    'e' => $expires,
    's' => lib_download_token($entry['name'], $expires),
    'manifest_path' => '/lib/protected/' . $manifest,
    'manifest_e' => $expires,
    'manifest_s' => lib_download_token($manifest, $expires),
    'expires' => $expires,
], JSON_UNESCAPED_SLASHES);
