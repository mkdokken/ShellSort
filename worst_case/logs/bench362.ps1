# Engine check at N = 362: two sequences, 100M moves per thread, Luby restarts with four base / start settings
$s = '1,4,9,11,30,36,70,107,163;1,4,9,11,30,47,59,127,150'
python scripts\maxout.py 362 $s --moves 100000000 --method luby --seed 61 --tag benchL4T6 --base 4000000 --T0 6.0
python scripts\maxout.py 362 $s --moves 100000000 --method luby --seed 61 --tag benchL8T9 --base 8000000 --T0 9.0
python scripts\maxout.py 362 $s --moves 100000000 --method luby --seed 61 --tag benchL16T13 --base 16000000 --T0 13.0
python scripts\maxout.py 362 $s --moves 100000000 --method luby --seed 61 --tag benchL32T13 --base 32000000 --T0 13.0
