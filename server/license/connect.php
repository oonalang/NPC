<?php
declare(strict_types=1);
ini_set('display_errors', '0');
header('Content-Type: application/json; charset=utf-8');
header('Cache-Control: no-store');
header('X-Content-Type-Options: nosniff');
require __DIR__ . '/SessionProtocol.php';
if (($_SERVER['REQUEST_METHOD'] ?? '') !== 'POST') shai_failure('POST required', 405);
if ((int)($_SERVER['CONTENT_LENGTH'] ?? 0) > 4096) shai_failure('Request too large', 413);
try {
    require __DIR__ . '/lib.php';
    $result = shai_issue();
    $encoded = json_encode($result, JSON_THROW_ON_ERROR);
    echo $encoded;
} catch (Throwable $error) {
    shai_failure('License service unavailable', 503);
}
