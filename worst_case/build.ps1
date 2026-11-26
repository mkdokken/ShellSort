# Build all C tools (MinGW-w64 gcc from Code::Blocks; static so the .exe runs without PATH changes)
$ErrorActionPreference = 'Stop'
$gccDir = 'C:\Program Files\CodeBlocks\MinGW\bin'
if (-not (Get-Command gcc -ErrorAction SilentlyContinue)) { $env:Path = "$gccDir;" + $env:Path }
$root = Split-Path -Parent $MyInvocation.MyCommand.Path
New-Item -ItemType Directory -Force -Path (Join-Path $root 'bin') | Out-Null
$flags = @('-O3', '-march=native', '-mno-avx512f', '-fopenmp', '-static', '-Wall', '-Wno-unused-function', '-D__USE_MINGW_ANSI_STDIO=1')
foreach ($t in @('shellwc', 'seqsearch')) {
    $src = Join-Path $root "src\$t.c"
    if (Test-Path $src) {
        & gcc @flags $src -o (Join-Path $root "bin\$t.exe") -lm
        if ($LASTEXITCODE -ne 0) { throw "build failed: $t" }
        Write-Host "built bin\$t.exe"
    }
}
# 16-bit cells for the layered DP (sequences whose residual exceeds one byte; twice the memory)
& gcc @flags '-DLAY_CELL=uint16_t' '-DLAY_CELL_MAX=65535' (Join-Path $root 'src\shellwc.c') -o (Join-Path $root 'bin\shellwc16.exe') -lm
if ($LASTEXITCODE -ne 0) { throw 'build failed: shellwc16' }
Write-Host 'built bin\shellwc16.exe'
