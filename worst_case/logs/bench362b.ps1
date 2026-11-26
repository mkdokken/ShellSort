# Engine check at N = 362, part 2: longer Luby base runs at an equal budget of 128M moves per thread
# (32M + 32M + 64M, 64M + 64M, one 128M run), T0 = 13, same two sequences, new seed
$s = '1,4,9,11,30,36,70,107,163;1,4,9,11,30,47,59,127,150'
python scripts\maxout.py 362 $s --moves 128000000 --method luby --seed 62 --tag benchL32T13 --base 32000000 --T0 13.0
python scripts\maxout.py 362 $s --moves 128000000 --method luby --seed 62 --tag benchL64T13 --base 64000000 --T0 13.0
python scripts\maxout.py 362 $s --moves 128000000 --method luby --seed 62 --tag benchL128T13 --base 128000000 --T0 13.0
