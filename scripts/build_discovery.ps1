<#
.SYNOPSIS
Build Climatron with library discovery and stamp its ESP-IDF application descriptor.
.EXAMPLE
.\scripts\build_discovery.ps1 -Version '1.2.3' -ApplicationName 'Climatron'
#>
[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [ValidatePattern('^[0-9A-Za-z][0-9A-Za-z.+_-]{0,30}\z')]
    [string]$Version,

    [ValidatePattern('^[0-9A-Za-z][0-9A-Za-z .+_-]{0,30}\z')]
    [string]$ApplicationName = 'Climatron',

    [string]$BuildRoot = 'C:\arduinobuild'
)
$ErrorActionPreference = 'Stop'

# Configuration
$SKETCH = Split-Path -Path $PSScriptRoot -Parent
$BOARD = "esp32:esp32:esp32:PartitionScheme=min_spiffs"
$BUILD_ROOT = [IO.Path]::GetFullPath($BuildRoot)
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
Write-Host "App:    $ApplicationName $Version"

$ELF_PATH = Join-Path $BUILD_DIR "$SKETCH_NAME.ino.elf"
$STAMP_SCRIPT = Join-Path $PSScriptRoot 'stamp_app_descriptor.ps1'
if (-not (Test-Path -LiteralPath $STAMP_SCRIPT -PathType Leaf)) {
    throw "Missing descriptor hook: $STAMP_SCRIPT"
}

# Encoding preserves spaces and quotes through CLI argument handling in PS 5.1/7.
$hookCommand = "& '{0}' -Elf '{1}' -Version '{2}' -ApplicationName '{3}'" -f `
    $STAMP_SCRIPT.Replace("'", "''"), $ELF_PATH.Replace("'", "''"), `
    $Version.Replace("'", "''"), $ApplicationName.Replace("'", "''")
$encodedHook = [Convert]::ToBase64String([Text.Encoding]::Unicode.GetBytes($hookCommand))
$descriptorHook = "recipe.hooks.linking.postlink.90.pattern=powershell.exe -NoLogo -NoProfile -NonInteractive -OutputFormat Text -ExecutionPolicy Bypass -EncodedCommand $encodedHook"

# Force relinking even when only metadata changes; retain cached object files.
if (Test-Path -LiteralPath $ELF_PATH -PathType Leaf) {
    Remove-Item -LiteralPath $ELF_PATH -ErrorAction Stop
}

Write-Host "--- Starting Windows Compile with library discovery ---" -ForegroundColor Cyan
# --jobs 0 uses all CPU cores. Retain build files for incremental compilation.
arduino-cli compile --fqbn $BOARD `
    --jobs 0 `
    --libraries $LIB_PATH `
    --build-path $BUILD_DIR `
    --build-property "compiler.c.elf.extra_flags=-Wl,-Map,$BUILD_DIR/output.map" `
    --build-property $descriptorHook `
    -v `
    $SKETCH_PATH

if ($LASTEXITCODE -ne 0) {
    throw "Compile failed (exit code $LASTEXITCODE). Check library path, FQBN, and compile output above."
}

# Verify the actual application image, whose descriptor begins at 0x20.
$binPath = Join-Path $BUILD_DIR "$SKETCH_NAME.ino.bin"
[byte[]]$image = [IO.File]::ReadAllBytes($binPath)
if ($image.Length -lt 288 -or $image[0] -ne 0xE9 -or
    [BitConverter]::ToUInt32($image, 32) -ne 0xABCD5432L) {
    throw 'Generated binary does not contain the expected ESP-IDF app descriptor.'
}
$actualVersion = [Text.Encoding]::ASCII.GetString($image, 48, 32).TrimEnd([char]0)
$actualName = [Text.Encoding]::ASCII.GetString($image, 80, 32).TrimEnd([char]0)
if ($actualVersion -cne $Version -or $actualName -cne $ApplicationName) {
    throw "Descriptor mismatch: expected '$ApplicationName $Version', found '$actualName $actualVersion'."
}
if ($image[23] -ne 1) { throw 'Expected an appended SHA-256 digest in the application image.' }
$sha = [Security.Cryptography.SHA256]::Create()
try { $digest = $sha.ComputeHash($image, 0, $image.Length - 32) } finally { $sha.Dispose() }
for ($i = 0; $i -lt 32; $i++) {
    if ($digest[$i] -ne $image[$image.Length - 32 + $i]) {
        throw 'Application image SHA-256 verification failed.'
    }
}
Write-Host "Verified descriptor: $actualName $actualVersion" -ForegroundColor Green
Write-Host "--- Compile Success! ---" -ForegroundColor Green
