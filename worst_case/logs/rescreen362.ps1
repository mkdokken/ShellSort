# N = 362 long-run rescreen: wait for the screens (screens362.ps1), then 2 single-thread runs of 48M moves for the
# 150 best main-screen finalists and the refinement's finalists
while (Get-CimInstance Win32_Process -Filter "Name='powershell.exe'" | Where-Object { $_.CommandLine -like '*screens362.ps1*' }) { Start-Sleep 15 }
python scripts\rescreen.py 362 famscan_362.json,famscan_362_nbr.json --top 150 --iters 48000000 --reps 2 --T0 13.0
