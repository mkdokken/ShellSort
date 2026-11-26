# Engine check at N = 181: two sequences, 100M moves per thread, three settings
$s = '1,4,9,11,30,47,59,86;1,4,9,11,30,58,81'
python scripts\maxout.py 181 $s --moves 100000000 --method luby --seed 31 --tag benchL2T3 --base 2000000 --T0 3.0
python scripts\maxout.py 181 $s --moves 100000000 --method luby --seed 31 --tag benchL4T45 --base 4000000 --T0 4.5
python scripts\maxout.py 181 $s --moves 100000000 --method pt --seed 31 --tag benchP30 --R 16 --Tmin 0.3 --Tmax 3.0 --mpe 1000
