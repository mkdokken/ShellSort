# N = 362 round B: wait for round A3, then one 512M-move anneal per thread for the 12 best sequences of rounds A and
# A2 (128M) and the README sequence
while (Get-CimInstance Win32_Process -Filter "Name='powershell.exe'" | Where-Object { $_.CommandLine -like '*roundA3_362.ps1*' }) { Start-Sleep 15 }
python scripts\maxout.py 362 (Get-Content logs\roundB_362.list) --moves 512000000 --method luby --seed 72 --tag roundB --base 512000000 --T0 13.0
