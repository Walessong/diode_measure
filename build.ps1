$ErrorActionPreference = "Stop"

$projectRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
$buildDir = Join-Path $projectRoot "build"
$toolchain = Join-Path $projectRoot "cmake\\toolchain-arm-none-eabi.cmake"

function Resolve-Tool([string]$commandName, [string[]]$fallbacks) {
    $cmd = Get-Command $commandName -ErrorAction SilentlyContinue
    if ($cmd) {
        return $cmd.Source
    }

    foreach ($candidate in $fallbacks) {
        if (Test-Path $candidate) {
            return $candidate
        }
    }

    throw "Tool not found: $commandName"
}

$ninja = Resolve-Tool "ninja" @(
    "C:\\ST\\STM32CubeCLT_1.18.0\\Ninja\\bin\\ninja.exe",
    "C:\\ST\\STM32CubeCLT_1.17.0\\Ninja\\bin\\ninja.exe"
)

if (-not (Test-Path (Join-Path $buildDir "CMakeCache.txt"))) {
    cmake -S $projectRoot -B $buildDir -G Ninja "-DCMAKE_TOOLCHAIN_FILE=$toolchain" "-DCMAKE_MAKE_PROGRAM=$ninja"
    if ($LASTEXITCODE -ne 0) { throw "CMake configure failed." }
} else {
    cmake -S $projectRoot -B $buildDir -G Ninja "-DCMAKE_MAKE_PROGRAM=$ninja"
    if ($LASTEXITCODE -ne 0) { throw "CMake reconfigure failed." }
}
cmake --build $buildDir
if ($LASTEXITCODE -ne 0) { throw "Build failed." }

Write-Host ""
Write-Host "Build complete:"
Write-Host "  ELF: $buildDir\\diode_measure.elf"
Write-Host "  HEX: $buildDir\\diode_measure.hex"
Write-Host "  BIN: $buildDir\\diode_measure.bin"
