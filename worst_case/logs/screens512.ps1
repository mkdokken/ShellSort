# N = 512 screen: wait for the start-temperature check (bench_t0.py), then the main screen (plan512, 15,990
# sequences; 2M / 4M / 8M moves per run, T0 = 9, the best of 4.5-35 in 4M-move runs)
while (Get-CimInstance Win32_Process -Filter "Name='python.exe'" | Where-Object { $_.CommandLine -like '*bench_t0*' }) { Start-Sleep 10 }
python scripts\famscan.py 512 --iters 2000000 --iters2 4000000 --iters3 8000000 --keep1 1200 --keep2 150 --T0 9.0 --slack 30
