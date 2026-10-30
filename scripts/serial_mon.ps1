# Change 115200 if necessary.
Write-Host "Detecting available boards..." -ForegroundColor Cyan
arduino-cli board list

do {
    $portNumber = Read-Host "Enter COM port number (e.g., 9)"
} while (-not ($portNumber -match '^\d+$'))

$selectedPortName = "COM$portNumber"
# Logs are saved under the sketch root, independent of the current working directory.

$port = $null
$logWriter = $null
$sketchPath = Split-Path -Path $PSScriptRoot -Parent
$logDirectory = Join-Path $sketchPath 'logs'
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

    $message = "$(Get-Date -Format 'MM/dd HH:mm:ss') Watching $($port.PortName). Press Ctrl+C to stop."
    Write-Host $message
    $logWriter.WriteLine($message)

    # Preserve line state when a serial line arrives across multiple reads.
    $atLineStart = $true

    while ($true) {
        $text = $port.ReadExisting()

        if ($text.Length -gt 0) {
            foreach ($part in [regex]::Split($text, '(?<=\n)')) {
                if ($part.Length -eq 0) { continue }

                if ($atLineStart) {
                    $timestamp = (Get-Date -Format 'MM/dd HH:mm:ss') + ' '
                    Write-Host -NoNewline $timestamp
                    $logWriter.Write($timestamp)
                }

                Write-Host -NoNewline $part
                $logWriter.Write($part)
                $atLineStart = $part.EndsWith("`n")
            }
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