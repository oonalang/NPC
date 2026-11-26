<?php

declare(strict_types=1);

require __DIR__ . '/lib.php';
require __DIR__ . '/libstore.php';

$config = licence_config();

session_set_cookie_params([
    'samesite' => 'Lax',
    'httponly' => true,
    'secure'   => !empty($_SERVER['HTTPS']),
]);
session_name('licencepanel');
session_start();

if (empty($_SESSION['in'])) {
    header('Location: admin.php', true, 302);
    exit;
}

if (empty($_SESSION['csrf'])) {
    $_SESSION['csrf'] = bin2hex(random_bytes(16));
}
$csrf = (string)$_SESSION['csrf'];

function h(string $value): string
{
    return htmlspecialchars($value, ENT_QUOTES, 'UTF-8');
}

function lib_bytes(int $bytes): string
{
    if ($bytes >= 1048576) {
        return number_format($bytes / 1048576, 2) . ' MiB';
    }
    if ($bytes >= 1024) {
        return number_format($bytes / 1024, 1) . ' KiB';
    }
    return $bytes . ' B';
}

$error = '';
$notice = '';
$action = (string)($_POST['action'] ?? '');

if ($action !== '' && !hash_equals($csrf, (string)($_POST['csrf'] ?? ''))) {
    http_response_code(400);
    echo '<!doctype html><meta charset="utf-8">';
    echo '<p style="font:16px/1.6 system-ui;padding:2rem">This form expired. Reload the page and try again.</p>';
    exit;
}

$product = (string)($config['library_product'] ?? 'ETHNIR_LITE');
$build = (string)($config['library_build'] ?? '1');
$keyPath = (string)($config['private_key'] ?? '');

if ($action === 'upload') {
    $channel = trim((string)($_POST['channel'] ?? 'lite'));
    $note = trim((string)($_POST['note'] ?? ''));
    if (!(bool)preg_match('/^[a-z0-9-]{1,32}$/', $channel)) {
        $error = 'Channel must be lowercase letters, digits or dashes.';
    } elseif (strlen($note) > 120) {
        $error = 'Keep the note under 120 characters.';
    } elseif (!is_readable($keyPath)) {
        $error = 'The signing key is not readable, so the library cannot be signed.';
    } else {
        try {
            $entry = lib_store($_FILES['library'] ?? [], $channel, $note,
                (string)file_get_contents($keyPath), $product, $build);
            $notice = 'Uploaded and signed ' . $entry['name'] . '.';
        } catch (Throwable $failure) {
            $error = $failure->getMessage();
        }
    }
}

if ($action === 'delete') {
    $name = (string)($_POST['file'] ?? '');
    $notice = lib_remove($name) ? 'Removed ' . $name . '.' : 'That file is already gone.';
}

if ($action === 'rotate') {
    try {
        lib_secret_rotate();
        $notice = 'Download key regenerated. Every installed launcher needs a rebuild with the new key.';
    } catch (Throwable $failure) {
        $error = 'Could not regenerate the download key.';
    }
}

$secret = '';
try {
    $secret = lib_secret();
} catch (Throwable $failure) {
    $error = $error !== '' ? $error : 'The download key is unavailable. Check permissions on private/.';
}

$signingId = '';
$signingPublic = '';
try {
    $signingKey = is_readable($keyPath) ? openssl_pkey_get_private((string)file_get_contents($keyPath)) : false;
    $signingDetails = $signingKey === false ? [] : openssl_pkey_get_details($signingKey);
    if (is_array($signingDetails) && !empty($signingDetails['key'])) {
        $signingDer = lib_pem_to_der((string)$signingDetails['key']);
        $signingId = hash('sha256', $signingDer);
        $signingPublic = base64_encode($signingDer);
    }
} catch (Throwable $failure) {
    $signingId = '';
    $signingPublic = '';
}

$libraries = [];
try {
    $libraries = lib_list();
} catch (Throwable $failure) {
    $error = $error !== '' ? $error : 'The library store is unavailable. Check permissions on private/.';
}

$host = (string)($_SERVER['HTTP_HOST'] ?? 'ethnir.x10.mx');
if (!(bool)preg_match('/^[A-Za-z0-9.-]+(:[0-9]+)?$/', $host)) {
    $host = 'ethnir.x10.mx';
}
$base = 'https://' . $host;
$uploadLimit = (string)ini_get('upload_max_filesize');
$postLimit = (string)ini_get('post_max_size');
?>
<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Libraries · Ethnir panel</title>
<style>
    :root {
        --ink:#171335; --dim:#5c5779; --line:rgba(23,19,53,.14); --paper:#f6f4ff;
        --violet:#6d4aff; --violet-ink:#3a1fb8; --good:#0f7a4a; --bad:#a3231f;
    }
    * { box-sizing:border-box; }
    body { margin:0; background:linear-gradient(160deg,#efeaff,#f8f6ff 45%,#eef3ff);
           color:var(--ink); font:15px/1.55 ui-sans-serif,system-ui,-apple-system,"Segoe UI",sans-serif; }
    .wrap { max-width:1080px; margin:0 auto; padding:28px 20px 64px; }
    header.top { display:flex; gap:18px; align-items:center; justify-content:space-between;
                 flex-wrap:wrap; margin-bottom:22px; }
    h1 { margin:0; font-size:22px; letter-spacing:-.02em; text-transform:uppercase; font-weight:800; }
    .sub { margin:4px 0 0; color:var(--dim); font-size:13px; }
    .card { background:rgba(255,255,255,.86); border:1px solid var(--line); border-radius:20px;
            padding:20px 22px; margin-bottom:18px; box-shadow:0 18px 40px -30px rgba(40,20,120,.6); }
    h2 { margin:0 0 4px; font-size:15px; text-transform:uppercase; letter-spacing:.06em; }
    .hint { color:var(--dim); font-size:13px; margin:0 0 14px; }
    label.field { display:block; margin:0 0 12px; font-size:12px; font-weight:700;
                  text-transform:uppercase; letter-spacing:.06em; color:var(--dim); }
    input[type=text], input[type=file], select { width:100%; margin-top:6px; padding:11px 12px;
        border:1px solid var(--line); border-radius:12px; background:#fff; font:inherit; color:var(--ink); }
    .row { display:flex; gap:12px; flex-wrap:wrap; }
    .row > * { flex:1 1 220px; }
    .btn { display:inline-block; padding:11px 16px; border-radius:12px; border:1px solid var(--line);
           background:#fff; color:var(--ink); font-weight:700; font-size:13px; cursor:pointer;
           text-decoration:none; }
    .btn.primary { background:var(--violet); border-color:var(--violet); color:#fff; }
    .btn.danger { color:var(--bad); border-color:rgba(163,35,31,.35); }
    .alert { border-radius:14px; padding:12px 14px; margin:0 0 16px; font-size:13px; font-weight:600; }
    .alert.bad { background:rgba(163,35,31,.09); color:var(--bad); }
    .alert.good { background:rgba(15,122,74,.1); color:var(--good); }
    table { width:100%; border-collapse:collapse; font-size:13px; }
    th, td { text-align:left; padding:10px 8px; border-bottom:1px solid var(--line); vertical-align:top; }
    th { font-size:11px; text-transform:uppercase; letter-spacing:.08em; color:var(--dim); }
    .mono { font-family:ui-monospace,SFMono-Regular,Menlo,Consolas,monospace; font-size:12px;
            word-break:break-all; }
    .dim { color:var(--dim); }
    .tag { display:inline-block; padding:2px 8px; border-radius:999px; background:rgba(109,74,255,.12);
           color:var(--violet-ink); font-size:11px; font-weight:700; text-transform:uppercase; }
    .empty { color:var(--dim); font-size:13px; }
    form.inline { display:inline; }
</style>
</head>
<body>
<main class="wrap">
    <header class="top">
        <div>
            <h1>Protected libraries</h1>
            <p class="sub">Upload a build, get a signed protected link, and the launcher fetches it with a short-lived ticket.</p>
        </div>
        <div>
            <a class="btn" href="admin.php">Back to keys</a>
        </div>
    </header>

    <?php if ($error !== ''): ?>
        <p class="alert bad" role="alert"><?= h($error) ?></p>
    <?php endif; ?>
    <?php if ($notice !== ''): ?>
        <p class="alert good" role="status"><?= h($notice) ?></p>
    <?php endif; ?>

    <section class="card">
        <h2>Upload a library</h2>
        <p class="hint">arm64-v8a ELF only, up to 64 MiB. The panel signs the release manifest with the licence key
            (product <?= h($product) ?>, build <?= h($build) ?>), so the launcher verifies every byte it downloads.</p>
        <form method="post" enctype="multipart/form-data">
            <input type="hidden" name="csrf" value="<?= h($csrf) ?>">
            <input type="hidden" name="action" value="upload">
            <label class="field">Library file (.so)
                <input type="file" name="library" accept=".so,application/octet-stream" required>
            </label>
            <div class="row">
                <label class="field">Channel
                    <input type="text" name="channel" value="lite" maxlength="32" spellcheck="false" required>
                </label>
                <label class="field">Note (optional)
                    <input type="text" name="note" maxlength="120" spellcheck="false">
                </label>
            </div>
            <button class="btn primary" type="submit">Upload and sign</button>
            <p class="hint" style="margin:12px 0 0">Host limits: upload_max_filesize <?= h($uploadLimit) ?>,
                post_max_size <?= h($postLimit) ?>. Raise them in .user.ini if a build is rejected.</p>
        </form>
    </section>

    <section class="card">
        <h2>Published builds</h2>
        <p class="hint">Links below are valid for five minutes and only work for the signed-in panel. The launcher mints
            its own ticket from <span class="mono"><?= h($base) ?>/lib/token</span>.</p>
        <?php if (!$libraries): ?>
            <p class="empty">Nothing published yet.</p>
        <?php else: ?>
            <table>
                <thead>
                <tr>
                    <th scope="col">File</th>
                    <th scope="col">Channel</th>
                    <th scope="col">Size</th>
                    <th scope="col">SHA-256</th>
                    <th scope="col">Uploaded</th>
                    <th scope="col">Protected link</th>
                    <th scope="col"></th>
                </tr>
                </thead>
                <tbody>
                <?php foreach ($libraries as $entry): ?>
                    <tr>
                        <td class="mono"><?= h($entry['name']) ?><?= $entry['signed'] ? '' : ' <span class="tag">unsigned</span>' ?></td>
                        <td><span class="tag"><?= h($entry['channel']) ?></span></td>
                        <td class="dim"><?= h(lib_bytes($entry['size'])) ?></td>
                        <td class="mono dim"><?= h(substr($entry['sha256'], 0, 16)) ?>…</td>
                        <td class="dim"><?= h(date('Y-m-d H:i', $entry['uploaded'])) ?></td>
                        <td class="mono">
                            <a href="<?= h($base . lib_download_path($entry['name'])) ?>"><?= h('/lib/protected/' . $entry['name']) ?></a>
                        </td>
                        <td>
                            <form class="inline" method="post" onsubmit="return confirm('Delete this build?');">
                                <input type="hidden" name="csrf" value="<?= h($csrf) ?>">
                                <input type="hidden" name="action" value="delete">
                                <input type="hidden" name="file" value="<?= h($entry['name']) ?>">
                                <button class="btn danger" type="submit">Delete</button>
                            </form>
                        </td>
                    </tr>
                <?php endforeach; ?>
                </tbody>
            </table>
        <?php endif; ?>
    </section>

    <section class="card">
        <h2>Launcher download key</h2>
        <p class="hint">Paste this into <span class="mono">MainActivity.SECRET</span> before building the launcher.
            It signs the ticket request, so a stranger cannot pull the library even if they find the URL.</p>
        <?php if ($secret !== ''): ?>
            <p class="mono" style="user-select:all;background:#fff;border:1px solid var(--line);border-radius:12px;padding:12px"><?= h($secret) ?></p>
        <?php endif; ?>
        <form method="post" onsubmit="return confirm('Regenerate the key? Every installed launcher stops working until it is rebuilt.');">
            <input type="hidden" name="csrf" value="<?= h($csrf) ?>">
            <input type="hidden" name="action" value="rotate">
            <button class="btn danger" type="submit">Regenerate key</button>
        </form>
    </section>

    <section class="card">
        <h2>Launcher signing key</h2>
        <p class="hint">Paste both values into <span class="mono">LicenseRoot.java</span>. The launcher rejects a
            manifest signed by any other key, even when the download key is correct.</p>
        <?php if ($signingPublic !== ''): ?>
            <p class="hint">Key ID</p>
            <p class="mono" style="user-select:all;background:#fff;border:1px solid var(--line);border-radius:12px;padding:12px"><?= h($signingId) ?></p>
            <p class="hint">Public key</p>
            <p class="mono" style="user-select:all;background:#fff;border:1px solid var(--line);border-radius:12px;padding:12px"><?= h($signingPublic) ?></p>
        <?php else: ?>
            <p class="alert bad">The signing key could not be read. Check private/licence_private.pem.</p>
        <?php endif; ?>
    </section>

    <section class="card">
        <h2>What the launcher does</h2>
        <p class="hint">1. POSTs a signed ticket request to <span class="mono">/lib/token</span>.<br>
            2. Downloads <span class="mono">/lib/protected/&lt;file&gt;.manifest</span> and verifies the RSA signature.<br>
            3. Downloads the library, checks its size and SHA-256, then swaps it in atomically and loads it.</p>
        <p class="hint">A wrong key, an expired ticket, a missing manifest or a modified file all fail closed: the old
            library keeps running and nothing unverified is loaded.</p>
    </section>
</main>
</body>
</html>
