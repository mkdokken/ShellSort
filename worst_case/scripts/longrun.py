"""Equal-budget long runs for tied large-N contenders: every sequence gets the same number of independent
long annealing runs (all threads), and the best re-simulated input is stored in results/strong.json.

usage: python scripts/longrun.py N "seq;seq;..." [--iters 200000000] [--reps 3]
"""
import json, os, re, subprocess, sys, time

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
RES = os.path.join(ROOT, "results")
SHELLWC = os.path.join(ROOT, "bin", "shellwc.exe")
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from witnesses import cost


def main():
    n, seqs = int(sys.argv[1]), sys.argv[2].split(";")
    iters = sys.argv[sys.argv.index("--iters") + 1] if "--iters" in sys.argv else "200000000"
    reps = int(sys.argv[sys.argv.index("--reps") + 1]) if "--reps" in sys.argv else 3
    p = os.path.join(RES, "strong.json")
    summary = []
    for s in seqs:
        best = 0
        for r in range(reps):
            t = time.time()
            out = subprocess.run([SHELLWC, "fwc", str(n), s, iters, str(5000 + 101 * r), "3.0", "0.05", "10"],
                                 capture_output=True, text=True).stdout
            m = re.search(r"perm=\[([\d,]+)\]", out)
            perm = [int(x) for x in m.group(1).split(",")]
            w = cost(perm, [int(x) for x in s.split(",")])
            st = json.load(open(p))
            key = f"{n}:{s}"
            rec = st.get(key, {"N": n, "seq": [int(x) for x in s.split(",")], "W_lower": 0, "runs": []})
            rec["runs"].append({"mode": "longrun", "iters": int(iters), "seed": 5000 + 101 * r, "W": w,
                                "seconds": round(time.time() - t)})
            if w > rec["W_lower"]:
                rec["W_lower"], rec["witness"] = w, perm
            st[key] = rec
            json.dump(st, open(p, "w"), indent=1)
            best = max(best, w)
            print(f"N={n} {{{s}}} run {r}: {w}  (stored best {rec['W_lower']}, {time.time()-t:.0f}s)", flush=True)
        summary.append((best, s))
    print(f"N={n} equal-budget ({reps} x {iters}/thread): " + ", ".join(f"{{{s}}} {w}" for w, s in sorted(summary)), flush=True)


if __name__ == "__main__":
    main()
