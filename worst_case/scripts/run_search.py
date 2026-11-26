"""Driver: rigorous worst-case gap-sequence search for a range of N.

For each N:
  1. T0 = exact worst case (DP) of the previous N's optimal sequences evaluated at N (a valid upper
     bound on the optimum; only used to start pruning early -- the search itself is exhaustive).
  2. run bin/seqsearch (parallel branch-and-bound) -> results/bb_N{N}.txt
  3. certification loop: every candidate the search could only lower-bound (exact DP too large for the
     in-search limit) and whose bound is <= T is evaluated exactly with the big parallel DP
     (shellwc dp2p, or the layered DP when even that does not fit in memory).  If a strictly better
     exact value appears, the search is re-run from that bound so that the final tie list is complete.
  4. average-case comparisons of every optimal sequence (exact DP if small, else sampling)
  5. update results/summary.json

usage: python scripts/run_search.py 31 38 [--force] [--exact M] [--trust]
"""
import json, os, re, subprocess, sys, time

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
BIN = os.path.join(ROOT, "bin")
RES = os.path.join(ROOT, "results")
SUMMARY = os.path.join(RES, "summary.json")
CACHE = os.path.join(RES, "exact_cache.json")
DENSE_MAX = 4_500_000_000      # states for the dense parallel DP (2 bytes/state)
LAYER_MAX = 11_800_000_000     # two-level states (one byte each); total bytes are capped at 12 GB in C
SHELLWC = os.path.join(BIN, "shellwc.exe")


def sh(args, timeout=None):
    return subprocess.run(args, capture_output=True, text=True, timeout=timeout).stdout


def load(path, default):
    if os.path.exists(path):
        with open(path) as f:
            return json.load(f)
    return default


def save(path, obj):
    tmp = path + ".tmp"
    with open(tmp, "w") as f:
        json.dump(obj, f, indent=1)
    os.replace(tmp, path)


def seqkey(n, seq):
    return f"{n}:{','.join(map(str, seq))}"


def dp_size(n, seq, with_kh=False):
    out = sh([SHELLWC, "size", str(n), ",".join(map(str, seq))])
    pairs = [(int(s), int(k)) for k, s in re.findall(r"kh=(\d+) states=(-?\d+)", out) if int(s) > 0]
    if not pairs:
        return (None, None) if with_kh else None
    s, k = min(pairs)
    return (s, k) if with_kh else s


def strong_lower_bound(n, seq, iters=6_000_000, T0=3.0):
    """best input found by a strong parallel anneal + window-DP polish (a certified lower bound)."""
    out = sh([SHELLWC, "fwc", str(n), ",".join(map(str, seq)), str(iters * 4), "17", str(T0), "0.05", "10"])
    m = re.search(r"fwc_worst=(\d+).*check=(\d+)", out)
    return int(m.group(2)) if m else None


def exact_worst(n, seq, cache, limit=200_000_000, allow_big=False, log=None, kill_above=None):
    """exact worst case (cached).  limit: dense states for a quick call; allow_big: certification.
    kill_above: if a strong anneal finds an input costing more than this, return that (> kill_above)
    instead of running an expensive certification (the sequence is then provably not optimal)."""
    key = seqkey(n, seq)
    if key in cache:
        return cache[key]["W"]
    s = ",".join(map(str, seq))
    size, kh = dp_size(n, seq, with_kh=True)
    W, how = None, None
    if allow_big and kill_above is not None and (size is None or size > 300_000_000):
        lb = strong_lower_bound(n, seq)
        if lb is not None and lb > kill_above:
            if log:
                log(f"    W{{{s}}}@{n} >= {lb} > {kill_above} (strong anneal witness; not certified further)")
            return lb
    if size is not None and size <= limit:
        out = sh([SHELLWC, "dp2", str(n), s, str(limit)])
        m = re.search(r"dp2_worst=(\d+).*check=(\d+)", out)
        if m:
            W, how = int(m.group(1)), "dp2"
    elif allow_big and size is not None and size <= DENSE_MAX and kh != 0:
        t = time.time()
        out = sh([SHELLWC, "dp2p", str(n), s, str(DENSE_MAX)])
        m = re.search(r"dp2p_worst=(\d+).*check=(\d+)", out)
        if m:
            W, how = int(m.group(1)), f"dp2p({size} states, {time.time()-t:.0f}s)"
    if W is None and allow_big:
        t = time.time()
        out = sh([SHELLWC, "lay", str(n), s, str(LAYER_MAX)])
        m = re.search(r"lay_worst=(\d+)", out)
        if m:
            W, how = int(m.group(1)), f"layered({time.time()-t:.0f}s)"
    if W is not None:
        cache[key] = {"W": W, "how": how}
        save(CACHE, cache)
        if log:
            log(f"    exact W{{{s}}}@{n} = {W}  [{how}]")
    return W


def average(n, seq):
    s = ",".join(map(str, seq))
    out = sh([SHELLWC, "xavg", str(n), s, "100000000"])
    m = re.search(r"xavg=([\d.]+)", out)
    if m:
        return float(m.group(1)), 0.0, "exact"
    samples = 20_000_000 if n <= 64 else 2_000_000
    out = sh([SHELLWC, "avg", str(n), s, str(samples), "7"])
    m = re.search(r"avg=([\d.]+) \+/- ([\d.]+)", out)
    return float(m.group(1)), float(m.group(2)), "sampled"


def parse_bb(path):
    head, rows = {}, []
    with open(path) as f:
        for line in f:
            if line.startswith("# N="):
                for k, v in re.findall(r"(\w+)=([\d.]+)", line):
                    head[k] = float(v) if "." in v else int(v)
            elif line.startswith("W="):
                m = re.match(r"W=(\d+) (\S+) seq=\{([\d,]+)\} states=(-?\d+) worst_input=\[([\d,]+)\]", line)
                rows.append({"W": int(m.group(1)), "exact": m.group(2) == "exact",
                             "seq": [int(x) for x in m.group(3).split(",")],
                             "states": int(m.group(4)),
                             "worst_input": [int(x) for x in m.group(5).split(",")]})
    return head, rows


def main():
    a, b = int(sys.argv[1]), int(sys.argv[2])
    extra = []
    force = "--force" in sys.argv
    if "--trust" in sys.argv:
        extra.append("--trust")
    if "--exact" in sys.argv:
        extra += ["--exact", sys.argv[sys.argv.index("--exact") + 1]]
    summ = load(SUMMARY, {})
    cache = load(CACHE, {})
    log = lambda msg: print(msg, flush=True)
    for n in range(a, b + 1):
        out = os.path.join(RES, f"bb_N{n}.txt")
        if not force and str(n) in summ:
            log(f"N={n}: already done"); continue
        # Initial threshold.  Any value works for rigour: if T0 < W* the search returns no candidate (every
        # sequence gets a witness > T0) and T0 is raised; the final value always comes from exact DPs.
        T0 = n * (n - 1) // 2
        prev = summ.get(str(n - 1))
        if prev:
            for r in prev["optimal"][:4]:
                w = exact_worst(n, r["seq"], cache)          # cheap exact DP only (<= 2e8 states)
                if w is None:
                    lb = strong_lower_bound(n, r["seq"])
                    w = lb + 2 if lb is not None else None
                if w is not None:
                    T0 = min(T0, w)
        log(f"N={n}: initial threshold T0={T0}")
        t = time.time()
        rounds = 0
        while True:
            rounds += 1
            subprocess.run([os.path.join(BIN, "seqsearch.exe"), str(n), "--T0", str(T0), "--quiet", "--out", out] + extra)
            head, rows = parse_bb(out)
            Tf = head["T_final"]
            if not rows:
                log(f"  N={n}: no sequence with W <= {T0}; raising threshold")
                T0 += 3
                continue
            # certification of lower-bound-only candidates
            for r in sorted([r for r in rows if not r["exact"]], key=lambda r: r["W"]):
                if r["W"] > Tf:
                    continue
                Tcur = min([Tf] + [x["W"] for x in rows if x["exact"]])
                w = exact_worst(n, r["seq"], cache, allow_big=True, log=log, kill_above=Tcur)
                if w is not None:
                    if seqkey(n, r["seq"]) in cache:
                        r["W"], r["exact"], r["certified"] = w, True, True
                    else:                      # killed by a witness above the threshold
                        r["W"], r["killed"] = w, True
            exact_vals = [r["W"] for r in rows if r["exact"]]
            Tbest = min([Tf] + exact_vals)
            if Tbest < Tf and rounds < 8:
                log(f"  N={n}: certified value {Tbest} < search threshold {Tf}; re-running search with T0={Tbest}")
                T0 = Tbest
                continue
            if not any(r["exact"] and r["W"] == Tbest for r in rows) and \
               not any(not r["exact"] and not r.get("killed") and r["W"] <= Tbest for r in rows):
                log(f"  N={n}: every candidate is above {Tbest}; raising threshold")
                T0 = Tbest + 3
                continue
            break
        opt = [r for r in rows if r["exact"] and r["W"] == Tbest]
        unresolved = [r for r in rows if not r["exact"] and not r.get("killed") and r["W"] <= Tbest]
        for r in opt:
            r["avg"], r["avg_se"], r["avg_kind"] = average(n, r["seq"])
        opt.sort(key=lambda r: r["avg"])
        summ[str(n)] = {"N": n, "W": Tbest, "T0": T0, "search": head, "rounds": rounds,
                        "all_exact": len(unresolved) == 0, "optimal": opt, "unresolved": unresolved,
                        "time_s": round(time.time() - t, 1)}
        save(SUMMARY, summ)
        best = opt[0] if opt else None
        log(f"N={n}: W*={Tbest} ties={len(opt)} best={best['seq'] if best else None} "
            f"avg={best['avg'] if best else float('nan'):.3f} unresolved={len(unresolved)} "
            f"rounds={rounds} time={time.time()-t:.0f}s")


if __name__ == "__main__":
    main()
