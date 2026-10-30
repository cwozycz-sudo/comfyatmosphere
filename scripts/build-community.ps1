param(
    [string]$CMake = "cmake",
    [string]$Generator = "Visual Studio 17 2022",
    [string]$BuildDirectory = "build-community"
)
$ErrorActionPreference = "Stop"
$projectRoot = Split-Path -Parent $PSScriptRoot
$releaseVersion = (Get-Content -LiteralPath (Join-Path $projectRoot "COMMUNITY_VERSION") -Raw).Trim()
& $CMake -S $projectRoot -B $BuildDirectory -G $Generator -A Win32 "-DCOMFYATMOS_RELEASE_VERSION=$releaseVersion" -DCOMFY_BUILD_PERFORMANCE_TESTS=ON
if ($LASTEXITCODE -ne 0) { throw "CMake configuration failed" }
& $CMake --build $BuildDirectory --config Release --parallel 2
if ($LASTEXITCODE -ne 0) { throw "Build failed" }
$ctestTool = Join-Path (Split-Path -Parent (Get-Command $CMake).Source) "ctest.exe"
& $ctestTool --test-dir $BuildDirectory -C Release --output-on-failure
if ($LASTEXITCODE -ne 0) { throw "Regression tests failed" }
