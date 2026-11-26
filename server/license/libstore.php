<?php

declare(strict_types=1);

const LIB_MAX_BYTES = 67108864;
const LIB_TOKEN_TTL = 300;
const LIB_CLOCK_SKEW = 120;
const LIB_NAME_PATTERN = '/^[A-Za-z0-9][A-Za-z0-9._-]{0,79}\.so$/';

function lib_root(): string
{
    $dir = __DIR__ . '/private/libs';
    if (!is_dir($dir) && !@mkdir($dir, 0770, true) && !is_dir($dir)) {
        throw new RuntimeException('Library store unavailable');
    }
    $guard = $dir . '/.htaccess';
    if (!is_file($guard)) {
        @file_put_contents($guard, "Require all denied\nDeny from all\n");
    }
    return $dir;
}

function lib_deny(): void
{
    http_response_code(404);
    header('Cache-Control: no-store');
    header('X-Content-Type-Options: nosniff');
    echo 'Not found';
    exit;
}

function lib_name_ok(string $name): bool
{
    return (bool)preg_match(LIB_NAME_PATTERN, $name);
}

function lib_is_manifest(string $name): bool
{
    return substr($name, -9) === '.manifest';
}

function lib_secret(): string
{
    $path = __DIR__ . '/private/lib_secret';
    if (is_readable($path)) {
        $value = trim((string)file_get_contents($path));
        if ((bool)preg_match('/^[0-9a-f]{64}$/', $value)) {
            return $value;
        }
    }

    $dir = dirname($path);
    if (!is_dir($dir) && !@mkdir($dir, 0770, true) && !is_dir($dir)) {
        throw new RuntimeException('Library secret unavailable');
    }
    $value = bin2hex(random_bytes(32));
    $tmp = $path . '.' . bin2hex(random_bytes(4));
    if (@file_put_contents($tmp, $value . "\n") === false || !@chmod($tmp, 0600) || !@rename($tmp, $path)) {
        @unlink($tmp);
        throw new RuntimeException('Library secret unavailable');
    }
    return $value;
}

function lib_secret_rotate(): string
{
    $path = __DIR__ . '/private/lib_secret';
    @unlink($path);
    return lib_secret();
}

function lib_meta_path(string $name): string
{
    return lib_root() . '/' . $name . '.json';
}

function lib_entry(string $name): ?array
{
    if (!lib_name_ok($name)) {
        return null;
    }
    $path = lib_root() . '/' . $name;
    if (!is_file($path)) {
        return null;
    }
    $meta = [];
    $raw = @file_get_contents(lib_meta_path($name));
    if (is_string($raw) && $raw !== '') {
        $decoded = json_decode($raw, true);
        if (is_array($decoded)) {
            $meta = $decoded;
        }
    }
    return [
        'name' => $name,
        'size' => (int)filesize($path),
        'sha256' => (string)($meta['sha256'] ?? hash_file('sha256', $path)),
        'original' => (string)($meta['original'] ?? $name),
        'channel' => (string)($meta['channel'] ?? 'lite'),
        'note' => (string)($meta['note'] ?? ''),
        'uploaded' => (int)($meta['uploaded'] ?? filemtime($path)),
        'signed' => is_file($path . '.manifest'),
    ];
}

function lib_list(): array
{
    $entries = [];
    foreach (scandir(lib_root()) ?: [] as $file) {
        if ($file === '.' || $file === '..') {
            continue;
        }
        $entry = lib_entry($file);
        if ($entry !== null) {
            $entries[] = $entry;
        }
    }
    usort($entries, static function (array $a, array $b): int {
        return $b['uploaded'] <=> $a['uploaded'];
    });
    return $entries;
}

function lib_remove(string $name): bool
{
    if (!lib_name_ok($name)) {
        return false;
    }
    $path = lib_root() . '/' . $name;
    @unlink($path . '.manifest');
    @unlink(lib_meta_path($name));
    return @unlink($path);
}

function lib_pem_to_der(string $pem): string
{
    $body = preg_replace('/-----(BEGIN|END)[^-]+-----/', '', $pem);
    $body = preg_replace('/\s+/', '', (string)$body);
    $der = base64_decode((string)$body, true);
    if ($der === false || $der === '') {
        throw new RuntimeException('Signing key unavailable');
    }
    return $der;
}

function lib_write_manifest(string $name, string $pem, string $product, string $build): string
{
    $path = lib_root() . '/' . $name;
    if (!is_file($path)) {
        throw new RuntimeException('Library missing');
    }
    $key = openssl_pkey_get_private($pem);
    if ($key === false) {
        throw new RuntimeException('Signing key unavailable');
    }
    $details = openssl_pkey_get_details($key);
    if (!is_array($details) || empty($details['key'])) {
        throw new RuntimeException('Signing key unavailable');
    }

    $keyId = hash('sha256', lib_pem_to_der((string)$details['key']));
    $payload = implode('|', [
        'v1',
        $product,
        $build,
        $name,
        (string)filesize($path),
        (string)hash_file('sha256', $path),
        $keyId,
    ]);
    $signature = '';
    if (!openssl_sign($payload, $signature, $key, OPENSSL_ALGO_SHA256)) {
        throw new RuntimeException('Signing failed');
    }

    $manifest = $payload . "\n" . base64_encode($signature) . "\n";
    $target = $path . '.manifest';
    $tmp = $target . '.' . bin2hex(random_bytes(4));
    if (@file_put_contents($tmp, $manifest) !== strlen($manifest) || !@chmod($tmp, 0640) || !@rename($tmp, $target)) {
        @unlink($tmp);
        throw new RuntimeException('Manifest could not be written');
    }
    return $payload;
}

function lib_store(array $upload, string $channel, string $note, string $pem, string $product,
                   string $build): array
{
    if (($upload['error'] ?? UPLOAD_ERR_NO_FILE) !== UPLOAD_ERR_OK || !isset($upload['tmp_name'])) {
        throw new RuntimeException('Upload failed. The file may exceed the host upload limit.');
    }
    $tmp = (string)$upload['tmp_name'];
    if (!is_uploaded_file($tmp)) {
        throw new RuntimeException('Upload rejected');
    }
    return lib_install($tmp, basename((string)($upload['name'] ?? 'library.so')), $channel, $note, $pem,
        $product, $build);
}

function lib_install(string $tmp, string $original, string $channel, string $note, string $pem,
                     string $product, string $build): array
{
    $size = (int)filesize($tmp);
    if ($size < 1 || $size > LIB_MAX_BYTES) {
        throw new RuntimeException('Library must be between 1 byte and 64 MiB');
    }

    $head = (string)file_get_contents($tmp, false, null, 0, 20);
    if (strlen($head) < 20 || substr($head, 0, 4) !== "\x7fELF") {
        throw new RuntimeException('That file is not an ELF library');
    }
    if (ord($head[4]) !== 2 || ord($head[5]) !== 1) {
        throw new RuntimeException('Library must be a little-endian ELF64 binary');
    }
    if (unpack('v', substr($head, 18, 2))[1] !== 0xB7) {
        throw new RuntimeException('Library must target arm64-v8a');
    }

    $original = basename($original);
    $base = preg_replace('/[^A-Za-z0-9._-]/', '_', $original) ?? 'library';
    if (substr($base, -3) === '.so') {
        $base = substr($base, 0, -3);
    }
    $base = substr(trim($base, '._-'), 0, 48);
    if ($base === '') {
        $base = 'library';
    }

    $root = lib_root();
    do {
        $name = $base . '.' . bin2hex(random_bytes(4)) . '.so';
        $target = $root . '/' . $name;
    } while (file_exists($target));

    if (!@rename($tmp, $target) && !@move_uploaded_file($tmp, $target)) {
        throw new RuntimeException('Library could not be stored');
    }
    @chmod($target, 0640);

    $entry = [
        'original' => $original,
        'channel' => $channel,
        'note' => $note,
        'size' => (int)filesize($target),
        'sha256' => (string)hash_file('sha256', $target),
        'uploaded' => time(),
    ];
    $meta = json_encode($entry, JSON_PRETTY_PRINT | JSON_UNESCAPED_SLASHES);
    if ($meta === false || @file_put_contents(lib_meta_path($name), $meta . "\n") === false) {
        lib_remove($name);
        throw new RuntimeException('Library metadata could not be written');
    }
    @chmod(lib_meta_path($name), 0640);

    lib_write_manifest($name, $pem, $product, $build);
    return lib_entry($name) ?? [];
}

function lib_resolve(string $file, string $channel): ?array
{
    if ($file !== '' && $file !== 'latest') {
        $entry = lib_entry($file);
        if ($entry === null || !$entry['signed']) {
            return null;
        }
        return ($channel === '' || $entry['channel'] === $channel) ? $entry : null;
    }
    foreach (lib_list() as $entry) {
        if (!$entry['signed']) {
            continue;
        }
        if ($channel === '' || $entry['channel'] === $channel) {
            return $entry;
        }
    }
    return null;
}

function lib_download_token(string $name, int $expires): string
{
    return hash_hmac('sha256', 'v1|' . $name . '|' . $expires, lib_secret());
}

function lib_download_allowed(string $name, $expires, $signature): bool
{
    if (!is_string($signature) || !(bool)preg_match('/^[0-9a-f]{64}$/', $signature)) {
        return false;
    }
    if (!is_string($expires) || !(bool)preg_match('/^[0-9]{1,10}$/', $expires)) {
        return false;
    }
    $expires = (int)$expires;
    $now = time();
    if ($expires < $now || $expires > $now + LIB_TOKEN_TTL + LIB_CLOCK_SKEW) {
        return false;
    }
    return hash_equals(lib_download_token($name, $expires), $signature);
}

function lib_request_token(string $file, string $channel, int $timestamp, string $nonce): string
{
    return hash_hmac('sha256', 'v1|' . $file . '|' . $channel . '|' . $timestamp . '|' . $nonce, lib_secret());
}

function lib_download_path(string $name, ?int $expires = null): string
{
    $expires = $expires ?? time() + LIB_TOKEN_TTL;
    return '/lib/protected/' . $name . '?e=' . $expires . '&s=' . lib_download_token($name, $expires);
}
