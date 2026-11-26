"""Certify candidates that the driver left unresolved (too large for its state cap) and merge the results.

  python scripts/manual_cert.py run N 1,4,9,11,21,40      # layered one-byte DP (C enforces a 12 GB cap)
  python scripts/manual_cert.py rules N 1,4,9,11,21,44 [DUMPDIR]   # same DP, leading pair passes as front
                                                           # pair rules (state space without them)
  python scripts/manual_cert.py merge                      # fold results/manual_cert.json into summary.json

Results are kept in results/manual_cert.json (the driver rewrites summary.json/exact_cache.json from memory,
so merging is a separate step run after the driver has finished).
"""
import json, os, re, shutil, subprocess, sys, time

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
RES = os.path.join(ROOT, "results")
SHELLWC = os.path.join(ROOT, "bin", "shellwc.exe")
MAN = os.path.join(RES, "manual_cert.json")
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from witnesses import cost


def load(p, d):
    return json.load(open(p)) if os.path.exists(p) else d


def run(n, seq):
    db = load(MAN, {})
    key = f"{n}:{seq}"
    if key in db:
        print(key, "already certified:", db[key]); return
    t = time.time()
    p = subprocess.run([SHELLWC, "lay", str(n), seq, "12000000000"], capture_output=True, text=True)
    m = re.search(r"lay_worst=(\d+)", p.stdout)
    if not m:
        print(key, "FAILED:", p.stdout.strip(), p.stderr.strip()[-300:]); return
    db = load(MAN, {})
    db[key] = {"W": int(m.group(1)), "how": f"layered one-byte DP ({time.time()-t:.0f}s)"}
    json.dump(db, open(MAN, "w"), indent=1)
    print(key, "=", db[key])


def rules(n, seq, dump=None):
    """exact value with the leading pair passes as front pair rules (shellwc layr, exact2.h x2_front_rules);
    with DUMPDIR the levels are dumped there (then deleted) and an attaining input is reconstructed"""
    key = f"{n}:{seq}"
    t = time.time()
    if dump:
        os.makedirs(dump, exist_ok=True)
    try:
        p = subprocess.run([SHELLWC, "layr", str(n), seq, "12000000000"] + ([dump] if dump else []),
                           capture_output=True, text=True)
    finally:
        if dump:
            shutil.rmtree(dump, ignore_errors=True)
    m = re.search(r"rules_worst=(\d+)", p.stdout)
    if not m:
        print(key, "FAILED:", p.stdout.strip(), p.stderr.strip()[-300:]); return
    rec = {"W": int(m.group(1)), "how": f"layered one-byte DP with front pair rules ({time.time()-t:.0f}s)"}
    mp = re.search(r"perm=\[([\d,]+)\]", p.stdout)
    if mp:
        perm = [int(x) for x in mp.group(1).split(",")]
        c = cost(perm, [int(x) for x in seq.split(",")])        # independent re-simulation
        if c != rec["W"]:
            sys.exit(f"{key}: DP value {rec['W']} but its input re-simulates to {c}")
        rec["witness"] = perm
    db = load(MAN, {})
    db[key] = rec
    json.dump(db, open(MAN, "w"), indent=1)
    print(key, "=", rec["W"], f"({rec['how']}{', input re-simulated' if mp else ''})", flush=True)


def kill(n, seq):
    """record a witness-based exclusion (lower bound from results/strong.json, re-simulated there)"""
    st = load(os.path.join(RES, "strong.json"), {})
    r = st[f"{n}:{seq}"]
    db = load(MAN, {})
    db[f"{n}:{seq}"] = {"W": r["W_lower"], "lower_bound": True, "how": "targeted-search witness (strong.json)"}
    json.dump(db, open(MAN, "w"), indent=1)
    print(f"{n}:{seq} >= {r['W_lower']} recorded")


def merge():
    db = load(MAN, {})
    sp = os.path.join(RES, "summary.json")
    summ = json.load(open(sp))
    for key, v in db.items():
        n, seq = key.split(":")
        s = summ.get(n)
        if not s:
            continue
        seql = [int(x) for x in seq.split(",")]
        for r in list(s["unresolved"]):
            if r["seq"] == seql and v.get("lower_bound"):
                if v["W"] > s["W"]:
                    s["unresolved"].remove(r)
                    r["W"], r["killed"] = v["W"], v["how"]
                    s.setdefault("certified_above", []).append(r)
                    print(f"N={n} {seq}: input with {v['W']} > W*={s['W']} -> not optimal")
                elif v["W"] > r["W"]:
                    r["W"] = v["W"]          # still unresolved (possible tie), but with a better lower bound
                    print(f"N={n} {seq}: lower bound raised to {v['W']} (possible tie, not certified)")
                continue
            if r["seq"] == seql:
                s["unresolved"].remove(r)
                r["W"], r["exact"], r["certified"] = v["W"], True, v["how"]
                if v.get("witness"):
                    r["worst_input"] = v["witness"]
                if v["W"] == s["W"]:
                    out = subprocess.run([SHELLWC, "xavg", n, seq, "100000000"], capture_output=True, text=True).stdout
                    m = re.search(r"xavg=([\d.]+)", out)
                    if m:
                        r["avg"], r["avg_se"], r["avg_kind"] = float(m.group(1)), 0.0, "exact"
                    else:
                        out = subprocess.run([SHELLWC, "avg", n, seq, "20000000", "7"], capture_output=True, text=True).stdout
                        m = re.search(r"avg=([\d.]+) \+/- ([\d.]+)", out)
                        r["avg"], r["avg_se"], r["avg_kind"] = float(m.group(1)), float(m.group(2)), "sampled"
                    s["optimal"].append(r)
                    s["optimal"].sort(key=lambda x: x["avg"])
                    print(f"N={n} {seq}: certified {v['W']} -> added to the optimal set (avg {r['avg']:.3f})")
                elif v["W"] < s["W"]:
                    print(f"N={n} {seq}: certified {v['W']} BELOW W*={s['W']} -> search must be re-run!")
                    s.setdefault("needs_rerun", True)
                else:
                    s.setdefault("certified_above", []).append(r)
                    print(f"N={n} {seq}: certified {v['W']} > W*={s['W']} -> not optimal")
        s["all_exact"] = len(s["unresolved"]) == 0
    json.dump(summ, open(sp, "w"), indent=1)


if __name__ == "__main__":
    if sys.argv[1] == "run":
        run(int(sys.argv[2]), sys.argv[3])
    elif sys.argv[1] == "rules":
        rules(int(sys.argv[2]), sys.argv[3], sys.argv[4] if len(sys.argv) > 4 else None)
    elif sys.argv[1] == "kill":
        kill(int(sys.argv[2]), sys.argv[3])
    else:
        merge()
