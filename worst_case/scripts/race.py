"""Race to a target: every contender gets the same maximum targeted-attack budget (restarts x iterations
per thread); an attack stops as soon as an input reaching the target is found.  Contenders that reach the
target are worse than (or equal to) the target-1 level; survivors are the best candidates at that level.
All inputs found are re-simulated and stored in results/strong.json; summary in results/race.json.

usage: python scripts/race.py N TARGET "seq;seq;..." [--iters 8000000] [--reps 40] [--seed 777]
                              [--fresh] [--tag NAME] [--T0 3.0] [--seeds FILE]
  --fresh  attack every contender even if a stored input already reaches the target (calibration: the time a
           known-reachable target takes under this budget)
  --tag    method label (e.g. "short", "rot"), kept in the race key so different methods are not merged;
           "luby" also selects the Luby restart schedule (run r has iters * luby(r+1) moves; FSA_LUBY=1)
  --seeds  every restart starts from an input drawn from FILE (one permutation per line; FSA_SEEDS);
           "auto" = the stored worst inputs of all *other* sequences of this N (never the contender's own)
"""
import json, os, re, subprocess, sys, time

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
RES = os.path.join(ROOT, "results")
SHELLWC = os.path.join(ROOT, "bin", "shellwc.exe")
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from witnesses import cost


def opt(k, d):
    return sys.argv[sys.argv.index(k) + 1] if k in sys.argv else d


def main():
    n, target, seqs = int(sys.argv[1]), int(sys.argv[2]), sys.argv[3].split(";")
    iters, reps, seed = opt("--iters", "8000000"), opt("--reps", "40"), opt("--seed", "777")
    fresh, tag = "--fresh" in sys.argv, opt("--tag", "")
    env = dict(os.environ)
    if "luby" in tag:                       # --tag luby selects the Luby restart schedule
        env["FSA_LUBY"] = "1"
    elif env.get("FSA_LUBY", "0") != "0":
        tag = (tag + "+luby").lstrip("+")
    if env.get("FSA_ROT", "0") != "0" and "rot" not in tag:
        tag = (tag + "+rot").lstrip("+")
    T0, seeds = opt("--T0", "3.0"), opt("--seeds", "")
    if seeds:                               # restarts start from these inputs (FSA_SEEDS), usually with a low T0
        if seeds != "auto":
            env["FSA_SEEDS"] = seeds
        if "seeded" not in tag:
            tag = (tag + "+seeded").lstrip("+")
    sp = os.path.join(RES, "strong.json")
    out_rows = []
    for s in seqs:
        st = json.load(open(sp))
        prev = st.get(f"{n}:{s}", {}).get("W_lower", 0)
        if prev >= target and not fresh:
            print(f"N={n} {{{s}}}: already has an input with {prev} >= {target}", flush=True)
            out_rows.append({"seq": s, "reached": prev, "hit": True, "seconds": 0, "already": True}); continue
        if seeds == "auto":                 # the worst inputs stored for the *other* sequences of this N
            sf = os.path.join(RES, f"_seeds_{n}.txt")
            with open(sf, "w") as f:
                for k, v in st.items():
                    if k.startswith(f"{n}:") and k != f"{n}:{s}" and v.get("witness"):
                        f.write(",".join(map(str, v["witness"])) + "\n")
            env["FSA_SEEDS"] = sf
        t = time.time()
        out = subprocess.run([SHELLWC, "fwc", str(n), s, iters, seed, T0, "0.05", "10", str(target), reps],
                             capture_output=True, text=True, env=env).stdout
        m = re.search(r"perm=\[([\d,]+)\]", out)
        perm = [int(x) for x in m.group(1).split(",")]
        mr = re.search(r"restarts=(\d+)", out)
        nrest = int(mr.group(1)) if mr else None
        w = cost(perm, [int(x) for x in s.split(",")])
        st = json.load(open(sp))
        key = f"{n}:{s}"
        rec = st.get(key, {"N": n, "seq": [int(x) for x in s.split(",")], "W_lower": 0, "runs": []})
        rec["runs"].append({"mode": "race" + (f":{tag}" if tag else ""), "target": target, "iters": int(iters),
                            "reps": int(reps), "seed": int(seed), "W": w, "restarts": nrest,
                            "seconds": round(time.time() - t)})
        if w > rec["W_lower"]:
            rec["W_lower"], rec["witness"] = w, perm
        st[key] = rec
        json.dump(st, open(sp, "w"), indent=1)
        hit = w >= target
        out_rows.append({"seq": s, "reached": w, "hit": hit, "seconds": round(time.time() - t), "restarts": nrest,
                         "best_stored": rec["W_lower"]})
        print(f"N={n} {{{s}}}: {'REACHED' if hit else 'survived at'} {w} (target {target}, {time.time()-t:.0f}s, "
              f"{nrest} restarts); best stored {rec['W_lower']}", flush=True)
    rp = os.path.join(RES, "race.json")
    db = json.load(open(rp)) if os.path.exists(rp) else {}
    key = f"{n}:{target}:{reps}x{iters}" + (f":{tag}" if tag else "")
    if key in db:           # same race settings: merge contenders (newer row wins for the same sequence)
        old = {x["seq"]: x for x in db[key]["rows"]}
        for x in out_rows:
            old[x["seq"]] = x
        out_rows = list(old.values())
    db[key] = {"N": n, "target": target, "iters": int(iters), "reps": int(reps), "tag": tag, "rows": out_rows}
    json.dump(db, open(rp, "w"), indent=1)
    surv = [r["seq"] for r in out_rows if not r["hit"]]
    print(f"N={n}: {len(surv)} of {len(out_rows)} survived the race to {target}: " + "; ".join("{" + s + "}" for s in surv), flush=True)


if __name__ == "__main__":
    main()
