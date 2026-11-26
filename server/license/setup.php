<?php

declare(strict_types=1);

$configFile = __DIR__ . '/config.php';
$keyFile = __DIR__ . '/private/licence_private.pem';
$dataDir = __DIR__ . '/data';

function s_h(string $value): string
{
    return htmlspecialchars($value, ENT_QUOTES, 'UTF-8');
}

function s_page(string $title, string $body): void
{
    echo '<!doctype html><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">';
    echo '<title>' . s_h($title) . '</title>';
    echo '<style>body{margin:0;background:#101216;color:#e7e9ee;font:16px/1.65 system-ui,-apple-system,Segoe UI,sans-serif}'
        . 'main{max-width:44rem;margin:0 auto;padding:2.5rem 1.25rem 4rem}'
        . 'h1{font-size:1.6rem;margin:0 0 .35rem}h2{font-size:1.05rem;margin:2rem 0 .6rem}'
        . 'p{margin:.6rem 0}code{background:#1b1f27;padding:.1em .35em;border-radius:4px}'
        . 'table{width:100%;border-collapse:collapse;margin:.6rem 0}'
        . 'td{padding:.4rem .5rem;border-top:1px solid #232833;vertical-align:top}'
        . 'td:first-child{width:45%;color:#aeb4c2}'
        . '.ok{color:#5cd08a}.bad{color:#f2746b}.warn{color:#e3b341}'
        . 'label{display:block;margin:1rem 0 .3rem;color:#aeb4c2;font-size:.9rem}'
        . 'input{width:100%;box-sizing:border-box;padding:.6rem .7rem;border-radius:8px;border:1px solid #2b3140;'
        . 'background:#161a21;color:#e7e9ee;font:inherit}'
        . 'button{margin-top:1.4rem;padding:.7rem 1.2rem;border:0;border-radius:8px;background:#6b8afd;color:#08101f;'
        . 'font:600 1rem system-ui,sans-serif;cursor:pointer}'
        . 'textarea{width:100%;box-sizing:border-box;height:9rem;padding:.6rem;border-radius:8px;background:#161a21;'
        . 'border:1px solid #2b3140;color:#cfd4df;font:13px/1.5 ui-monospace,monospace}'
        . '.note{border-left:3px solid #e3b341;padding:.2rem 0 .2rem .9rem;margin:1.2rem 0;color:#d9cba6}'
        . 'a{color:#8fa8ff}</style>';
    echo '<main>' . $body . '</main>';
}

$checks = [];
$checks[] = ['PHP 7.4 or newer', PHP_VERSION_ID >= 70400, PHP_VERSION];
$checks[] = ['openssl extension', extension_loaded('openssl'), extension_loaded('openssl') ? 'loaded' : 'missing'];
$checks[] = ['json extension', extension_loaded('json'), extension_loaded('json') ? 'loaded' : 'missing'];
$checks[] = ['session extension', extension_loaded('session'), extension_loaded('session') ? 'loaded' : 'missing'];

@mkdir(__DIR__ . '/private', 0770, true);
@mkdir($dataDir, 0770, true);

foreach ([__DIR__ . '/private', $dataDir] as $dir) {
    $checks[] = ['writable ' . basename($dir) . '/', is_dir($dir) && is_writable($dir), is_dir($dir) ? 'exists' : 'missing'];
}

$reuseKey = is_readable($keyFile);
$checks[] = ['signing key', true, $reuseKey ? 'found at private/licence_private.pem, will be reused' : 'not found, a new 3072-bit key will be created'];

if (is_file($configFile)) {
    $body = '<h1>Already configured</h1>'
        . '<p><code>config.php</code> exists, so this installer is locked.</p>'
        . '<p>Open <a href="admin.php">admin.php</a> to sign in. Delete <code>setup.php</code> from the server when you are done.</p>';
    s_page('Already configured', $body);
    exit;
}

session_set_cookie_params(['samesite' => 'Lax', 'httponly' => true, 'secure' => !empty($_SERVER['HTTPS'])]);
session_name('licencesetup');
session_start();

if (empty($_SESSION['csrf'])) {
    $_SESSION['csrf'] = bin2hex(random_bytes(16));
}
$csrf = (string)$_SESSION['csrf'];
$error = '';
$installed = null;
$user = '';
$modulus = '';

if (($_SERVER['REQUEST_METHOD'] ?? '') === 'POST') {
    $user = trim((string)($_POST['user'] ?? ''));
    $pass = (string)($_POST['pass'] ?? '');
    $pass2 = (string)($_POST['pass2'] ?? '');
    $modulus = strtoupper((string)preg_replace('/[^0-9a-f]/i', '', (string)($_POST['modulus'] ?? '')));
    $token = (string)($_POST['csrf'] ?? '');

    if (!hash_equals($csrf, $token)) {
        $error = 'This form expired. Reload the page and send it again.';
    } elseif (!(bool)preg_match('/^[A-Za-z0-9._@-]{1,64}$/', $user)) {
        $error = 'Use 1 to 64 letters, digits, dots, dashes, underscores or the @ sign for the username.';
    } elseif (strlen($pass) < 10) {
        $error = 'The password needs at least 10 characters.';
    } elseif (!hash_equals($pass, $pass2)) {
        $error = 'The two passwords do not match.';
    } elseif ($modulus !== '' && strlen($modulus) % 2 !== 0) {
        $error = 'The modulus must be an even number of hex digits.';
    } elseif (is_file($configFile)) {
        $error = 'config.php appeared while this form was open. Reload the page.';
    } else {
        $pem = '';
        try {
            if ($reuseKey) {
                $pem = (string)file_get_contents($keyFile);
                if (!(bool)preg_match('/-----BEGIN (?:RSA )?PRIVATE KEY-----/', $pem)) {
                    throw new RuntimeException('The existing key file is not a PEM private key.');
                }
            } else {
                $resource = openssl_pkey_new(['private_key_bits' => 3072, 'private_key_type' => OPENSSL_KEYTYPE_RSA]);
                if ($resource === false || !openssl_pkey_export($resource, $pem)) {
                    throw new RuntimeException('OpenSSL could not create a key. Ask the host to enable openssl.');
                }
                if (@file_put_contents($keyFile, $pem) === false) {
                    throw new RuntimeException('private/ is not writable, so the key could not be saved.');
                }
                @chmod($keyFile, 0600);
            }

            $details = openssl_pkey_get_details(openssl_pkey_get_private($pem));
            if (!is_array($details) || empty($details['key'])) {
                throw new RuntimeException('The private key could not be read back.');
            }
            $der = base64_decode((string)preg_replace('/-----[^-]+-----|\s/', '', (string)$details['key']), true);
            $keyId = hash('sha256', (string)$der);

            $config = [
                'admin_user' => $user,
                'admin_hash' => password_hash($pass, PASSWORD_DEFAULT),
                'build' => '1',
                'app_modulus' => $modulus,
                'private_key' => __DIR__ . '/private/licence_private.pem',
                'store' => __DIR__ . '/data/keys.json',
                'library_product' => 'ETHNIR_LITE',
                'library_build' => '1',
            ];

            $bytes = "<?php\n\ndeclare(strict_types=1);\n\nreturn " . var_export($config, true) . ";\n";
            $tmp = $configFile . '.' . bin2hex(random_bytes(4));
            if (@file_put_contents($tmp, $bytes) === false) {
                throw new RuntimeException('The panel folder is not writable, so config.php could not be written.');
            }
            @chmod($tmp, 0600);
            if (!@rename($tmp, $configFile)) {
                @unlink($tmp);
                throw new RuntimeException('config.php could not be moved into place.');
            }

            foreach ([__DIR__ . '/private' => "Require all denied\nDeny from all\n", $dataDir => "Require all denied\nDeny from all\n"] as $dir => $rule) {
                $guard = $dir . '/.htaccess';
                if (!is_file($guard)) {
                    @file_put_contents($guard, $rule);
                }
            }

            if (!is_file($dataDir . '/keys.json')) {
                @file_put_contents($dataDir . '/keys.json', "{}\n");
                @chmod($dataDir . '/keys.json', 0600);
            }

            $installed = ['user' => $user, 'key_id' => $keyId, 'public' => (string)$details['key'], 'fresh' => !$reuseKey];
        } catch (Throwable $failure) {
            $error = $failure->getMessage();
        }
    }
}

if ($installed !== null) {
    $body = '<h1>Panel configured</h1>'
        . '<p>Sign in at <a href="admin.php">admin.php</a> as <strong>' . s_h($installed['user']) . '</strong>.</p>'
        . ($installed['fresh']
            ? '<div class="note">A new signing key was created. The launcher APK pins the old public key, so rebuild it with this one before publishing libraries or licences.</div>'
            : '<p>The existing signing key was kept, so licences and manifests already issued stay valid.</p>')
        . '<h2>Public key ID</h2>'
        . '<p class="mono"><code>' . s_h($installed['key_id']) . '</code></p>'
        . '<h2>Public key</h2>'
        . '<textarea readonly aria-label="Public key">' . s_h($installed['public']) . '</textarea>'
        . '<h2>Last step</h2>'
        . '<p>Delete <code>setup.php</code> from the server. It refuses to run again while <code>config.php</code> exists, but it does not need to stay there.</p>';
    s_page('Panel configured', $body);
    exit;
}

$rows = '';
foreach ($checks as $check) {
    $state = $check[1] ? 'ok' : 'bad';
    $rows .= '<tr><td>' . s_h($check[0]) . '</td><td class="' . $state . '">' . s_h($check[2]) . '</td></tr>';
}

$body = '<h1>Panel setup</h1>'
    . '<p>This writes <code>config.php</code> for the panel next to it. It only runs while that file is missing.</p>'
    . '<h2>Server checks</h2><table>' . $rows . '</table>';

if ($error !== '') {
    $body .= '<p class="bad">' . s_h($error) . '</p>';
}

$body .= '<form method="post" action="setup.php">'
    . '<input type="hidden" name="csrf" value="' . s_h($csrf) . '">'
    . '<label for="user">Admin username</label>'
    . '<input id="user" name="user" value="' . s_h($user) . '" autocomplete="username" required autofocus>'
    . '<label for="pass">Admin password (10+ characters)</label>'
    . '<input id="pass" name="pass" type="password" autocomplete="new-password" required>'
    . '<label for="pass2">Repeat the password</label>'
    . '<input id="pass2" name="pass2" type="password" autocomplete="new-password" required>'
    . '<label for="modulus">App modulus (optional, hex, from the APK build)</label>'
    . '<input id="modulus" name="modulus" value="' . s_h($modulus) . '" autocomplete="off">'
    . '<button type="submit">Create config.php</button>'
    . '</form>';

s_page('Panel setup', $body);
