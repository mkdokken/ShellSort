# N = 362 screens: wait for the engine check (bench362.ps1), then the main screen and the refinement around its
# 40 best finalists
while (Get-CimInstance Win32_Process -Filter "Name='powershell.exe'" | Where-Object { $_.CommandLine -like '*bench362.ps1*' }) { Start-Sleep 15 }
python scripts\famscan.py 362 --iters 2000000 --iters2 4000000 --iters3 8000000 --keep1 2500 --keep2 300 --T0 13.0 --slack 20
python scripts\famscan.py 362 --plan nbr --from famscan_362.json --top 40 --iters 4000000 --iters2 8000000 --iters3 16000000 --keep1 400 --keep2 60 --T0 13.0 --slack 20
