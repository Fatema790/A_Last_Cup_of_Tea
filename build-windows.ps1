$ErrorActionPreference = 'Stop'
Set-Location -LiteralPath $PSScriptRoot
$localCompiler = Join-Path $PSScriptRoot '.tools/w64devkit/bin'
if (Test-Path -LiteralPath $localCompiler) { $env:PATH = "$localCompiler;$env:PATH" }
$localCmake = Join-Path $PSScriptRoot '.tools/cmake-3.31.6-windows-x86_64/bin/cmake.exe'
if (Test-Path -LiteralPath $localCmake) { $cmake = $localCmake } else { $cmake = (Get-Command cmake -ErrorAction Stop).Source }
$ctest = Join-Path (Split-Path $cmake) 'ctest.exe'
$options = @('-S', '.', '-B', 'build', '-G', 'MinGW Makefiles', '-DCMAKE_BUILD_TYPE=Release')
$localGlut = Join-Path $PSScriptRoot '.tools/freeglut-3.6.0'
if (Test-Path -LiteralPath $localGlut) { $options += "-DFETCHCONTENT_SOURCE_DIR_FREEGLUT=$localGlut" }
& $cmake @options
if ($LASTEXITCODE -ne 0) { throw 'CMake configuration failed.' }
& $cmake --build build --parallel 4
if ($LASTEXITCODE -ne 0) { throw 'Compilation failed.' }
& $ctest --test-dir build --output-on-failure
if ($LASTEXITCODE -ne 0) { throw 'Campaign tests failed.' }
New-Item -ItemType Directory -Force dist | Out-Null
Copy-Item -LiteralPath 'build/last_cup.exe' -Destination 'dist/last_cup.exe'
$glutNotice = Join-Path $localGlut 'COPYING'
if (!(Test-Path -LiteralPath $glutNotice)) { $glutNotice = Join-Path $PSScriptRoot 'build/_deps/freeglut-src/COPYING' }
if (Test-Path -LiteralPath $glutNotice) { Copy-Item -LiteralPath $glutNotice -Destination 'dist/FreeGLUT-LICENSE.txt' }
$runtimeNotice = Join-Path $PSScriptRoot '.tools/w64devkit/COPYING.MinGW-w64-runtime.txt'
if (Test-Path -LiteralPath $runtimeNotice) { Copy-Item -LiteralPath $runtimeNotice -Destination dist }
Write-Host 'Build and tests passed. Run dist/last_cup.exe.'
