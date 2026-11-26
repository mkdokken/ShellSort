"""Does a top 'pair pass' (gap h >= N/2, excluded by the reference for N >= 91) improve the best-known
large-N sequences?  For a base sequence: screen base+{h} for h in [N-span, N-1] (one candidate per thread),
strongly evaluate the best few, then attack the best with target = base's best-known value.

usage: python scripts/pairpass.py N BASE [--span 12] [--iters 10000000] [--strong 40000000] [--reps 30]
"""
import json, os, re, subprocess, sys, time

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
RES = os.path.join(ROOT, "results")
SHELLWC = os.path.join(ROOT, "bin", "shellwc.exe")
DB = os.path.join(RES, "pairpass.json")
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from witnesses import cost


def arg(k, d):
    return sys.argv[sys.argv.index(k) + 1] if k in sys.argv else d


def best_known(n, s):
    st = json.load(open(os.path.join(RES, "strong.json")))
    r = st.get(f"{n}:{s}")
    return (r["W_lower"], r.get("witness")) if r else (None, None)


def store(n, s, w, perm, note):
    p = os.path.join(RES, "strong.json")
    st = json.load(open(p))
    key = f"{n}:{s}"
    rec = st.get(key, {"N": n, "seq": [int(x) for x in s.split(",")], "W_lower": 0, "runs": []})
    rec["runs"].append(note)
    if w > rec["W_lower"]:
        rec["W_lower"], rec["witness"] = w, perm
    st[key] = rec
    json.dump(st, open(p, "w"), indent=1)


def fwc(n, s, iters, seed, target=None, reps=None):
    args = [SHELLWC, "fwc", str(n), s, str(iters), str(seed), "3.0", "0.05", "10"]
    if target is not None:
        args += [str(target), str(reps)]
    out = subprocess.run(args, capture_output=True, text=True).stdout
    m = re.search(r"perm=\[([\d,]+)\]", out)
    perm = [int(x) for x in m.group(1).split(",")]
    return cost(perm, [int(x) for x in s.split(",")]), perm


def main():
    n, base = int(sys.argv[1]), sys.argv[2]
    span, iters = int(arg("--span", "12")), arg("--iters", "10000000")
    siters, reps = int(arg("--strong", "40000000")), int(arg("--reps", "30"))
    t = time.time()
    bw, _ = best_known(n, base)
    if bw is None:
        bw, perm = fwc(n, base, siters, 5)
        store(n, base, bw, perm, {"mode": "strong", "iters": siters, "W": bw})
    cands = [base + "," + str(h) for h in range(n - span, n)]
    path = os.path.join(RES, f"_pp_{n}.txt")
    open(path, "w").write("\n".join(cands) + "\n")
    out = subprocess.run([SHELLWC, "batch", str(n), path, iters, "21"], capture_output=True, text=True).stdout
    screen = sorted((int(l.split()[0]), l.split()[1]) for l in out.splitlines() if l.strip())
    print(f"N={n} base {{{base}}} best-known {bw}; screen: {screen[:5]}", flush=True)
    rows = []
    for w0, s in screen[:3]:
        w, perm = fwc(n, s, siters, 7)
        w = max(w, w0) if False else w
        store(n, s, w, perm, {"mode": "strong", "iters": siters, "W": w})
        rows.append((w, s))
        print(f"  strong {{{s}}}: {w}", flush=True)
    rows.sort()
    verdict = None
    if rows and rows[0][0] < bw:
        w, s = rows[0]
        wa, perm = fwc(n, s, siters // 4, 9, target=bw, reps=reps)
        store(n, s, wa, perm, {"mode": "attack", "target": bw, "reps": reps, "W": wa})
        verdict = "survived" if wa < bw else "reached base value"
        print(f"  attack {{{s}}} to {bw}: reached {wa} -> {verdict}", flush=True)
    db = json.load(open(DB)) if os.path.exists(DB) else {}
    db[f"{n}:{base}"] = {"N": n, "base": base, "base_best_known": bw, "screen": screen,
                         "strong": rows, "verdict": verdict, "seconds": round(time.time() - t)}
    json.dump(db, open(DB, "w"), indent=1)


if __name__ == "__main__":
    main()
