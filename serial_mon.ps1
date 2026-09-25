# Change COM3 and 115200 if necessary.

$port = New-Object System.IO.Ports.SerialPort COM3,115200,None,8,One
$port.Handshake = 'None'
$port.ReadTimeout = 100

try {
    $port.Open()
    Write-Host "Watching $($port.PortName). Press Ctrl+C to stop."

    while ($true) {
        $text = $port.ReadExisting()

        if ($text.Length -gt 0) {
            Write-Host -NoNewline $text
        }

        Start-Sleep -Milliseconds 50
    }
}
finally {
    if ($port.IsOpen) {
        $port.Close()
    }
    $port.Dispose()
}