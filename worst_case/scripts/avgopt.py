"""Worst case of the README's average-case-optimal sequences (README_AVGSEQ_WORST, plus USER_AVGSEQ supplied by
the README author for N = 46): exact where the DP fits (N <= 46: dp2p, or the layered DP with one-byte or 16-bit
cells and disk-backed input reconstruction), otherwise the best input found by Luby-restart annealing (a lower
bound).  Every value is backed by an input re-simulated here.  Results: results/avgopt.json.

usage: python scripts/avgopt.py exact DUMPDIR [N ...] [--cell16]   exact values (dumps go to DUMPDIR, then
                                                            deleted; --cell16 skips the one-byte layered attempt)
       python scripts/avgopt.py lower N BASE REPS [...] [--seed S]   lower bounds, FSA_LUBY=1, best over restarts
       python scripts/avgopt.py push N TARGET ITERS REPS [...]   fixed-length restarts until TARGET is reached
       python scripts/avgopt.py all DUMPDIR                 both, with the default budgets below
       python scripts/avgopt.py fixed N MOVES [--method luby|pt] [--seed S] [--tag T] [--base B] [--T0 T]
                                [--R R] [--Tmin T] [--Tmax T] [--mpe M]
                                                            one fixed-budget attack as in maxout.py (no target; --base
                                                            defaults to MOVES: one anneal of the whole budget per thread)
       python scripts/avgopt.py effort                      annealing moves per thread spent on each sequence so far,
                                                            against the best worst-case sequence of the same size
"""
import json, os, re, shutil, subprocess, sys, time

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
RES = os.path.join(ROOT, "results")
SHELLWC = os.path.join(ROOT, "bin", "shellwc.exe")
SHELLWC16 = os.path.join(ROOT, "bin", "shellwc16.exe")       # layered DP with 16-bit cells (build.ps1)
DB = os.path.join(RES, "avgopt.json")
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from witnesses import cost
from readme_data import README_AVGSEQ_WORST, USER_AVGSEQ

# Luby base moves and restarts per thread for the sizes where no exact DP fits (15-38 min each here)
BUDGET = {64: (2000000, 255), 91: (2000000, 255), 128: (2000000, 255), 181: (4000000, 127), 256: (4000000, 31),
          362: (4000000, 63)}


def readme(n):
    """(sequence, README worst case or None for a sequence supplied outside the README table)"""
    return README_AVGSEQ_WORST.get(n) or (USER_AVGSEQ[n], None)


def load():
    return json.load(open(DB)) if os.path.exists(DB) else {}


def record(n, seq, W, exact, how, secs, perm, **extra):
    db = load()
    r = db.get(str(n), {"N": n, "seq": seq, "readme_W": readme(n)[1], "W": 0, "exact": False, "runs": []})
    if perm:
        c = cost(perm, seq)
        if exact and c != W:
            sys.exit(f"N={n} {seq}: DP value {W} but its input re-simulates to {c}")
        W = c
    r["runs"].append(dict({"how": how, "W": W, "exact": exact, "seconds": round(secs)}, **extra))
    if exact:
        r["W"], r["exact"] = W, True
        if perm:
            r["witness"] = perm
    elif not r["exact"] and W > r["W"]:
        r["W"], r["witness"] = W, perm
    db[str(n)] = r
    json.dump(db, open(DB, "w"), indent=1)
    rw = r["readme_W"]
    verdict = ("no README value" if rw is None else "matches README" if r["W"] == rw else
               f"README {rw} too low" if r["W"] > rw else f"BELOW README {rw}" + (" (contradiction!)" if exact else ""))
    print(f"N={n} {{{','.join(map(str, seq))}}}: {'exact' if r['exact'] else '>='} {r['W']} ({how}, {secs:.0f}s) -> {verdict}",
          flush=True)
    return r


def exact(n, dump):
    seq = readme(n)[0]
    s = ",".join(map(str, seq))
    t = time.time()
    out = subprocess.run([SHELLWC, "dp2p", str(n), s, "6000000000"], capture_output=True, text=True).stdout
    m = re.search(r"dp2p_worst=(\d+) states=(\d+) check=(\d+) time=[\d.]+s perm=\[([\d,]+)\]", out)
    if m:
        return record(n, seq, int(m.group(1)), True, f"dp2p, {int(m.group(2)):.3g} states", time.time() - t,
                      [int(x) for x in m.group(4).split(",")])
    # layered DP, every level dumped for the reconstruction; 16-bit cells if the residual exceeds one byte
    tries = ((SHELLWC, "layered one-byte DP"), (SHELLWC16, "layered 16-bit DP"))
    for exe, how in (tries[1:] if "--cell16" in sys.argv else tries):
        d = os.path.join(dump, f"avgopt_N{n}")
        os.makedirs(d, exist_ok=True)
        try:
            p = subprocess.run([exe, "lay", str(n), s, "12000000000", "-1", d], capture_output=True, text=True)
        finally:
            shutil.rmtree(d, ignore_errors=True)
        m = re.search(r"lay_worst=(\d+) check=(\d+).*perm=\[([\d,]+)\]", p.stdout)
        if m:
            return record(n, seq, int(m.group(1)), True, how, time.time() - t, [int(x) for x in m.group(3).split(",")])
        print(f"N={n} {{{s}}}: {how} failed: {p.stdout.strip()} {p.stderr.strip()[-200:]}", flush=True)
    return None


def lower(n, base, reps, seed=4242):
    seq = readme(n)[0]
    s = ",".join(map(str, seq))
    t = time.time()
    out = subprocess.run([SHELLWC, "fwc", str(n), s, str(base), str(seed), "3.0", "0.05", "10", "1000000000", str(reps)],
                         capture_output=True, text=True, env=dict(os.environ, FSA_LUBY="1")).stdout
    m = re.search(r"fwc_worst=(\d+).*restarts=(\d+).*perm=\[([\d,]+)\]", out)
    return record(n, seq, int(m.group(1)), False, f"Luby restarts, {int(m.group(2))} x base {base // 1000000}M moves",
                  time.time() - t, [int(x) for x in m.group(3).split(",")])


def push(n, target, iters, reps):
    """targeted attack with fixed-length restarts (no Luby): stops as soon as an input reaches TARGET"""
    seq = readme(n)[0]
    t = time.time()
    out = subprocess.run([SHELLWC, "fwc", str(n), ",".join(map(str, seq)), str(iters), "99", "3.0", "0.05", "10",
                          str(target), str(reps)], capture_output=True, text=True).stdout
    m = re.search(r"fwc_worst=(\d+).*restarts=(\d+).*perm=\[([\d,]+)\]", out)
    return record(n, seq, int(m.group(1)), False, f"fixed {iters // 1000000}M-move restarts towards {target} "
                  f"({int(m.group(2))} restarts)", time.time() - t, [int(x) for x in m.group(3).split(",")])


def fixed(n, moves, method, seed, a, tag):
    """one fixed-budget attack exactly as in scripts/maxout.py: every thread anneals MOVES moves (Luby restarts of
    base * luby(r) moves from T0, or one parallel-tempering ladder), with no target"""
    from maxout import attack
    seq = readme(n)[0]
    perm, secs, desc = attack(n, ",".join(map(str, seq)), method, moves, seed, a)
    if method == "pt":
        m = desc["sweeps"] * desc["R"] * desc["mpe"]
        how = f"parallel tempering (R = {desc['R']}, T in [{desc['Tmin']:g}, {desc['Tmax']:g}])"
    else:
        m = desc["base"] * lsum(desc["reps"])
        how = ("one anneal" if desc["reps"] == 1 else f"Luby restarts, {desc['reps']} x base {desc['base'] // 10 ** 6}M moves")
        how += f" from T0 = {desc['T0']:g}"
    return record(n, seq, 0, False, f"fixed budget ({tag}): {how}, {m / 1e6:g}M moves per thread, seed {seed}", secs,
                  perm, moves=m, seed=seed, tag=tag)


THREADS = 24        # every annealing run in this project used all 24 hardware threads


def lsum(reps):
    """moves of the first REPS Luby restarts, in units of the base"""
    from maxout import luby
    return sum(luby(i) for i in range(1, reps + 1))


def run_moves(r, rate=0.0):
    """annealing moves per thread of one recorded run (avgopt.json or strong.json), None for an exact DP; a race
    stopped at its target without a restart count is costed at its run time * RATE (moves per second per thread)"""
    if "moves" in r:
        return r["moves"]
    how, mode = r.get("how", ""), r.get("mode") or ""
    if how:                                                    # avgopt.json, before the "moves" field
        m = re.match(r"Luby restarts, (\d+) x base (\d+)M moves", how)
        if m:
            return lsum(int(m.group(1)) // THREADS) * int(m.group(2)) * 10 ** 6
        m = re.match(r"fixed (\d+)M-move restarts towards \d+ \((\d+) restarts\)", how)
        return int(m.group(1)) * 10 ** 6 * int(m.group(2)) // THREADS if m else None
    if mode == "maxout:pt":
        return r["sweeps"] * r["R"] * r["mpe"]
    if mode == "maxout:luby":
        return r["base"] * lsum(r["reps"])
    if mode == "race:luby":                                    # every thread stops when one reaches the target
        return r["iters"] * lsum(min(r["reps"], r["restarts"] // THREADS))
    if "restarts" in r:                                        # fixed-length restarts
        return r["iters"] * min(r["reps"], r["restarts"] / THREADS)
    full = r["iters"] * r.get("reps", 1)
    return full if r["W"] < r["target"] else min(full, r["seconds"] * rate)


def seq_moves(runs):
    """total annealing moves per thread over RUNS; the rate for early-stopped races comes from the fixed-budget runs"""
    fb = [(run_moves(r), r["seconds"]) for r in runs if str(r.get("mode", "")).startswith("maxout")]
    rate = sum(m for m, _ in fb) / max(1, sum(s for _, s in fb))
    return sum(run_moves(r, rate) or 0 for r in runs)


def leaders(n):
    """the best worst-case sequence(s) at N: lowest stored bound among the sequences of fixed-budget rounds of >= 1G
    moves per thread (as make_report.heavily_attacked), so a lightly attacked sequence cannot pose as best"""
    st, mo = (json.load(open(os.path.join(RES, f))) for f in ("strong.json", "maxout.json"))
    seqs = {row["seq"] for r in mo.values() if r["N"] == n and r["moves_per_thread"] >= 10 ** 9 for row in r["rows"]}
    low = min(st[f"{n}:{s}"]["W_lower"] for s in seqs)
    return [(s, low, seq_moves(st[f"{n}:{s}"]["runs"])) for s in sorted(seqs) if st[f"{n}:{s}"]["W_lower"] == low]


def effort():
    db = load()
    print("| N | best worst-case sequence | its bound | its moves/thread | average-optimal sequence | its bound | "
          "its moves/thread | still to run |")
    print("|---:|---|---:|---:|---|---:|---:|---:|")
    for n in sorted(set(README_AVGSEQ_WORST) | set(USER_AVGSEQ)):
        if n < 64:
            continue
        lead = leaders(n)
        s, low, lm = max(lead, key=lambda x: x[2])             # ties: the most attacked of them
        r = db.get(str(n), {"seq": readme(n)[0], "W": None, "runs": []})
        am = seq_moves(r["runs"])
        print(f"| {n} | {{{s}}}" + (f" (+{len(lead) - 1} tied)" if len(lead) > 1 else "") + f" | {low} | {lm / 1e9:.3f}G | "
              f"{{{','.join(map(str, r['seq']))}}} | {r['W'] or '–'} | {am / 1e9:.3f}G | {max(0, lm - am) / 1e9:.3f}G |")


def main():
    mode = sys.argv[1]
    if mode == "effort":
        return effort()
    if mode == "fixed":
        o = lambda k, d: sys.argv[sys.argv.index(k) + 1] if k in sys.argv else d
        moves = int(sys.argv[3])
        a = {"R": o("--R", "16"), "Tmin": o("--Tmin", "0.2"), "Tmax": o("--Tmax", "3.0"), "mpe": o("--mpe", "1000"),
             "base": o("--base", str(moves)), "T0": o("--T0", "13.0")}
        return fixed(int(sys.argv[2]), moves, o("--method", "luby"), int(o("--seed", "1")), a, o("--tag", ""))
    if mode in ("exact", "all"):
        ns = [int(x) for x in sys.argv[3:] if not x.startswith("--")] or [n for n in sorted(README_AVGSEQ_WORST) if n <= 45]
        for n in ns:
            exact(n, sys.argv[2])
    if mode == "all":
        for n in sorted(BUDGET):
            lower(n, *BUDGET[n])
    if mode == "lower":
        seed = int(sys.argv[sys.argv.index("--seed") + 1]) if "--seed" in sys.argv else 4242
        a = [x for i, x in enumerate(sys.argv[2:], 2) if x != "--seed" and sys.argv[i - 1] != "--seed"]
        for i in range(0, len(a), 3):
            lower(int(a[i]), int(a[i + 1]), int(a[i + 2]), seed)
    if mode == "push":
        a = sys.argv[2:]
        for i in range(0, len(a), 4):
            push(int(a[i]), int(a[i + 1]), int(a[i + 2]), int(a[i + 3]))


if __name__ == "__main__":
    main()
