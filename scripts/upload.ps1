# Upload previously compiled firmware for the sketch above this scripts folder.
$SKETCH = Split-Path -Path $PSScriptRoot -Parent
$BOARD = "esp32:esp32:esp32:PartitionScheme=min_spiffs"
$BUILD_ROOT = "C:\arduinobuild"

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

Write-Host "Detecting available boards..." -ForegroundColor Cyan
arduino-cli board list

do {
    $portNumber = Read-Host "Enter COM port number (e.g., 9)"
} while (-not ($portNumber -match '^\d+$'))

$PORT = "COM$portNumber"

Write-Host "--- Uploading to $PORT ---" -ForegroundColor Cyan
arduino-cli upload `
    -p $PORT `
    --fqbn $BOARD `
    --input-dir $BUILD_DIR `
    $SKETCH_PATH

if ($LASTEXITCODE -ne 0) {
    Write-Host "--- Upload Failed! ---" -ForegroundColor Red
    exit $LASTEXITCODE
}

Write-Host "--- Upload Success! ---" -ForegroundColor Green

