# Engine benchmark at N = 128: two sequences, 100M moves per thread, two seeds, four settings
$s = '1,4,9,11,30,36,59;1,4,9,11,30,47,48'
foreach ($seed in 11, 12) {
    python scripts\maxout.py 128 $s --moves 100000000 --method luby --seed $seed --tag benchL2T3 --base 2000000 --T0 3.0
    python scripts\maxout.py 128 $s --moves 100000000 --method luby --seed $seed --tag benchL4T45 --base 4000000 --T0 4.5
    python scripts\maxout.py 128 $s --moves 100000000 --method pt --seed $seed --tag benchP30 --R 16 --Tmin 0.3 --Tmax 3.0 --mpe 1000
    python scripts\maxout.py 128 $s --moves 100000000 --method pt --seed $seed --tag benchP45 --R 16 --Tmin 0.3 --Tmax 4.5 --mpe 1000
}
