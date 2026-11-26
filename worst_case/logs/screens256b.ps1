# N = 256 follow-up screens: {1,4,9,15} at the density {1,4,9,11,30} had, then refinement around the main finalists
python scripts\famscan.py 256 --plan 256b --iters 2000000 --iters2 4000000 --iters3 8000000 --keep1 1500 --keep2 150 --T0 9.0 --slack 15
python scripts\famscan.py 256 --plan nbr --from famscan_256.json --top 40 --iters 4000000 --iters2 8000000 --iters3 16000000 --keep1 400 --keep2 60 --T0 9.0 --slack 15
