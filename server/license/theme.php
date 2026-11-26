<?php

declare(strict_types=1);

function ui_css(): string
{
    return <<<'CSS'
:root{
  --bg:#ece5ff;--bg-2:#dccfff;--surface:#f8f4ff;--surface-2:#f1eaff;
  --ink:#221938;--muted:#6b5f8f;--violet:#6d28d9;--violet-2:#8b5cf6;
  --mint:#cdf7e6;--mint-ink:#0b5c44;--rose:#ffd9e7;--rose-ink:#8f1d47;
  --line:rgba(109,40,217,.12);
  --r-xl:30px;--r-lg:22px;--r-md:16px;--r-sm:12px;
  --clay:14px 14px 32px rgba(109,40,217,.15),-10px -10px 26px rgba(255,255,255,.9),inset 2px 2px 4px rgba(255,255,255,.8),inset -2px -2px 6px rgba(109,40,217,.07);
  --clay-sm:8px 8px 18px rgba(109,40,217,.13),-6px -6px 14px rgba(255,255,255,.9);
  --sunk:inset 4px 4px 10px rgba(109,40,217,.13),inset -4px -4px 10px rgba(255,255,255,.92);
  color-scheme:light}
*{box-sizing:border-box}
html,body{min-height:100%;max-width:100%;overflow-x:hidden}
body{margin:0;background:radial-gradient(900px 520px at 8% -12%,#f7f1ff 0%,rgba(247,241,255,0) 62%),linear-gradient(172deg,var(--bg) 0%,var(--bg-2) 100%);background-attachment:fixed;color:var(--ink);font:15px/1.6 "Manrope",ui-sans-serif,system-ui,-apple-system,"Segoe UI",Roboto,sans-serif;-webkit-font-smoothing:antialiased;overflow-x:hidden}
::selection{background:var(--violet);color:#fff}
a{color:var(--violet)}
h1,h2,h3,h4{letter-spacing:-.025em}
code,.mono{font:12.5px/1.5 ui-monospace,SFMono-Regular,Menlo,Consolas,monospace;word-break:break-all}
.dim{color:var(--muted)}
.wrap{max-width:1020px;margin:0 auto;padding:26px 18px 72px}
.wrap.narrow{max-width:680px}
.centered{min-height:100dvh;display:grid;place-items:center;padding:24px 16px 44px}
.clay{background:var(--surface);border-radius:var(--r-xl);box-shadow:var(--clay)}
@keyframes rise{from{opacity:0;transform:translateY(14px)}to{opacity:1;transform:none}}
.topbar{display:flex;align-items:center;justify-content:space-between;gap:16px;flex-wrap:wrap;padding:16px 20px;margin:0 0 20px;animation:rise .5s ease both}
.brand{display:flex;align-items:center;gap:13px}
.mark{display:grid;place-items:center;width:44px;height:44px;border-radius:var(--r-md);background:linear-gradient(148deg,#a78bfa,#6d28d9);box-shadow:var(--clay-sm);color:#fff;font-weight:800;font-size:14px;letter-spacing:.02em}
.brand h1{margin:0;font-size:17px}
.brand p{margin:1px 0 0;color:var(--muted);font-size:12.5px}
.topbar-actions{display:flex;align-items:center;flex-wrap:wrap;gap:10px;min-width:0}
.panel{padding:22px 22px 22px;margin:0 0 20px;animation:rise .55s ease both}
.panel h1{margin:0 0 6px;font-size:28px}
.panel-head{display:flex;align-items:baseline;justify-content:space-between;gap:12px;margin:0 0 16px}
.panel-head h2,.panel>h2{margin:0 0 12px;font-size:15px}
.panel-head h2{margin:0}
.panel p{margin:0 0 14px}
.hint{color:var(--muted);font-size:13.5px}
.fine{color:var(--muted);font-size:12.5px}
label.field{display:block;margin:0 0 16px}
.field>span,.label-text{display:block;margin:0 0 7px 4px;font-size:12.5px;font-weight:600;color:var(--muted)}
input,select,textarea{width:100%;padding:14px 16px;font:inherit;color:var(--ink);background:#fbf8ff;border:0;border-radius:var(--r-md);box-shadow:var(--sunk);transition:box-shadow .18s ease,background .18s ease;-webkit-appearance:none}
textarea{font:12.5px/1.5 ui-monospace,SFMono-Regular,Menlo,Consolas,monospace;min-height:130px;resize:vertical}
input[type=file]{padding:11px 14px}
input::placeholder,textarea::placeholder{color:#a498c2}
input:focus,select:focus,textarea:focus{outline:none;background:#fff;box-shadow:var(--sunk),0 0 0 3px rgba(109,40,217,.25)}
.row{display:flex;gap:14px;flex-wrap:wrap}
.row>*{flex:1 1 220px}
.btn{position:relative;display:inline-flex;align-items:center;justify-content:center;gap:8px;min-height:46px;padding:12px 20px;font:700 13.5px/1 "Manrope",ui-sans-serif,system-ui,sans-serif;color:var(--ink);background:var(--surface);border:0;border-radius:var(--r-md);cursor:pointer;text-decoration:none;box-shadow:var(--clay-sm);transition:transform .15s ease,box-shadow .15s ease}
.btn:hover{transform:translateY(-1px)}
.btn:active{transform:translateY(1px);box-shadow:var(--sunk)}
.btn:focus-visible{outline:3px solid rgba(109,40,217,.4);outline-offset:2px}
.btn.primary{color:#fff;background:linear-gradient(148deg,#8b5cf6,#6d28d9);box-shadow:10px 10px 22px rgba(109,40,217,.26),-6px -6px 16px rgba(255,255,255,.65),inset 1px 1px 2px rgba(255,255,255,.4)}
.btn.primary:active{box-shadow:inset 3px 3px 8px rgba(49,16,110,.35)}
.btn.quiet{color:var(--violet)}
.btn.danger{color:var(--rose-ink);background:var(--rose)}
.btn.small{min-height:36px;padding:8px 13px;font-size:12.5px;border-radius:var(--r-sm)}
.btn.block{width:100%}
form.inline{display:inline}
.alert{margin:0 0 18px;padding:13px 15px;border-radius:var(--r-md);font-weight:600;font-size:13.5px;animation:rise .3s ease both}
.alert.bad{background:var(--rose);color:var(--rose-ink);box-shadow:var(--sunk)}
.alert.good{background:var(--mint);color:var(--mint-ink);box-shadow:var(--sunk)}
.alert.warn{background:#fff1cf;color:#6b4a00;box-shadow:var(--sunk)}
.table-wrap{position:relative;border-radius:var(--r-lg);background:#fbf8ff;box-shadow:var(--sunk);padding:6px;overflow-x:auto;margin:0 0 14px}
table{width:100%;border-collapse:separate;border-spacing:0 5px;font-size:13.5px}
thead th{padding:9px 13px;text-align:left;font-size:12px;font-weight:600;color:var(--muted);white-space:nowrap}
tbody tr{background:var(--surface-2);box-shadow:0 5px 12px rgba(109,40,217,.06)}
tbody td{padding:13px;vertical-align:middle}
tbody tr td:first-child{border-radius:var(--r-sm) 0 0 var(--r-sm)}
tbody tr td:last-child{border-radius:0 var(--r-sm) var(--r-sm) 0}
.pill{display:inline-block;padding:4px 10px;border-radius:999px;font-size:11.5px;font-weight:700;white-space:nowrap;background:#e8dafe;color:#6728a7}
.pill.ok{background:var(--mint);color:var(--mint-ink)}
.pill.bad{background:var(--rose);color:var(--rose-ink)}
.keybox{padding:14px 16px;border-radius:var(--r-md);background:#fbf8ff;box-shadow:var(--sunk);user-select:all;margin:0 0 14px}
.empty{color:var(--muted);font-size:13.5px}
@media(max-width:520px){.wrap{padding:18px 12px 56px}.topbar{padding:14px}.topbar-actions{width:100%}.topbar-actions .btn{flex:1}.panel{padding:18px 16px}}
@media(prefers-reduced-motion:reduce){*{animation:none!important;transition:none!important}}
CSS;
}

function ui_head(string $title, string $themeColor = '#ece5ff'): string
{
    return '<meta charset="utf-8">'
        . '<meta name="viewport" content="width=device-width, initial-scale=1, viewport-fit=cover">'
        . '<meta name="theme-color" content="' . $themeColor . '">'
        . '<title>' . htmlspecialchars($title, ENT_QUOTES, 'UTF-8') . '</title>'
        . '<link rel="preconnect" href="https://fonts.googleapis.com">'
        . '<link rel="preconnect" href="https://fonts.gstatic.com" crossorigin>'
        . '<link rel="stylesheet" href="https://fonts.googleapis.com/css2?family=Manrope:wght@400;500;600;700;800&display=swap">'
        . '<style>' . ui_css() . '</style>';
}
