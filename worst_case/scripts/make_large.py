"""docs/LARGE_N.md: every large-N contender with its source, best stored worst-case input (lower bound,
re-simulated), and equal-budget race outcomes."""
import json, os, re, sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from readme_data import README_WORST

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
RES = os.path.join(ROOT, "results")

# candidates communicated by the reference author during this work (from earlier, lighter searches)
AUTHOR = {
    64: ["1,4,9,11,21,61", "1,4,9,11,21,62", "1,4,9,11,21,60", "1,4,9,11,21,59", "1,4,9,11,21,57", "1,4,9,11,32,58",
         "1,4,9,11,32,62", "1,4,9,11,32,63", "1,4,9,11,32,61", "1,4,9,11,32,57", "1,4,9,11,30,58"],
    91: ["1,4,9,11,21,38", "1,4,9,11,30,42", "1,4,9,11,30,36", "1,4,9,15,28,35", "1,4,9,15,35,37", "1,4,9,11,32,44"],
    181: ["1,4,9,11,30,47,65,75", "1,4,9,11,30,36,59,87", "1,4,9,11,30,36,59,86", "1,4,9,11,30,47,67,85",
          "1,4,9,11,30,47,84", "1,4,9,15,31,44,81", "1,4,9,15,32,43,89"],
    256: ["1,4,9,11,30,36,70,121", "1,4,9,11,30,36,70,123", "1,4,9,11,30,36,70,109", "1,4,9,11,30,36,62,125",
          "1,4,9,15,31,41,90,115", "1,4,9,11,30,36,65,103,118"],
    362: ["1,4,9,11,30,36,70,113,175", "1,4,9,11,30,36,70,133,159", "1,4,9,11,30,36,70,97,159",
          "1,4,9,11,30,47,59,127,150"],
}
AVG64 = {"1,4,9,11,21,57": 434.694, "1,4,9,11,21,59": 433.832, "1,4,9,11,21,60": 433.730, "1,4,9,11,21,62": 434.235,
         "1,4,9,11,32,58": 424.015, "1,4,9,11,32,60": 423.305, "1,4,9,11,32,62": 423.394, "1,4,9,11,21,61": 433.837}


def J(name, d):
    p = os.path.join(RES, name)
    return json.load(open(p)) if os.path.exists(p) else d


def race_line(r):
    """one bullet per race: method, budget, who reached the target (with restarts used) and who survived"""
    method = {"": "long annealing runs", "short": "short restarts", "luby": "Luby restarts",
              "seeded": "seeded restarts"}.get(r.get("tag", ""), r.get("tag", ""))
    if r.get("note"):
        method += f" — {r['note']}"
    budget = (f"≤ {r['reps']} restarts per thread of 1,1,2,1,1,2,4,… × {r['iters']:,} moves" if r.get("tag") == "luby"
              else f"≤ {r['reps']} × {r['iters']:,} moves per thread")
    hit = [x for x in r["rows"] if x["hit"]]
    surv = [x for x in r["rows"] if not x["hit"]]
    fh = lambda x: "{" + x["seq"] + "}" + (f" ({x['restarts']} restarts, {x['seconds']} s)" if x.get("restarts") else "")
    fs = lambda x: ("{" + x["seq"] + "}" + (f" ({x['restarts']} restarts)" if x.get("restarts") else "")
                    + (f" — *known ≥ {x['best_stored']}: missed by this budget*" if x.get("best_stored", 0) >= r["target"] else ""))
    return (f"* Race to {r['target']}, {method} ({budget}): reached by "
            + (", ".join(fh(x) for x in hit) or "none") + "; **survived**: "
            + (", ".join(fs(x) for x in surv) or "none") + ".")


def maxout_line(key, r):
    """one bullet per fixed-budget round: method, budget, every sequence's value (lowest first)"""
    p = r["params"]
    if r["method"] == "pt":
        method = f"parallel tempering (R = {p['R']}, T ∈ [{p['Tmin']}, {p['Tmax']}])"
    else:
        base, T0 = int(p.get("base", 2000000)), float(p.get("T0", 3.0))
        method = "Luby restarts" + (f" (base {base // 10 ** 6}M moves, T₀ = {T0:g})" if (base, T0) != (2000000, 3.0) else "")
    rows = sorted(r["rows"], key=lambda x: (x["W"], x["seq"]))
    return (f"* Fixed budget, {method}, {r['moves_per_thread']:,} moves per thread, seed {r['seed']}"
            + (f" ({r['tag']})" if r.get("tag") else "") + ": "
            + ", ".join(f"{{{x['seq']}}} {x['W']}" for x in rows) + ".")


def main():
    st = J("strong.json", {})
    races = J("race.json", {})
    maxouts = J("maxout.json", {})
    r1 = J("race_round1.json", {})
    for k, v in r1.items():
        races.setdefault(f"{v['N']}:{v['target']}:{v['reps']}x{v['iters']}", v)
    srcs = {}
    for db in ("transfer.json", "transfer_div.json", "transfer_15.json"):
        for n, d in J(db, {}).items():
            for r in d.get("final", []) + ([d["readme"]] if d.get("readme") else []):
                t = ",".join(map(str, r["seq"]))
                if r is not d.get("readme") or r.get("found_by_search"):
                    srcs.setdefault((int(n), t), set()).add("this work")
                key = f"{n}:{t}"
                if key not in st or st[key]["W_lower"] < r["W_lower"]:
                    st.setdefault(key, {"W_lower": r["W_lower"], "seq": r["seq"], "N": int(n), "runs": []})
                    st[key]["W_lower"] = max(st[key]["W_lower"], r["W_lower"])
    for n in (64, 91, 128, 181, 256, 362, 512):      # finalists of the family screens (scripts/famscan.py)
        for fn in sorted(f for f in os.listdir(RES) if re.fullmatch(rf"famscan_{n}(_\w+)?\.json", f)):
            fs = J(fn, None)
            if fs:
                # a refinement started from a ranked list (not a screen) did not find its starting sequences
                seeded = {tuple(s) for s in fs.get("base", [])} if not fs.get("from", ".json").endswith(".json") else set()
                for r in fs["final"]:
                    if tuple(r["seq"]) not in seeded:
                        srcs.setdefault((n, ",".join(map(str, r["seq"]))), set()).add("this work")
    # from N = 256 on, the fixed-budget rounds also attack this work's own variants of its leaders (N = 362 round A2:
    # neighbours of the round-A leader); everything in them but the README sequence and author candidates is ours
    for r in maxouts.values():
        if r["N"] >= 256 and r.get("tag", "").startswith("round"):
            for x in r["rows"]:
                rd = ",".join(map(str, README_WORST[r["N"]][0])) if r["N"] in README_WORST else None
                if x["seq"] != rd and x["seq"] not in AUTHOR.get(r["N"], []):
                    srcs.setdefault((r["N"], x["seq"]), set()).add("this work")
    effort = {}                     # fixed-budget attack per sequence: moves per thread summed over every maxout run
    for r in maxouts.values():
        for x in r["rows"]:
            effort[(r["N"], x["seq"])] = effort.get((r["N"], x["seq"]), 0) + r["moves_per_thread"]
    mv = lambda m: "–" if not m else f"{m / 1e9:.1f}G" if m >= 10 ** 9 else f"{m // 10 ** 6}M"
    L = ["# Large N (N ≥ 64): contenders, certified lower bounds and equal-budget races", "",
         "Every number is the cost of a stored input permutation, re-simulated (`scripts/verify.py`); the true worst "
         "case of a sequence can only be higher. Gaps ≤ N/2 for N ≥ 91 (reference convention). Sources: **R** = "
         "reference README sequence, **A** = candidate communicated by the reference author from earlier searches, "
         "**W** = found by this work's beam searches or family screens. The last column is the fixed-budget attack "
         "a sequence received (moves per thread, all its rounds of `scripts/maxout.py` together): a low bound "
         "behind few moves is the least reliable, since bounds keep rising with effort.", ""]
    for n in (64, 91, 128, 181, 256, 362, 512):
        rd = ",".join(map(str, README_WORST[n][0])) if n in README_WORST else None
        rows = []
        for key, v in st.items():
            N, s = key.split(":")
            if int(N) != n:
                continue
            if n >= 91 and max(int(x) for x in s.split(",")) > n // 2:
                continue
            src = []
            if s == rd:
                src.append("R")
            if s in AUTHOR.get(n, []):
                src.append("A")
            if (n, s) in srcs:
                src.append("W")
            if not src:
                continue
            rows.append((v["W_lower"], s, "+".join(src)))
        rows.sort()
        L += [f"## N = {n}", "", f"README: {{{rd}}}, published worst case {README_WORST[n][1]}." if rd else
              "No README sequence: the README table ends at N = 362 (`docs/CAMPAIGN_512.md`).", ""]
        race_rows = [(k, r) for k, r in races.items() if r["N"] == n]
        hdr = "| sequence | source | best stored input (≥) |" + ("" if n != 64 else " average |") + " fixed-budget moves |"
        L += [hdr, "|---|---|---:|" + ("" if n != 64 else "---:|") + "---:|"]
        for w, s, src in rows[:14] + [x for x in rows[14:] if x[1] == rd]:     # the README sequence always shown
            L.append(f"| {{{s}}} | {src} | {w} |" + ("" if n != 64 else (f" {AVG64[s]:.2f} |" if s in AVG64 else " – |"))
                     + f" {mv(effort.get((n, s), 0))} |")
        L.append("")
        for k, r in sorted(race_rows, key=lambda kr: (kr[1]["target"], kr[1].get("tag", ""), kr[1]["reps"] * kr[1]["iters"])):
            L.append(race_line(r))
        for k, r in sorted(((k, r) for k, r in maxouts.items() if r["N"] == n),
                           key=lambda kr: (kr[1]["moves_per_thread"], kr[1]["seed"])):
            L.append(maxout_line(k, r))
        L.append("")
    open(os.path.join(ROOT, "docs", "LARGE_N.md"), "w", encoding="utf-8").write("\n".join(L) + "\n")
    print("\n".join(L[:40]))


if __name__ == "__main__":
    main()
