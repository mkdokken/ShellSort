"""Prefix-transfer beam search for large N (no exact DP possible; values are certified lower bounds).

Candidates are COMPLETE sequences: every top sequence found at the previous size, truncated by 0..2 of
its largest gaps, combined with 0..2 new largest gaps chosen jointly (all pairs on a grid up to maxgap).
All candidates are annealed in parallel (incremental evaluator, one candidate per thread), the best are
re-evaluated with 10x effort, and the finalists (plus the README sequence, for comparison only) get an
equal heavy multi-thread search.  Lower worst case = better.

usage: python scripts/transfer.py N PREVN [--maxgap G] [--top 8] [--grid 1] [--iters 1000000]
                                        [--finalists 10] [--strong 20000000]
"""
import json, os, re, subprocess, sys, time, itertools

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from readme_data import README_WORST

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
RES = os.path.join(ROOT, "results")
SHELLWC = os.path.join(ROOT, "bin", "shellwc.exe")
DB = os.path.join(RES, "transfer.json")


def arg(k, d):
    return sys.argv[sys.argv.index(k) + 1] if k in sys.argv else d


def batch(n, seqs, iters, seed):
    path = os.path.join(RES, f"_batch_{n}.txt")
    with open(path, "w") as f:
        for s in seqs:
            f.write(",".join(map(str, s)) + "\n")
    out = subprocess.run([SHELLWC, "batch", str(n), path, str(iters), str(seed)], capture_output=True, text=True).stdout
    res = {}
    for line in out.splitlines():
        w, s = line.split()
        res[tuple(int(x) for x in s.split(","))] = int(w)
    return res


def strong(n, seq, iters, seed):
    out = subprocess.run([SHELLWC, "fwc", str(n), ",".join(map(str, seq)), str(iters), str(seed), "3.0", "0.05", "10"],
                         capture_output=True, text=True).stdout
    m = re.search(r"fwc_worst=(\d+).*check=(\d+).*perm=\[([\d,]+)\]", out)
    return int(m.group(2)), [int(x) for x in m.group(3).split(",")]


def previous_top(prevn, k):
    """our own best sequences at the previous size (transfer.json, large.json or summary.json)"""
    db = json.load(open(DB)) if os.path.exists(DB) else {}
    if str(prevn) in db:
        return [tuple(r["seq"]) for r in db[str(prevn)]["final"][:k]]
    lg = os.path.join(RES, "large.json")
    if os.path.exists(lg):
        L = json.load(open(lg))
        if str(prevn) in L:
            ev = sorted(((v["W"], len(s.split(",")), s) for s, v in L[str(prevn)]["evals"].items()))
            return [tuple(int(x) for x in s.split(",")) for _, _, s in ev[:k]]
    S = json.load(open(os.path.join(RES, "summary.json")))
    return [tuple(r["seq"]) for r in S[str(prevn)]["optimal"][:k]]


def main():
    n, prevn = int(sys.argv[1]), int(sys.argv[2])
    maxgap = int(arg("--maxgap", str(n // 2)))
    top = int(arg("--top", "8"))
    grid = int(arg("--grid", "1"))
    iters = int(arg("--iters", "1000000"))
    nfin = int(arg("--finalists", "10"))
    siters = int(arg("--strong", "20000000"))
    nadd = int(arg("--add", "2"))
    ncut = int(arg("--cut", "2"))
    global DB
    DB = os.path.join(RES, arg("--db", "transfer.json"))
    t = time.time()
    prefixes = set()
    explicit = arg("--from-seqs", None)          # "1,4,7,9,19,34;1,4,9,11,21,35;..." (our own results)
    sources = [tuple(int(x) for x in t.split(",")) for t in explicit.split(";")] if explicit else previous_top(prevn, top)
    for s in sources:
        s = tuple(g for g in s if g <= maxgap)
        for cut in range(0, ncut + 1):
            if len(s) - cut >= 2:
                prefixes.add(s[:len(s) - cut])
    cands = set()
    for p in prefixes:
        lo = max(p) + 1
        rng = list(range(lo, maxgap + 1, grid))
        cands.add(p)
        for k in range(1, nadd + 1):            # up to nadd new gaps chosen jointly
            for comb in itertools.combinations(rng, k):
                cands.add(p + comb)
    cands = sorted(cands)
    print(f"N={n}: {len(prefixes)} prefixes from N={prevn}, {len(cands)} candidates", flush=True)
    r1 = batch(n, cands, iters, 11)
    best1 = sorted(r1.items(), key=lambda kv: (kv[1], len(kv[0])))
    print(f"  phase 1 ({time.time()-t:.0f}s): best {best1[:3]}", flush=True)
    keep = [s for s, _ in best1[:max(60, nfin * 6)]]
    r2 = batch(n, keep, iters * 10, 12)
    for s in keep:
        r2[s] = max(r2[s], r1[s])
    best2 = sorted(r2.items(), key=lambda kv: (kv[1], len(kv[0])))
    print(f"  phase 2 ({time.time()-t:.0f}s): best {best2[:3]}", flush=True)
    # finalists: round-robin over families (first four gaps) so one family cannot crowd out the others
    groups = {}
    for s, w in best2:
        groups.setdefault(s[:4], []).append(s)
    order = sorted(groups, key=lambda g: r2[groups[g][0]])
    finals, depth = [], 0
    while len(finals) < nfin and any(len(groups[g]) > depth for g in order):
        for g in order:
            if len(groups[g]) > depth and len(finals) < nfin:
                finals.append(groups[g][depth])
        depth += 1
    readme = tuple(README_WORST[n][0]) if n in README_WORST else None
    out = []
    for s in finals + ([readme] if readme and readme not in finals else []):
        w, perm = strong(n, s, siters, 13)
        w = max(w, r2.get(s, 0))
        out.append({"seq": list(s), "W_lower": w, "witness": perm, "readme": s == readme,
                    "found_by_search": s in finals})
        print(f"  strong {{{','.join(map(str, s))}}}: {w}{'  (README sequence)' if s == readme else ''}", flush=True)
    out.sort(key=lambda r: (r["W_lower"], len(r["seq"])))
    db = json.load(open(DB)) if os.path.exists(DB) else {}
    db[str(n)] = {"N": n, "prev": prevn, "maxgap": maxgap, "n_candidates": len(cands),
                  "final": [r for r in out if r["found_by_search"]],
                  "readme": next((r for r in out if r["readme"]), None),
                  "readme_reported": README_WORST.get(n, [None, None])[1],
                  "time_s": round(time.time() - t)}
    json.dump(db, open(DB, "w"), indent=1)
    print(f"N={n}: best {out[0]['seq']} W>={out[0]['W_lower']} ({time.time()-t:.0f}s)", flush=True)


if __name__ == "__main__":
    main()
