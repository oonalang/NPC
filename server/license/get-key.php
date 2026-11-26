<?php

declare(strict_types=1);

require __DIR__ . '/giveaway_lib.php';

header('Cache-Control: no-store, private');
header('Referrer-Policy: no-referrer');
header('X-Content-Type-Options: nosniff');
header('X-Frame-Options: DENY');

session_set_cookie_params([
    'samesite' => 'Lax',
    'httponly' => true,
    'secure' => !empty($_SERVER['HTTPS']),
]);
ini_set('session.use_strict_mode', '1');
session_name('ethnirfree');
session_start();
if (empty($_SESSION['csrf'])) {
    $_SESSION['csrf'] = bin2hex(random_bytes(24));
}

function free_h(string $value): string
{
    return htmlspecialchars($value, ENT_QUOTES, 'UTF-8');
}

function free_redirect(): void
{
    header('Location: get-key.php', true, 303);
    exit;
}

$settings = giveaway_settings();
$ready = giveaway_ready($settings);
$now = time();
$error = '';
$notice = '';

if (!isset($_SESSION['flow']) || !is_array($_SESSION['flow']) ||
    (empty($_SESSION['flow']['issued']) && ((int)($_SESSION['flow']['started'] ?? 0) + 1800 < $now ||
    !is_array($_SESSION['flow']['hashes'] ?? null) || count($_SESSION['flow']['hashes']) !== (int)($_SESSION['flow']['progress'] ?? 0)))) {
    $_SESSION['flow'] = ['progress' => 0, 'started' => $now, 'pending' => 0, 'hashes' => []];
}
$flow = &$_SESSION['flow'];

if (!empty($flow['issued']) && (int)($flow['issued']['expires'] ?? 0) <= $now) {
    $flow = ['progress' => 0, 'started' => $now, 'pending' => 0, 'hashes' => []];
}

if ($_SERVER['REQUEST_METHOD'] === 'GET' && isset($_GET['checkpoint'], $_GET['hash'])) {
    $step = (int)$_GET['checkpoint'];
    $hash = (string)$_GET['hash'];
    if (!$ready) {
        $error = 'Free key claims are temporarily unavailable.';
    } elseif ($step < 1 || $step > GIVEAWAY_STEPS
        || $step !== (int)($flow['pending'] ?? 0)
        || $step !== ((int)$flow['progress'] + 1)) {
        $error = 'Unexpected checkpoint. Use the current step on this page.';
    } elseif (in_array(strtolower($hash), (array)($flow['hashes'] ?? []), true)) {
        $error = 'This checkpoint proof was already used. Complete the next ad.';
    } elseif (giveaway_verify_hash($hash, (string)$settings['antibypass_token'])) {
        $flow['hashes'][] = strtolower($hash);
        $flow['progress'] = $step;
        $flow['pending'] = 0;
        $notice = 'Checkpoint ' . $step . ' verified.';
        if ($step === GIVEAWAY_STEPS) {
            try {
                $flow['issued'] = giveaway_issue(licence_store_open(), $flow['hashes']);
                $notice = 'Your 10-hour free key is ready.';
            } catch (Throwable $e) {
                if ($e->getCode() === 409) {
                    $flow = ['progress' => 0, 'started' => $now, 'pending' => 0, 'hashes' => []];
                    $error = 'These checkpoints were already claimed. Please start a new claim.';
                } else {
                    $flow['progress'] = GIVEAWAY_STEPS - 1;
                    array_pop($flow['hashes']);
                    $error = 'Could not issue your key. Please retry the last checkpoint.';
                }
            }
        }
    } else {
        $flow['pending'] = 0;
        $error = 'Linkvertise did not verify this step. Please try again.';
    }
    $_SESSION['free_notice'] = $notice;
    $_SESSION['free_error'] = $error;
    free_redirect();
}

if ($_SERVER['REQUEST_METHOD'] === 'POST') {
    if (!hash_equals((string)$_SESSION['csrf'], (string)($_POST['csrf'] ?? ''))) {
        http_response_code(400);
        exit('Expired form. Refresh and try again.');
    }
    $action = (string)($_POST['action'] ?? '');
    if ($action === 'restart' && empty($flow['issued'])) {
        $flow = ['progress' => 0, 'started' => $now, 'pending' => 0, 'hashes' => []];
        free_redirect();
    }
    if ($action === 'go' && $ready && empty($flow['issued'])) {
        $step = (int)($_POST['step'] ?? 0);
        if ($step === (int)$flow['progress'] + 1 && $step >= 1 && $step <= GIVEAWAY_STEPS) {
            $flow['pending'] = $step;
            $url = (string)$settings['links'][$step - 1];
            session_write_close();
            header('Location: ' . $url, true, 303);
            exit;
        }
    }
    free_redirect();
}

$error = $error ?: (string)($_SESSION['free_error'] ?? '');
$notice = $notice ?: (string)($_SESSION['free_notice'] ?? '');
unset($_SESSION['free_error'], $_SESSION['free_notice']);
$progress = min(GIVEAWAY_STEPS, max(0, (int)$flow['progress']));
$issued = (!empty($flow['issued']) && is_array($flow['issued'])) ? $flow['issued'] : null;
?>
<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1, viewport-fit=cover">
<meta name="theme-color" content="#ece5ff">
<title>Free 10-Hour Key | SHAI ETHNIR</title>
<link rel="preconnect" href="https://fonts.googleapis.com">
<link rel="preconnect" href="https://fonts.gstatic.com" crossorigin>
<link href="https://fonts.googleapis.com/css2?family=Manrope:wght@400;500;600;700;800&display=swap" rel="stylesheet">
<style>
:root {
  --bg:#ece5ff;--bg2:#dccfff;--panel:#f8f4ff;--ink:#221938;--muted:#6b5f8f;--violet:#6d28d9;--soft:#eedfff;
  --clay:14px 14px 32px rgba(109,40,217,.15),-10px -10px 26px rgba(255,255,255,.9),inset 2px 2px 4px rgba(255,255,255,.8),inset -2px -2px 6px rgba(109,40,217,.07);
  --raise:8px 8px 18px rgba(109,40,217,.13),-6px -6px 14px rgba(255,255,255,.9);
  --sink:inset 4px 4px 10px rgba(109,40,217,.13),inset -4px -4px 10px rgba(255,255,255,.92);
}
* {box-sizing:border-box} html,body{min-height:100%}body{margin:0;font:15px/1.6 'Manrope',ui-sans-serif,system-ui,-apple-system,'Segoe UI',Roboto,sans-serif;-webkit-font-smoothing:antialiased;color:var(--ink);background:radial-gradient(900px 520px at 8% -12%,#f7f1ff 0%,rgba(247,241,255,0) 62%),linear-gradient(172deg,var(--bg),var(--bg2));background-attachment:fixed}
main{max-width:650px;margin:0 auto;padding:35px 16px 70px}
.head{text-align:center;margin:0 0 26px}.brand{display:flex;justify-content:center;gap:12px;align-items:center;margin:0 0 18px}.logo{width:44px;height:44px;border-radius:16px;background:linear-gradient(148deg,#a78bfa,#6d28d9);box-shadow:var(--raise);display:grid;place-items:center;color:#fff;font-size:14px;font-weight:800;letter-spacing:.02em}.brandtext{text-align:left}.brandtext strong{display:block;line-height:1.2;letter-spacing:.04em}.brandtext small{color:var(--muted);font-size:11px;font-weight:700;letter-spacing:.12em}
h1{font-size:clamp(23px,5vw,32px);margin:0 0 8px;letter-spacing:-.025em;line-height:1.15}.sub{margin:0;color:var(--muted);font-size:13px}
.pill{display:inline-block;padding:9px 17px;border-radius:99px;background:#e8dafe;box-shadow:var(--sink);font-size:11px;font-weight:800;letter-spacing:.06em;color:var(--violet);margin-bottom:14px}
.clay{background:var(--panel);box-shadow:var(--clay);border-radius:30px;padding:24px;margin-bottom:19px}.hero{display:flex;justify-content:space-between;align-items:center;gap:16px}.eyebrow{font-size:11px;font-weight:800;text-transform:uppercase;letter-spacing:.12em;color:var(--muted)}.huge{font-size:32px;font-weight:800;line-height:1.25;letter-spacing:-.025em}.fine{font-size:12px;color:var(--muted);margin:6px 0 0}.clock{flex-shrink:0;width:95px;height:95px;border-radius:32px;background:linear-gradient(148deg,#a78bfa,#6d28d9);display:grid;place-items:center;color:white;box-shadow:var(--raise)}.clock svg{width:45px;height:45px}
.progrow{display:flex;gap:9px;margin:16px 0 24px}.segment{height:10px;flex:1;border-radius:10px;background:#f1eaff;box-shadow:var(--sink)}.segment.done{background:linear-gradient(95deg,#8b5cf6,#6d28d9);box-shadow:var(--raise)}
.step{padding:16px 17px;border-radius:22px;background:#f1eaff;box-shadow:var(--raise);margin:16px 0;display:flex;gap:13px;align-items:center}.stepnum{height:43px;width:43px;display:grid;place-items:center;border-radius:16px;flex:none;font-size:16px;font-weight:800;color:#6d28d9;background:#e8dafe;box-shadow:var(--sink)}.step.done .stepnum{color:#0b5c44;background:#cdf7e6}.step.active{outline:2px solid #8b5cf6;background:#f8f4ff}.stepinfo{flex:1;min-width:0}.stepinfo strong{display:block;font-size:13px}.stepinfo span{font-size:11px;color:var(--muted)}.state{font-weight:800;font-size:11px;color:#6d28d9}.step.done .state{color:#0b5c44}
button{font:inherit;cursor:pointer}.btn{width:100%;padding:15px;border:0;min-height:46px;border-radius:16px;color:#fff;font-weight:700;font-size:13.5px;background:linear-gradient(148deg,#8b5cf6,#6d28d9);box-shadow:10px 10px 22px rgba(109,40,217,.26),-6px -6px 16px rgba(255,255,255,.65),inset 1px 1px 2px rgba(255,255,255,.4);transition:transform .16s,box-shadow .16s}.btn:active{transform:translateY(1px);box-shadow:inset 3px 3px 8px rgba(49,16,110,.35)}.btn:focus-visible{outline:3px solid rgba(109,40,217,.4);outline-offset:2px}.btn:disabled{opacity:.5;cursor:not-allowed}.btnquiet{background:var(--panel);color:var(--violet);box-shadow:var(--raise);margin-top:12px}.msg{padding:13px 16px;border-radius:16px;font-size:13.5px;font-weight:600;margin-bottom:18px;box-shadow:var(--sink)}.ok{background:#cdf7e6;color:#0b5c44}.bad{background:#ffd9e7;color:#8f1d47}.keybox{border-radius:16px;padding:14px 16px;background:#fbf8ff;box-shadow:var(--sink);word-break:break-all;font:600 12.5px/1.5 ui-monospace,SFMono-Regular,Menlo,Consolas,monospace;letter-spacing:.02em;margin:16px 0}.tag{display:inline-flex;padding:7px 11px;background:#cdf7e6;color:#0b5c44;border-radius:99px;font-weight:800;font-size:11px}.offline{text-align:center}.offline h2{margin:3px 0 12px}.offline p{color:var(--muted);font-size:13px}
footer{text-align:center;font-size:11px;color:var(--muted);margin-top:24px}@media(max-width:420px){main{padding:24px 12px 55px}.clay{padding:17px 15px;border-radius:24px}.clock{height:75px;width:75px;border-radius:26px}.clock svg{height:34px;width:34px}.step{padding:13px}.stepnum{width:38px;height:38px;border-radius:13px}.huge{font-size:27px}}@media(prefers-reduced-motion:reduce){*{transition:none!important}}
</style>
</head>
<body>
<main>
  <header class="head">
    <div class="brand"><div class="logo" aria-hidden="true">SE</div><div class="brandtext"><strong>SHAI ETHNIR</strong><small>FREE LICENSE PORTAL</small></div></div>
    <div class="pill">FREE ACCESS · THREE CHECKPOINTS</div>
    <h1>Your key, your access.</h1>
    <p class="sub">Complete three Linkvertise checkpoints to receive a 10-hour Free key.</p>
  </header>
  <section class="clay hero" aria-label="Free key details">
    <div><div class="eyebrow">Access duration</div><div class="huge">10 hours</div><p class="fine">Starts when the key is issued · One device</p></div>
    <div class="clock" aria-hidden="true"><svg viewBox="0 0 48 48" fill="none" stroke="currentColor" stroke-width="3" stroke-linecap="round"><circle cx="24" cy="24" r="17"/><path d="M24 14v11l8 5"/></svg></div>
  </section>
  <?php if ($error): ?><p class="msg bad" role="alert"><?= free_h($error) ?></p><?php endif; ?>
  <?php if ($notice): ?><p class="msg ok" role="status"><?= free_h($notice) ?></p><?php endif; ?>
  <?php if (!$ready): ?>
    <section class="clay offline"><h2>Giveaway is not active</h2><p>The owner has not enabled the three ad links yet.</p></section>
  <?php elseif ($issued): ?>
    <section class="clay"><span class="tag">3 / 3 VERIFIED</span><h2>Your Free key is ready</h2><p class="fine">Your license expires on <?= free_h(gmdate('M j, Y · H:i', (int)$issued['expires'])) ?> UTC.</p>
      <div class="keybox" id="your-key"><?= free_h((string)$issued['key']) ?></div>
      <button class="btn" type="button" id="copy-key">Copy free key</button>
      <p class="fine" id="remaining" data-expires="<?= (int)$issued['expires'] ?>">Valid for 10 hours from issuance.</p>
    </section>
  <?php else: ?>
    <section class="clay">
      <div class="eyebrow">Claim progress</div><div class="huge"><?= $progress ?> <span style="font-size:17px;color:var(--muted)">/ <?= GIVEAWAY_STEPS ?> completed</span></div>
      <div class="progrow" aria-label="<?= $progress ?> of 3 checkpoints complete">
        <?php for ($i=1;$i<=GIVEAWAY_STEPS;$i++): ?><span class="segment <?= $i<=$progress?'done':'' ?>"></span><?php endfor; ?>
      </div>
      <?php for ($i=1;$i<=GIVEAWAY_STEPS;$i++): ?>
      <div class="step <?= $i<=$progress?'done':($i===$progress+1?'active':'') ?>">
        <div class="stepnum"><?= $i<=$progress?'&#10003;':$i ?></div>
        <div class="stepinfo"><strong>Linkvertise checkpoint <?= $i ?></strong><span><?= $i<=$progress?'Completion verified':($i===$progress+1?'Ready to open':'Unlock the previous checkpoint first') ?></span></div>
        <span class="state"><?= $i<=$progress?'DONE':($i===$progress+1?'NEXT':'LOCKED') ?></span>
      </div>
      <?php endfor; ?>
      <form method="post"><input type="hidden" name="csrf" value="<?= free_h((string)$_SESSION['csrf']) ?>"><input type="hidden" name="step" value="<?= $progress+1 ?>"><button class="btn" name="action" value="go">Complete checkpoint <?= $progress+1 ?> of 3</button></form>
      <?php if ($progress>0): ?><form method="post"><input type="hidden" name="csrf" value="<?= free_h((string)$_SESSION['csrf']) ?>"><button class="btn btnquiet" name="action" value="restart">Restart progress</button></form><?php endif; ?>
      <p class="fine">After every ad, return to this page using the Linkvertise destination. Keep the same browser tab.</p>
    </section>
  <?php endif; ?>
  <footer>SHAI ETHNIR · Free keys expire automatically · Premium licenses are managed separately</footer>
</main>
<script>
const copy = document.getElementById('copy-key');
if (copy) copy.addEventListener('click', async () => {
  const value = document.getElementById('your-key').textContent;
  try { await navigator.clipboard.writeText(value); copy.textContent='Copied to clipboard'; }
  catch (_) { const range=document.createRange();range.selectNodeContents(document.getElementById('your-key'));const sel=window.getSelection();sel.removeAllRanges();sel.addRange(range);copy.textContent='Key selected. Copy it'; }
});
const remain = document.getElementById('remaining');
if (remain) {
  function tick(){let s=Math.max(0,Number(remain.dataset.expires)-Math.floor(Date.now()/1000));let h=Math.floor(s/3600),m=Math.floor((s%3600)/60);remain.textContent=s===0?'Key expired. Refresh to start a new claim.':'Time remaining: '+h+'h '+m+'m';}
  tick();setInterval(tick,30000);
}
</script>
</body>
</html>
