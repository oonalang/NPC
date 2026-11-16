<?php

declare(strict_types=1);

require __DIR__ . '/lib.php';
require __DIR__ . '/giveaway_lib.php';

$config = licence_config();
$store  = licence_store_open();
$auth   = licence_auth();

session_set_cookie_params([
    'samesite' => 'Lax',
    'httponly' => true,
    'secure'   => !empty($_SERVER['HTTPS']),
]);
session_name('licencepanel');
session_start();

if (empty($_SESSION['csrf'])) {
    $_SESSION['csrf'] = bin2hex(random_bytes(16));
}
$csrf = $_SESSION['csrf'];

function h(string $s): string
{
    return htmlspecialchars($s, ENT_QUOTES, 'UTF-8');
}

function flash(string $kind, string $msg): void
{
    $_SESSION['flash'] = ['kind' => $kind, 'msg' => $msg];
    header('Location: ' . basename(__FILE__), true, 303);
    exit;
}

function valid_device(string $hwid): bool
{
    return (bool)preg_match('/^[A-Za-z0-9-]{8,64}$/', $hwid);
}

function split_devices(string $raw): array
{
    $devices = [];
    $bad     = null;
    foreach (preg_split('/[\s,;]+/', trim($raw)) ?: [] as $d) {
        if ($d === '') {
            continue;
        }
        if (!valid_device($d)) {
            $bad = $d;
            break;
        }
        $devices[] = $d;
    }
    return [array_values(array_unique($devices)), $bad];
}

$error      = '';
$notice     = '';
$openCreate = false;
$draft      = ['key' => '', 'days' => '30', 'seats' => '0', 'hwid' => '', 'note' => '', 'tier' => 'premium', 'count' => '1'];

$pending = $_SESSION['flash'] ?? null;
$recentKeys = $_SESSION['recent_keys'] ?? null;
unset($_SESSION['flash'], $_SESSION['recent_keys']);
if (is_array($pending)) {
    if (($pending['kind'] ?? '') === 'bad') {
        $error = (string)($pending['msg'] ?? '');
    } else {
        $notice = (string)($pending['msg'] ?? '');
    }
}

$action = (string)($_POST['action'] ?? '');

if ($action !== '' && !hash_equals($csrf, (string)($_POST['csrf'] ?? ''))) {
    http_response_code(400);
    echo '<!doctype html><meta charset="utf-8">';
    echo '<p style="font:16px/1.6 system-ui;padding:2rem">This form expired. Reload the page and try again.</p>';
    exit;
}

if ($action === 'logout') {
    $_SESSION = [];
    session_destroy();
    header('Location: ' . basename(__FILE__));
    exit;
}

if ($action === 'login') {
    $user = trim((string)($_POST['user'] ?? ''));
    $pass = (string)($_POST['pass'] ?? '');
    if ((int)($_SESSION['fails'] ?? 0) >= 3) {
        usleep(400000);
    }
    if (licence_login($user, $pass)) {
        $_SESSION['in'] = true;
        $_SESSION['fails'] = 0;
        session_regenerate_id(true);
        header('Location: ' . basename(__FILE__), true, 303);
        exit;
    }
    $_SESSION['fails'] = (int)($_SESSION['fails'] ?? 0) + 1;
    $error = 'Wrong username or password.';
}

$signedIn = !empty($_SESSION['in']);


if ($signedIn && $action === 'save_giveaway') {
    $previous = giveaway_settings();
    $submitted = [
        'enabled' => isset($_POST['giveaway_enabled']),
        'links' => [
            trim((string)($_POST['giveaway_link_1'] ?? '')),
            trim((string)($_POST['giveaway_link_2'] ?? '')),
            trim((string)($_POST['giveaway_link_3'] ?? '')),
        ],
        'antibypass_token' => trim((string)($_POST['giveaway_token'] ?? '')) ?: (string)$previous['antibypass_token'],
    ];
    if ($submitted['enabled'] && ($reason = giveaway_validate($submitted)) !== '') {
        flash('bad', $reason);
    }
    if (!$submitted['enabled']) {
        foreach ($submitted['links'] as $link) {
            if ($link !== '' && !giveaway_valid_url($link)) {
                flash('bad', 'Each configured ad URL must be an HTTPS Linkvertise link.');
            }
        }
    }
    if (!giveaway_save_settings($submitted)) {
        flash('bad', 'Could not save giveaway settings. Check permissions in data/.');
    }
    flash('good', $submitted['enabled'] ? '10-hour giveaway enabled.' : 'Giveaway settings saved. Public claims are paused.');
}

if ($signedIn && $action === 'create') {
    $rawKey = trim((string)($_POST['key'] ?? ''));
    $days   = max(0, (int)($_POST['days'] ?? 30));
    $note   = trim((string)($_POST['note'] ?? ''));
    $seats  = max(0, (int)($_POST['seats'] ?? 1));
    $rawDev = trim((string)($_POST['hwid'] ?? ''));
    $tier   = (string)($_POST['tier'] ?? 'premium');
    $count  = (int)($_POST['count'] ?? 1);
    $draft  = [
        'key'   => $rawKey,
        'days'  => (string)$days,
        'seats' => (string)$seats,
        'hwid'  => $rawDev,
        'note'  => $note,
        'tier'  => $tier,
        'count' => (string)$count,
    ];
    [$devices, $badDevice] = split_devices($rawDev);

    if ($tier !== 'free' && $tier !== 'premium') {
        $error = 'Choose Free or Premium.';
    } elseif ($count < 1 || $count > 50) {
        $error = 'Create between 1 and 50 keys at a time.';
    } elseif ($rawKey !== '' && ($count !== 1 || strlen($rawKey) > 128 || preg_match('/[\r\n|]/', $rawKey))) {
        $error = 'A custom key must be 128 characters or fewer and cannot be used for batches or contain pipes or line breaks.';
    } elseif ($badDevice !== null) {
        $error = 'That is not a device id: ' . $badDevice;
    } elseif ($count > 1 && count($devices) > 0) {
        $error = 'Use the device field only when creating one key.';
    } elseif ($seats !== 0 && count($devices) > $seats) {
        $error = 'You listed ' . count($devices) . ' devices but allowed ' . $seats . '.';
    } elseif ($tier === 'free' && ($days < 1 || $days > 30 || $seats !== 1)) {
        $error = 'Free keys must last 1 to 30 days and allow exactly one device.';
    } else {
        $all  = licence_all($store);
        $made = [];
        for ($i = 0; $i < $count; $i++) {
            $name = $rawKey !== '' ? $rawKey : 'ETHNIR-' . ($tier === 'free' ? 'FREE-' : 'PREM-') . strtoupper(bin2hex(random_bytes(16)));
            if (isset($all[$name])) {
                if ($rawKey !== '') {
                    $error = 'That key already exists.';
                    break;
                }
                $i--;
                continue;
            }
            $all[$name] = licence_record($days, $note, $devices, $seats, $tier);
            $made[] = $name;
        }
        if ($error === '') {
            if (!licence_save($store, $all)) {
                $error = 'The keys could not be saved. Check storage permissions.';
            } else {
                $_SESSION['recent_keys'] = ['tier' => $tier, 'keys' => $made];
                flash('good', 'Created ' . count($made) . ' ' . ucfirst($tier) . ' key' . (count($made) === 1 ? '' : 's') . '.');
            }
        }
    }
    $openCreate = $error !== '';
}

if ($signedIn && $action === 'delete') {
    $key = (string)($_POST['key'] ?? '');
    $all = licence_all($store);
    if (isset($all[$key])) {
        unset($all[$key]);
        licence_save($store, $all);
        flash('good', 'Deleted ' . $key . '.');
    }
    flash('bad', 'That key is gone. Reload the page.');
}

if ($signedIn && $action === 'revoke') {
    $key = (string)($_POST['key'] ?? '');
    $lic = licence_find($store, $key);
    if ($lic !== null) {
        $lic['revoked'] = empty($lic['revoked']) ? 1 : 0;
        licence_update($store, $key, $lic);
        flash('good', $lic['revoked'] ? 'Revoked ' . $key . '.' : 'Re-enabled ' . $key . '.');
    }
    flash('bad', 'That key is gone. Reload the page.');
}

if ($signedIn && $action === 'bind') {
    $key = (string)($_POST['key'] ?? '');
    $lic = licence_find($store, $key);
    [$devices, $badDevice] = split_devices((string)($_POST['hwid'] ?? ''));
    if ($lic === null) {
        flash('bad', 'That key is gone. Reload the page.');
    }
    if ($badDevice !== null) {
        flash('bad', 'That is not a device id: ' . $badDevice);
    }
    if (!$devices) {
        flash('bad', 'Type a device id first.');
    }

    $current = licence_devices($lic);
    $seats   = licence_seats($lic);
    $merged  = array_values(array_unique(array_merge($current, $devices)));

    if ($seats !== 0 && count($merged) > $seats) {
        flash('bad', 'This key allows ' . $seats . ' device' . ($seats === 1 ? '' : 's') . '. Raise the limit first.');
    }

    $lic   = licence_apply_devices($lic, $merged);
    licence_update($store, $key, $lic);
    $added = array_values(array_diff($merged, $current));
    flash('good', $added ? 'Added ' . implode(', ', $added) . '.' : 'That device was already allowed.');
}

if ($signedIn && $action === 'dropdevice') {
    $key  = (string)($_POST['key'] ?? '');
    $hwid = trim((string)($_POST['hwid'] ?? ''));
    $lic  = licence_find($store, $key);
    if ($lic !== null && $hwid !== '') {
        $lic = licence_apply_devices($lic, array_diff(licence_devices($lic), [$hwid]));
        licence_update($store, $key, $lic);
        flash('good', 'Removed ' . $hwid . '.');
    }
    flash('bad', 'That device was not on the key.');
}

if ($signedIn && $action === 'seats') {
    $key   = (string)($_POST['key'] ?? '');
    $seats = max(0, (int)($_POST['seats'] ?? 1));
    $lic   = licence_find($store, $key);
    if ($lic === null) {
        flash('bad', 'That key is gone. Reload the page.');
    }
    if (licence_tier($lic) === 'free' && $seats !== 1) {
        flash('bad', 'Free keys are limited to one device.');
    }
    if ($seats !== 0 && $seats < count(licence_devices($lic))) {
        flash('bad', 'Remove a device first. This key already allows ' . count(licence_devices($lic)) . '.');
    }
    $lic = licence_apply_devices($lic, licence_devices($lic), $seats);
    licence_update($store, $key, $lic);
    flash('good', $key . ($seats === 0 ? ' now works on any device.' : ' now allows ' . $seats . '.'));
}

if ($signedIn && $action === 'rebind') {
    $key = (string)($_POST['key'] ?? '');
    $lic = licence_find($store, $key);
    if ($lic !== null) {
        $lic = licence_apply_devices($lic, []);
        licence_update($store, $key, $lic);
        flash('good', 'Cleared the device list on ' . $key . '.');
    }
    flash('bad', 'That key is gone. Reload the page.');
}

$docRoot = isset($_SERVER['DOCUMENT_ROOT']) ? rtrim((string)$_SERVER['DOCUMENT_ROOT'], '/') : '';
$webRootNote = static function (string $path, string $guard) use ($docRoot): array {
    if ($docRoot === '') {
        return ['unknown', true];
    }
    return strpos($path, $docRoot) === 0
        ? ['inside the web root, guarded by ' . $guard, false]
        : ['outside the web root', true];
};

$hasOpenssl = extension_loaded('openssl');
$keyOk      = is_readable($config['private_key']);
$storeDir   = dirname($store);
$storeOk    = is_writable($storeDir);

$appModulus = strtoupper(preg_replace('/[^0-9a-f]/i', '', (string)($config['app_modulus'] ?? '')));
$keyModulus = '';
if ($hasOpenssl && $keyOk) {
    $pem = @file_get_contents($config['private_key']);
    $key = $pem !== false ? @openssl_pkey_get_private($pem) : false;
    if ($key !== false) {
        $details = openssl_pkey_get_details($key);
        if (is_array($details) && isset($details['rsa']['n'])) {
            $keyModulus = ltrim(strtoupper(bin2hex($details['rsa']['n'])), '0');
        }
    }
}
if ($appModulus === '') {
    $modulusNote = 'no app modulus in config.php';
    $modulusOk = true;
} elseif ($keyModulus === '') {
    $modulusNote = 'cannot read the private key';
    $modulusOk = false;
} elseif (ltrim($appModulus, '0') === $keyModulus) {
    $modulusNote = 'matches the app';
    $modulusOk = true;
} else {
    $modulusNote = 'does not match the app, so every sign-in is refused';
    $modulusOk = false;
}

$diag = [
    ['PHP', PHP_VERSION . ' (7.4 or newer)', version_compare(PHP_VERSION, '7.4.0', '>=')],
    ['openssl', $hasOpenssl ? 'loaded' : 'missing', $hasOpenssl],
    ['signing key', $keyOk ? 'readable' : 'not readable', $keyOk],
    ['key matches the app', $modulusNote, $modulusOk],
    ['key store', $storeOk ? 'writable' : 'not writable', $storeOk],
    ['payload build', (string)$config['build'], true],
    ['key file', ...$webRootNote($config['private_key'], 'private/.htaccess')],
    ['key store path', ...$webRootNote($store, 'data/.htaccess')],
];

$giveawaySettings = giveaway_settings();
$keys = $signedIn ? licence_all($store) : [];
ksort($keys);
$now = time();

$total  = count($keys);
$active = 0;
$tierCounts = ['free' => 0, 'premium' => 0];
foreach ($keys as $lic) {
    $tierCounts[licence_tier($lic)]++;
    if (empty($lic['revoked']) && ((int)($lic['expires'] ?? 0) === 0 || (int)$lic['expires'] >= $now)) {
        $active++;
    }
}
?>
<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1, viewport-fit=cover">
<meta name="theme-color" content="#ece5ff">
<title><?= $signedIn ? 'Licence keys' : 'Sign in' ?></title>
<link rel="preconnect" href="https://fonts.googleapis.com">
<link rel="preconnect" href="https://fonts.gstatic.com" crossorigin>
<link rel="stylesheet" href="https://fonts.googleapis.com/css2?family=Manrope:wght@400;500;600;700;800&display=swap">
<script>document.documentElement.dataset.js = '1';</script>
<style>
    :root {
        --bg: #ece5ff;
        --bg-2: #dccfff;
        --surface: #f8f4ff;
        --surface-2: #f1eaff;
        --ink: #221938;
        --muted: #6b5f8f;
        --violet: #6d28d9;
        --violet-2: #8b5cf6;
        --mint: #cdf7e6;
        --mint-ink: #0b5c44;
        --rose: #ffd9e7;
        --rose-ink: #8f1d47;
        --line: rgba(109, 40, 217, .12);
        --r-xl: 30px;
        --r-lg: 22px;
        --r-md: 16px;
        --r-sm: 12px;
        --clay:
            14px 14px 32px rgba(109, 40, 217, .15),
            -10px -10px 26px rgba(255, 255, 255, .9),
            inset 2px 2px 4px rgba(255, 255, 255, .8),
            inset -2px -2px 6px rgba(109, 40, 217, .07);
        --clay-sm:
            8px 8px 18px rgba(109, 40, 217, .13),
            -6px -6px 14px rgba(255, 255, 255, .9);
        --sunk:
            inset 4px 4px 10px rgba(109, 40, 217, .13),
            inset -4px -4px 10px rgba(255, 255, 255, .92);
        color-scheme: light;
    }

    * { box-sizing: border-box }

    html, body { min-height: 100% }

    body {
        margin: 0;
        background:
            radial-gradient(900px 520px at 8% -12%, #f7f1ff 0%, rgba(247, 241, 255, 0) 62%),
            linear-gradient(172deg, var(--bg) 0%, var(--bg-2) 100%);
        background-attachment: fixed;
        color: var(--ink);
        font: 15px/1.6 "Manrope", ui-sans-serif, system-ui, -apple-system, "Segoe UI", Roboto, sans-serif;
        -webkit-font-smoothing: antialiased;
        overflow-x: hidden;
    }

    ::selection { background: var(--violet); color: #fff }

    a { color: var(--violet) }

    h1, h2, h3, h4 { letter-spacing: -.025em }

    .mono {
        font: 12.5px/1.5 ui-monospace, SFMono-Regular, Menlo, Consolas, monospace;
        word-break: break-all;
    }

    .dim { color: var(--muted) }

    .wrap { max-width: 1020px; margin: 0 auto; padding: 26px 18px 72px }

    .clay {
        background: var(--surface);
        border-radius: var(--r-xl);
        box-shadow: var(--clay);
    }

    @keyframes rise {
        from { opacity: 0; transform: translateY(14px) }
        to   { opacity: 1; transform: none }
    }

    @keyframes pop {
        from { opacity: 0; transform: translateY(18px) scale(.97) }
        to   { opacity: 1; transform: none }
    }

    @keyframes pop-out {
        to { opacity: 0; transform: translateY(10px) scale(.98) }
    }

    @keyframes fade { from { opacity: 0 } to { opacity: 1 } }

    @keyframes spin { to { transform: rotate(360deg) } }

    @keyframes shake {
        20%, 80% { transform: translateX(-3px) }
        45%, 65% { transform: translateX(4px) }
        55%      { transform: translateX(-4px) }
    }

    @keyframes shimmer {
        from { background-position: -160% 0 }
        to   { background-position: 160% 0 }
    }

    .progress {
        position: fixed;
        inset: 0 auto auto 0;
        height: 3px;
        width: 0;
        opacity: 0;
        background: linear-gradient(90deg, var(--violet-2), var(--violet));
        transition: width .35s ease, opacity .25s ease;
        z-index: 60;
    }
    html.loading .progress { width: 82%; opacity: 1 }

    /* Login */

    .login {
        min-height: 100dvh;
        display: grid;
        place-items: center;
        padding: 24px 16px 44px;
    }

    .login-card {
        width: 100%;
        max-width: 386px;
        padding: 32px 28px 26px;
        animation: rise .6s cubic-bezier(.2, .9, .2, 1) both;
    }
    .login-card.bad { animation: shake .5s ease-in-out }

    .login-card h1 { margin: 0 0 6px; font-size: 28px }

    .sub { margin: 0 0 26px; color: var(--muted); font-size: 14px }

    label.field { display: block; margin: 0 0 16px }

    .field > span,
    .label-text {
        display: block;
        margin: 0 0 7px 4px;
        font-size: 12.5px;
        font-weight: 600;
        color: var(--muted);
    }

    input, select, textarea {
        width: 100%;
        padding: 14px 16px;
        font: inherit;
        color: var(--ink);
        background: #fbf8ff;
        border: 0;
        border-radius: var(--r-md);
        box-shadow: var(--sunk);
        transition: box-shadow .18s ease, background .18s ease;
        -webkit-appearance: none;
    }
    input::placeholder, textarea::placeholder { color: #a498c2 }
    input:focus, select:focus, textarea:focus {
        outline: none;
        background: #fff;
        box-shadow: var(--sunk), 0 0 0 3px rgba(109, 40, 217, .25);
    }

    .btn {
        position: relative;
        display: inline-flex;
        align-items: center;
        justify-content: center;
        gap: 8px;
        min-height: 46px;
        padding: 12px 20px;
        font: 700 13.5px/1 "Manrope", ui-sans-serif, system-ui, sans-serif;
        color: var(--ink);
        background: var(--surface);
        border: 0;
        border-radius: var(--r-md);
        cursor: pointer;
        box-shadow: var(--clay-sm);
        transition: transform .15s ease, box-shadow .15s ease, filter .15s ease;
    }
    .btn:hover { transform: translateY(-1px) }
    .btn:active { transform: translateY(1px); box-shadow: var(--sunk) }
    .btn:focus-visible { outline: 3px solid rgba(109, 40, 217, .4); outline-offset: 2px }

    .btn.primary {
        color: #fff;
        background: linear-gradient(148deg, #8b5cf6, #6d28d9);
        box-shadow:
            10px 10px 22px rgba(109, 40, 217, .26),
            -6px -6px 16px rgba(255, 255, 255, .65),
            inset 1px 1px 2px rgba(255, 255, 255, .4);
    }
    .btn.primary:active { box-shadow: inset 3px 3px 8px rgba(49, 16, 110, .35) }

    .btn.quiet { color: var(--violet) }
    .btn.danger { color: var(--rose-ink); background: var(--rose) }
    .btn.small { min-height: 36px; padding: 8px 13px; font-size: 12.5px; border-radius: var(--r-sm) }
    .btn.block { width: 100% }

    .btn .spin {
        position: absolute;
        width: 16px;
        height: 16px;
        border-radius: 50%;
        border: 2.5px solid rgba(255, 255, 255, .45);
        border-top-color: #fff;
        opacity: 0;
        animation: spin .7s linear infinite;
    }
    .btn.busy { color: transparent; pointer-events: none }
    .btn.busy .spin { opacity: 1 }
    .btn.quiet.busy .spin { border-color: rgba(109, 40, 217, .25); border-top-color: var(--violet) }

    .alert {
        margin: 0 0 18px;
        padding: 13px 15px;
        border-radius: var(--r-md);
        font-weight: 600;
        font-size: 13.5px;
        animation: rise .3s ease both;
    }
    .alert.bad  { background: var(--rose); color: var(--rose-ink); box-shadow: var(--sunk) }
    .alert.good { background: var(--mint); color: var(--mint-ink); box-shadow: var(--sunk) }

    .fine { margin: 16px 0 0; color: var(--muted); font-size: 12.5px; text-align: center }

    /* App */

    .topbar {
        display: flex;
        align-items: center;
        justify-content: space-between;
        gap: 16px;
        flex-wrap: wrap;
        padding: 16px 20px;
        margin: 0 0 20px;
        animation: rise .5s ease both;
    }

    .brand { display: flex; align-items: center; gap: 13px }

    .mark {
        display: grid;
        place-items: center;
        width: 44px;
        height: 44px;
        border-radius: var(--r-md);
        background: linear-gradient(148deg, #a78bfa, #6d28d9);
        box-shadow: var(--clay-sm);
        color: #fff;
        font-weight: 800;
        font-size: 14px;
        letter-spacing: .02em;
    }

    .brand h1 { margin: 0; font-size: 17px }
    .brand p { margin: 1px 0 0; color: var(--muted); font-size: 12.5px }

    .topbar-actions { display: flex; align-items: center; gap: 10px }
    .topbar-actions form { display: contents }

    .panel {
        padding: 22px 22px 8px;
        margin: 0 0 20px;
        animation: rise .55s ease both;
        animation-delay: .05s;
    }

    .panel + .panel { animation-delay: .1s }

    .panel-head {
        display: flex;
        align-items: baseline;
        justify-content: space-between;
        gap: 12px;
        margin: 0 0 16px;
    }
    .panel-head h2 { margin: 0; font-size: 15px }
    .panel-head .meta { color: var(--muted); font-size: 12.5px; font-weight: 600 }

    .table-wrap {
        position: relative;
        border-radius: var(--r-lg);
        background: #fbf8ff;
        box-shadow: var(--sunk);
        padding: 6px;
        overflow-x: auto;
        margin: 0 0 14px;
    }

    table {
        width: 100%;
        border-collapse: separate;
        border-spacing: 0 5px;
        font-size: 13.5px;
    }

    .keys { min-width: 780px }
    .diag { min-width: 480px }

    thead th {
        padding: 9px 13px;
        text-align: left;
        font-size: 12px;
        font-weight: 600;
        color: var(--muted);
        white-space: nowrap;
    }

    tbody tr.key-row {
        background: var(--surface-2);
        box-shadow: 0 5px 12px rgba(109, 40, 217, .06);
        transition: transform .15s ease, box-shadow .15s ease;
    }
    tbody tr.key-row:hover {
        transform: translateY(-1px);
        box-shadow: 0 9px 18px rgba(109, 40, 217, .12);
    }
    body.ready tbody tr.key-row {
        animation: rise .38s ease both;
        animation-delay: calc(var(--i, 0) * 35ms);
    }

    tbody td {
        padding: 13px;
        vertical-align: middle;
    }

    .key-row td:first-child { border-radius: var(--r-sm) 0 0 var(--r-sm) }
    .key-row td:last-child  { border-radius: 0 var(--r-sm) var(--r-sm) 0 }

    .cell-key { display: flex; align-items: center; gap: 6px }

    .cell-key .mono { font-size: 12.5px }

    .btn.icon {
        min-height: 28px;
        padding: 4px 8px;
        font-size: 11.5px;
        border-radius: 9px;
        color: var(--muted);
        box-shadow: none;
        background: rgba(255, 255, 255, .6);
    }
    .btn.icon:hover { color: var(--violet) }

    .pill {
        display: inline-block;
        padding: 4px 10px;
        border-radius: 999px;
        font-size: 11.5px;
        font-weight: 700;
        white-space: nowrap;
    }
    .pill.ok  { background: var(--mint); color: var(--mint-ink) }
    .pill.bad { background: var(--rose); color: var(--rose-ink) }
    .pill.tier-free { background: #e8efff; color: #24518a }
    .pill.tier-premium { background: #e8dafe; color: #6728a7 }
    .filter-label { display: inline-flex; align-items: center; gap: 8px; color: var(--muted); font-size: 12.5px; font-weight: 600 }
    .filter-label select { width: auto; min-width: 118px; padding: 8px 10px; border-radius: var(--r-sm) }
    .generated-list { display: block; width: 100%; min-height: 100px; max-height: 230px; resize: vertical; margin-bottom: 10px }


    .note {
        max-width: 190px;
        overflow: hidden;
        text-overflow: ellipsis;
        white-space: nowrap;
    }

    .cell-actions { text-align: right; white-space: nowrap }

    /* Details row */

    tr.detail-row > td {
        padding: 0 0 8px;
    }

    .detail {
        display: grid;
        grid-template-columns: 1.35fr 1fr;
        gap: 18px 26px;
        padding: 18px 18px 14px;
        margin: 0 2px;
        background: var(--surface);
        border-radius: var(--r-md);
        box-shadow: var(--sunk);
        animation: rise .26s ease both;
    }

    .detail h4 {
        margin: 0 0 10px;
        font-size: 12.5px;
        font-weight: 700;
        color: var(--violet);
    }

    .detail .wide { grid-column: 1 / -1 }

    .chips { display: flex; flex-wrap: wrap; gap: 6px; margin: 0 0 10px }

    .chip {
        display: inline-flex;
        align-items: center;
        gap: 4px;
        padding: 4px 4px 4px 9px;
        border-radius: 10px;
        background: #fbf8ff;
        box-shadow: var(--clay-sm);
        font: 11.5px/1.4 ui-monospace, SFMono-Regular, Menlo, Consolas, monospace;
    }
    .chip button {
        width: 22px;
        height: 22px;
        display: grid;
        place-items: center;
        border: 0;
        border-radius: 7px;
        background: var(--rose);
        color: var(--rose-ink);
        font-size: 12px;
        font-weight: 700;
        line-height: 1;
        cursor: pointer;
    }

    .inline-form { display: flex; flex-wrap: wrap; gap: 7px }
    .inline-form input {
        width: auto;
        flex: 1 1 150px;
        padding: 9px 11px;
        border-radius: 11px;
        font-size: 12.5px;
    }
    .inline-form input[type=number] { flex: 0 0 70px }
    .inline-form .btn { flex: 0 0 auto }

    .detail-meta {
        display: flex;
        flex-wrap: wrap;
        gap: 6px 22px;
        margin: 0 0 14px;
        font-size: 12.5px;
        color: var(--muted);
    }

    .detail-actions { display: flex; flex-wrap: wrap; gap: 7px }

    .empty {
        padding: 30px 18px 34px;
        text-align: center;
    }
    .empty p { margin: 0 0 14px; color: var(--muted) }

    .diag td:first-child { font-weight: 600; width: 42% }
    .diag td.ok  { color: var(--mint-ink) }
    .diag td.bad { color: var(--rose-ink); font-weight: 700 }

    /* Skeleton */

    .skeleton {
        display: none;
        position: absolute;
        inset: 0;
        background: #fbf8ff;
        border-radius: var(--r-lg);
        padding: 20px;
        z-index: 2;
    }
    html[data-js] .skeleton { display: block }
    body.ready .skeleton {
        opacity: 0;
        visibility: hidden;
        transition: opacity .3s ease, visibility .3s ease;
    }

    .sk-row {
        display: flex;
        gap: 16px;
        align-items: center;
        height: 44px;
        margin: 0 0 9px;
    }

    .sk {
        height: 13px;
        border-radius: 7px;
        background:
            linear-gradient(100deg, rgba(226, 212, 255, .6) 22%, rgba(255, 255, 255, .95) 42%, rgba(226, 212, 255, .6) 62%)
            0 0 / 210% 100%;
        animation: shimmer 1.4s linear infinite;
    }
    .sk.a { width: 26% }
    .sk.b { width: 11% }
    .sk.c { width: 15% }
    .sk.d { width: 13% }
    .sk.e { width: 8%; margin-left: auto }

    /* Modal */

    dialog.sheet {
        width: 100%;
        max-width: min(540px, calc(100vw - 24px));
        padding: 0;
        border: 0;
        background: transparent;
        color: var(--ink);
        margin: auto;
    }
    dialog.sheet::backdrop {
        background: rgba(34, 25, 56, .42);
        backdrop-filter: blur(5px);
        animation: fade .22s ease both;
    }
    dialog.sheet[open] .sheet-card { animation: pop .34s cubic-bezier(.2, .9, .2, 1) both }
    dialog.sheet.closing .sheet-card { animation: pop-out .18s ease both }
    dialog.sheet.closing::backdrop { animation: fade .18s ease reverse both }

    .sheet-card {
        padding: 24px 24px 20px;
        background: var(--surface);
        border-radius: var(--r-xl);
        box-shadow:
            22px 22px 48px rgba(34, 25, 56, .3),
            -10px -10px 28px rgba(255, 255, 255, .5);
    }

    .sheet-head {
        display: flex;
        align-items: flex-start;
        justify-content: space-between;
        gap: 14px;
        margin: 0 0 18px;
    }
    .sheet-head h2 { margin: 0; font-size: 20px }
    .sheet-head p { margin: 3px 0 0; color: var(--muted); font-size: 13px }

    .fields {
        display: grid;
        grid-template-columns: 1fr 1fr;
        gap: 0 14px;
    }
    .fields .span-2 { grid-column: 1 / -1 }

    .trials { display: flex; flex-wrap: wrap; gap: 7px; margin: 0 0 12px }

    .chip-btn {
        padding: 7px 13px;
        font: 600 12.5px/1 "Manrope", ui-sans-serif, system-ui, sans-serif;
        color: var(--violet);
        background: var(--surface);
        border: 0;
        border-radius: 999px;
        box-shadow: var(--clay-sm);
        cursor: pointer;
        transition: transform .15s ease, box-shadow .15s ease, background .15s ease;
    }
    .chip-btn:hover { transform: translateY(-1px) }
    .chip-btn[aria-pressed="true"] {
        color: #fff;
        background: linear-gradient(148deg, #8b5cf6, #6d28d9);
        box-shadow: 0 6px 14px rgba(109, 40, 217, .3);
    }

    .hint { margin: 0 0 18px; color: var(--muted); font-size: 12.5px }

    .sheet-foot { display: flex; justify-content: flex-end; gap: 9px }

    /* Toast */

    .toast {
        position: fixed;
        left: 50%;
        bottom: 24px;
        padding: 11px 18px;
        border-radius: 999px;
        background: #2c1f4a;
        color: #f6f1ff;
        font-size: 13px;
        font-weight: 600;
        box-shadow: 0 14px 30px rgba(34, 25, 56, .35);
        opacity: 0;
        transform: translate(-50%, 16px);
        transition: opacity .22s ease, transform .22s ease;
        pointer-events: none;
        z-index: 70;
    }
    .toast.show { opacity: 1; transform: translate(-50%, 0) }

    .foot { margin: 26px 0 0; text-align: center; color: var(--muted); font-size: 12px }

    @media (max-width: 640px) {
        .wrap { padding: 18px 12px 56px }
        .panel { padding: 18px 14px 6px }
        .topbar { padding: 14px }
        .detail { grid-template-columns: 1fr; padding: 16px 14px 12px }
        .fields { grid-template-columns: 1fr }
        .login-card { padding: 28px 22px 22px }
        .topbar-actions { width: 100% }
        .topbar-actions .btn { flex: 1 }
    }

    @media (prefers-reduced-motion: reduce) {
        *, *::before, *::after {
            animation-duration: .001ms !important;
            animation-iteration-count: 1 !important;
            transition-duration: .001ms !important;
        }
    }
</style>
</head>
<body class="<?= $signedIn ? 'app' : 'login' ?>">
<div class="progress" aria-hidden="true"></div>

<?php if (!$signedIn): ?>
    <main class="login">
        <form class="login-card clay <?= $error ? 'bad' : '' ?>" method="post" data-busy>
            <input type="hidden" name="action" value="login">
            <input type="hidden" name="csrf" value="<?= h($csrf) ?>">
            <h1>Sign in</h1>
            <p class="sub">Manage the keys this build accepts.</p>

            <?php if ($error): ?>
                <p class="alert bad" role="alert"><?= h($error) ?></p>
            <?php endif; ?>

            <label class="field">
                <span>Username</span>
                <input name="user" autocomplete="username" autocapitalize="none" spellcheck="false"
                       placeholder="Your username" required autofocus>
            </label>

            <label class="field">
                <span>Password</span>
                <input name="pass" type="password" autocomplete="current-password"
                       placeholder="Your password" required>
            </label>

            <button class="btn primary block" type="submit">
                <span>Sign in</span>
                <span class="spin" aria-hidden="true"></span>
            </button>

            <p class="fine">No registration. One account, already set up.</p>
        </form>
    </main>
<?php else: ?>
    <main class="wrap">
        <header class="topbar clay">
            <div class="brand">
                <span class="mark" aria-hidden="true">EK</span>
                <div>
                    <h1>Licence keys</h1>
                    <p><?= $active ?> active of <?= $total ?> issued. <?= $tierCounts['free'] ?> free, <?= $tierCounts['premium'] ?> premium.</p>
                </div>
            </div>
            <div class="topbar-actions">
                <button class="btn quiet" type="button" data-open-sheet data-preset="free">Free giveaway</button>
                <button class="btn primary" type="button" data-open-sheet data-preset="premium">New key</button>
                <form method="post">
                    <input type="hidden" name="csrf" value="<?= h($csrf) ?>">
                    <button class="btn quiet" name="action" value="logout">Sign out</button>
                </form>
            </div>
        </header>

        <?php if ($error && !$openCreate): ?>
            <p class="alert bad" role="alert"><?= h($error) ?></p>
        <?php endif; ?>
        <?php if ($notice): ?>
            <p class="alert good" role="status"><?= h($notice) ?></p>
        <?php endif; ?>

        <?php if (is_array($recentKeys) && !empty($recentKeys['keys'])): ?>
            <section class="panel clay">
                <div class="panel-head">
                    <h2>New <?= h(ucfirst((string)$recentKeys['tier'])) ?> keys</h2>
                    <button type="button" class="btn quiet small" data-copy-list="issued-keys">Copy all</button>
                </div>
                <textarea id="issued-keys" class="generated-list mono" readonly aria-label="Newly generated keys"><?= h(implode("\n", $recentKeys['keys'])) ?></textarea>
                <p class="hint">Copy these keys to share with your users. The full list is saved below.</p>
            </section>
        <?php endif; ?>

        <section class="panel clay" id="giveaway-settings">
            <div class="panel-head">
                <h2>Free Key Portal</h2>
                <span class="meta">3 Linkvertise checkpoints · 10-hour key</span>
            </div>
            <p class="hint">Visitors complete three verified ad steps before receiving a Free key. Keys expire 10 hours after issuance and bind to one device.</p>
            <p><a class="btn quiet small" href="get-key.php" target="_blank" rel="noopener">Open public key portal</a></p>
            <form method="post" data-busy>
                <input type="hidden" name="csrf" value="<?= h($csrf) ?>">
                <input type="hidden" name="action" value="save_giveaway">
                <div class="fields">
                    <?php for ($i=0;$i<3;$i++): ?>
                    <label class="field span-2">
                        <span>Linkvertise ad <?= $i+1 ?> URL</span>
                        <input type="url" name="giveaway_link_<?= $i+1 ?>" value="<?= h((string)($giveawaySettings['links'][$i] ?? '')) ?>" placeholder="https://linkvertise.com/..." autocomplete="off" spellcheck="false">
                    </label>
                    <?php endfor; ?>
                    <label class="field span-2">
                        <span>Linkvertise Anti-Bypass Token (64 characters)</span>
                        <input type="password" name="giveaway_token" placeholder="<?= empty($giveawaySettings['antibypass_token']) ? 'Paste your token here' : 'Already saved. Leave blank to keep it' ?>" autocomplete="new-password" spellcheck="false">
                    </label>
                </div>
                <label style="display:flex;align-items:center;gap:10px;margin:16px 0;font-weight:700;font-size:13px">
                    <input type="checkbox" name="giveaway_enabled" value="1" <?= !empty($giveawaySettings['enabled'])?'checked':'' ?> style="width:20px;height:20px;accent-color:var(--violet)">
                    Enable public giveaway after configuration
                </label>
                <div style="display:flex;gap:12px;align-items:center;flex-wrap:wrap;margin-bottom:14px">
                    <button class="btn primary" type="submit"><span>Save giveaway settings</span><span class="spin" aria-hidden="true"></span></button>
                    <span class="meta">Status: <?= giveaway_ready($giveawaySettings)?'Active':'Inactive' ?></span>
                </div>
                <p class="hint">Each Linkvertise target URL must point to your public portal: <span class="mono">https://YOUR-DOMAIN/get-key.php?checkpoint=1</span>, then 2 and 3. Enable Linkvertise Anti-Bypassing only after all three targets are set. The token is stored in the protected data directory.</p>
            </form>
        </section>

        <section class="panel clay">
            <div class="panel-head">
                <h2>Keys</h2>
                <label class="filter-label">Show
                    <select id="tier-filter" aria-label="Filter keys by tier">
                        <option value="all">All (<?= $total ?>)</option>
                        <option value="free">Free (<?= $tierCounts['free'] ?>)</option>
                        <option value="premium">Premium (<?= $tierCounts['premium'] ?>)</option>
                    </select>
                </label>
            </div>

            <?php if (!$keys): ?>
                <div class="empty">
                    <p>No keys yet.</p>
                    <button class="btn primary" type="button" data-open-sheet>Create the first key</button>
                </div>
            <?php else: ?>
                <div class="table-wrap">
                    <div class="skeleton" aria-hidden="true">
                        <?php for ($s = 0; $s < 4; $s++): ?>
                            <div class="sk-row">
                                <span class="sk a"></span><span class="sk b"></span>
                                <span class="sk c"></span><span class="sk d"></span>
                                <span class="sk e"></span>
                            </div>
                        <?php endfor; ?>
                    </div>
                    <table class="keys">
                        <thead>
                            <tr>
                                <th scope="col">Key</th>
                                <th scope="col">Plan</th>
                                <th scope="col">Status</th>
                                <th scope="col">Expires</th>
                                <th scope="col">Devices</th>
                                <th scope="col">Note</th>
                                <th scope="col"><span class="dim">Detail</span></th>
                            </tr>
                        </thead>
                        <tbody>
                        <?php $i = 0; foreach ($keys as $key => $lic): ?>
                            <?php
                            $devs    = licence_devices($lic);
                            $seats   = licence_seats($lic);
                            $tier    = licence_tier($lic);
                            $expires = (int)($lic['expires'] ?? 0);
                            $state   = !empty($lic['revoked']) ? 'revoked' : ($expires !== 0 && $expires < $now ? 'expired' : 'active');
                            $note    = (string)($lic['note'] ?? '');
                            $detId   = 'detail-' . $i;
                            ?>
                            <tr class="key-row" data-tier="<?= $tier ?>" data-detail="<?= $detId ?>" style="--i:<?= min($i, 12) ?>">
                                <td>
                                    <span class="cell-key">
                                        <span class="mono"><?= h((string)$key) ?></span>
                                        <button class="btn icon" type="button" data-copy="<?= h((string)$key) ?>">copy</button>
                                    </span>
                                </td>
                                <td><span class="pill tier-<?= $tier ?>"><?= h(ucfirst($tier)) ?></span></td>
                                <td><span class="pill <?= $state === 'active' ? 'ok' : 'bad' ?>"><?= $state ?></span></td>
                                <td class="mono dim"><?= $expires === 0 ? 'never' : h(date('j M Y', $expires)) ?></td>
                                <td class="dim">
                                    <?= $devs ? count($devs) . ' of ' . ($seats === 0 ? 'unlimited' : $seats) : 'unbound' ?>
                                </td>
                                <td class="dim note" title="<?= h($note) ?>"><?= $note === '' ? '<span class="dim">none</span>' : h($note) ?></td>
                                <td class="cell-actions">
                                    <button class="btn quiet small" type="button"
                                            data-toggle="<?= $detId ?>" aria-expanded="false" aria-controls="<?= $detId ?>">Details</button>
                                </td>
                            </tr>
                            <tr class="detail-row" id="<?= $detId ?>" hidden>
                                <td colspan="7">
                                    <div class="detail">
                                        <div>
                                            <h4>Devices</h4>
                                            <div class="chips">
                                                <?php if (!$devs): ?>
                                                    <span class="dim">Nothing bound. The first sign-in claims a slot.</span>
                                                <?php endif; ?>
                                                <?php foreach ($devs as $d): ?>
                                                    <form method="post" style="display:inline">
                                                        <input type="hidden" name="key" value="<?= h((string)$key) ?>">
                                                        <input type="hidden" name="csrf" value="<?= h($csrf) ?>">
                                                        <input type="hidden" name="action" value="dropdevice">
                                                        <span class="chip"><?= h($d) ?><button name="hwid" value="<?= h($d) ?>"
                                                            aria-label="Remove <?= h($d) ?>" title="Remove">&times;</button></span>
                                                    </form>
                                                <?php endforeach; ?>
                                            </div>
                                            <form method="post" class="inline-form">
                                                <input type="hidden" name="key" value="<?= h((string)$key) ?>">
                                                <input type="hidden" name="csrf" value="<?= h($csrf) ?>">
                                                <input name="hwid" class="mono" placeholder="Add a device id" aria-label="Device id to allow">
                                                <button class="btn quiet small" name="action" value="bind">Add</button>
                                            </form>
                                        </div>

                                        <div>
                                            <h4>Limit</h4>
                                            <form method="post" class="inline-form">
                                                <input type="hidden" name="key" value="<?= h((string)$key) ?>">
                                                <input type="hidden" name="csrf" value="<?= h($csrf) ?>">
                                                <input name="seats" type="number" min="0" value="<?= $seats ?>" aria-label="Max devices">
                                                <button class="btn quiet small" name="action" value="seats">Save</button>
                                            </form>
                                            <p class="hint" style="margin:10px 0 0">
                                                <?= $seats === 0 ? 'Any number of devices.' : 'Up to ' . $seats . ' device' . ($seats === 1 ? '' : 's') . '.' ?>
                                            </p>
                                        </div>

                                        <div class="wide">
                                            <h4>History</h4>
                                            <div class="detail-meta">
                                                <span><?= h(ucfirst($tier)) ?> plan</span>
                                                <span>Created <?= h(date('j M Y, H:i', (int)($lic['created'] ?? $now))) ?></span>
                                                <span><?= $expires === 0 ? 'Never expires' : 'Expires ' . h(date('j M Y, H:i', $expires)) ?></span>
                                                <span><?= $seats === 0 ? 'Unlimited devices' : $seats . ' device slot' . ($seats === 1 ? '' : 's') ?></span>
                                            </div>
                                            <div class="detail-actions">
                                                <form method="post">
                                                    <input type="hidden" name="key" value="<?= h((string)$key) ?>">
                                                    <input type="hidden" name="csrf" value="<?= h($csrf) ?>">
                                                    <button class="btn quiet small" name="action" value="revoke">
                                                        <?= empty($lic['revoked']) ? 'Revoke' : 'Enable' ?>
                                                    </button>
                                                </form>
                                                <?php if ($devs): ?>
                                                    <form method="post">
                                                        <input type="hidden" name="key" value="<?= h((string)$key) ?>">
                                                        <input type="hidden" name="csrf" value="<?= h($csrf) ?>">
                                                        <button class="btn quiet small" name="action" value="rebind">Clear devices</button>
                                                    </form>
                                                <?php endif; ?>
                                                <form method="post" onsubmit="return confirm('Delete this key for good?')">
                                                    <input type="hidden" name="key" value="<?= h((string)$key) ?>">
                                                    <input type="hidden" name="csrf" value="<?= h($csrf) ?>">
                                                    <button class="btn danger small" name="action" value="delete">Delete key</button>
                                                </form>
                                            </div>
                                        </div>
                                    </div>
                                </td>
                            </tr>
                            <?php $i++; endforeach; ?>
                        </tbody>
                    </table>
                </div>
            <?php endif; ?>
        </section>

        <section class="panel clay">
            <div class="panel-head">
                <h2>Server checks</h2>
                <span class="meta">Run on every load</span>
            </div>
            <div class="table-wrap">
                <table class="diag">
                    <thead><tr><th scope="col">Check</th><th scope="col">Result</th></tr></thead>
                    <tbody>
                    <?php foreach ($diag as [$name, $detail, $ok]): ?>
                        <tr class="key-row">
                            <td><?= h((string)$name) ?></td>
                            <td class="<?= $ok ? 'ok' : 'bad' ?>"><?= h((string)$detail) ?></td>
                        </tr>
                    <?php endforeach; ?>
                    </tbody>
                </table>
            </div>
        </section>

        <p class="foot">RSA-2048 signatures, build <?= h((string)$config['build']) ?></p>

        <dialog class="sheet" id="new-key" aria-labelledby="new-key-title" <?= $openCreate ? 'data-open' : '' ?>>
            <form method="post" class="sheet-card" data-busy>
                <input type="hidden" name="action" value="create">
                <input type="hidden" name="csrf" value="<?= h($csrf) ?>">

                <div class="sheet-head">
                    <div>
                        <h2 id="new-key-title">New key</h2>
                        <p>Generate keys for Free or Premium access.</p>
                    </div>
                    <button class="btn icon" type="button" data-close aria-label="Close">&times;</button>
                </div>

                <?php if ($openCreate && $error): ?>
                    <p class="alert bad" role="alert"><?= h($error) ?></p>
                <?php endif; ?>

                <div class="fields">
                    <label class="field">
                        <span>Key plan</span>
                        <select name="tier" id="key-tier" aria-label="Key plan">
                            <option value="premium" <?= $draft['tier'] === 'premium' ? 'selected' : '' ?>>Premium</option>
                            <option value="free" <?= $draft['tier'] === 'free' ? 'selected' : '' ?>>Free</option>
                        </select>
                    </label>
                    <label class="field">
                        <span>Number of keys</span>
                        <input name="count" id="key-count" type="number" min="1" max="50" value="<?= h($draft['count']) ?>" inputmode="numeric">
                    </label>
                    <label class="field span-2">
                        <span>Access key</span>
                        <input name="key" class="mono" value="<?= h($draft['key']) ?>"
                               placeholder="Leave blank and the panel generates one">
                    </label>
                    <label class="field">
                        <span>Days</span>
                        <input name="days" id="days" type="number" min="0" value="<?= h($draft['days']) ?>" inputmode="numeric">
                    </label>
                    <label class="field">
                        <span>Max devices</span>
                        <input name="seats" type="number" min="0" value="<?= h($draft['seats']) ?>" inputmode="numeric">
                    </label>
                    <label class="field span-2">
                        <span>Device ids</span>
                        <input name="hwid" class="mono" value="<?= h($draft['hwid']) ?>"
                               placeholder="Optional, separated by commas">
                    </label>
                    <label class="field span-2">
                        <span>Note</span>
                        <input name="note" value="<?= h($draft['note']) ?>" placeholder="Who it is for">
                    </label>
                </div>

                <div class="trials" role="group" aria-label="Quick expiry">
                    <button class="chip-btn" type="button" data-days="1">1 day</button>
                    <button class="chip-btn" type="button" data-days="3">3 days</button>
                    <button class="chip-btn" type="button" data-days="7">7 days</button>
                    <button class="chip-btn" type="button" data-days="0">No expiry</button>
                </div>

                <p class="hint" id="tier-hint">Premium supports unlimited days and devices. Free keys last 1 to 30 days with one device each. Batches require an empty access key and device list.</p>

                <div class="sheet-foot">
                    <button class="btn quiet" type="button" data-close>Cancel</button>
                    <button class="btn primary" type="submit">
                        <span>Create key</span>
                        <span class="spin" aria-hidden="true"></span>
                    </button>
                </div>
            </form>
        </dialog>
    </main>
<?php endif; ?>

<div class="toast" id="toast" role="status" aria-live="polite"></div>

<script>
(function () {
    var root = document.documentElement;
    var sheet = document.getElementById('new-key');
    var days = document.getElementById('days');
    var keyTier = document.getElementById('key-tier');
    var count = document.getElementById('key-count');
    var toast = document.getElementById('toast');
    var toastTimer;

    function say(message) {
        if (!toast) return;
        toast.textContent = message;
        toast.classList.add('show');
        clearTimeout(toastTimer);
        toastTimer = setTimeout(function () { toast.classList.remove('show'); }, 1600);
    }

    function openSheet() {
        if (!sheet || sheet.open) return;
        if (typeof sheet.showModal === 'function') {
            sheet.showModal();
        } else {
            sheet.setAttribute('open', '');
        }
        var first = sheet.querySelector('input[name="key"]');
        if (first) first.focus({ preventScroll: true });
    }

    function closeSheet() {
        if (!sheet || !sheet.open) return;
        sheet.classList.add('closing');
        setTimeout(function () {
            sheet.classList.remove('closing');
            if (typeof sheet.close === 'function') {
                sheet.close();
            } else {
                sheet.removeAttribute('open');
            }
        }, 180);
    }

    function usePreset(tier) {
        if (!sheet || !keyTier) return;
        keyTier.value = tier;
        var seats = sheet.querySelector('input[name="seats"]');
        if (tier === 'free') {
            if (days) days.value = '1';
            if (seats) seats.value = '1';
            if (count) count.value = '10';
        } else {
            if (days) days.value = '30';
            if (seats) seats.value = '0';
            if (count) count.value = '1';
        }
        if (days) days.dispatchEvent(new Event('input'));
        syncTier();
    }

    function syncTier() {
        if (!sheet || !keyTier) return;
        var isFree = keyTier.value === 'free';
        var seats = sheet.querySelector('input[name="seats"]');
        var hint = document.getElementById('tier-hint');
        if (seats) {
            if (isFree) seats.value = '1';
            seats.readOnly = isFree;
        }
        if (days && isFree && (+days.value < 1 || +days.value > 30)) days.value = '1';
        var noExpiry = sheet.querySelector('[data-days="0"]');
        if (noExpiry) noExpiry.hidden = isFree;
        if (hint) hint.textContent = isFree
            ? 'Free keys: 1 to 30 days, one device per key. Set a batch size up to 50 to share multiple keys.'
            : 'Premium supports unlimited days and devices. Leave the access key blank to generate multiple keys.';
    }

    document.querySelectorAll('[data-open-sheet]').forEach(function (button) {
        button.addEventListener('click', function () {
            if (button.dataset.preset) usePreset(button.dataset.preset);
            openSheet();
        });
    });

    if (keyTier) keyTier.addEventListener('change', syncTier);
    if (days) days.addEventListener('change', syncTier);
    syncTier();

    if (sheet) {
        sheet.querySelectorAll('[data-close]').forEach(function (button) {
            button.addEventListener('click', closeSheet);
        });

        sheet.addEventListener('cancel', function (event) {
            event.preventDefault();
            closeSheet();
        });

        sheet.addEventListener('click', function (event) {
            if (event.target === sheet) closeSheet();
        });

        if (sheet.hasAttribute('data-open')) {
            openSheet();
        }
    }

    if (days && sheet) {
        var chips = sheet.querySelectorAll('[data-days]');
        var sync = function () {
            chips.forEach(function (chip) {
                chip.setAttribute('aria-pressed', chip.dataset.days === days.value ? 'true' : 'false');
            });
        };
        chips.forEach(function (chip) {
            chip.addEventListener('click', function () {
                days.value = chip.dataset.days;
                sync();
            });
        });
        days.addEventListener('input', sync);
        sync();
    }

    document.querySelectorAll('[data-toggle]').forEach(function (button) {
        button.addEventListener('click', function () {
            var row = document.getElementById(button.dataset.toggle);
            if (!row) return;
            var opening = row.hasAttribute('hidden');
            if (opening) {
                row.removeAttribute('hidden');
            } else {
                row.setAttribute('hidden', '');
            }
            button.setAttribute('aria-expanded', opening ? 'true' : 'false');
            button.textContent = opening ? 'Close' : 'Details';
        });
    });

    document.querySelectorAll('[data-copy-list]').forEach(function (button) {
        button.addEventListener('click', function () {
            var field = document.getElementById(button.dataset.copyList);
            if (!field) return;
            var copy = function () {
                var original = button.textContent;
                button.textContent = 'Copied';
                setTimeout(function () { button.textContent = original; }, 1200);
                say('All keys copied');
            };
            if (navigator.clipboard && navigator.clipboard.writeText) {
                navigator.clipboard.writeText(field.value).then(copy, function () { say('Clipboard access denied'); });
            } else {
                field.select();
                try { if (document.execCommand('copy')) copy(); } catch (error) { say('Clipboard access denied'); }
            }
        });
    });

    var filter = document.getElementById('tier-filter');
    if (filter) {
        filter.addEventListener('change', function () {
            document.querySelectorAll('tr.key-row[data-tier]').forEach(function (row) {
                var hidden = filter.value !== 'all' && row.dataset.tier !== filter.value;
                row.hidden = hidden;
                if (hidden) {
                    var detail = document.getElementById(row.dataset.detail);
                    if (detail) detail.hidden = true;
                    var toggle = row.querySelector('[data-toggle]');
                    if (toggle) {
                        toggle.setAttribute('aria-expanded', 'false');
                        toggle.textContent = 'Details';
                    }
                }
            });
        });
    }

    document.querySelectorAll('[data-copy]').forEach(function (button) {
        button.addEventListener('click', function () {
            var text = button.dataset.copy || '';
            var done = function () {
                button.textContent = 'copied';
                setTimeout(function () { button.textContent = 'copy'; }, 1200);
                say('Key copied');
            };
            if (navigator.clipboard && navigator.clipboard.writeText) {
                navigator.clipboard.writeText(text).then(done, function () {
                    say('Copy blocked by the browser');
                });
                return;
            }
            var box = document.createElement('textarea');
            box.value = text;
            document.body.appendChild(box);
            box.select();
            try { document.execCommand('copy'); done(); } catch (error) { say('Copy blocked by the browser'); }
            document.body.removeChild(box);
        });
    });

    document.querySelectorAll('form[data-busy]').forEach(function (form) {
        form.addEventListener('submit', function () {
            var button = form.querySelector('button[type=submit]');
            if (button) button.classList.add('busy');
            root.classList.add('loading');
        });
    });

    function ready() {
        requestAnimationFrame(function () {
            requestAnimationFrame(function () {
                setTimeout(function () { document.body.classList.add('ready'); }, 380);
            });
        });
    }

    if (document.readyState === 'complete') {
        ready();
    } else {
        window.addEventListener('load', ready);
    }
})();
</script>
</body>
</html>
