# Configuration
$SKETCH = "."
$BOARD = "esp32:esp32:esp32:PartitionScheme=min_spiffs"
$BUILD_DIR = "c:\arduinobuild"
$LIB_PATH = "$env:USERPROFILE\Dropbox\make\arduino\libraries"

Write-Host "Detecting available boards..." -ForegroundColor Cyan
arduino-cli board list

do {
    $portNumber = Read-Host "Enter COM port number (e.g., 9)"
} while (-not ($portNumber -match '^\d+$'))

$PORT = "COM$portNumber"


# Ensure directories exist
if (!(Test-Path $BUILD_DIR)) { New-Item -ItemType Directory -Path $BUILD_DIR }

Write-Host "--- Starting Ultra-Fast Windows Compile with library discovery---" -ForegroundColor Cyan

# Execute arduino-cli
# --jobs 0 uses all CPU cores
arduino-cli compile --fqbn $BOARD `
  --jobs 0 `
  --libraries $LIB_PATH `
  --build-path $BUILD_DIR `
  --build-property "compiler.c.elf.extra_flags=-Wl,-Map,$BUILD_DIR/output.map" `
  -v `
  $SKETCH

# Check exit code ($?)
if ($LASTEXITCODE -eq 0) {
    Write-Host "--- Compile Success! ---" -ForegroundColor Green
    arduino-cli upload `
        -p $PORT `
        --fqbn $BOARD `
        --input-dir $BUILD_DIR

    $logport = $null
    $logWriter = $null
    $logDirectory = Join-Path (Get-Location).Path 'logs'
    $logPath = Join-Path $logDirectory ('climatron_{0}.txt' -f (Get-Date -Format 'dd-MM-yy+HH-mm'))

    try {
        [System.IO.Directory]::CreateDirectory($logDirectory) | Out-Null
        # Append prevents overwriting an earlier log started in the same minute.
        $logWriter = New-Object System.IO.StreamWriter($logPath, $true, (New-Object System.Text.UTF8Encoding($false)))
        $logWriter.AutoFlush = $true

        $logPort = New-Object System.IO.Ports.SerialPort $PORT,115200,None,8,One
        $logPort.Handshake = 'None'
        $logPort.ReadTimeout = 100
        $logPort.Open()

        $message = "Watching $($logPort.PortName). Press Ctrl+C to stop."
        Write-Host $message
        $logWriter.WriteLine($message)

        while ($true) {
            $text = $logPort.ReadExisting()

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
            if ($null -ne $logPort) {
                try {
                    if ($logPort.IsOpen) {
                        $logPort.Close()
                    }
                }
                finally {
                    $logPort.Dispose()
                }
            }
        }
        finally {
            if ($null -ne $logWriter) {
                $logWriter.Dispose()
            }
        }
    }
} 
else {
    Write-Host "--- Compile Failed ---" -ForegroundColor Red
    Write-Host "Tip: Check library path, FQBN, and compile output above."
}