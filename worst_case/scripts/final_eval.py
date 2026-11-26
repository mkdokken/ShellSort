"""Equal-effort comparison at large N: the README sequence and our top candidates (from both transfer
chains) each get the same number of independent strong runs; the best certified input per sequence
is kept in results/strong.json (via strong_eval.py's format).

usage: python scripts/final_eval.py [--iters 40000000] [--reps 3] [--top 3] [--ns 64,91,128,181,256,362]
"""
import json, os, subprocess, sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from readme_data import README_WORST

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
RES = os.path.join(ROOT, "results")


def arg(k, d):
    return sys.argv[sys.argv.index(k) + 1] if k in sys.argv else d


def ours(n, top):
    seen, out = set(), []
    for db in ("transfer.json", "transfer_div.json"):
        p = os.path.join(RES, db)
        if not os.path.exists(p):
            continue
        d = json.load(open(p)).get(str(n))
        if not d:
            continue
        for r in d["final"][:top]:
            t = tuple(r["seq"])
            if t not in seen:
                seen.add(t); out.append((r["W_lower"], t))
    out.sort()
    return [t for _, t in out[:top + 1]]


def main():
    iters = arg("--iters", "40000000")
    reps = arg("--reps", "3")
    top = int(arg("--top", "3"))
    ns = [int(x) for x in arg("--ns", "64,91,128,181,256,362").split(",")]
    for n in ns:
        seqs = ours(n, top)
        rd = tuple(README_WORST[n][0])
        if rd not in seqs:
            seqs.append(rd)
        args = [",".join(map(str, s)) for s in seqs]
        print(f"N={n}: {len(args)} sequences x {reps} runs x {iters} iterations/thread", flush=True)
        subprocess.run([sys.executable, os.path.join(ROOT, "scripts", "strong_eval.py"), str(n)] + args +
                       ["--iters", iters, "--reps", reps])


if __name__ == "__main__":
    main()
