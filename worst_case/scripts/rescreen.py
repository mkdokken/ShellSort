"""Long-run rescreen of family-screen finalists (heuristic; values are lower bounds found by annealing).

The best `--top` finalists of each given screen (results/famscan_N*.json, ranked by their screen score) get
`--reps` single-thread anneals of `--iters` moves each (shellwc batch, one thread per run); a sequence's score is
the worst input found over these runs only.  At N = 362 the bound a run reaches grows strongly with its length
(docs/CAMPAIGN_362.md), so the short screen runs rank the finalists only coarsely; this picks the field for the
fixed-budget rounds (scripts/maxout.py).  Results -> results/rescreen_N.json.

usage: python scripts/rescreen.py N famscan_N.json[,famscan_N_nbr.json,...] [--top 150] [--iters 48000000]
                                  [--reps 2] [--T0 13.0] [--seed 401]
"""
import json, os, sys, time

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from famscan import RES, batch, opt


def main():
    n, srcs = int(sys.argv[1]), sys.argv[2].split(",")
    top, iters, reps = int(opt("--top", "150")), int(opt("--iters", "48000000")), int(opt("--reps", "2"))
    T0, seed = opt("--T0", "13.0"), int(opt("--seed", "401"))
    screen = {}                                         # sequence -> its best screen score over the given screens
    for f in srcs:
        for r in json.load(open(os.path.join(RES, f)))["final"][:top]:
            s = tuple(r["seq"])
            screen[s] = min(screen.get(s, 10 ** 9), r["score"])
    seqs, runs, t = sorted(screen), {s: [] for s in screen}, time.time()
    print(f"N={n}: rescreen of {len(seqs)} finalists, {reps} x {iters} moves", flush=True)
    for r in range(reps):
        for s, w in batch(n, seqs, iters, seed + r, f"rescreen{r}", T0).items():
            runs[s].append(w)
        print(f"  run {r + 1}/{reps} ({time.time() - t:.0f}s)", flush=True)
    rows = sorted(({"seq": list(s), "score": max(runs[s]), "runs": runs[s], "screen": screen[s]} for s in seqs),
                  key=lambda r: (r["score"], r["seq"]))
    json.dump({"N": n, "from": srcs, "top": top, "iters": iters, "reps": reps, "T0": float(T0), "seed": seed,
               "time_s": round(time.time() - t), "rows": rows}, open(os.path.join(RES, f"rescreen_{n}.json"), "w"),
              indent=1)
    print(f"N={n}: done in {time.time() - t:.0f}s; top 30:", flush=True)
    for r in rows[:30]:
        print(f"  {r['score']}  {{{','.join(map(str, r['seq']))}}}  runs={sorted(r['runs'])}  screen {r['screen']}",
              flush=True)


if __name__ == "__main__":
    main()
