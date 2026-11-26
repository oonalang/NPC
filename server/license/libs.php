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
<?php require_once __DIR__ . '/theme.php'; ?>
<!doctype html>
<html lang="en">
<head>
<?= ui_head('Libraries · Ethnir panel') ?>
</head>
<body>
<main class="wrap">
    <header class="topbar clay">
        <div class="brand">
            <span class="mark" aria-hidden="true">EK</span>
            <div>
                <h1>Protected libraries</h1>
                <p>Upload a build, get a signed link, and the launcher fetches it with a short-lived ticket.</p>
            </div>
        </div>
        <div class="topbar-actions">
            <a class="btn quiet" href="admin.php">Back to keys</a>
        </div>
    </header>

    <?php if ($error !== ''): ?>
        <p class="alert bad" role="alert"><?= h($error) ?></p>
    <?php endif; ?>
    <?php if ($notice !== ''): ?>
        <p class="alert good" role="status"><?= h($notice) ?></p>
    <?php endif; ?>

    <section class="panel clay">
        <h2>Upload a library</h2>
        <p class="hint">arm64-v8a ELF only, up to 64 MiB. The panel signs the release manifest with the licence key
            (product <?= h($product) ?>, build <?= h($build) ?>), so the launcher verifies every byte it downloads.</p>
        <form method="post" enctype="multipart/form-data">
            <input type="hidden" name="csrf" value="<?= h($csrf) ?>">
            <input type="hidden" name="action" value="upload">
            <label class="field"><span>Library file (.so)</span>
                <input type="file" name="library" accept=".so,application/octet-stream" required>
            </label>
            <div class="row">
                <label class="field"><span>Channel</span>
                    <input type="text" name="channel" value="lite" maxlength="32" spellcheck="false" required>
                </label>
                <label class="field"><span>Note (optional)</span>
                    <input type="text" name="note" maxlength="120" spellcheck="false">
                </label>
            </div>
            <button class="btn primary block" type="submit">Upload and sign</button>
            <p class="hint" style="margin:14px 0 0">Host limits: upload_max_filesize <?= h($uploadLimit) ?>,
                post_max_size <?= h($postLimit) ?>. Raise them in .user.ini if a build is rejected.</p>
        </form>
    </section>

    <section class="panel clay">
        <h2>Published builds</h2>
        <p class="hint">Links below are valid for five minutes and only work for the signed-in panel. The launcher mints
            its own ticket from <span class="mono"><?= h($base) ?>/lib/token</span>.</p>
        <?php if (!$libraries): ?>
            <p class="empty">Nothing published yet.</p>
        <?php else: ?>
            <div class="table-wrap"><table>
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
                        <td class="mono"><?= h($entry['name']) ?><?= $entry['signed'] ? '' : ' <span class="pill bad">unsigned</span>' ?></td>
                        <td><span class="pill"><?= h($entry['channel']) ?></span></td>
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
                                <button class="btn danger small" type="submit">Delete</button>
                            </form>
                        </td>
                    </tr>
                <?php endforeach; ?>
                </tbody>
            </table></div>
        <?php endif; ?>
    </section>

    <section class="panel clay">
        <h2>Launcher download key</h2>
        <p class="hint">Paste this into <span class="mono">MainActivity.SECRET</span> before building the launcher.
            It signs the ticket request, so a stranger cannot pull the library even if they find the URL.</p>
        <?php if ($secret !== ''): ?>
            <p class="mono keybox"><?= h($secret) ?></p>
        <?php endif; ?>
        <form method="post" onsubmit="return confirm('Regenerate the key? Every installed launcher stops working until it is rebuilt.');">
            <input type="hidden" name="csrf" value="<?= h($csrf) ?>">
            <input type="hidden" name="action" value="rotate">
            <button class="btn danger" type="submit">Regenerate key</button>
        </form>
    </section>

    <section class="panel clay">
        <h2>Launcher signing key</h2>
        <p class="hint">Paste both values into <span class="mono">LicenseRoot.java</span>. The launcher rejects a
            manifest signed by any other key, even when the download key is correct.</p>
        <?php if ($signingPublic !== ''): ?>
            <p class="hint">Key ID</p>
            <p class="mono keybox"><?= h($signingId) ?></p>
            <p class="hint">Public key</p>
            <p class="mono keybox"><?= h($signingPublic) ?></p>
        <?php else: ?>
            <p class="alert bad">The signing key could not be read. Check private/licence_private.pem.</p>
        <?php endif; ?>
    </section>

    <section class="panel clay">
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
