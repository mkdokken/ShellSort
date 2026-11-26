"""Validation of the front pair rules (exact2.h x2_front_rules): on random small sequences whose leading pair
passes join chain heads to chain tails of the next gap, the rules DP (dp2r: state space without the pair passes)
must equal the general exact DP (dp2: pair passes absorbed), and its reconstructed input must attain the value.

usage: python scripts/test_rules.py [COUNT] [--exe bin/shellwc.exe]
"""
import os, random, re, subprocess, sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
EXE = sys.argv[sys.argv.index("--exe") + 1] if "--exe" in sys.argv else os.path.join(ROOT, "bin", "shellwc.exe")


def run(*args):
    return subprocess.run([EXE, *map(str, args)], capture_output=True, text=True).stdout


def main():
    count = int(sys.argv[1]) if len(sys.argv) > 1 and sys.argv[1].isdigit() else 200
    rng = random.Random(4646)
    done = skipped = binding = 0
    while done < count:
        L = rng.randint(3, 9)
        n = rng.randint(2 * L + 1, min(3 * L + 2, 24))
        pairs = [h for h in range(max(n - L, (n + 1) // 2, L), n) if h % L]
        if not pairs:
            continue
        top = rng.sample(pairs, rng.randint(1, min(3, len(pairs))))
        low = rng.sample(range(2, L), rng.randint(0, min(3, L - 2)))
        seq = ",".join(map(str, sorted({1, L, *low, *top})))
        r = run("dp2r", n, seq)
        if "NOT_APPLICABLE" in r:
            skipped += 1
            continue
        g = run("dp2", n, seq)
        mr = re.search(r"rules_worst=(\d+) states=\d+ check=(\d+)", r)
        mg = re.search(r"dp2_worst=(\d+)", g)
        if not mr or not mg:
            print("FAILED to run", n, seq, r.strip(), g.strip()); sys.exit(1)
        wr, chk, wg = int(mr.group(1)), int(mr.group(2)), int(mg.group(1))
        if wr != wg or chk != wr:
            print(f"MISMATCH N={n} {{{seq}}}: rules {wr} (witness {chk}) vs general {wg}"); sys.exit(1)
        # do the constraints bind?  (unconstrained: worst case without the pair passes plus their fixed cost)
        free = int(re.search(r"dp2_worst=(\d+)", run("dp2", n, ",".join(map(str, sorted({1, L, *low}))))).group(1))
        binding += wr < free + sum(n - h for h in top)
        done += 1
    print(f"front pair rules: {done} random sequences agree with the general DP (witness attains the value), "
          f"constraints binding in {binding}; {skipped} drawn sequences were not of the rules form")


if __name__ == "__main__":
    main()
