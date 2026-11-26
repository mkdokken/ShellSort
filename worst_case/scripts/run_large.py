"""Large-N driver (N > 45): exact DP is out of reach, so worst cases are certified LOWER bounds
(every number is attained by a concrete input permutation that is re-simulated).

  1. seed threshold: strong search (parallel anneal + window-DP polish) on our own optimum prefixes
     from a smaller N, each extended by one top gap  ->  T0 = best seed value
  2. branch-and-bound (bin/seqsearch --trust): every sequence/family that has an input costing > T0
     is pruned by that witness; survivors are candidates
  3. every candidate is re-evaluated with the strong search; T1 = best strong value.  If T1 < T0 the
     search is repeated with T0 = T1 (fixed-point iteration), so no sequence is pruned with a falsely
     low threshold
  4. README sequence for this N is evaluated with the same strong search (for comparison only)

usage: python scripts/run_large.py N --prefix 1,4,9,11,21 [--maxgap G] [--iters 6000000] [--seedgaps a-b]
"""
import json, os, re, subprocess, sys, time

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from readme_data import README_WORST

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
BIN = os.path.join(ROOT, "bin")
RES = os.path.join(ROOT, "results")
SHELLWC = os.path.join(BIN, "shellwc.exe")
SEQSEARCH = os.path.join(BIN, "seqsearch.exe")
LARGE = os.path.join(RES, "large.json")


def sh(args):
    return subprocess.run(args, capture_output=True, text=True).stdout


def strong(n, seq, iters, seed=17, T0=3.0):
    out = sh([SHELLWC, "fwc", str(n), ",".join(map(str, seq)), str(iters), str(seed), str(T0), "0.05", "10"])
    m = re.search(r"fwc_worst=(\d+).*check=(\d+).*perm=\[([\d,]+)\]", out)
    return (int(m.group(2)), [int(x) for x in m.group(3).split(",")]) if m else (None, None)


def parse_bb(path):
    rows = []
    with open(path) as f:
        for line in f:
            if line.startswith("W="):
                m = re.match(r"W=(\d+) (\S+) seq=\{([\d,]+)\}", line)
                rows.append({"W": int(m.group(1)), "exact": m.group(2) == "exact",
                             "seq": [int(x) for x in m.group(3).split(",")]})
    return rows


def main():
    n = int(sys.argv[1])
    arg = lambda k, d: sys.argv[sys.argv.index(k) + 1] if k in sys.argv else d
    prefix = [int(x) for x in arg("--prefix", "1,4,9,11,21").split(",")]
    maxgap = int(arg("--maxgap", str(n - 1)))
    iters = int(arg("--iters", "6000000"))
    lo, hi = [int(x) for x in arg("--seedgaps", f"{max(prefix)+1}-{maxgap}").split("-")]
    restrict = arg("--prefixes", None)          # "1,4,9;1,4,10" : explore only these families
    db = json.load(open(LARGE)) if os.path.exists(LARGE) else {}
    rec = db.get(str(n), {"N": n, "evals": {}})
    evals = rec["evals"]
    log = lambda m: print(m, flush=True)

    def ev(seq, it=iters):
        key = ",".join(map(str, seq))
        if key in evals and evals[key]["iters"] >= it:
            return evals[key]["W"]
        w, perm = strong(n, seq, it)
        old = evals.get(key)
        if old is None or w > old["W"]:
            evals[key] = {"W": w, "iters": it, "witness": perm}
        else:
            old["iters"] = max(old["iters"], it)
        rec["evals"] = evals; db[str(n)] = rec
        json.dump(db, open(LARGE, "w"), indent=1)
        return evals[key]["W"]

    t = time.time()
    # 1. seeds
    seeds = [prefix] + [prefix + [x] for x in range(lo, hi + 1, max(1, (hi - lo) // 24))]
    T0 = min(ev(s) for s in seeds)
    log(f"N={n}: seed threshold T0={T0} from {len(seeds)} seeds ({time.time()-t:.0f}s)")
    # 2-3. fixed-point search
    for rnd in range(1, 6):
        out = os.path.join(RES, f"bb_N{n}_r{rnd}.txt")
        cmd = [SEQSEARCH, str(n), "--trust", "--T0", str(T0), "--maxgap", str(maxgap),
               "--temp", "2.0", "--exact", "2000000", "--out", out]
        if restrict:
            cmd += ["--prefixes", restrict]
        with open(os.path.join(RES, f"bb_N{n}_r{rnd}.log"), "w") as lf:
            subprocess.run(cmd, stderr=lf)
        cands = parse_bb(out)
        log(f"  round {rnd}: {len(cands)} candidates with search value <= {T0}")
        for c in cands:
            c["strong"] = ev(c["seq"])
        T1 = min([T0] + [c["strong"] for c in cands])
        rec.setdefault("rounds", []).append({"T0": T0, "candidates": cands, "T1": T1})
        db[str(n)] = rec; json.dump(db, open(LARGE, "w"), indent=1)
        if T1 < T0:
            log(f"  strong re-evaluation lowered the threshold {T0} -> {T1}; repeating search")
            T0 = T1
            continue
        break
    best = sorted(((v["W"], len(k.split(",")), k) for k, v in evals.items()))[:10]
    rec["best"] = [{"seq": [int(x) for x in k.split(",")], "W_lower": w} for w, _, k in best]
    # 4. README comparison (same strong search, more effort)
    if n in README_WORST:
        rs, rw = README_WORST[n][0], README_WORST[n][1]
        rec["readme"] = {"seq": rs, "W_readme": rw, "W_ours_on_readme_seq": ev(rs, iters * 2)}
    rec["time_s"] = round(time.time() - t)
    db[str(n)] = rec
    json.dump(db, open(LARGE, "w"), indent=1)
    log(f"N={n}: best {rec['best'][:3]}  readme={rec.get('readme')}  time={time.time()-t:.0f}s")


if __name__ == "__main__":
    main()
