$ErrorActionPreference = 'Stop'

$projectRoot = Split-Path -Parent $PSScriptRoot
$ioc = Get-Content (Join-Path $projectRoot 'PanViewF407.ioc') -Raw
$usart = Get-Content (Join-Path $projectRoot 'Core/Src/usart.c') -Raw
$main = Get-Content (Join-Path $projectRoot 'Core/Src/main.c') -Raw

if ($ioc -notmatch '(?m)^PA3\.GPIO_PuPd=GPIO_PULLUP$') {
    throw 'PA3 must be configured with an internal pull-up in the CubeMX source of truth.'
}

$usart2Block = [regex]::Match($usart, 'else if\(uartHandle->Instance==USART2\)(?<body>.*?)(?=\n\s*else if\(uartHandle->Instance==USART3\))', [System.Text.RegularExpressions.RegexOptions]::Singleline).Groups['body'].Value
if ($usart2Block -notmatch 'GPIO_InitStruct\.Pull = GPIO_PULLUP;') {
    throw 'USART2 RX must use a pull-up while K230 is unpowered.'
}

if ($main -notmatch 'void HAL_UART_ErrorCallback\(UART_HandleTypeDef \*huart\)') {
    throw 'UART error callback must recover a stopped DMA reception.'
}

if ($main -notmatch 'k230_uart_rx_restart_pending = true;') {
    throw 'K230 UART errors must request a recovery retry.'
}

$armIndex = $main.IndexOf('(void)StartK230UartRxDma();')
$probeIndex = $main.IndexOf('if (MotorTtlProbe())')
if (($armIndex -lt 0) -or ($probeIndex -lt 0) -or ($armIndex -gt $probeIndex)) {
    throw 'K230 DMA must be armed before blocking motor diagnostics.'
}

$startFunction = [regex]::Match($main, 'static bool StartK230UartRxDma\(void\)(?<body>.*?)(?=\n\s*/\*)', [System.Text.RegularExpressions.RegexOptions]::Singleline).Groups['body'].Value
if ($startFunction -match 'Error_Handler\(\)') {
    throw 'K230 DMA startup must not permanently trap the MCU on a transient link error.'
}

Write-Output 'PASS: K230 startup recovery contracts are present.'
