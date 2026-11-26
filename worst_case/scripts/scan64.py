"""Systematic N=64 scan of the {1,4,9,11,x} and {1,4,9,11,x,y} families (12 <= x < y <= 63) under one
identical screening budget, followed by an equal-budget race to TARGET for the most resistant candidates.
A candidate that survives the race is a candidate for a worst case below TARGET.

usage: python scripts/scan64.py [--screen 20000000] [--top 30] [--target 631] [--race-iters 100000000] [--race-reps 2]
"""
import json, os, subprocess, sys, time

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
RES = os.path.join(ROOT, "results")
SHELLWC = os.path.join(ROOT, "bin", "shellwc.exe")


def arg(k, d):
    return sys.argv[sys.argv.index(k) + 1] if k in sys.argv else d


def main():
    screen, top = arg("--screen", "20000000"), int(arg("--top", "30"))
    target, riters, rreps = arg("--target", "631"), arg("--race-iters", "100000000"), arg("--race-reps", "2")
    cands = [f"1,4,9,11,{x}" for x in range(12, 64)] + \
            [f"1,4,9,11,{x},{y}" for x in range(12, 64) for y in range(x + 1, 64)]
    path = os.path.join(RES, "_scan64.txt")
    open(path, "w").write("\n".join(cands) + "\n")
    t = time.time()
    out = subprocess.run([SHELLWC, "batch", "64", path, screen, "64"], capture_output=True, text=True).stdout
    res = sorted((int(l.split()[0]), l.split()[1]) for l in out.splitlines() if l.strip())
    json.dump({"screen_iters": int(screen), "results": res}, open(os.path.join(RES, "scan64.json"), "w"), indent=1)
    print(f"screened {len(res)} sequences in {time.time()-t:.0f}s; best: {res[:10]}", flush=True)
    finalists = [s for _, s in res[:top]]
    subprocess.run([sys.executable, os.path.join(ROOT, "scripts", "race.py"), "64", target, ";".join(finalists),
                    "--iters", riters, "--reps", rreps])


if __name__ == "__main__":
    main()
