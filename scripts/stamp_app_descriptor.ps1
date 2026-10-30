[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)][string]$Elf,
    [Parameter(Mandatory = $true)][string]$Version,
    [Parameter(Mandatory = $true)][string]$ApplicationName
)
$ErrorActionPreference = 'Stop'
$ProgressPreference = 'SilentlyContinue'
try {
    # Restrict to safe release identifiers; esp_app_desc_t.version includes its NUL.
    if ($Version -notmatch '^[0-9A-Za-z][0-9A-Za-z.+_-]{0,30}\z') {
        throw 'Version must be 1-31 ASCII letters, digits, dots, pluses, underscores or hyphens.'
    }
    if ($ApplicationName -notmatch '^[0-9A-Za-z][0-9A-Za-z .+_-]{0,30}\z') {
        throw 'ApplicationName must be 1-31 ASCII letters, digits, spaces, dots, pluses, underscores or hyphens.'
    }
    $Elf = (Resolve-Path -LiteralPath $Elf).Path
    [byte[]]$bytes = [IO.File]::ReadAllBytes($Elf)
    function U16([long]$Offset) {
        if ($Offset -lt 0 -or $Offset + 2 -gt $bytes.LongLength) { throw 'Truncated ELF.' }
        [BitConverter]::ToUInt16($bytes, [int]$Offset)
    }
    function U32([long]$Offset) {
        if ($Offset -lt 0 -or $Offset + 4 -gt $bytes.LongLength) { throw 'Truncated ELF.' }
        [BitConverter]::ToUInt32($bytes, [int]$Offset)
    }
    if ($bytes.Length -lt 52 -or (U32 0) -ne 0x464C457F -or
        $bytes[4] -ne 1 -or $bytes[5] -ne 1 -or (U16 16) -ne 2 -or (U16 18) -ne 94) {
        throw 'Expected an ELF32 little-endian Xtensa executable.'
    }
    [long]$sectionTable = U32 32
    [long]$sectionSize = U16 46
    [long]$sectionCount = U16 48
    [long]$nameIndex = U16 50
    if ($sectionSize -ne 40 -or $sectionCount -eq 0 -or $nameIndex -ge $sectionCount -or
        $sectionTable + $sectionSize * $sectionCount -gt $bytes.LongLength) {
        throw 'Unsupported or truncated ELF section table.'
    }
    $nameHeader = $sectionTable + $sectionSize * $nameIndex
    [long]$namesOffset = U32 ($nameHeader + 16)
    [long]$namesSize = U32 ($nameHeader + 20)
    if ($namesOffset + $namesSize -gt $bytes.LongLength) { throw 'Truncated ELF section names.' }
    $matches = @()
    for ($i = 0; $i -lt $sectionCount; $i++) {
        $header = $sectionTable + $sectionSize * $i
        [long]$nameStart = U32 $header
        if ($nameStart -ge $namesSize) { throw 'Invalid ELF section name.' }
        $end = $namesOffset + $nameStart
        while ($end -lt $namesOffset + $namesSize -and $bytes[$end] -ne 0) { $end++ }
        if ($end -eq $namesOffset + $namesSize) { throw 'Unterminated ELF section name.' }
        $name = [Text.Encoding]::ASCII.GetString($bytes, [int]($namesOffset + $nameStart), [int]($end - $namesOffset - $nameStart))
        if ($name -eq '.flash.appdesc') { $matches += $header }
    }
    if ($matches.Count -ne 1) { throw 'Expected exactly one .flash.appdesc section.' }
    [long]$descOffset = U32 ($matches[0] + 16)
    [long]$descSize = U32 ($matches[0] + 20)
    if ((U32 ($matches[0] + 4)) -ne 1 -or $descSize -lt 256 -or
        $descOffset + $descSize -gt $bytes.LongLength -or (U32 $descOffset) -ne 0xABCD5432L) {
        throw 'Expected an ESP-IDF app descriptor at the start of .flash.appdesc; refusing to stamp.'
    }
    # ESP-IDF version[32] and project_name[32] are at offsets 16 and 48.
    # Preserve other metadata, including secure_version and chip compatibility.
    foreach ($field in @(
        @{ Offset = 16; Value = $Version },
        @{ Offset = 48; Value = $ApplicationName }
    )) {
        $offset = [int]($descOffset + $field.Offset)
        [Array]::Clear($bytes, $offset, 32)
        [byte[]]$valueBytes = [Text.Encoding]::ASCII.GetBytes($field.Value)
        [Array]::Copy($valueBytes, 0, $bytes, $offset, $valueBytes.Length)
    }
    [IO.File]::WriteAllBytes($Elf, $bytes)
    [Console]::WriteLine("ESP-IDF app descriptor: $ApplicationName $Version")
} catch {
    [Console]::Error.WriteLine($_.Exception.Message)
    exit 1
}

