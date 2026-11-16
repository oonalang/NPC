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
  --bg:#ece5ff;--bg2:#dbccff;--panel:#f8f4ff;--ink:#25193e;--muted:#75678e;--violet:#7535df;--soft:#eedfff;
  --clay:16px 16px 30px rgba(103,63,177,.15),-12px -12px 25px rgba(255,255,255,.92),inset 2px 2px 4px rgba(255,255,255,.85),inset -3px -3px 7px rgba(127,80,218,.09);
  --raise:8px 8px 17px rgba(112,78,170,.16),-7px -7px 17px #fff,inset 1px 1px 2px #fff;
  --sink:inset 5px 5px 10px rgba(126,80,190,.15),inset -5px -5px 11px #fff;
}
* {box-sizing:border-box} html,body{min-height:100%}body{margin:0;font:15px/1.55 'Manrope',system-ui,sans-serif;color:var(--ink);background:radial-gradient(circle at 15% 10%,#fffafa 0%,transparent 36%),radial-gradient(circle at 90% 85%,#d0c3ff 0%,transparent 48%),linear-gradient(145deg,var(--bg),var(--bg2));background-attachment:fixed}
main{max-width:650px;margin:0 auto;padding:35px 16px 70px}
.head{text-align:center;margin:0 0 26px}.brand{display:flex;justify-content:center;gap:12px;align-items:center;margin:0 0 18px}.logo{width:55px;height:55px;border-radius:20px;background:linear-gradient(140deg,#be9aff,#7d3fea);box-shadow:6px 7px 14px #ad91d4,-5px -6px 13px #fff,inset 2px 2px 4px #e9d5ff;display:grid;place-items:center;color:#fff;font-size:23px;font-weight:800}.brandtext{text-align:left}.brandtext strong{display:block;line-height:1.2;letter-spacing:.04em}.brandtext small{color:var(--muted);font-size:11px;font-weight:700;letter-spacing:.12em}
h1{font-size:clamp(23px,5vw,34px);margin:0 0 8px;letter-spacing:-.045em;line-height:1.15}.sub{margin:0;color:var(--muted);font-size:13px}
.pill{display:inline-block;padding:9px 17px;border-radius:99px;background:#e5d6ff;box-shadow:var(--sink);font-size:11px;font-weight:800;letter-spacing:.06em;color:var(--violet);margin-bottom:14px}
.clay{background:var(--panel);box-shadow:var(--clay);border-radius:30px;padding:24px;margin-bottom:19px}.hero{display:flex;justify-content:space-between;align-items:center;gap:16px}.eyebrow{font-size:11px;font-weight:800;text-transform:uppercase;letter-spacing:.12em;color:var(--muted)}.huge{font-size:32px;font-weight:800;line-height:1.25;letter-spacing:-.05em}.fine{font-size:12px;color:var(--muted);margin:6px 0 0}.clock{flex-shrink:0;width:95px;height:95px;border-radius:32px;background:linear-gradient(145deg,#c3a7ff,#955deb);display:grid;place-items:center;color:white;box-shadow:8px 8px 18px #b5a0df,-7px -7px 14px #fff,inset 2px 2px 4px #e7d9ff}.clock svg{width:45px;height:45px}
.progrow{display:flex;gap:9px;margin:16px 0 24px}.segment{height:10px;flex:1;border-radius:10px;background:#ece3ff;box-shadow:var(--sink)}.segment.done{background:linear-gradient(95deg,#9d68f0,#7d3fea);box-shadow:inset 1px 2px 3px #7240b3,2px 2px 4px #d2baed}
.step{padding:16px 17px;border-radius:22px;background:#f3edff;box-shadow:var(--raise);margin:16px 0;display:flex;gap:13px;align-items:center}.stepnum{height:43px;width:43px;display:grid;place-items:center;border-radius:16px;flex:none;font-size:16px;font-weight:800;color:#7a42cb;background:#e9dcff;box-shadow:var(--sink)}.step.done .stepnum{color:#1b7453;background:#d8f4e9}.step.active{outline:2px solid #b48df7;background:#f8f4ff}.stepinfo{flex:1;min-width:0}.stepinfo strong{display:block;font-size:13px}.stepinfo span{font-size:11px;color:var(--muted)}.state{font-weight:800;font-size:11px;color:#7542ba}.step.done .state{color:#20745a}
button{font:inherit;cursor:pointer}.btn{width:100%;padding:15px;border:0;border-radius:17px;color:white;font-weight:800;font-size:13px;background:linear-gradient(150deg,#a771f5,#6f2ed5);box-shadow:7px 7px 16px rgba(102,53,187,.29),-5px -5px 14px #fff,inset 2px 2px 2px #cbaaff,inset -2px -3px 4px #6d2db3;transition:transform .16s,box-shadow .16s}.btn:active{transform:translateY(2px);box-shadow:var(--sink)}.btn:focus-visible{outline:3px solid #b488fa;outline-offset:3px}.btn:disabled{opacity:.5;cursor:not-allowed}.btnquiet{background:#eadfff;color:#6c32be;box-shadow:var(--raise);margin-top:12px}.msg{padding:13px 16px;border-radius:17px;font-size:12px;margin-bottom:16px;box-shadow:var(--sink)}.ok{background:#dbf7ed;color:#216b4e}.bad{background:#ffe1eb;color:#8d3053}.keybox{border-radius:16px;padding:15px;background:#eee6fa;box-shadow:var(--sink);word-break:break-all;font:600 13px/1.5 ui-monospace,Consolas,monospace;letter-spacing:.02em;margin:16px 0}.tag{display:inline-flex;padding:7px 11px;background:#d8f8e9;color:#126b50;border-radius:99px;font-weight:800;font-size:11px}.offline{text-align:center}.offline h2{margin:3px 0 12px}.offline p{color:var(--muted);font-size:13px}
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
