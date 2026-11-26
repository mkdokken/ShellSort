"""Fixed-budget attack ("max-out"): every sequence gets the same number of annealing moves per thread
(all threads), with no target, and the worst input found is re-simulated and stored in results/strong.json.
Unlike a race (which stops at a target), this measures how high each sequence's lower bound can be pushed
by a given budget, so the sequences can be ranked by it.  Summary rows go to results/maxout.json.

usage: python scripts/maxout.py N "seq;seq;..." [--moves 200000000] [--method pt|luby] [--seed 1]
                                [--tag NAME] [--R 16] [--Tmin 0.2] [--Tmax 3.0] [--mpe 1000]
                                [--base 2000000] [--T0 3.0]
  pt:   one parallel-tempering ladder per thread (R replicas; moves = sweeps * R * mpe per thread)
  luby: restarts of base * luby(r) moves annealed from T0, as in race.py --tag luby
"""
import json, os, re, subprocess, sys, time

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
RES = os.path.join(ROOT, "results")
SHELLWC = os.path.join(ROOT, "bin", "shellwc.exe")
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from witnesses import cost


def opt(k, d):
    return sys.argv[sys.argv.index(k) + 1] if k in sys.argv else d


def luby(i):
    k = 1
    while (1 << k) - 1 < i:
        k += 1
    return 1 << (k - 1) if (1 << k) - 1 == i else luby(i - (1 << (k - 1)) + 1)


def attack(n, s, method, moves, seed, a):
    env = dict(os.environ)
    if method == "pt":
        R, mpe = int(a["R"]), int(a["mpe"])
        sweeps = max(1, moves // (R * mpe))
        cmd = [SHELLWC, "pt", str(n), s, str(sweeps), str(seed), str(R), a["Tmin"], a["Tmax"], str(mpe)]
        desc = {"sweeps": sweeps, "R": R, "Tmin": float(a["Tmin"]), "Tmax": float(a["Tmax"]), "mpe": mpe}
    else:
        base, reps, tot = int(a["base"]), 0, 0
        while tot + base * luby(reps + 1) <= moves or reps == 0:
            reps += 1; tot += base * luby(reps)
        env["FSA_LUBY"] = "1"
        cmd = [SHELLWC, "fwc", str(n), s, str(base), str(seed), a["T0"], "0.05", "10", "99999", str(reps)]
        desc = {"base": base, "reps": reps, "T0": float(a["T0"])}
    t = time.time()
    out = subprocess.run(cmd, capture_output=True, text=True, env=env).stdout
    perm = [int(x) for x in re.search(r"perm=\[([\d,]+)\]", out).group(1).split(",")]
    return perm, round(time.time() - t), desc


def main():
    n, seqs = int(sys.argv[1]), [x for x in sys.argv[2].split(";") if x]
    moves, method, seed = int(opt("--moves", "200000000")), opt("--method", "pt"), int(opt("--seed", "1"))
    tag = opt("--tag", "")
    a = {"R": opt("--R", "16"), "Tmin": opt("--Tmin", "0.2"), "Tmax": opt("--Tmax", "3.0"), "mpe": opt("--mpe", "1000"),
         "base": opt("--base", "2000000"), "T0": opt("--T0", "3.0")}
    sp, mp = os.path.join(RES, "strong.json"), os.path.join(RES, "maxout.json")
    rows = []
    for s in seqs:
        perm, secs, desc = attack(n, s, method, moves, seed, a)
        w = cost(perm, [int(x) for x in s.split(",")])
        st = json.load(open(sp))
        key = f"{n}:{s}"
        rec = st.get(key, {"N": n, "seq": [int(x) for x in s.split(",")], "W_lower": 0, "runs": []})
        prev = rec["W_lower"]
        rec["runs"].append(dict({"mode": f"maxout:{method}", "moves_per_thread": moves, "seed": seed, "W": w,
                                 "seconds": secs}, **desc))
        if w > rec["W_lower"]:
            rec["W_lower"], rec["witness"] = w, perm
        st[key] = rec
        json.dump(st, open(sp, "w"), indent=1)
        rows.append({"seq": s, "W": w, "prev_best": prev, "best": rec["W_lower"], "seconds": secs})
        print(f"N={n} {{{s}}}: {w} in {secs}s ({method}, {moves} moves/thread); stored best {prev} -> {rec['W_lower']}",
              flush=True)
    db = json.load(open(mp)) if os.path.exists(mp) else {}
    key = f"{n}:{method}:{moves}:{seed}" + (f":{tag}" if tag else "")
    old = {r["seq"]: r for r in db.get(key, {}).get("rows", [])}
    for r in rows:
        old[r["seq"]] = r
    db[key] = {"N": n, "method": method, "moves_per_thread": moves, "seed": seed, "tag": tag, "params": a,
               "rows": sorted(old.values(), key=lambda r: (r["W"], r["seq"]))}
    json.dump(db, open(mp, "w"), indent=1)


if __name__ == "__main__":
    main()
