<?php

declare(strict_types=1);

const LICENCE_NEVER = 4102444800;

$licenceConfigPath = __DIR__ . '/config.php';

if (!is_file($licenceConfigPath)) {
    http_response_code(503);
    if (basename((string)($_SERVER['SCRIPT_FILENAME'] ?? '')) === 'connect.php') {
        header('Content-Type: application/json; charset=utf-8');
        echo json_encode(['status' => false, 'reason' => 'Licence service is not configured']);
        exit;
    }
    header('Content-Type: text/html; charset=utf-8');
    echo '<!doctype html><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">';
    echo '<title>Panel not configured</title>';
    echo '<div style="max-width:44rem;margin:0 auto;padding:2.5rem 1.25rem;font:16px/1.65 system-ui,sans-serif;background:#101216;color:#e7e9ee">';
    echo '<h1 style="font-size:1.5rem">Panel not configured</h1>';
    echo '<p style="margin:.8rem 0"><code>config.php</code> is missing from the panel folder, so every page here stops at this step.</p>';
    echo '<p style="margin:.8rem 0">Open <a style="color:#8fa8ff" href="setup.php">setup.php</a> once to create it together with your administrator login. If you already have a <code>config.php</code> from another host, upload it next to <code>admin.php</code> instead.</p>';
    echo '</div>';
    exit;
}

$config = require $licenceConfigPath;
unset($licenceConfigPath);

if (!is_array($config)) {
    http_response_code(503);
    header('Content-Type: text/html; charset=utf-8');
    echo '<!doctype html><meta charset="utf-8"><title>Panel not configured</title>';
    echo '<div style="max-width:44rem;margin:0 auto;padding:2.5rem 1.25rem;font:16px/1.65 system-ui,sans-serif;background:#101216;color:#e7e9ee">';
    echo '<p><code>config.php</code> has to return an array. Open <a style="color:#8fa8ff" href="setup.php">setup.php</a> to rebuild it.</p>';
    echo '</div>';
    exit;
}

function licence_config(): array
{
    global $config;
    return $config;
}

function licence_auth(): array
{
    $cfg = licence_config();
    return [
        'user' => (string)($cfg['admin_user'] ?? ''),
        'hash' => (string)($cfg['admin_hash'] ?? ''),
    ];
}

function licence_login(string $user, string $pass): bool
{
    $auth = licence_auth();
    $userOk = $auth['user'] !== '' && hash_equals($auth['user'], $user);
    $passOk = $auth['hash'] !== '' && password_verify($pass, $auth['hash']);
    return $userOk && $passOk;
}

function licence_private_key(): string
{
    static $pem = null;
    if ($pem !== null) {
        return $pem;
    }

    $path = licence_config()['private_key'];
    if (!is_readable($path)) {
        http_response_code(500);
        echo json_encode(['status' => false, 'reason' => 'Licence service is not configured']);
        exit;
    }

    $pem = (string)file_get_contents($path);
    return $pem;
}

function licence_store_open(): string
{
    $store = licence_config()['store'];
    $dir = dirname($store);
    if (!is_dir($dir) && !@mkdir($dir, 0770, true) && !is_dir($dir)) {
        http_response_code(500);
        echo json_encode(['status' => false, 'reason' => 'Licence store unavailable']);
        exit;
    }
    if (!file_exists($store)) {
        $file = @fopen($store, 'x');
        if ($file !== false) {
            @chmod($store, 0600);
            fwrite($file, "{}\n");
            fclose($file);
        }
    }
    return $store;
}

function licence_all(string $store): array
{
    $raw = @file_get_contents($store);
    if ($raw === false || trim($raw) === '') throw new RuntimeException('Licence store unavailable');
    $data = json_decode($raw, true);
    if (!is_array($data)) throw new RuntimeException('Invalid licence store');
    return $data;
}

function licence_save(string $store, array $all): bool
{
    licence_lock_store($store);
    $tmp = tempnam(dirname($store), '.licence-');
    if ($tmp === false) return false;
    $bytes = json_encode($all, JSON_PRETTY_PRINT);
    if ($bytes === false || file_put_contents($tmp, $bytes . "\n") !== strlen($bytes) + 1) {
        @unlink($tmp);
        return false;
    }
    @chmod($tmp, 0600);
    if (!rename($tmp, $store)) {
        @unlink($tmp);
        return false;
    }
    return true;
}

function licence_signable_expiry(int $stored): int
{
    return $stored === 0 ? LICENCE_NEVER : $stored;
}

function licence_find(string $store, string $key): ?array
{
    $all = licence_all($store);
    return $all[$key] ?? null;
}

function licence_update(string $store, string $key, array $lic): bool
{
    licence_lock_store($store);
    $all = licence_all($store);
    $all[$key] = $lic;
    return licence_save($store, $all);
}

function licence_devices(array $lic): array
{
    $out = [];
    if (isset($lic['devices']) && is_array($lic['devices'])) {
        foreach ($lic['devices'] as $d) {
            if (is_string($d) && $d !== '') {
                $out[] = $d;
            }
        }
    } elseif (isset($lic['hwid']) && is_string($lic['hwid']) && $lic['hwid'] !== '') {
        $out[] = $lic['hwid'];
    }
    return array_values(array_unique($out));
}

function licence_seats(array $lic): int
{
    if (array_key_exists('seats', $lic)) {
        return max(0, (int)$lic['seats']);
    }
    return 1;
}

function licence_has_room(array $lic): bool
{
    $seats = licence_seats($lic);
    return $seats === 0 || count(licence_devices($lic)) < $seats;
}

function licence_apply_devices(array $lic, array $devices, ?int $seats = null): array
{
    $lic['devices'] = array_values(array_unique(array_filter($devices, static function ($d) {
        return is_string($d) && $d !== '';
    })));
    $lic['hwid'] = $lic['devices'][0] ?? '';
    if ($seats !== null) {
        $lic['seats'] = max(0, $seats);
    } elseif (!array_key_exists('seats', $lic)) {
        $lic['seats'] = 1;
    }
    return $lic;
}

function licence_tier(array $lic): string
{
    return ($lic['tier'] ?? 'premium') === 'free' ? 'free' : 'premium';
}

function licence_record(int $days, string $note = '', array $devices = [], int $seats = 1,
                        string $tier = 'premium'): array
{
    $lic = [
        'expires' => $days > 0 ? time() + $days * 86400 : 0,
        'devices' => [],
        'seats'   => max(0, $seats),
        'revoked' => 0,
        'note'    => $note,
        'created' => time(),
        'tier'    => $tier === 'free' ? 'free' : 'premium',
    ];
    return licence_apply_devices($lic, $devices, max(0, $seats));
}

function licence_create(string $store, string $key, int $days, string $note = '',
                        array $devices = [], int $seats = 1, string $tier = 'premium'): array
{
    licence_lock_store($store);
    $lic = licence_record($days, $note, $devices, $seats, $tier);
    $all = licence_all($store);
    $all[$key] = $lic;
    if (!licence_save($store, $all)) {
        throw new RuntimeException('Unable to save the licence');
    }
    return $lic;
}

function licence_lock_store(string $store): void
{
    static $locks = [];
    if (isset($locks[$store])) return;
    $path = $store . '.lock';
    $lock = @fopen($path, 'c');
    if ($lock === false) throw new RuntimeException('Licence store unavailable');
    @chmod($path, 0600);
    $deadline = microtime(true) + 1.0;
    while (!flock($lock, LOCK_EX | LOCK_NB)) {
        if (microtime(true) >= $deadline) {
            fclose($lock);
            throw new RuntimeException('Licence store busy');
        }
        usleep(10000);
    }
    $locks[$store] = $lock;
    register_shutdown_function(static function () use ($lock): void {
        flock($lock, LOCK_UN);
        fclose($lock);
    });
}

function licence_request_lock(): void
{
    if (($_SERVER['REQUEST_METHOD'] ?? '') !== 'POST' ||
        basename((string)($_SERVER['SCRIPT_FILENAME'] ?? '')) !== 'admin.php') return;
    $action = $_POST['action'] ?? '';
    if (!is_string($action) || !in_array($action,
        ['create','delete','revoke','bind','dropdevice','seats','rebind'], true)) return;
    try {
        licence_lock_store(licence_store_open());
    } catch (Throwable $error) {
        http_response_code(503);
        header('Retry-After: 1');
        exit('The licence store is busy. Please retry.');
    }
}

licence_request_lock();
