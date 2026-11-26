# Engine check at N = 256: two sequences, 100M moves per thread, Luby restarts with three base / start settings
$s = '1,4,9,15,35,49,101,123;1,4,9,11,30,36,70,121'
python scripts\maxout.py 256 $s --moves 100000000 --method luby --seed 51 --tag benchL4T45 --base 4000000 --T0 4.5
python scripts\maxout.py 256 $s --moves 100000000 --method luby --seed 51 --tag benchL8T9 --base 8000000 --T0 9.0
python scripts\maxout.py 256 $s --moves 100000000 --method luby --seed 51 --tag benchL16T9 --base 16000000 --T0 9.0
