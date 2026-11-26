"""Adversarial check of every large-N finalist whose stored lower bound is below the README sequence's
best-known value: a targeted search tries to reach that value.  Success = the finalist is not better.

usage: python scripts/attack_all.py [--ns 64,91,...] [--iters 8000000] [--reps 12]
"""
import json, os, subprocess, sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from readme_data import README_WORST

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
RES = os.path.join(ROOT, "results")


def lb_table(n):
    cand = {}
    for db in ("transfer.json", "transfer_div.json"):
        p = os.path.join(RES, db)
        if os.path.exists(p):
            d = json.load(open(p)).get(str(n), {})
            for r in d.get("final", []) + ([d["readme"]] if d.get("readme") else []):
                t = ",".join(map(str, r["seq"]))
                cand[t] = max(cand.get(t, 0), r["W_lower"])
    st = json.load(open(os.path.join(RES, "strong.json")))
    for k, v in st.items():
        N, s = k.split(":")
        if int(N) == n:
            cand[s] = max(cand.get(s, 0), v["W_lower"])
    return cand


def main():
    arg = lambda k, d: sys.argv[sys.argv.index(k) + 1] if k in sys.argv else d
    ns = [int(x) for x in arg("--ns", "64,91,128,181,256,362").split(",")]
    iters, reps = arg("--iters", "8000000"), arg("--reps", "12")
    for n in ns:
        cand = lb_table(n)
        rkey = ",".join(map(str, README_WORST[n][0]))
        target = cand.get(rkey)
        if not target:
            continue
        weaker = sorted((w, s) for s, w in cand.items() if s != rkey and w < target)
        print(f"N={n}: README sequence best-known {target}; {len(weaker)} finalists below it", flush=True)
        for w, s in weaker:
            subprocess.run([sys.executable, os.path.join(ROOT, "scripts", "challenge.py"), str(n), s, str(target),
                            "--iters", iters, "--reps", reps])


if __name__ == "__main__":
    main()
