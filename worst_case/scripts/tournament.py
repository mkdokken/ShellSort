"""Challenge tournament for large N (robust against the winner's curse of noisy lower bounds).

Every contender has a certified lower bound LB (a stored, re-simulated input).  Repeatedly:
    leader = contender with the lowest LB, runner-up = next one
    attack the leader with a targeted search (stop as soon as an input reaches target),
        target = LB(runner-up)       if LB(leader) < LB(runner-up)
               = LB(runner-up) + 1   if they are tied
    success -> the leader's LB rises (ranking may change); failure -> the leader survived.
The tournament stops when the leader survives a full-budget attack.  The survivor is the best-known
sequence; the report states the budget it survived.  All witnesses go to results/strong.json.

usage: python scripts/tournament.py N "seq;seq;..." [--iters 4000000] [--reps 20] [--rounds 40]
"""
import json, os, re, subprocess, sys, time

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
RES = os.path.join(ROOT, "results")
SHELLWC = os.path.join(ROOT, "bin", "shellwc.exe")
STRONG = os.path.join(RES, "strong.json")
TOUR = os.path.join(RES, "tournament.json")


def cost(perm, gaps):
    a, n, c = list(perm), len(perm), 0
    for h in sorted({g for g in gaps if g < n} | {1}, reverse=True):
        for i in range(h, n):
            v, j = a[i], i
            while j >= h:
                c += 1
                if a[j - h] > v:
                    a[j] = a[j - h]; j -= h
                else:
                    break
            a[j] = v
    return c


def known_lb(n, s):
    best, wit = 0, None
    st = json.load(open(STRONG)) if os.path.exists(STRONG) else {}
    r = st.get(f"{n}:{s}")
    if r and r.get("witness"):
        best, wit = r["W_lower"], r["witness"]
    for db in ("transfer.json", "transfer_div.json"):
        p = os.path.join(RES, db)
        if os.path.exists(p):
            d = json.load(open(p)).get(str(n), {})
            for x in d.get("final", []) + ([d["readme"]] if d.get("readme") else []):
                if ",".join(map(str, x["seq"])) == s and x.get("witness") and x["W_lower"] > best:
                    if cost(x["witness"], x["seq"]) == x["W_lower"]:
                        best, wit = x["W_lower"], x["witness"]
    return best, wit


def store(n, s, w, perm, note):
    st = json.load(open(STRONG)) if os.path.exists(STRONG) else {}
    key = f"{n}:{s}"
    rec = st.get(key, {"N": n, "seq": [int(x) for x in s.split(",")], "W_lower": 0, "runs": []})
    rec["runs"].append(note)
    if w > rec["W_lower"]:
        rec["W_lower"], rec["witness"] = w, perm
    st[key] = rec
    json.dump(st, open(STRONG, "w"), indent=1)


def attack(n, s, target, iters, reps, seed):
    out = subprocess.run([SHELLWC, "fwc", str(n), s, str(iters), str(seed), "3.0", "0.05", "10", str(target), str(reps)],
                         capture_output=True, text=True).stdout
    m = re.search(r"perm=\[([\d,]+)\]", out)
    perm = [int(x) for x in m.group(1).split(",")]
    return cost(perm, [int(x) for x in s.split(",")]), perm


def main():
    n = int(sys.argv[1])
    cands = sys.argv[2].split(";")
    arg = lambda k, d: sys.argv[sys.argv.index(k) + 1] if k in sys.argv else d
    iters, reps, rounds = int(arg("--iters", "4000000")), int(arg("--reps", "20")), int(arg("--rounds", "40"))
    LB = {}
    for s in cands:
        w, perm = known_lb(n, s)
        if perm is None:
            w, perm = attack(n, s, 1 << 40, iters * 5, 1, 99)
            store(n, s, w, perm, {"mode": "initial", "iters": iters * 5, "W": w})
        LB[s] = w
    log, survived = [], {}
    for rnd in range(rounds):
        order = sorted(cands, key=lambda s: (LB[s], s))
        leader, runner = order[0], order[1]
        target = LB[runner] if LB[leader] < LB[runner] else LB[runner] + 1
        if survived.get(leader, 0) >= target:
            break
        t = time.time()
        w, perm = attack(n, leader, target, iters, reps, 1000 + rnd)
        ok = w >= target
        store(n, leader, w, perm, {"mode": "attack", "target": target, "iters": iters, "reps": reps, "W": w,
                                   "seconds": round(time.time() - t)})
        log.append({"round": rnd, "leader": leader, "target": target, "reached": w, "success": ok})
        print(f"N={n} round {rnd}: attack {{{leader}}} (LB {LB[leader]}) to {target}: reached {w} "
              f"{'SUCCESS' if ok else 'survived'} ({time.time()-t:.0f}s)", flush=True)
        LB[leader] = max(LB[leader], w)
        if not ok:
            survived[leader] = target
            break
    order = sorted(cands, key=lambda s: (LB[s], s))
    db = json.load(open(TOUR)) if os.path.exists(TOUR) else {}
    db[str(n)] = {"N": n, "ranking": [{"seq": s, "LB": LB[s]} for s in order], "log": log,
                  "survivor": order[0], "survived_target": survived.get(order[0]),
                  "budget": {"iters_per_thread": iters, "restarts": reps}}
    json.dump(db, open(TOUR, "w"), indent=1)
    print(f"N={n}: ranking " + ", ".join(f"{{{s}}}>={LB[s]}" for s in order))


if __name__ == "__main__":
    main()
