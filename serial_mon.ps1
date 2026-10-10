# Change 115200 if necessary.
Write-Host "Detecting available boards..." -ForegroundColor Cyan
arduino-cli board list

do {
    $portNumber = Read-Host "Enter COM port number (e.g., 9)"
} while (-not ($portNumber -match '^\d+$'))

$selectedPortName = "COM$portNumber"
# Logs are saved in the current working folder's logs subfolder using dd-mm-yy+hh-mm ordering.

$port = $null
$logWriter = $null
$logDirectory = Join-Path (Get-Location).Path 'logs'
$logPath = Join-Path $logDirectory ('climatron_{0}.txt' -f (Get-Date -Format 'dd-MM-yy+HH-mm'))

try {
    [System.IO.Directory]::CreateDirectory($logDirectory) | Out-Null
    # Append prevents overwriting an earlier log started in the same minute.
    $logWriter = New-Object System.IO.StreamWriter($logPath, $true, (New-Object System.Text.UTF8Encoding($false)))
    $logWriter.AutoFlush = $true

    $port = New-Object System.IO.Ports.SerialPort $selectedPortName,115200,None,8,One
    $port.Handshake = 'None'
    $port.ReadTimeout = 100
    $port.Open()

    $message = "Watching $($port.PortName). Press Ctrl+C to stop."
    Write-Host $message
    $logWriter.WriteLine($message)

    while ($true) {
        $text = $port.ReadExisting()

        if ($text.Length -gt 0) {
            Write-Host -NoNewline $text
            $logWriter.Write($text)
        }

        Start-Sleep -Milliseconds 50
    }
}
finally {
    # Nested finally blocks ensure both resources are disposed even if cleanup fails.
    try {
        if ($null -ne $port) {
            try {
                if ($port.IsOpen) {
                    $port.Close()
                }
            }
            finally {
                $port.Dispose()
            }
        }
    }
    finally {
        if ($null -ne $logWriter) {
            $logWriter.Dispose()
        }
    }
}