# N = 362 round A3 (catch-up): wait for round A2, then the round-A budget (one 128M-move anneal per thread) for
# the three best older sequences known only from the transfer searches (results/transfer*.json)
while (Get-CimInstance Win32_Process -Filter "Name='python.exe'" | Where-Object { $_.CommandLine -like '*roundA2*' }) { Start-Sleep 15 }
python scripts\maxout.py 362 '1,4,9,11,30,47,78,135;1,4,9,15,34,56,125,141;1,4,9,15,25,53,82,146' --moves 128000000 --method luby --seed 74 --tag roundA3 --base 128000000 --T0 13.0
