[CmdletBinding()]
param(
    [Parameter(Mandatory = $true, Position = 0)]
    [ValidateNotNullOrEmpty()]
    [string]$Project,

    [string]$Port
)

$ErrorActionPreference = "Stop"

$repositoryRoot = Split-Path -Parent $PSScriptRoot
$projectCandidate = Join-Path $repositoryRoot $Project
$projectPath = (Resolve-Path -LiteralPath $projectCandidate).Path
$configurationPath = Join-Path $projectPath "platformio.ini"
$pioPath = Join-Path $env:USERPROFILE ".platformio\penv\Scripts\pio.exe"

if (-not (Test-Path -LiteralPath $configurationPath -PathType Leaf)) {
    throw "No platformio.ini found in $projectPath"
}

if (-not (Test-Path -LiteralPath $pioPath -PathType Leaf)) {
    throw "PlatformIO was not found at $pioPath"
}

if (-not $Port) {
    $devices = @(& $pioPath device list --json-output | ConvertFrom-Json)
    $espDevices = @($devices | Where-Object { $_.hwid -match "VID:PID=303A:1001" })

    if ($espDevices.Count -eq 0) {
        throw "No ESP32-C3 USB device was found. Connect a board or pass -Port COMx."
    }

    if ($espDevices.Count -gt 1) {
        $ports = ($espDevices.port -join ", ")
        throw "Multiple ESP32-C3 devices were found ($ports). Pass -Port COMx."
    }

    $Port = $espDevices[0].port
}

Write-Host "Uploading $Project to $Port"
& $pioPath run --project-dir $projectPath --target upload --upload-port $Port

if ($LASTEXITCODE -ne 0) {
    throw "PlatformIO upload failed with exit code $LASTEXITCODE"
}

Write-Host "Upload complete. Open a 115200-baud serial monitor on $Port to inspect output."
