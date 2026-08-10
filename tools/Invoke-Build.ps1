[CmdletBinding()]
param(
    [ValidateSet('Configure', 'Build', 'Test', 'RunApp', 'Analyze')]
    [string]$Operation = 'Build',

    [string]$Preset = 'windows-clang-debug',

    [string]$Target,

    [string]$LlvmRoot,

    [string]$BuildDirectory,

    [string]$TestDirectory,

    [string[]]$AnalysisPath
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

# DevShell may replace VCPKG_ROOT with Visual Studio's bundled copy. Prefer an
# explicit repository override, then preserve the caller's original selection.
$requestedVcpkgRoot = $env:RENDER_VCPKG_ROOT
if (-not $requestedVcpkgRoot) {
    $requestedVcpkgRoot = $env:VCPKG_ROOT
}

if (-not $BuildDirectory) {
    $BuildDirectory = Join-Path 'out\build' $Preset
}
if (-not $TestDirectory) {
    $TestDirectory = Join-Path $BuildDirectory 'shader'
}

# Capture the caller's LLVM selection before DevShell changes PATH.
$pathClang = Get-Command clang-cl.exe -ErrorAction SilentlyContinue |
    Select-Object -ExpandProperty Source -First 1

function Stop-Build([string]$Message) {
    throw "Build environment error: $Message"
}

function Add-PathPrefix([string]$Directory) {
    if (-not (Test-Path -LiteralPath $Directory)) {
        Stop-Build "Required directory does not exist: $Directory"
    }

    $entries = $env:Path -split ';'
    if ($entries -notcontains $Directory) {
        $env:Path = "$Directory;$env:Path"
    }
}

function Find-FirstFile([string[]]$Candidates, [string]$Description) {
    foreach ($candidate in $Candidates) {
        if ($candidate -and (Test-Path -LiteralPath $candidate -PathType Leaf)) {
            return (Resolve-Path -LiteralPath $candidate).Path
        }
    }

    Stop-Build "$Description was not found."
}

$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
if (-not (Test-Path -LiteralPath $vswhere -PathType Leaf)) {
    Stop-Build "vswhere.exe was not found at $vswhere"
}

$vsPath = & $vswhere -latest -products * `
    -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 `
    -property installationPath
if (-not $vsPath) {
    Stop-Build 'A Visual Studio installation with C++ Build Tools was not found.'
}

$devShellModule = Join-Path $vsPath 'Common7\Tools\Microsoft.VisualStudio.DevShell.dll'
if (-not (Test-Path -LiteralPath $devShellModule -PathType Leaf)) {
    Stop-Build "Visual Studio DevShell module was not found at $devShellModule"
}

# Some launchers export both `Path` and `PATH`. DevShell rejects that duplicate
# dictionary key even though Windows treats environment-variable names case-insensitively.
$savedPath = $env:Path
[Environment]::SetEnvironmentVariable('Path', $null, 'Process')
[Environment]::SetEnvironmentVariable('PATH', $null, 'Process')
[Environment]::SetEnvironmentVariable('Path', $savedPath, 'Process')

Import-Module $devShellModule
Enter-VsDevShell -VsInstallPath $vsPath -SkipAutomaticLocation -Arch amd64 -HostArch amd64 | Out-Null

if ($requestedVcpkgRoot) {
    $env:VCPKG_ROOT = $requestedVcpkgRoot
}

$llvmCandidates = @()
foreach ($root in @($LlvmRoot, $env:RENDER_LLVM_ROOT)) {
    if ($root) {
        $llvmCandidates += (Join-Path $root 'bin')
        $llvmCandidates += $root
    }
}
if ($pathClang) {
    $llvmCandidates += (Split-Path -Parent $pathClang)
}
if ($env:ProgramFiles) {
    $llvmCandidates += (Join-Path $env:ProgramFiles 'LLVM\bin')
}
$llvmCandidates += (Join-Path $vsPath 'VC\Tools\Llvm\x64\bin')
$llvmBin = $llvmCandidates | Where-Object {
    $_ -and
    (Test-Path -LiteralPath (Join-Path $_ 'clang-cl.exe') -PathType Leaf) -and
    (Test-Path -LiteralPath (Join-Path $_ 'lld-link.exe') -PathType Leaf)
} | Select-Object -Unique -First 1
if (-not $llvmBin) {
    Stop-Build 'A matching LLVM clang-cl and lld-link installation was not found. Pass -LlvmRoot, set RENDER_LLVM_ROOT, or add LLVM bin to PATH.'
}
$llvmBin = (Resolve-Path -LiteralPath $llvmBin).Path
Add-PathPrefix $llvmBin

$cmake = Find-FirstFile @(
    $(if ($env:RENDER_CMAKE_ROOT) { Join-Path $env:RENDER_CMAKE_ROOT 'bin\cmake.exe' }),
    $(if ($env:RENDER_CMAKE_ROOT) { Join-Path $env:RENDER_CMAKE_ROOT 'cmake.exe' }),
    (Get-Command cmake.exe -ErrorAction SilentlyContinue | Select-Object -ExpandProperty Source -First 1),
    (Join-Path $vsPath 'Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe')
) 'CMake'
$cmakeBin = Split-Path -Parent $cmake
Add-PathPrefix $cmakeBin

$ctest = Find-FirstFile @(
    (Join-Path $cmakeBin 'ctest.exe'),
    (Get-Command ctest.exe -ErrorAction SilentlyContinue | Select-Object -ExpandProperty Source -First 1)
) 'CTest'

$ninja = Find-FirstFile @(
    $(if ($env:RENDER_NINJA_ROOT) { Join-Path $env:RENDER_NINJA_ROOT 'ninja.exe' }),
    (Get-Command ninja.exe -ErrorAction SilentlyContinue | Select-Object -ExpandProperty Source -First 1),
    (Join-Path $vsPath 'Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja\ninja.exe')
) 'Ninja'
Add-PathPrefix (Split-Path -Parent $ninja)

if (-not $env:VCPKG_ROOT) {
    Stop-Build 'VCPKG_ROOT is not set. Set it to the vcpkg checkout used by this repository.'
}
if (-not (Test-Path -LiteralPath (Join-Path $env:VCPKG_ROOT 'scripts\buildsystems\vcpkg.cmake') -PathType Leaf)) {
    Stop-Build "VCPKG_ROOT does not contain scripts/buildsystems/vcpkg.cmake: $env:VCPKG_ROOT"
}

Write-Host "Visual Studio: $vsPath"
Write-Host "LLVM: $llvmBin"
Write-Host "CMake: $cmake"
Write-Host "Ninja: $ninja"
Write-Host "vcpkg: $env:VCPKG_ROOT"

switch ($Operation) {
    'Configure' {
        & $cmake --preset $Preset
    }
    'Build' {
        $arguments = @('--build', '--preset', $Preset)
        if ($Target) {
            $arguments += @('--target', $Target)
        }
        & $cmake @arguments
    }
    'Test' {
        if (-not (Test-Path -LiteralPath $TestDirectory -PathType Container)) {
            Stop-Build "Test directory does not exist: $TestDirectory. Run -Operation Configure first."
        }
        & $ctest --test-dir $TestDirectory --output-on-failure
    }
    'RunApp' {
        $app = Get-ChildItem -Path $BuildDirectory -Recurse -File -Filter 'app.exe' -ErrorAction SilentlyContinue |
            Select-Object -First 1
        if (-not $app) {
            Stop-Build "app.exe was not found under $BuildDirectory. Build target 'app' first."
        }
        & $app.FullName
    }
    'Analyze' {
        $compileDatabase = Join-Path $BuildDirectory 'compile_commands.json'
        if (-not (Test-Path -LiteralPath $compileDatabase -PathType Leaf)) {
            Stop-Build "Compilation database does not exist: $compileDatabase. Run -Operation Configure first."
        }

        $clangTidy = Find-FirstFile @(
            (Join-Path $llvmBin 'clang-tidy.exe'),
            (Get-Command clang-tidy.exe -ErrorAction SilentlyContinue | Select-Object -ExpandProperty Source -First 1)
        ) 'clang-tidy'

        $analysisTargets = $AnalysisPath
        if (-not $analysisTargets) {
            $analysisTargets = Get-ChildItem -Path 'app', 'core' -Include *.cpp, *.h -Recurse |
                Select-Object -ExpandProperty FullName
        }
        if (-not $analysisTargets) {
            Stop-Build 'No C++ files were found to analyze.'
        }

        & $clangTidy -p $BuildDirectory @analysisTargets
    }
}

if ($LASTEXITCODE -ne 0) {
    exit $LASTEXITCODE
}
