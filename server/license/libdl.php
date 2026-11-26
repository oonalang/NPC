<?php

declare(strict_types=1);

ini_set('display_errors', '0');
require __DIR__ . '/libstore.php';

$name = (string)($_GET['f'] ?? '');
$expires = $_GET['e'] ?? '';
$signature = $_GET['s'] ?? '';

$manifest = lib_is_manifest($name);
if ($manifest) {
    $name = substr($name, 0, -9);
}
if (!lib_name_ok($name)) {
    lib_deny();
}

try {
    $requested = $manifest ? $name . '.manifest' : $name;
    if (!lib_download_allowed($requested, $expires, $signature)) {
        lib_deny();
    }
    $path = lib_root() . '/' . $requested;
    if (!is_file($path)) {
        lib_deny();
    }
} catch (Throwable $error) {
    lib_deny();
}

header('Content-Type: application/octet-stream');
header('Content-Length: ' . (string)filesize($path));
header('Content-Disposition: attachment; filename="' . $requested . '"');
header('Cache-Control: no-store, max-age=0');
header('X-Content-Type-Options: nosniff');
header('Referrer-Policy: no-referrer');
readfile($path);
