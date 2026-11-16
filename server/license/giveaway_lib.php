<?php

declare(strict_types=1);

require_once __DIR__ . '/lib.php';

const GIVEAWAY_HOURS = 10;
const GIVEAWAY_STEPS = 3;

function giveaway_path(): string
{
    return __DIR__ . '/data/giveaway.json';
}

function giveaway_defaults(): array
{
    return [
        'enabled' => false,
        'links' => ['', '', ''],
        'antibypass_token' => '',
    ];
}

function giveaway_settings(): array
{
    $path = giveaway_path();
    $raw = is_file($path) ? @file_get_contents($path) : false;
    $data = $raw !== false ? json_decode($raw, true) : null;
    $result = array_replace(giveaway_defaults(), is_array($data) ? $data : []);
    $result['links'] = array_slice(array_pad(is_array($result['links']) ? $result['links'] : [], GIVEAWAY_STEPS, ''), 0, GIVEAWAY_STEPS);
    return $result;
}

function giveaway_valid_url(string $url): bool
{
    if (strlen($url) > 2048 || !filter_var($url, FILTER_VALIDATE_URL)) {
        return false;
    }
    $parts = parse_url($url);
    if (!is_array($parts) || strtolower((string)($parts['scheme'] ?? '')) !== 'https') {
        return false;
    }
    $host = strtolower((string)($parts['host'] ?? ''));
    return in_array($host, ['linkvertise.com', 'www.linkvertise.com', 'link-to.net', 'www.link-to.net'], true)
        && !isset($parts['user']) && !isset($parts['pass']);
}

function giveaway_validate(array $settings): string
{
    $links = $settings['links'] ?? [];
    if (!is_array($links) || count($links) !== GIVEAWAY_STEPS) {
        return 'Configure exactly three Linkvertise URLs.';
    }
    if (count(array_unique($links)) !== GIVEAWAY_STEPS || array_filter($links, 'giveaway_valid_url') !== $links) {
        return 'Enter three distinct HTTPS Linkvertise URLs.';
    }
    $token = (string)($settings['antibypass_token'] ?? '');
    if (!preg_match('/^[a-fA-F0-9]{64}$/', $token)) {
        return 'Enter your 64-character Linkvertise anti-bypass token.';
    }
    return '';
}

function giveaway_ready(array $settings): bool
{
    return !empty($settings['enabled']) && giveaway_validate($settings) === '';
}

function giveaway_save_settings(array $settings): bool
{
    $dir = dirname(giveaway_path());
    return is_dir($dir) && licence_save(giveaway_path(), $settings);
}

function giveaway_verify_hash(string $hash, string $token): bool
{
    if (!preg_match('/^[a-fA-F0-9]{64}$/', $hash) || !preg_match('/^[a-fA-F0-9]{64}$/', $token)) {
        return false;
    }
    $url = 'https://publisher.linkvertise.com/api/v1/anti_bypassing?' . http_build_query([
        'token' => $token,
        'hash' => $hash,
    ], '', '&', PHP_QUERY_RFC3986);
    $body = false;
    $status = 0;
    if (function_exists('curl_init')) {
        $curl = curl_init($url);
        if ($curl === false) {
            return false;
        }
        curl_setopt_array($curl, [
            CURLOPT_POST => true,
            CURLOPT_POSTFIELDS => '',
            CURLOPT_RETURNTRANSFER => true,
            CURLOPT_CONNECTTIMEOUT => 3,
            CURLOPT_TIMEOUT => 7,
            CURLOPT_FOLLOWLOCATION => false,
            CURLOPT_SSL_VERIFYPEER => true,
            CURLOPT_SSL_VERIFYHOST => 2,
        ]);
        $body = curl_exec($curl);
        $status = (int)curl_getinfo($curl, CURLINFO_RESPONSE_CODE);
        curl_close($curl);
    } else {
        $ctx = stream_context_create([
            'http' => [
                'method' => 'POST', 'content' => '', 'timeout' => 7,
                'ignore_errors' => true, 'follow_location' => 0,
            ],
            'ssl' => ['verify_peer' => true, 'verify_peer_name' => true],
        ]);
        $body = @file_get_contents($url, false, $ctx);
        if (isset($http_response_header[0]) && preg_match('/^HTTP\/\S+ (\d{3})/', $http_response_header[0], $matches)) {
            $status = (int)$matches[1];
        }
    }
    return $status === 200 && is_string($body) && trim($body) === 'TRUE';
}

function giveaway_issue(string $store, array $verifiedHashes): array
{
    if (count($verifiedHashes) !== GIVEAWAY_STEPS) throw new RuntimeException('Three verified checkpoints required');
    foreach ($verifiedHashes as $hash) {
        if (!is_string($hash) || !preg_match('/^[a-fA-F0-9]{64}$/D', $hash)) {
            throw new RuntimeException('Invalid checkpoint proof');
        }
    }
    $proofs = array_map(static function (string $hash): string {
        return hash('sha256', strtolower($hash));
    }, $verifiedHashes);
    if (count(array_unique($proofs)) !== GIVEAWAY_STEPS) throw new RuntimeException('Distinct checkpoint proofs required');
    licence_lock_store($store);
    $all = licence_all($store);
    foreach ($all as $prior) {
        if (is_array($prior) && array_intersect($proofs, (array)($prior['claim_proofs'] ?? []))) {
            throw new RuntimeException('Checkpoint proof already claimed', 409);
        }
    }
    do {
        $key = 'ETHNIR-FREE-' . strtoupper(bin2hex(random_bytes(16)));
    } while (isset($all[$key]));
    $now = time();
    $record = licence_record(1, 'Linkvertise giveaway: 3 verified checkpoints', [], 1, 'free');
    $record['created'] = $now;
    $record['expires'] = $now + GIVEAWAY_HOURS * 3600;
    $record['origin'] = 'giveaway';
    $record['claim_proofs'] = $proofs;
    $all[$key] = $record;
    if (!licence_save($store, $all)) throw new RuntimeException('Unable to save free key');
    return ['key' => $key, 'expires' => $record['expires']];
}
