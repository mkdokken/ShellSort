"""Targeted attack on a candidate sequence: search until an input reaching `target` comparisons is found
(or the budget runs out), and record the witness in results/strong.json.

Used to test large-N challengers against an incumbent: a challenger is only "better" if a long targeted
search cannot push its worst case up to the incumbent's value.

usage: python scripts/challenge.py N 1,4,9,11,32,62 TARGET [--iters 4000000] [--reps 1000] [--luby] [--seed S]
       --luby: Luby restart schedule (restart r runs iters * luby(r+1) moves; FSA_LUBY=1)
"""
import json, os, re, subprocess, sys, time

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SHELLWC = os.path.join(ROOT, "bin", "shellwc.exe")
OUT = os.path.join(ROOT, "results", "strong.json")


def cost(perm, gaps):
    a, n, c = list(perm), len(perm), 0
    for h in sorted({g for g in gaps if g < n} | {1}, reverse=True):
        for i in range(h, n):
            v, j = a[i], i
            while j >= h:
                c += 1
                if a[j - h] > v:
                    a[j] = a[j - h]; j -= h
                else:
                    break
            a[j] = v
    return c


def main():
    n, s, target = int(sys.argv[1]), sys.argv[2], int(sys.argv[3])
    iters = sys.argv[sys.argv.index("--iters") + 1] if "--iters" in sys.argv else "4000000"
    reps = sys.argv[sys.argv.index("--reps") + 1] if "--reps" in sys.argv else "1000"
    seed = sys.argv[sys.argv.index("--seed") + 1] if "--seed" in sys.argv else "4242"
    luby = "--luby" in sys.argv
    t = time.time()
    out = subprocess.run([SHELLWC, "fwc", str(n), s, iters, seed, "3.0", "0.05", "10", str(target), reps],
                         capture_output=True, text=True, env=dict(os.environ, FSA_LUBY="1" if luby else "0")).stdout
    m = re.search(r"fwc_worst=(\d+).*restarts=(\d+).*perm=\[([\d,]+)\]", out)
    perm = [int(x) for x in m.group(3).split(",")]
    w = cost(perm, [int(x) for x in s.split(",")])           # independent re-simulation
    db = json.load(open(OUT)) if os.path.exists(OUT) else {}
    key = f"{n}:{s}"
    rec = db.get(key, {"N": n, "seq": [int(x) for x in s.split(",")], "W_lower": 0, "runs": []})
    run = {"iters": int(iters), "target": target, "W": w, "seconds": round(time.time() - t)}
    if luby:
        run.update(schedule="luby", restarts=int(m.group(2)), seed=int(seed))
    rec["runs"].append(run)
    if w > rec["W_lower"]:
        rec["W_lower"], rec["witness"] = w, perm
    db[key] = rec
    json.dump(db, open(OUT, "w"), indent=1)
    print(f"N={n} {{{s}}}: reached {w} (target {target}, {m.group(2)} restarts{', Luby' if luby else ''}) in "
          f"{time.time()-t:.0f}s; best recorded {rec['W_lower']}", flush=True)


if __name__ == "__main__":
    main()
