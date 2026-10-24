# Configuration
$SKETCH = Split-Path -Path $PSScriptRoot -Parent
$BOARD = "esp32:esp32:esp32:PartitionScheme=min_spiffs"
$BUILD_ROOT = "C:\arduinobuild"
$LIB_PATH = "$env:USERPROFILE\Dropbox\make\arduino\libraries"

# Resolve the sketch from the parent of this scripts folder, independent of the working directory.
$sketchItem = Get-Item -LiteralPath (Resolve-Path -LiteralPath $SKETCH -ErrorAction Stop).ProviderPath -ErrorAction Stop
if ($sketchItem.PSIsContainer) {
    $SKETCH_PATH = $sketchItem.FullName
} elseif ($sketchItem.Extension -eq '.ino') {
    $SKETCH_PATH = $sketchItem.Directory.FullName
} else {
    throw "SKETCH must be a sketch directory or an .ino file."
}
$SKETCH_NAME = Split-Path -Path $SKETCH_PATH -Leaf
if (-not (Test-Path -LiteralPath (Join-Path $SKETCH_PATH "$SKETCH_NAME.ino") -PathType Leaf)) {
    throw "No $SKETCH_NAME.ino found in $SKETCH_PATH. Check the sketch directory configured by $SKETCH."
}

# Use the sketch folder name for all build and upload scripts.
$BUILD_DIR = Join-Path $BUILD_ROOT $SKETCH_NAME
New-Item -ItemType Directory -Path $BUILD_DIR -Force -ErrorAction Stop | Out-Null
Write-Host "Sketch: $SKETCH_PATH"
Write-Host "Build:  $BUILD_DIR"

Write-Host "Detecting available boards..." -ForegroundColor Cyan
arduino-cli board list

do {
    $portNumber = Read-Host "Enter COM port number (e.g., 9)"
} while (-not ($portNumber -match '^\d+$'))
$PORT = "COM$portNumber"

Write-Host "--- Starting Windows Compile with library discovery ---" -ForegroundColor Cyan
# --jobs 0 uses all CPU cores. Retain build files for incremental compilation.
arduino-cli compile --fqbn $BOARD `
    --jobs 0 `
    --libraries $LIB_PATH `
    --build-path $BUILD_DIR `
    --build-property "compiler.c.elf.extra_flags=-Wl,-Map,$BUILD_DIR/output.map" `
    -v `
    $SKETCH_PATH

if ($LASTEXITCODE -eq 0) {
    Write-Host "--- Compile Success! ---" -ForegroundColor Green
    arduino-cli upload `
        -p $PORT `
        --fqbn $BOARD `
        --input-dir $BUILD_DIR `
        $SKETCH_PATH

    $logport = $null
    $logWriter = $null
    $logDirectory = Join-Path $SKETCH_PATH 'logs'
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

        $message = "$(Get-Date -Format 'MM/dd HH:mm:ss') Watching $($logPort.PortName). Press Ctrl+C to stop."
        Write-Host $message
        $logWriter.WriteLine($message)

        # Keep line state across reads: one serial line can arrive in several chunks.
        $atLineStart = $true

        while ($true) {
            $text = $logPort.ReadExisting()

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
} else {
    Write-Host "--- Compile Failed ---" -ForegroundColor Red
    Write-Host "Tip: Check library path, FQBN, and compile output above."
}
