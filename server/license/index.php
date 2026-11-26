<?php

declare(strict_types=1);

header('Location: admin.php', true, 302);
?>
<?php require_once __DIR__ . '/theme.php'; ?>
<!doctype html>
<html lang="en">
<head>
<?= ui_head('Licence panel') ?>
<meta http-equiv="refresh" content="0; url=admin.php">
</head>
<body>
<main class="centered">
    <section class="clay panel" style="max-width:386px;text-align:center">
        <span class="mark" aria-hidden="true" style="margin:0 auto 14px">EK</span>
        <h1 style="font-size:22px">Licence panel</h1>
        <p class="hint">Redirecting to the panel&hellip;</p>
        <a class="btn primary block" href="admin.php">Open the panel</a>
    </section>
</main>
</body>
</html>
