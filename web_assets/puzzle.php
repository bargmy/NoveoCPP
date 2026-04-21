<?php
declare(strict_types=1);

$backendBaseUrl = 'https://noveo.ir:8443';

function send_json_error(int $status, string $message): never {
    http_response_code($status);
    header('Content-Type: application/json; charset=UTF-8');
    echo json_encode(['error' => $message], JSON_UNESCAPED_UNICODE | JSON_UNESCAPED_SLASHES);
    exit;
}

function get_request_header_value(string $name): string {
    $serverKey = 'HTTP_' . strtoupper(str_replace('-', '_', $name));
    return isset($_SERVER[$serverKey]) ? trim((string) $_SERVER[$serverKey]) : '';
}

if (isset($_GET['proxy'])) {
    $target = trim((string) ($_GET['target'] ?? ''));
    if ($target === '' || strncmp($target, '/captcha/', 9) !== 0) {
        send_json_error(400, 'Invalid captcha proxy target.');
    }
    if (preg_match('/[\r\n]/', $target)) {
        send_json_error(400, 'Invalid captcha proxy target.');
    }

    $method = strtoupper((string) ($_SERVER['REQUEST_METHOD'] ?? 'GET'));
    if (!in_array($method, ['GET', 'POST'], true)) {
        send_json_error(405, 'Method not allowed.');
    }

    if (!function_exists('curl_init')) {
        send_json_error(500, 'cURL is required for captcha proxying.');
    }

    $curl = curl_init($backendBaseUrl . $target);
    if ($curl === false) {
        send_json_error(500, 'Failed to initialize captcha proxy.');
    }

    $forwardHeaders = ['Accept: application/json'];
    $contentType = trim((string) ($_SERVER['CONTENT_TYPE'] ?? ''));
    if ($contentType !== '') {
        $forwardHeaders[] = 'Content-Type: ' . $contentType;
    }
    $forwardedFor = trim((string) ($_SERVER['HTTP_X_FORWARDED_FOR'] ?? ''));
    if ($forwardedFor === '') {
        $forwardedFor = trim((string) ($_SERVER['REMOTE_ADDR'] ?? ''));
    }
    if ($forwardedFor !== '') {
        $forwardHeaders[] = 'X-Forwarded-For: ' . $forwardedFor;
        $forwardHeaders[] = 'X-Real-IP: ' . $forwardedFor;
    }

    $authToken = get_request_header_value('X-Auth-Token');
    if ($authToken !== '') {
        $forwardHeaders[] = 'X-Auth-Token: ' . $authToken;
    }
    $userId = get_request_header_value('X-User-ID');
    if ($userId !== '') {
        $forwardHeaders[] = 'X-User-ID: ' . $userId;
    }

    curl_setopt($curl, CURLOPT_RETURNTRANSFER, true);
    curl_setopt($curl, CURLOPT_CUSTOMREQUEST, $method);
    curl_setopt($curl, CURLOPT_HTTPHEADER, $forwardHeaders);
    curl_setopt($curl, CURLOPT_HEADER, true);
    curl_setopt($curl, CURLOPT_FOLLOWLOCATION, false);
    curl_setopt($curl, CURLOPT_CONNECTTIMEOUT, 10);
    curl_setopt($curl, CURLOPT_TIMEOUT, 30);

    if ($method === 'POST') {
        $body = file_get_contents('php://input');
        curl_setopt($curl, CURLOPT_POSTFIELDS, $body === false ? '' : $body);
    }

    $rawResponse = curl_exec($curl);
    if ($rawResponse === false) {
        $error = curl_error($curl);
        curl_close($curl);
        send_json_error(502, $error !== '' ? $error : 'Captcha proxy request failed.');
    }

    $statusCode = (int) curl_getinfo($curl, CURLINFO_RESPONSE_CODE);
    $headerSize = (int) curl_getinfo($curl, CURLINFO_HEADER_SIZE);
    $responseHeaders = substr($rawResponse, 0, $headerSize);
    $responseBody = substr($rawResponse, $headerSize);
    curl_close($curl);

    $responseContentType = 'application/json; charset=UTF-8';
    foreach (preg_split('/\r\n|\r|\n/', (string) $responseHeaders) as $headerLine) {
        if (stripos($headerLine, 'Content-Type:') === 0) {
            $responseContentType = trim(substr($headerLine, strlen('Content-Type:')));
            break;
        }
    }

    http_response_code($statusCode > 0 ? $statusCode : 502);
    header('Content-Type: ' . $responseContentType);
    echo $responseBody;
    exit;
}

if (!function_exists('curl_init')) {
    http_response_code(500);
    header('Content-Type: text/plain; charset=UTF-8');
    echo 'cURL is required for puzzle proxying.';
    exit;
}

$templateCurl = curl_init($backendBaseUrl . '/puzzle.html');
if ($templateCurl === false) {
    http_response_code(500);
    header('Content-Type: text/plain; charset=UTF-8');
    echo 'Failed to initialize puzzle template request.';
    exit;
}

curl_setopt($templateCurl, CURLOPT_RETURNTRANSFER, true);
curl_setopt($templateCurl, CURLOPT_FOLLOWLOCATION, false);
curl_setopt($templateCurl, CURLOPT_CONNECTTIMEOUT, 10);
curl_setopt($templateCurl, CURLOPT_TIMEOUT, 30);

$template = curl_exec($templateCurl);
$templateStatus = (int) curl_getinfo($templateCurl, CURLINFO_RESPONSE_CODE);
$templateError = curl_error($templateCurl);
curl_close($templateCurl);

if (!is_string($template) || $template === '' || $templateStatus < 200 || $templateStatus >= 300) {
    http_response_code(500);
    header('Content-Type: text/plain; charset=UTF-8');
    echo $templateError !== '' ? $templateError : 'Failed to load remote puzzle template.';
    exit;
}

$proxyHelper = <<<'JS'
        function buildCaptchaProxyUrl(path) {
            const proxyUrl = new URL(window.location.pathname, window.location.origin);
            proxyUrl.search = '';
            proxyUrl.searchParams.set('proxy', '1');
            proxyUrl.searchParams.set('target', String(path || ''));
            return proxyUrl.toString();
        }

JS;

$captchaStyleOverride = <<<'CSS'
        .hero,
        .status {
            display: none !important;
        }

        body {
            padding: 0;
            background: transparent;
        }

        .shell {
            width: 100%;
            max-width: 100%;
            border-radius: 0;
            border: 0;
            box-shadow: none;
            background: transparent;
            backdrop-filter: none;
        }

        .content {
            padding: 18px;
        }
CSS;

$template = str_replace(
    "function notifyParent(type, payload = {}) {",
    $proxyHelper . "        function notifyParent(type, payload = {}) {",
    $template
);

$template = str_replace(
    "</style>",
    $captchaStyleOverride . "\n    </style>",
    $template
);

$template = str_replace(
    "const response = await fetch(path, { ...options, headers });",
    "const response = await fetch(buildCaptchaProxyUrl(path), { ...options, headers });",
    $template
);

header('Content-Type: text/html; charset=UTF-8');
echo $template;
