# Extra N = 181 runs in the remaining budget: a third 4G attack on the leader, and rounds C and D for the
# one sequence at 2652 that only had rounds A and B
python scripts\maxout.py 181 '1,4,9,11,25,37,58,81' --moves 1000000000 --method luby --seed 44 --tag roundC --base 8000000 --T0 4.5
python scripts\maxout.py 181 '1,4,9,11,25,37,58,81' --moves 2000000000 --method luby --seed 45 --tag roundD --base 4000000 --T0 4.5
python scripts\maxout.py 181 '1,4,9,11,30,47,59,86' --moves 4000000000 --method luby --seed 48 --tag roundG --base 8000000 --T0 4.5
