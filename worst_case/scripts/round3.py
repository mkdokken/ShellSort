"""Round 3 of the large-N races: Luby restarts (FSA_LUBY=1), every race *calibrated* by a contender that is
already known to reach the target (attacked fresh: if the budget cannot re-find a known value, survivals in
that race mean nothing).  Runs the races one after the other; output in results/round3.log.

usage: python scripts/round3.py [first-index]
"""
import os, subprocess, sys, time

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
RACES = [
    # N, target, contenders (calibration sequence = the one already known to reach the target), base moves, reps
    (91, 1045, "1,4,9,11,30,42;1,4,9,11,32,44;1,4,9,11,30,44", 2000000, 127),
    (128, 1661, "1,4,9,11,30,47,59;1,4,9,11,30,36,59", 2000000, 127),
    (256, 4152, "1,4,9,11,30,36,65,103,118;1,4,9,15,35,49,101,123", 4000000, 63),
    (362, 6429, "1,4,9,11,30,36,70,107,163;1,4,9,11,30,47,70,131,179;1,4,9,11,30,47,59,127,150", 4000000, 63),
    (181, 2632, "1,4,9,11,30,47,59,86;1,4,9,11,30,36,59,86;1,4,9,11,30,36,59,87;1,4,9,15,31,44,81", 4000000, 63),
    # round 4 (index 5-): the ties left by round 3, one level up
    (256, 4160, "1,4,9,15,34,56,80;1,4,9,15,35,49,101,123;1,4,9,11,30,36,65,103,118", 4000000, 63),
    (91, 1046, "1,4,9,15,35,37;1,4,9,11,30,44;1,4,9,11,30,42;1,4,9,11,32,44;1,4,9,11,21,38;1,4,9,11,30,36;"
               "1,4,9,11,32,38;1,4,9,11,34,42", 2000000, 127),
    (181, 2635, "1,4,9,11,30,47,65,75;1,4,9,11,30,47,59,86;1,4,9,11,30,36,59,86;1,4,9,11,30,36,59,87;"
                "1,4,9,15,31,44,81", 4000000, 63),
    (362, 6440, "1,4,9,11,30,36,70,113,175;1,4,9,11,30,36,70,107,163;1,4,9,11,30,47,70,131,179;"
                "1,4,9,11,30,47,59,127,150", 4000000, 63),
    # round 5 (index 9-): N = 181 and 256 were still tied after round 4
    (181, 2637, "1,4,9,11,30,47,59;1,4,9,11,30,47,59,86;1,4,9,11,30,36,59,86;1,4,9,11,30,36,59,87;"
                "1,4,9,15,31,44,81", 4000000, 63),
    (256, 4165, "1,4,9,11,30,36,70,121;1,4,9,15,35,49,101,123;1,4,9,11,30,36,65,103,118;1,4,9,15,34,56,80;"
                "1,4,9,11,30,36,70,109", 4000000, 63),
    # round 6 (index 11-): the two README leads with twice the restart budget (the others hit within 2,255)
    (91, 1046, "1,4,9,11,30,44", 2000000, 255),
    (128, 1661, "1,4,9,11,30,36,59", 2000000, 255),
    # round 7 (index 13-): N = 256 separated at 4165, so the remaining ties go one level higher as well
    (181, 2640, "1,4,9,11,30,47,84;1,4,9,11,30,47,59,86;1,4,9,11,30,36,59,86;1,4,9,11,30,36,59,87;"
                "1,4,9,15,31,44,81", 4000000, 63),
    (362, 6450, "1,4,9,11,30,36,70,113,175;1,4,9,11,30,36,70,107,163;1,4,9,11,30,47,70,131,179;"
                "1,4,9,11,30,47,59,127,150", 4000000, 63),
    # round 8 (index 15): at twice the budget the README sequence also reached 1661 at N = 128
    (128, 1662, "1,4,9,11,34,43,62;1,4,9,11,30,36,59;1,4,9,11,30,47,59", 2000000, 127),
    # the two other N = 181 sequences at 2637 join the race to 2640 (same settings: rows are merged)
    (181, 2640, "1,4,9,11,30,47,59;1,4,9,11,30,47,65,75", 4000000, 63),
]


def main():
    first = int(sys.argv[1]) if len(sys.argv) > 1 else 0
    env = dict(os.environ, FSA_LUBY="1")
    for n, target, seqs, base, reps in RACES[first:]:
        t = time.time()
        print(f"=== N={n} target {target}: {seqs}", flush=True)
        subprocess.run([sys.executable, os.path.join(ROOT, "scripts", "race.py"), str(n), str(target), seqs,
                        "--iters", str(base), "--reps", str(reps), "--fresh", "--tag", "luby"], env=env)
        print(f"=== N={n} done in {time.time() - t:.0f}s", flush=True)


if __name__ == "__main__":
    main()
