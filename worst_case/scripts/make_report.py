"""Assemble REPORT.md (self-contained research report) from the result files."""
import json, os, re, sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from readme_data import README_WORST, README_ALTS, README_AVGSEQ_WORST, README_AVGSEQ_AVG, USER_AVGSEQ

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
RES = os.path.join(ROOT, "results")


def J(name, default=None):
    p = os.path.join(RES, name)
    return json.load(open(p)) if os.path.exists(p) else default


def fs(seq):
    return "{" + ",".join(map(str, seq)) + "}"


def exact_rows(summ):
    rows = ["| N | W* | optimal sequence (lowest average among ties) | average (≈ sampled) | ties | vs README |",
            "|---:|---:|---|---:|---:|---|"]
    notes, stats = [], {"same_W": 0, "identical": 0, "total": 0, "certified": 0}
    for key in sorted(summ, key=int):
        s = summ[key]; n = s["N"]
        if not s["optimal"]:
            continue
        if n in README_WORST:
            stats["total"] += 1
            stats["certified"] += 1 if s["all_exact"] else 0
        else:
            possible = [r["seq"] for r in s.get("unresolved", []) if r["W"] >= s["W"]]
            proven = all(r["W"] >= s["W"] for r in s.get("unresolved", []))
            killed = [r["seq"] for r in s.get("certified_above", []) if r.get("killed")]
            ruled = [r["seq"] for r in s["optimal"] if "front pair rules" in str(r.get("certified", ""))]
            stats.setdefault("beyond", []).append((n, s["W"], [r["seq"] for r in s["optimal"]], proven, possible,
                                                   s["optimal"][0]["avg"], s["optimal"][0].get("avg_kind"), killed, ruled))
        b = s["optimal"][0]
        rd = README_WORST.get(n)
        oset = {tuple(r["seq"]) for r in s["optimal"]}
        rset = ({tuple(rd[0])} | {tuple(a) for a in README_ALTS.get(n, [])}) if rd else set()
        if rd is None:
            vs = "–"
        elif s["W"] != rd[1]:
            vs = f"**differs** (README {rd[1]})"
        else:
            stats["same_W"] += 1
            if tuple(b["seq"]) == tuple(rd[0]) and oset == rset:
                vs = "identical"; stats["identical"] += 1
            elif tuple(b["seq"]) == tuple(rd[0]):
                vs = "same optimum, tie list differs"
            else:
                vs = f"same W; lower-average tie than README's {fs(rd[0])} ({rd[3]})"
        if rd and oset != rset:
            new = sorted(oset - rset, key=lambda t: (len(t), t)); bad = sorted(rset - oset, key=lambda t: (len(t), t))
            msg = []
            if new:
                msg.append("additional optimal sequence(s): " + ", ".join(fs(t) for t in new))
            if bad:
                msg.append("README-listed but **not optimal**: " + ", ".join(fs(t) for t in bad))
            notes.append(f"* **N = {n}** (W* = {s['W']}) — " + "; ".join(msg))
        flag = "" if s["all_exact"] else " †"
        av = f"≈ {b['avg']:.2f}" if b.get("avg_kind") == "sampled" else f"{b['avg']:.3f}"
        rows.append(f"| {n} | {s['W']}{flag} | {fs(b['seq'])} | {av} | {len(s['optimal'])} | {vs} |")
    return rows, notes, stats


def heavily_attacked(n, min_moves=10 ** 9):
    """sequences (as tuples) that took part in a fixed-budget round of >= min_moves per thread at N; when there are
    any, the large-N comparisons only use these, so a lightly attacked sequence's low bound cannot pose as best"""
    return {tuple(int(x) for x in row["seq"].split(",")) for r in J("maxout.json", {}).values()
            if r["N"] == n and r["moves_per_thread"] >= min_moves for row in r["rows"]}


def large_rows():
    LB = all_lbs()
    rows = ["| N | README sequence | README value | input found here for it (≥) | best other sequence found here | its value (≥) | verdict |",
            "|---:|---|---:|---:|---|---:|---|"]
    detail = {}
    for n in (64, 91, 128, 181, 256, 362, 512):
        rd = README_WORST.get(n)
        cand = {}
        for k, w in LB.items():
            N, s = k.split(":")
            if int(N) == n:
                cand[tuple(int(x) for x in s.split(","))] = w
        if n >= 91:     # reference convention for N >= 91: gaps <= N/2 only
            cand = {t: w for t, w in cand.items() if max(t) <= n // 2}
        if not cand:
            continue
        heavy = heavily_attacked(n)
        if not rd:      # beyond the README table (N = 512): this work's best bound only
            bw, _, bt = sorted((w, len(t), t) for t, w in cand.items() if not heavy or t in heavy)[0]
            rows.append(f"| {n} | – (the README table ends at N = 362) | – | – | {fs(bt)} | {bw} | no README sequence at this size |")
            continue
        rv = cand.get(tuple(rd[0]))
        others = sorted(((w, len(t), t) for t, w in cand.items() if t != tuple(rd[0]) and (not heavy or t in heavy)))
        bw, _, bt = others[0] if others else (None, 0, None)
        detail[n] = {"readme_value": rd[1], "readme_lb": rv, "best": bt, "best_lb": bw}
        if bw is not None and rv is not None:
            verdict = ("README sequence remains best known" if bw > rv else
                       "tie at best-known level (no improvement)" if bw == rv else
                       "other sequence lower (lower bounds only, not a proof)")
        else:
            verdict = "–"
        rows.append(f"| {n} | {fs(rd[0])} | {rd[1]} | **{rv if rv else '–'}** | {fs(bt) if bt else '–'} | {bw if bw else '–'} | {verdict} |")
    return rows, detail


def avgopt_data():
    """The README's average-optimal sequences (plus the one its author supplied for N = 46): README worst case vs
    this work's (exact for N <= 46, else a lower bound), and the worst-case price of optimising the average (vs W*,
    or for N >= 64 vs the lowest bound after long attack)"""
    db, summ, LB = J("avgopt.json", {}), J("summary.json", {}), all_lbs()
    rows = []
    for n in sorted(set(README_AVGSEQ_WORST) | set(USER_AVGSEQ)):
        seq, rw = README_AVGSEQ_WORST.get(n) or (USER_AVGSEQ[n], None)
        r = db.get(str(n))
        if str(n) in summ:
            wstar, wbest_avg = summ[str(n)]["W"], summ[str(n)]["optimal"][0]["avg"]
        else:   # N >= 64: this work's lowest bound after long attack, i.e. the best worst-case sequence, whose
                # annealing effort the average-optimal sequence matches (docs/CAMPAIGN_AVG.md)
            wstar = min(LB[f"{n}:{','.join(map(str, s))}"] for s in heavily_attacked(n))
            wbest_avg = README_WORST[n][3] if n in README_WORST else None
        row = {"n": n, "seq": seq, "readme": rw, "readme_lb": n >= 91, "avg": README_AVGSEQ_AVG.get(n), "W": None,
               "exact": False, "wstar": wstar, "wstar_exact": str(n) in summ, "wopt_avg": wbest_avg}
        if row["avg"] is None and r and r.get("avg_here"):
            row["avg"], row["avg_here"] = r["avg_here"], True          # no README average: sampled here
        if r:
            row["W"], row["exact"] = r["W"], r["exact"]
            row["delta"] = None if rw is None else r["W"] - rw
        rows.append(row)
    return rows


def avgopt_finding(rows):
    """one data-driven paragraph on the README's average-optimal sequences"""
    ex = [r for r in rows if r["W"] is not None and r["exact"]]
    lb = [r for r in rows if r["W"] is not None and not r["exact"] and r["readme"] is not None]
    exr = [r for r in ex if r["readme"] is not None]
    low = [r for r in exr if r["delta"] > 0]
    txt = (f"The worst cases the README lists for its average-optimal sequences are confirmed exactly at "
           f"{sum(r['delta'] == 0 for r in exr)} of {len(exr)} sizes N ≤ 45")
    txt += ("; the listed value is too low at " + ", ".join(f"N = {r['n']} ({r['readme']} → {r['W']})" for r in low)
            if low else "") + "."
    for r in ex:
        if r["readme"] is None:
            txt += (f" At N = {r['n']} (not in the README table) the README author's average-optimal {fs(r['seq'])} "
                    f"has exact worst case {r['W']}, against W* = {r['wstar']}.")
    if lb:
        imp = [r for r in lb if r["delta"] > 0]
        same = [r for r in lb if r["delta"] == 0]
        short = [r for r in lb if r["delta"] < 0]
        parts = []
        if same:
            parts.append("reproduce the listed value exactly at " + ", ".join(f"N = {r['n']} ({r['W']})" for r in same))
        if imp:
            parts.append("find worse inputs than listed at " + ", ".join(f"N = {r['n']} ({r['readme']} → ≥ {r['W']})"
                                                                        for r in imp))
        if short:
            parts.append("stay below the listed value at " + ", ".join(f"N = {r['n']} (≥ {r['W']} vs {r['readme']})"
                                                                      for r in short))
        txt += " For N ≥ 64 (lower bounds only) the searches here " + "; ".join(parts) + "."
    for r in rows:
        if r["W"] is not None and not r["exact"] and r["readme"] is None:
            txt += (f" At N = {r['n']} (no worst case in the README) the README author's average-optimal {fs(r['seq'])} has "
                    f"worst case ≥ {r['W']}, about {100.0 * (r['W'] - r['wstar']) / r['wstar']:.0f}% above the lowest bound "
                    f"found here for any sequence (≥ {r['wstar']}).")
    pr =[(100.0 * (r["W"] - r["wstar"]) / r["wstar"], r["n"]) for r in ex if r["wstar"]]
    if pr:
        hi = max(pr)
        txt += (f" Optimising the average costs up to {hi[0]:.0f}% more comparisons in the worst case "
                f"(N = {hi[1]}, exact).")
    return txt


def avgopt_status(r):
    if r["W"] is None:
        return "not run"
    if r["readme"] is None:
        return ("exact" if r["exact"] else "lower bound") + ", no README value"
    d = r["delta"]
    if r["exact"]:
        return "exact, matches README" if d == 0 else (f"exact, README {r['readme']} too low by {d}" if d > 0
                                                       else f"exact {r['W']} BELOW README {r['readme']}")
    return (f"README {'≥ ' if r['readme_lb'] else ''}{r['readme']} improved by +{d}" if d > 0 else
            "README value reached" if d == 0 else f"README value not reached ({d})")


AVG64 = {"1,4,9,11,21,57": 434.694, "1,4,9,11,21,59": 433.832, "1,4,9,11,21,60": 433.730, "1,4,9,11,21,62": 434.235,
         "1,4,9,11,32,58": 424.015, "1,4,9,11,32,60": 423.305, "1,4,9,11,32,62": 423.394, "1,4,9,11,21,61": 433.837}


def all_lbs():
    """best stored (witness-backed) lower bound per (N, sequence string) over every result file"""
    best = {}
    for k, v in J("strong.json", {}).items():
        best[k] = max(best.get(k, 0), v["W_lower"])
    for db in ("transfer.json", "transfer_div.json", "transfer_15.json"):
        for n, d in J(db, {}).items():
            for r in d.get("final", []) + ([d["readme"]] if d.get("readme") else []):
                k = f"{n}:{','.join(map(str, r['seq']))}"
                best[k] = max(best.get(k, 0), r["W_lower"])
    return best


METHOD = {"": "long annealing runs", "short": "short restarts", "luby": "Luby restarts", "seeded": "seeded restarts"}


def method(r):
    if r.get("tag") == "luby":
        return f"Luby restarts (≤ {r['reps']} per thread, lengths 1,1,2,1,1,2,4,… × {r['iters'] // 1000000}M moves)"
    return f"{METHOD.get(r.get('tag', ''), r.get('tag', ''))} (≤ {r['reps']} × {r['iters'] // 1000000}M moves/thread)"


def race_verdicts():
    """one bullet per large N: the race participants with the lowest stored lower bound (the leaders), the races
    to a higher target they all survived, and who was pushed above them; from results/race.json + stored bounds"""
    race = J("race.json", {}); LB = all_lbs()
    lb = lambda n, s: LB.get(f"{n}:{s}")
    out = []
    for n in (64, 91, 128, 181, 256, 362):
        rd = ",".join(map(str, README_WORST[n][0]))
        rs = [r for r in race.values() if r["N"] == n]
        if not rs:
            out.append(f"* N = {n}: README {{{rd}}} ≥ {lb(n, rd)} remains best known (all other finalists ≥ it by a margin)."
                       if n in (128, 256) else f"* N = {n}: no race run.")
            continue
        seqs = sorted({x["seq"] for r in rs for x in r["rows"]})
        low = min(lb(n, s) for s in seqs)
        lead = [s for s in seqs if lb(n, s) == low]
        up = [r for r in rs if r["target"] > low]                    # races above the leaders' level
        tested = [s for s in lead if any(x["seq"] == s for r in up for x in r["rows"])]
        untested = [s for s in lead if s not in tested]
        above = sorted({s for s in seqs if lb(n, s) > low}, key=lambda s: (lb(n, s), s))
        txt = f"* N = {n}: lowest lower bound among the raced sequences is {low}"
        if tested:
            meth = sorted({f"{method(r)} to {r['target']}" for r in up if any(x["seq"] in tested for x in r["rows"])})
            txt += (f"; raced above it and never pushed there: " + ", ".join(f"{{{s}}}" for s in tested)
                    + " (" + "; ".join(meth) + ")")
        if untested:
            txt += (f"; {'also ' if tested else ''}at {low} but not raced higher: "
                    + (", ".join(f"{{{s}}}" for s in untested) if len(untested) <= 8 else f"{len(untested)} sequences"))
        txt += ((". Pushed higher: " + ", ".join(f"{{{s}}} ≥ {lb(n, s)}" for s in above[:12])
                 + (f" and {len(above) - 12} more" if len(above) > 12 else "")) if above else "") + "."
        if rd not in lead:
            txt += f" README {{{rd}}} ≥ {lb(n, rd)}."
        # calibration: a contender already known to reach the target, attacked afresh, that the budget missed
        miss = sorted({(r["target"], x["seq"]) for r in up for x in r["rows"]
                       if not x["hit"] and x.get("best_stored", 0) >= r["target"]})
        if tested and miss:
            txt += (" **Calibration failed:** " + ", ".join(f"{{{s}}} (known ≥ {t})" for t, s in miss)
                    + " was not pushed there either when attacked afresh with the same budgets, so these survivals do "
                    "not rank the sequences.")
        elif tested:
            cal = sorted({(r["target"], x["seq"], x["restarts"], x["seconds"]) for r in up
                          if any(y["seq"] in tested for y in r["rows"]) for x in r["rows"] if x["hit"] and x.get("restarts")})
            if cal:
                txt += (" Calibrated: the same budget reached the target for "
                        + ", ".join(f"{{{s}}} ({k} restarts, {sec} s)" for t, s, k, sec in cal) + ".")
        if n == 64 and len(tested) > 1 and all(s in AVG64 for s in tested):
            best = min(tested, key=lambda s: AVG64[s])
            txt += (f" Should their worst cases be equal, the lowest average among them is {{{best}}} ({AVG64[best]:.2f} vs "
                    f"{AVG64[rd]:.2f} for the README sequence).")
        out.append(txt)
    return out


def campaign_lines():
    """2026-10-29 … 11-02 campaigns (docs/CAMPAIGN_64_91.md, docs/CAMPAIGN_128_181.md, docs/CAMPAIGN_256.md,
    docs/CAMPAIGN_362.md, docs/CAMPAIGN_512.md): family screens + fixed-budget rounds at N = 64 … 512, from
    results/famscan_N*.json and results/maxout.json; one bullet per N (the README table ends at N = 362)"""
    mo, LB, out = J("maxout.json", {}), all_lbs(), []
    for n in (64, 91, 128, 181, 256, 362, 512):
        rounds = sorted((r for r in mo.values() if r["N"] == n and r.get("tag", "").startswith("round")),
                        key=lambda r: (r["moves_per_thread"], r.get("tag", "")))
        if not rounds:
            continue
        rd = ",".join(map(str, README_WORST[n][0])) if n in README_WORST else None
        screened = len(set().union(*(J(f, {}).get("all", {}).keys()          # distinct sequences over all screens
                                     for f in os.listdir(RES) if re.fullmatch(rf"famscan_{n}(_\w+)?\.json", f))))
        late = [r for r in rounds if r["moves_per_thread"] >= 10 ** 9] or rounds[-1:]    # the long rounds
        seqs = sorted({x["seq"] for r in late for x in r["rows"]})
        low = min(LB[f"{n}:{s}"] for s in seqs)
        lead = [s for s in seqs if LB[f"{n}:{s}"] == low]
        tot = lambda s: sum(r["moves_per_thread"] for r in rounds if any(x["seq"] == s for x in r["rows"]))
        rv = [x["W"] for r in rounds for x in r["rows"] if x["seq"] == rd]
        mv = lambda m: f"{m / 1e9:g}G" if m >= 10 ** 9 else f"{m // 10 ** 6}M"
        txt = (f"* N = {n}: {screened:,} sequences screened; fixed-budget rounds of "
               + ", ".join(mv(r["moves_per_thread"]) for r in rounds)
               + f" moves per thread ({', '.join(str(len(r['rows'])) for r in rounds)} sequences). Lowest stored bound "
               f"{low}: " + ", ".join(f"{{{s}}}" for s in lead) + f" ({tot(lead[0]) / 1e9:.1f}G moves per thread in total)")
        if rd is None:
            txt += " (no README sequence at this size)"
        elif rd not in lead:
            txt += f"; README {{{rd}}} ≥ {LB[f'{n}:{rd}']} ({'/'.join(map(str, rv))} in the equal-budget rounds)"
        others = sorted(set(seqs) - set(lead) - {rd}, key=lambda s: (LB[f"{n}:{s}"], s))
        if others:
            txt += ". Pushed higher: " + ", ".join(f"{{{s}}} ≥ {LB[f'{n}:{s}']}" for s in others)
        out.append(txt + ".")
    return out


def main():
    summ = J("summary.json", {})
    rows, notes, stats = exact_rows(summ)
    lrows, ldetail = large_rows()
    anat = open(os.path.join(RES, "anatomy.md")).read() if os.path.exists(os.path.join(RES, "anatomy.md")) else ""
    last = max(map(int, summ)) if summ else 0
    P = []
    A = P.append
    A("# Worst-case-optimal Shellsort gap sequences: independent derivation and certification")
    A("")
    A("Goal: find gap sequences minimising the **worst-case** number of comparisons of Shellsort, after first "
      "replicating the reference table (github.com/mkdokken/ShellSort, README) independently.")
    A("")
    A("## Summary")
    A("")
    last_rd = max(int(k) for k in summ if int(k) in README_WORST) if summ else 0
    A(f"* **Replication (N = 6 … {last_rd}).** An exhaustive, parallel branch-and-bound over *all* gap sets reproduces the "
      f"reference optimum value at {stats['same_W']} of {stats['total']} sizes, with the same reported sequence or a tie of it. "
      f"For {stats['identical']} sizes the complete tie list is identical as well.")
    A(f"* **Certified.** Every optimum (and every tie) is proved by exact dynamic programming over threshold sets; "
      f"the reference values for N ≤ 45 were previously only believed exact (hill-climbing lower bounds). "
      f"{stats['certified']} of {stats['total']} sizes are fully certified (no unresolved candidate).")
    for n, W, opt, proven, possible, bavg, akind, killed, ruled in stats.get("beyond", []):
        tail = ""
        if ruled:
            tail += (f" {' and '.join(fs(t) for t in ruled)} {'were' if len(ruled) > 1 else 'was'} certified with front pair "
                     f"rules (METHODS §1.3), a DP on the state space of the sequence without its leading pair passes; "
                     f"targeted annealing alone had stalled at {W} for them.")
        if killed:
            tail += (f" {len(killed)} candidate{'s' if len(killed) > 1 else ''} too large for the exact DP here ("
                     + ", ".join(fs(t) for t in killed) + f") {'were' if len(killed) > 1 else 'was'} excluded by inputs "
                     f"costing more than {W}, found by targeted annealing and re-simulated.")
        if possible:
            st = J("strong.json", {})
            tried = [sum(x.get("restarts", 0) for x in st.get(f"{n}:{','.join(map(str, t))}", {}).get("runs", [])
                         if x.get("target") == W + 1 and x["W"] <= W) for t in possible]
            tail += (f" {len(possible)} further sequences ({', '.join(fs(t) for t in possible)}) have worst case ≥ {W}"
                     + (f"; targeted Luby-restart annealing ({min(tried):,} restarts each) never pushed them to {W + 1}"
                        if tried and min(tried) else "")
                     + f", but they could not be checked for equality (the exact DP needs more than this machine's "
                       f"31 GB); all have higher averages, so the choice is unaffected. N = 45 is therefore the largest "
                       f"size with a complete, certified tie list.")
        elif killed:
            tail += f" The tie list is therefore complete."
        A(f"* **Beyond the reference (N = {n}).** W*({n}) = {W} {'(proved)' if proven else '(upper bound)'}, attained by "
          + ", ".join(fs(t) for t in opt) + f"; best by average: {fs(opt[0])} ("
          + (f"{bavg:.2f}, sampled" if akind == "sampled" else f"{bavg:.3f}, exact") + ")." + tail)
    A("* **Corrections.** The search returns all optimal sequences: it adds ties the reference missed and removes one that "
      "is not optimal ({1,3,7,12} at N = 14 has worst case 67, input `[3,8,0,6,11,1,5,10,13,4,9,12,2,7]`).")
    A("* **Better tie-break at N = 22.** {1,3,5} has the same optimal worst case (131) as the reference's {1,4,7,9} but a "
      "lower exact average (94.507 vs 97.469).")
    lw = [(n, d["readme_lb"], d["readme_value"]) for n, d in sorted(ldetail.items()) if d["readme_lb"] and d["readme_lb"] > d["readme_value"]]
    if lw:
        big = max(lw, key=lambda u: (u[1] - u[2]) / u[2])
        A("* **Large N: published worst cases are too low.** Exact evaluation is impossible, so values are lower bounds "
          "attained by stored, re-simulated inputs. For the reference's *own* sequences this work finds inputs costing more "
          "than the published worst case at N = " + ", ".join(str(u[0]) for u in lw) +
          f" (largest: N = {big[0]}, ≥ {big[1]} vs {big[2]}, +{100.0 * (big[1] - big[2]) / big[2]:.1f}%).")
    A("* **Large N: comparisons under equal budgets.** The independent beam search (seeded only from this work's own "
      "optima) rediscovers the reference's sequences at N = 91 and N = 128. Candidates from this work and from the "
      "reference author's earlier searches were then compared in equal-budget races (see `docs/LARGE_N.md`):")
    for line in race_verdicts():
        A("  " + line)
    cl = campaign_lines()
    if cl:
        A("* **Large N: family screens and fixed-budget attacks (N = 64 … 512; `docs/CAMPAIGN_64_91.md`, "
          "`docs/CAMPAIGN_128_181.md`, `docs/CAMPAIGN_256.md`, `docs/CAMPAIGN_362.md`, `docs/CAMPAIGN_512.md`).** "
          "Family screens over complete sequences (counts below) found no 4-gap core that competes with {1,4,9,11}; "
          "only {1,4,9,15} comes close (18 behind at N = 128, 7 at N = 181, 23 at N = 256, 31 at N = 362 and 113 at "
          "N = 512 in the first fixed-budget round). Every candidate then got the same number of moves with no target, which pushed many earlier "
          "race-stopped bounds up, by as much as 30 at N = 181 and 110 at N = 362:")
        for line in cl:
            A("  " + line)
    A("* **Large N: lower-bound races must be calibrated.** Which restart length finds a sequence's worst inputs "
      "depends on the sequence, so single-method races mislead. Long runs left {1,4,9,11,21,60} and {1,4,9,11,21,62} "
      "at 630 while 4M-move restarts reached 631 in seconds; an earlier lone survival of the README sequence at "
      "N = 91 and its 10-comparison lead at N = 256 also disappeared under Luby restarts. Every verdict above "
      "therefore comes from a race that also re-attacks a contender already known to reach the target. At N = 64 "
      "even that check fails for 632, so N = 64 stays open (METHODS §2.2).")
    if any(r["W"] is not None for r in avgopt_data()):
        A("* **README's average-optimal sequences (§3).** " + avgopt_finding(avgopt_data()))
    A("")
    A("Details: `docs/METHODS.md` (algorithms, proofs, validation), `docs/RATIONALE.md` (why the optima look the way they do).")
    A("")
    A(f"## 1. Exact optima for N = 6 … {last}")
    A("")
    A("W* = min over gap sets of max over all N! inputs of the number of comparisons (convention: every evaluation of "
      "`a[j-h] > t` in gapped insertion sort; gaps ≥ N are no-ops). † = some tie candidate could not be certified (see notes).")
    A("")
    P.extend(rows)
    A("")
    if notes:
        A("### Tie-list corrections (all values exact; every non-optimal entry has a re-simulated witness input)")
        A("")
        P.extend(notes)
        A("")
    A("## 2. Large N (certified lower bounds)")
    A("")
    A("Exact DP needs 10¹³–10¹⁷ states per sequence at N = 64 (1.5 TB to ≈ 14 PB of memory, against 11 GB and 2.3 h "
      "for all of N = 46; METHODS §1.7), so here every value is attained by a concrete input (re-simulated); a "
      "sequence's true worst case can only be higher. The search is a prefix-transfer beam (complete candidate sequences = "
      "this work's best sequences from the previous size, truncated by ≤ 2 gaps, plus up to 3 jointly chosen new gaps; "
      "gaps ≤ N/2 for N ≥ 91 as in the reference; a second, diversified chain starts from the whole {1,4,9,·,·,·}, "
      "{1,4,7,9,·,·} and {1,4,10,·,·,·} families at N = 91), followed by strong evaluation of the finalists and of the "
      "reference sequence.")
    A("")
    A("**Winner's curse.** Picking the sequence with the lowest *lower bound* favours sequences whose worst inputs are "
      "merely hard to find. This happened here: at N = 64 the beam's apparent winner {1,4,9,11,32,62} (≥ 627 after one strong "
      "run) was shown by the reference author's hill-climber to reach 631, which a targeted search here reproduced in 16 s; "
      "the next apparent winner {1,4,9,11,32,63} survived an 80-restart targeted attack at 630 but was also reported at 631 "
      "by the reference author. Large-N verdicts below therefore only use *adversarial* comparisons: a candidate counts as "
      "better only if a targeted search cannot push it up to the best-known value of the reference sequence. The "
      "comparisons must also be *calibrated*: the same budget has to re-find a value that some contender is known to "
      "reach, and restarts of every length must be tried (Luby schedule), because the best restart length differs "
      "between sequences (METHODS §2.2).")
    A("")
    P.extend(lrows)
    A("")
    race = J("race.json", {})
    if race:
        A("### Equal-budget races")
        A("")
        A("Each contender is attacked with the same maximum budget (targeted annealing restarts, all stored inputs "
          "re-simulated); a contender that reaches the target is at least that bad, a survivor has not been pushed there.")
        A("")
        for key in sorted(race, key=lambda k: (race[k]["N"], race[k]["target"], race[k].get("tag", ""), race[k]["reps"] * race[k]["iters"])):
            r = race[key]
            hits = [x for x in r["rows"] if x["hit"]]; surv = [x for x in r["rows"] if not x["hit"]]
            A(f"* **N = {r['N']}, target {r['target']}**, {method(r)}: "
              f"reached by {len(hits)} — " + ", ".join(fs([int(v) for v in x['seq'].split(',')]) for x in hits)
              + f"; **survived ({len(surv)})** — " + (", ".join(fs([int(v) for v in x['seq'].split(',')]) + f" (≥ {x.get('best_stored', x['reached'])})"
                                                               + (" *known ≥ target: missed by this budget*" if x.get("best_stored", 0) >= r["target"] else "")
                                                               for x in surv) or "none"))
        A("")
    A("Gaps above N/2 are excluded for N ≥ 91 (reference convention). One side check at N = 91: adding a single top gap "
      "h ∈ [79, 90] to {1,4,9,11,30,44} looked better in screening, but the best such sequence reached the base value 1044 "
      "under a targeted attack, so no improvement is distinguishable; candidates of this kind differ by too little for "
      "lower-bound comparisons to rank reliably.")
    A("")
    arows = avgopt_data()
    if any(r["W"] is not None for r in arows):
        A("## 3. Worst case of the README's average-optimal sequences")
        A("")
        A("The README also lists, for each N, the sequence with the lowest *average* and that sequence's worst case, found by "
          "hill-climbing (explicitly lower bounds for N ≥ 91). These worst cases are recomputed here: exactly by the same "
          "DP wherever it fits in 12 GB (every N ≤ 46), otherwise by annealing, which gives lower bounds. For N ≥ 64 "
          "each sequence received as many annealing moves per thread as the best worst-case sequence of its size, in "
          "runs of the same kinds (`docs/CAMPAIGN_AVG.md`). N = 46 is not in the README table; its average-optimal "
          "sequence {1,4,13,41} was supplied by the README author, and its average was sampled here. At N = 512 the "
          "README names {1,4,10,23,57,189,484} as the best average, without a worst case; its author has since found with "
          "more samples that {1,4,10,23,57,189,483} is slightly better, and only that sequence is attacked. Every value is "
          "attained by a stored, re-simulated input (`scripts/avgopt.py`, `results/avgopt.json`). The last column is the "
          "worst-case price of optimising the average: the excess over the optimal worst case W* (for N ≥ 64 over the "
          "best worst-case sequence found here, i.e. the lowest bound after long attack, which rests on the same effort; "
          "both are lower bounds there, hence ≈).")
        A("")
        A(avgopt_finding(arows))
        A("")
        A("N = 45 and 46 needed the 16-bit layered DP (METHODS §1.4). For N ≥ 64 the attacks were matched to the best "
          "worst-case sequence of each size, from 3.8G moves per thread at N = 64 to 17.6G at N = 181. At N = 64 and 91 no "
          "run ever exceeded the README value: parallel tempering, Luby restarts and fixed restarts reach 723 and 1244 but "
          "never more, after 3.8G and 15.0G moves per thread in all. That makes both plausible true worst cases, though not "
          "proved. At N = 128 and 181 the long runs added 1 and 3 (2017, 3390; three of the four runs at N = 181 stop at "
          "exactly 3390). At N = 256, 362 and 512 the value still grows with effort (N = 362: 8260, 8312 and 8335 after "
          "0.55G, 2G and 4G moves per thread; N = 512: 12782 after 128M, 12960 after 1G, 12973 after 4G), so the true "
          "worst cases there are probably higher still. At equal effort the average-optimal sequences stay 15–33% above "
          "the best worst-case sequences.")
        A("")
        A("| N | README best-average sequence | README average | README worst case | worst case here | status | vs W* |")
        A("|---:|---|---:|---:|---:|---|---:|")
        for r in arows:
            price = "–"
            if r["W"] is not None and r["wstar"]:
                price = ("" if r["wstar_exact"] and r["exact"] else "≈ ") + f"+{100.0 * (r['W'] - r['wstar']) / r['wstar']:.1f}%"
            here = "–" if r["W"] is None else ("" if r["exact"] else "≥ ") + str(r["W"])
            avg = "–" if r["avg"] is None else f"≈ {r['avg']:.2f} (here)" if r.get("avg_here") else str(r["avg"])
            rw = "– (not in README)" if r["readme"] is None else f"{'≥ ' if r['readme_lb'] else ''}{r['readme']}"
            A(f"| {r['n']} | {fs(r['seq'])} | {avg} | {rw} | {here} | {avgopt_status(r)} | {price} |")
        A("")
    A("## 4. Why these sequences (summary of `docs/RATIONALE.md`)")
    A("")
    A("* A pass with gap h costs at least N − h, plus one comparison per inversion its chains still contain; before pass h the "
      "array is already d-sorted for every d in the numerical semigroup generated by the earlier (larger) gaps. Each gap is "
      "worth what it removes from later passes minus its own fixed price.")
    A("* **16 ≤ N ≤ 37: core {1,4,7,9}** (optimal at 19 of 22 sizes; exceptions N = 17, 18: {1,4,6,9}…, N = 20: {1,5,7,11}). "
      "After the 9-, 7- and 4-passes only six distances {1,2,3,5,6,10} can still be "
      "inverted, so the final insertion pass costs ≈ 3 comparisons per element. A single pair pass (gap h ≥ N/2, cost exactly "
      "N − h) is added exactly when it saves more than it costs (N = 25, 27, 30, 33–35, 37; two of them at N = 36).")
    A("* **38 ≤ N ≤ 42: transition** (both cores appear among the optima); **43 ≤ N ≤ 45: core {1,4,9,11} plus 21.** "
      "The decoupled top pass of {4,7,9} (cost Σ m(m−1)/2 over 9-chains) grows "
      "too fast; the bare {4,9,11} core overtakes it from N ≈ 40 (exact: 382 vs 393 at N = 45). 21 = 3·4 + 9 lies inside ⟨4,9⟩, so it does "
      "not help the final pass; it pays in the 4-pass, where distances 21, 32 = 11 + 21 and 42 become pre-sorted "
      "(potentially inverted chain pairs 191 → 178 at N = 45).")
    A("* **Gaps interact sharply.** At N = 36, moving the gap 19 of the optimum {1,4,7,9,19,34} by ±1 costs +10 / +13 "
      "comparisons, while deleting it costs +2. Single-gap local search sees a landscape of isolated spikes; the method "
      "therefore only ever evaluates complete sequences and prunes whole families.")
    A("")
    A("## 5. Method (summary of `docs/METHODS.md`)")
    A("")
    A("1. **Exact worst case of a sequence:** threshold-set DP (a longest path in a lattice of sets), with the first pass "
      "decoupled exactly, pair passes absorbed as compare-exchange constraints, free positions back-filled (exchange "
      "argument), parallel level sweeps, and a two-level/one-byte layered variant that certifies 2.5·10¹⁰-state instances in "
      "≈ 5 GB.")
    A("2. **Lower bounds at any size:** simulated annealing with *incremental* evaluation (swapping values x < y only "
      "changes the threshold sets between them) plus an exact DP that optimally re-orders windows of consecutive values; "
      "every value is attained by a re-simulated input.")
    A("3. **Continuous pruning:** branch-and-bound over gap prefixes. A permutation that every larger gap leaves untouched "
      "(it is *local*) lower-bounds the whole family of extensions, so one witness prunes a whole subtree; witnesses are "
      "pooled and reused across the tree; the threshold only falls when a value is proved exactly; staged evaluation "
      "(cheap → expensive) with a certification loop.")
    A("4. **Holistic candidates:** only complete sequences are ever evaluated, generated family by family (exact range) or "
      "by joint re-selection of the top gaps (large N); no step perturbs one gap in isolation.")
    A("5. **Parallel:** subtrees, candidate batches and DP levels are independent units (OpenMP, 24 threads).")
    A("")
    A("## 6. Validation")
    A("")
    A("* DP = brute force over all N! inputs on 115 random sequences (N ≤ 12, 479M inputs each at N = 12) and N = 13 checks.")
    A("* Three independent DP formulations (plain, pair-absorbing, layered one-byte) agree on hundreds of random cases.")
    A("* Incremental annealing totals = full re-simulation on 2.9M random moves (N = 12 … 362); kills inside the search are "
      "re-simulated before acceptance.")
    A("* Exact averages = full enumeration (N ≤ 10) and match the reference's sampled averages to its precision.")
    A("")
    if anat:
        A("## 7. Anatomy of the certified worst cases")
        A("")
        A("Comparisons per pass on the certified worst-case input of each optimum (count@gap), and the distances that are "
          "still unsorted before the final insertion pass.")
        A("")
        A(anat)
    A("## 8. Limitations")
    A("")
    A("* N ≥ 64: values are lower bounds; the sequence ranking can change with more search effort (equal effort was used for "
      "the comparisons above).")
    A("* An exhaustive branch-and-bound at N = 64 (all gaps) was attempted and abandoned: the locality-based family bound "
      "is weak for prefixes made of small gaps, so the tree is too wide for the available time.")
    A("")
    A("## 9. Reproduce")
    A("")
    A("```\npowershell -File build.ps1\npython scripts/run_search.py 6 45        # exhaustive search + certification (hours)\n"
      "python scripts/transfer.py 91 64 ...      # large-N beam (see script header)\npython scripts/final_eval.py              # equal-effort comparison\n"
      "python scripts/analyze.py; python scripts/make_report.py; python scripts/make_html.py\n```")
    open(os.path.join(ROOT, "REPORT.md"), "w", encoding="utf-8").write("\n".join(P) + "\n")
    print(f"REPORT.md written ({len(P)} lines); sizes {stats}")


if __name__ == "__main__":
    main()
