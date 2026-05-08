$ErrorActionPreference = "Stop"

$projectRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
$elfPath = Join-Path $projectRoot "build\\diode_measure.elf"

function Resolve-ProgrammerCli {
    $cmd = Get-Command STM32_Programmer_CLI -ErrorAction SilentlyContinue
    if ($cmd) {
        return $cmd.Source
    }

    $fallbacks = @(
        "C:\\Program Files\\STMicroelectronics\\STM32Cube\\STM32CubeProgrammer\\bin\\STM32_Programmer_CLI.exe",
        "C:\\ST\\STM32CubeCLT_1.18.0\\STM32CubeProgrammer\\bin\\STM32_Programmer_CLI.exe"
    )

    foreach ($candidate in $fallbacks) {
        if (Test-Path $candidate) {
            return $candidate
        }
    }

    throw "STM32_Programmer_CLI not found. Install STM32CubeProgrammer or add it to PATH."
}

Write-Host "==> Building firmware..."
powershell -ExecutionPolicy Bypass -File (Join-Path $projectRoot "build.ps1")
if ($LASTEXITCODE -ne 0) {
    throw "Build failed. Flash aborted."
}

if (-not (Test-Path $elfPath)) {
    throw "ELF not found after build: $elfPath"
}

$programmerCli = Resolve-ProgrammerCli

Write-Host ""
Write-Host "==> Flashing firmware via ST-Link..."
& $programmerCli -c port=SWD mode=UR -w $elfPath -v -rst
if ($LASTEXITCODE -ne 0) {
    throw "Flash failed. Check ST-Link connection, target power, and SWD wiring."
}

Write-Host ""
Write-Host "Flash complete."
