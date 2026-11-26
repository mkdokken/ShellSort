# Consolidate: rebuild the canonical binaries from the final sources, point every script at them,
# remove the intermediate versioned binaries, and run a short regression.  Run when no job is active.
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent (Split-Path -Parent $MyInvocation.MyCommand.Path)
Set-Location $root
& (Join-Path $root 'build.ps1')
Get-ChildItem scripts\*.py | ForEach-Object {
    $t = Get-Content $_.FullName -Raw
    $u = $t -replace 'shellwc_v\d+\.exe', 'shellwc.exe' -replace 'seqsearch_v\d+\.exe', 'seqsearch.exe'
    if ($u -ne $t) { [IO.File]::WriteAllText($_.FullName, $u, (New-Object Text.UTF8Encoding $false)); "updated $($_.Name)" }
}
Get-ChildItem bin -Filter '*_v*.exe' | Remove-Item -Force
Get-ChildItem bin -Filter '*_dev.exe' -ErrorAction SilentlyContinue | Remove-Item -Force
$ErrorActionPreference = 'Continue'      # the DPs print progress on stderr; do not treat it as an error
$env:OMP_NUM_THREADS = '8'
$checks = @(
    @('dp2', '36', '1,4,7,9,19,34', 'dp2_worst=275'),
    @('lay', '30', '1,4,7,9,29', 'lay_worst=207'),
    @('dp', '16', '1,4,7,9', 'dp_worst=78'),
    @('xavg', '16', '1,4,7,9', 'xavg=60.61')
)
foreach ($c in $checks) {
    $o = cmd /c "bin\shellwc.exe $($c[0]) $($c[1]) $($c[2]) 2>nul"
    if ($o -match [regex]::Escape($c[3])) { "ok   $($c[0]) $($c[1]) {$($c[2])}" } else { "FAIL $($c[0]) $($c[1]) {$($c[2])}: $o" }
}
$o = & .\bin\shellwc.exe fwc 32 '1,4,7,9' 2000000 3 3.0 0.05 10 229 20
if ($o -match 'fwc_worst=229.*check=229') { 'ok   fwc 32 {1,4,7,9}' } else { "FAIL fwc: $o" }
$o = & .\bin\shellwc.exe pt 32 '1,4,7,9' 2000 3 12 0.3 2.0 1000 229
if ($o -match 'pt_worst=229 check=229') { 'ok   pt 32 {1,4,7,9}' } else { "FAIL pt: $o" }
$o = cmd /c "bin\shellwc.exe beam 30 1,4,7,9,29 100000 1 2>nul"
if ($o -match 'beam_worst=207 final=207 check=207') { 'ok   beam 30 {1,4,7,9,29}' } else { "FAIL beam: $o" }
$o = & .\bin\seqsearch.exe 20 --quiet
if (($o -join "`n") -match 'T_final=112' -and ($o -join "`n") -match 'seq=\{1,5,7,11\}') { 'ok   seqsearch 20' } else { "FAIL seqsearch 20" }
$o = python scripts\test_rules.py 40
if ($o -match '40 random sequences agree') { 'ok   front pair rules (40 random sequences vs dp2)' } else { "FAIL front pair rules: $o" }
