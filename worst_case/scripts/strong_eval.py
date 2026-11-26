"""Equal-effort strong evaluation of gap sequences at large N (fair comparison of lower bounds).

Each sequence gets `reps` independent runs of the parallel anneal + window-DP polish (different seeds,
hot start), and the best certified input is kept.  Results are merged into results/strong.json
(keeping the best witness ever found per (N, sequence)).

usage: python scripts/strong_eval.py N "1,4,9,11,30,44" ["1,4,9,11,30,45" ...] [--iters 20000000] [--reps 3]
"""
import json, os, re, subprocess, sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SHELLWC = os.path.join(ROOT, "bin", "shellwc.exe")
OUT = os.path.join(ROOT, "results", "strong.json")


def main():
    n = int(sys.argv[1])
    seqs = [a for a in sys.argv[2:] if re.fullmatch(r"[\d,]+", a)]
    iters = int(sys.argv[sys.argv.index("--iters") + 1]) if "--iters" in sys.argv else 20_000_000
    reps = int(sys.argv[sys.argv.index("--reps") + 1]) if "--reps" in sys.argv else 3
    db = json.load(open(OUT)) if os.path.exists(OUT) else {}
    for s in seqs:
        key = f"{n}:{s}"
        rec = db.get(key, {"N": n, "seq": [int(x) for x in s.split(",")], "W_lower": 0, "runs": []})
        for r in range(reps):
            seed = 1000 + 37 * len(rec["runs"]) + r
            out = subprocess.run([SHELLWC, "fwc", str(n), s, str(iters), str(seed), "3.0", "0.05", "10"],
                                 capture_output=True, text=True).stdout
            m = re.search(r"fwc_worst=(\d+).*check=(\d+).*perm=\[([\d,]+)\]", out)
            w = int(m.group(2))
            rec["runs"].append({"iters": iters, "seed": seed, "W": w})
            if w > rec["W_lower"]:
                rec["W_lower"] = w
                rec["witness"] = [int(x) for x in m.group(3).split(",")]
            print(f"N={n} {{{s}}} run {r}: {w}  (best {rec['W_lower']})", flush=True)
        db[key] = rec
        json.dump(db, open(OUT, "w"), indent=1)


if __name__ == "__main__":
    main()
