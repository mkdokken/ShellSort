"""Broad family screen at large N (heuristic; values are lower bounds found by annealing).

Enumerates complete sequences from family patterns (cores of 3-4 gaps, completed by 1-3 larger gaps on
full or coarse grids), then successive halving with short independent annealing runs: the score of a
sequence is the WORST input found over all its runs so far (higher = worse sequence), every survivor of a
stage gets the same number of runs.  Stage 1 (1 run) keeps the best `--keep1` sequences plus the best of
every 5-gap prefix within 6 of the cut; stage 2 (3 more runs) keeps the best `--keep2` plus the best of
every 4-gap core within 6 of the cut; stage 3 adds 8 runs of twice the length.  Everything is stored in
results/famscan_N.json for the family analysis; finalists go to race.py / tournament runs.

usage: python scripts/famscan.py N [--maxgap G] [--iters 2000000] [--keep1 2000] [--keep2 300] [--plan NAME]
                                  [--iters2 I2] [--iters3 I3] [--T0 3.0] [--slack 6]
       --iters2/--iters3: moves per run in stages 2 and 3 (default iters and 2*iters); --T0: annealing start
       temperature; --slack: how far above the cut a family's best member may score and still be kept;
       --plan nbr --from famscan_N.json|logs/X.list [--top 25] [--tag T]: refinement around an earlier screen's
       finalists or a ranked list (output results/famscan_N_nbrT.json)
       env BATCH_PT="R,Tmin,Tmax,mpe" screens with parallel tempering instead of annealing (same moves)
"""
import itertools, json, math, os, random, subprocess, sys, time

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
RES = os.path.join(ROOT, "results")
SHELLWC = os.path.join(ROOT, "bin", "shellwc.exe")


def opt(k, d):
    return sys.argv[sys.argv.index(k) + 1] if k in sys.argv else d


def completions(core, maxgap, nmax, step=1, lo=None):
    """core + every set of 1..nmax larger gaps (grid `step`, starting above the core's largest gap)"""
    lo = max(core) + 1 if lo is None else lo
    rng = list(range(lo, maxgap + 1, step))
    out = []
    for k in range(1, nmax + 1):
        for comb in itertools.combinations(rng, k):
            out.append(tuple(core) + comb)
    return out


def plan64(maxgap):
    c = set()
    for cc in [10] + list(range(12, 21)):                       # {1,4,9,c,x[,y]}, c != 11 (scan64 did c = 11)
        c.update(completions((1, 4, 9, cc), maxgap, 2))
    c.update(completions((1, 4, 7, 9), maxgap, 2))              # the N <= 42 core
    for cc in range(12, 21):                                    # {1,4,10,c,x[,y]}
        c.update(completions((1, 4, 10, cc), maxgap, 2))
    for x in range(14, 37):                                     # 7 gaps {1,4,9,11,x,y,z}
        c.update(completions((1, 4, 9, 11, x), maxgap, 2))
    for a in (3, 5):                                            # other second gaps, coarse upper grid
        for b in range(7, 14):
            for d in range(b + 1, b + 13):
                c.update(completions((1, a, b, d), maxgap, 2, step=4))
    for b in (5, 6, 8, 11, 12, 13):                             # {1,4,b,d,...} with b not in {7,9,10}
        for d in range(b + 1, b + 17):
            c.update(completions((1, 4, b, d), maxgap, 2, step=3))
    return sorted(c)


def plan64b(maxgap):
    """refinement: 8 gaps {1,4,9,11,x,y,z,w} (x in the leading middle gaps, y,z,w >= 36), and the 7-gap
    {1,4,9,11,x,y,z} with x above the plan64 range"""
    c = set()
    for x in (21, 23, 30, 32, 34):
        for comb in itertools.combinations(range(36, maxgap + 1), 3):
            c.add((1, 4, 9, 11, x) + comb)
    for x in range(37, maxgap - 1):
        c.update(s for s in completions((1, 4, 9, 11, x), maxgap, 2) if len(s) == 7)
    return sorted(c)


def plan91(maxgap):
    c = set()
    for cc in range(10, 21):                                    # {1,4,9,c,x[,y]} incl. c = 11
        c.update(completions((1, 4, 9, cc), maxgap, 2))
    for cc in (11, 15):                                         # 7 gaps {1,4,9,c,x,y,z}
        for x in range(cc + 3, 41):
            c.update(completions((1, 4, 9, cc, x), maxgap, 2))
    c.update(completions((1, 4, 7, 9), maxgap, 3))
    for cc in range(12, 21):
        c.update(completions((1, 4, 10, cc), maxgap, 2))
    for a in (3, 5):
        for b in range(7, 14):
            for d in range(b + 1, b + 13):
                c.update(completions((1, a, b, d), maxgap, 2, step=3))
    for b in (5, 6, 8, 11, 12, 13):
        for d in range(b + 1, b + 17):
            c.update(completions((1, 4, b, d), maxgap, 2, step=2))
    return sorted(c)


def sample(rng, core, maxgap, k, m):
    """`m` random sets of `k` larger gaps for `core` (all of them when there are at most `m`)"""
    pool = range(max(core) + 1, maxgap + 1)
    if math.comb(len(pool), k) > 200000:            # too many to list: draw sets until m distinct ones
        out = set()
        while len(out) < m:
            out.add(tuple(core) + tuple(sorted(rng.sample(pool, k))))
        return sorted(out)
    combs = list(itertools.combinations(pool, k))
    return [tuple(core) + c for c in (rng.sample(combs, m) if len(combs) > m else combs)]


def plan128(maxgap):
    """the two leading cores completed in full; the cores within ~25 of the best at N = 91 completed in full
    up to 6 gaps and sampled at 7; 8-gap sequences sampled for every fifth gap 18-40"""
    rng = random.Random(128)
    c = set()
    for core in ((1, 4, 9, 11), (1, 4, 9, 15)):
        c.update(completions(core, maxgap, 3))
    for core in ((1, 4, 7, 9), (1, 4, 11, 13), (1, 4, 10, 15), (1, 3, 8, 10)):
        c.update(completions(core, maxgap, 2))
        c.update(sample(rng, core, maxgap, 3, 2500))
    for core in ((1, 4, 10, 17), (1, 3, 7, 17), (1, 4, 10, 13), (1, 3, 7, 11), (1, 5, 7, 8), (1, 4, 13, 15),
                 (1, 5, 8, 12), (1, 5, 8, 17), (1, 5, 9, 11), (1, 3, 10, 11), (1, 4, 9, 10), (1, 4, 9, 12),
                 (1, 4, 9, 13), (1, 4, 9, 14), (1, 4, 9, 16), (1, 4, 9, 17)):
        c.update(completions(core, maxgap, 1))
        c.update(sample(rng, core, maxgap, 2, 200))
        c.update(sample(rng, core, maxgap, 3, 400))
    for x in range(18, 41):
        c.update(sample(rng, (1, 4, 9, 11, x), maxgap, 3, 250))
    for x in range(22, 41):
        c.update(sample(rng, (1, 4, 9, 15, x), maxgap, 3, 150))
    return sorted(c)


def plan181(maxgap):
    """{1,4,9,11}: all 5-6 gap sequences, all 7-gap ones for the fifth gaps that led at N = 64-128 (19-36) and
    samples for the others (17-48), sampled 8-gap ones (denser for the fifth gaps 21, 30, 34 that led the N = 128
    attacks); {1,4,9,15} and {1,4,7,9} (the next cores at N = 128): all 6-gap, sampled 7-8 gap; {1,4,11,13} and
    {1,3,8,10}: samples"""
    rng = random.Random(181)
    c = set()
    c.update(completions((1, 4, 9, 11), maxgap, 2))
    for x in range(17, 49):
        core = (1, 4, 9, 11, x)
        c.update(completions(core, maxgap, 2) if x in (19, 21, 23, 25, 26, 28, 30, 32, 34, 36)
                 else sample(rng, core, maxgap, 2, 400))
    for x in range(19, 41):
        c.update(sample(rng, (1, 4, 9, 11, x), maxgap, 3, 3000 if x in (21, 30, 34) else 500))
    c.update(completions((1, 4, 9, 15), maxgap, 2))
    for x in range(20, 45):
        c.update(sample(rng, (1, 4, 9, 15, x), maxgap, 2, 400))
    for x in range(22, 41):
        c.update(sample(rng, (1, 4, 9, 15, x), maxgap, 3, 200))
    c.update(completions((1, 4, 7, 9), maxgap, 2))
    c.update(sample(rng, (1, 4, 7, 9), maxgap, 3, 3000))
    c.update(sample(rng, (1, 4, 7, 9), maxgap, 4, 1500))
    for core in ((1, 4, 11, 13), (1, 3, 8, 10)):
        c.update(completions(core, maxgap, 1))
        c.update(sample(rng, core, maxgap, 2, 800))
        c.update(sample(rng, core, maxgap, 3, 1500))
        c.update(sample(rng, core, maxgap, 4, 500))
    return sorted(c)


def plan256(maxgap):
    """built on the N = 128 / 181 results: {1,4,9,11,30} (the fifth gap that led both) with every 6-7 gap
    completion and sampled 8-9 gap ones, denser for {1,4,9,11,30,36} and {1,4,9,11,30,47} (the leaders' sixth
    gaps at N = 181); other fifth gaps of {1,4,9,11} and {1,4,9,15} (25-40) sampled; a few samples of the
    cores that fell behind at N = 128 / 181, for the cross-N family table"""
    rng = random.Random(256)
    c = set()
    for core in ((1, 4, 9, 11), (1, 4, 9, 15)):
        c.update(completions(core, maxgap, 1))
        c.update(sample(rng, core, maxgap, 2, 1000))
    c.update(completions((1, 4, 9, 11, 30), maxgap, 2))
    c.update(sample(rng, (1, 4, 9, 11, 30), maxgap, 3, 10000))
    c.update(sample(rng, (1, 4, 9, 11, 30), maxgap, 4, 4000))
    for g6 in (36, 47):
        c.update(completions((1, 4, 9, 11, 30, g6), maxgap, 2))
        c.update(sample(rng, (1, 4, 9, 11, 30, g6), maxgap, 3, 2000))
    for x in (21, 23, 25, 27, 28, 29, 31, 32, 33, 34, 35, 36, 38, 40):
        for k, m in ((2, 300), (3, 350), (4, 150)):
            c.update(sample(rng, (1, 4, 9, 11, x), maxgap, k, m))
    for x in range(25, 41):
        for k, m in ((2, 300), (3, 350), (4, 150)):
            c.update(sample(rng, (1, 4, 9, 15, x), maxgap, k, m))
    for core in ((1, 4, 7, 9), (1, 4, 11, 13), (1, 3, 8, 10)):
        c.update(sample(rng, core, maxgap, 3, 300))
        c.update(sample(rng, core, maxgap, 4, 300))
    return sorted(c)


def plan256b(maxgap):
    """{1,4,9,15} at the sampling density plan256 gave {1,4,9,11,30}: fifth gaps 29-37 (31 led {1,4,9,15} at
    N = 181), 7-9 gap completions sampled"""
    rng = random.Random(2562)
    c = set()
    for x in range(29, 38):
        for k, m in ((2, 300), (3, 1500), (4, 500)):
            c.update(sample(rng, (1, 4, 9, 15, x), maxgap, k, m))
    return sorted(c)


# the N = 256 sequences with bounds <= 4193 after rounds of >= 1G moves per thread, README sequences excluded
# (docs/CAMPAIGN_256.md)
N256_BEST = ((1, 4, 9, 11, 30, 47, 67, 95), (1, 4, 9, 11, 30, 36, 59, 122), (1, 4, 9, 11, 30, 36, 62, 89),
             (1, 4, 9, 11, 30, 36, 70, 109), (1, 4, 9, 11, 30, 36, 70, 113), (1, 4, 9, 15, 31, 56, 74, 126),
             (1, 4, 9, 11, 30, 36, 70, 123), (1, 4, 9, 11, 30, 36, 59, 109), (1, 4, 9, 11, 32, 36, 62, 119),
             (1, 4, 9, 11, 30, 36, 59, 106, 127), (1, 4, 9, 11, 30, 36, 70, 117), (1, 4, 9, 11, 30, 47, 75, 87),
             (1, 4, 9, 11, 30, 36, 59, 104), (1, 4, 9, 11, 30, 47, 67, 106))


def plan362(maxgap):
    """built on the N = 256 attacks: the 7-gap prefixes of the four best N = 256 sequences with sampled 9- and
    10-gap completions; {1,4,9,11,30,36} and {1,4,9,11,30,47} (their sixth gaps) and {1,4,9,11,30} with any sixth
    gap, sampled 8-10 gap completions; N256_BEST extended by one gap; other fifth gaps of {1,4,9,11} and
    {1,4,9,15} sampled (denser for {1,4,9,15,31} and {1,4,9,15,35}, its best prefixes at N = 256); a few samples
    of the cores that fell behind earlier, for the cross-N family table"""
    rng = random.Random(362)
    c = set()
    for p in ((1, 4, 9, 11, 30, 47, 67), (1, 4, 9, 11, 30, 36, 59), (1, 4, 9, 11, 30, 36, 62),
              (1, 4, 9, 11, 30, 36, 70)):
        c.update(sample(rng, p, maxgap, 2, 2000)); c.update(sample(rng, p, maxgap, 3, 400))
    for g6 in (36, 47):
        for k, m in ((2, 800), (3, 3000), (4, 1000)):
            c.update(sample(rng, (1, 4, 9, 11, 30, g6), maxgap, k, m))
    for k, m in ((3, 600), (4, 3000), (5, 600)):
        c.update(sample(rng, (1, 4, 9, 11, 30), maxgap, k, m))
    for s in N256_BEST:
        c.update(s + (g,) for g in range(s[-1] + 1, maxgap + 1))
    for x in (21, 25, 27, 28, 29, 31, 32, 33, 34, 35, 36, 38, 40):
        for k, m in ((3, 80), (4, 250), (5, 80)):
            c.update(sample(rng, (1, 4, 9, 11, x), maxgap, k, m))
    for x in (29, 31, 32, 34, 35, 37):
        for k, m in ((3, 200), (4, 1200), (5, 300)) if x in (31, 35) else ((3, 100), (4, 500), (5, 100)):
            c.update(sample(rng, (1, 4, 9, 15, x), maxgap, k, m))
    c.update(sample(rng, (1, 4, 9, 15, 31, 56, 74), maxgap, 2, 800))
    for core in ((1, 4, 7, 9), (1, 4, 11, 13), (1, 3, 8, 10)):
        c.update(sample(rng, core, maxgap, 5, 300))
    return sorted(c)


# the N = 362 sequences found by this work with bounds <= 6545 after rounds of >= 512M moves per thread (README
# sequence and the author's candidates excluded; docs/CAMPAIGN_362.md)
N362_BEST = ((1, 4, 9, 11, 30, 36, 70, 109, 171), (1, 4, 9, 11, 30, 36, 70, 117, 143), (1, 4, 9, 11, 30, 36, 62, 121, 161),
             (1, 4, 9, 11, 30, 47, 67, 132, 180), (1, 4, 9, 11, 30, 47, 67, 95, 169), (1, 4, 9, 15, 31, 56, 74, 126, 177),
             (1, 4, 9, 11, 30, 36, 59, 98, 162))


def plan512(maxgap):
    """built on the N = 362 attacks: the 7-gap prefixes that held up there ({1,4,9,11,30,36,70|62}) with sampled 9-11
    gap completions; the 8-gap prefixes of the three N = 362 leaders with every ninth gap and sampled tenth gaps;
    N362_BEST extended by every tenth gap or with its last gap moved; {1,4,9,11,30,36}, {1,4,9,11,30,47} and
    {1,4,9,11,30} with any next gap, sampled 9-11 gap completions; {1,4,9,15,31} and {1,4,9,15,31,56,74}; a few
    samples of other fifth gaps and other cores, for the cross-N family table"""
    rng = random.Random(512)
    c = set()
    for p in ((1, 4, 9, 11, 30, 36, 70), (1, 4, 9, 11, 30, 36, 62)):
        for k, m in ((2, 500), (3, 1500), (4, 400)):
            c.update(sample(rng, p, maxgap, k, m))
    for p in ((1, 4, 9, 11, 30, 36, 70, 109), (1, 4, 9, 11, 30, 36, 70, 117), (1, 4, 9, 11, 30, 36, 62, 121)):
        c.update(sample(rng, p, maxgap, 1, 1000)); c.update(sample(rng, p, maxgap, 2, 500))
    for s in N362_BEST:
        c.update(s + (g,) for g in range(s[-1] + 1, maxgap + 1))
        c.update(s[:-1] + (g,) for g in range(s[-2] + 1, maxgap + 1))
    for p, sizes in (((1, 4, 9, 11, 30, 36), ((3, 400), (4, 1500), (5, 500))),
                     ((1, 4, 9, 11, 30, 47), ((3, 200), (4, 700), (5, 200))),
                     ((1, 4, 9, 11, 30, 47, 67), ((3, 300),)),
                     ((1, 4, 9, 11, 30), ((4, 300), (5, 1000), (6, 300))),
                     ((1, 4, 9, 15, 31), ((4, 200), (5, 600), (6, 200))),
                     ((1, 4, 9, 15, 31, 56, 74), ((2, 300), (3, 500))),
                     ((1, 4, 9, 15, 35), ((5, 200),))):
        for k, m in sizes:
            c.update(sample(rng, p, maxgap, k, m))
    for x in (25, 28, 32, 34):
        c.update(sample(rng, (1, 4, 9, 11, x), maxgap, 5, 100))
    for core in ((1, 4, 7, 9), (1, 4, 11, 13), (1, 3, 8, 10)):
        c.update(sample(rng, core, maxgap, 6, 100))
    return sorted(c)


def nbr_base():
    """the starting sequences of a refinement: the `--top` finalists of an earlier screen (`--from famscan_N.json`)
    or the first `--top` sequences of a ';'-separated list (`--from logs/roundB_N.list`, e.g. ranked by attacks)"""
    src, top = opt("--from", ""), int(opt("--top", "25"))
    if src.endswith(".json"):
        return [tuple(r["seq"]) for r in json.load(open(os.path.join(RES, src)))["final"][:top]]
    return [tuple(int(x) for x in s.split(",")) for s in open(os.path.join(ROOT, src)).read().strip().split(";")][:top]


def plan_nbr(maxgap):
    """local refinement: the starting sequences (nbr_base) and every sequence one step away from one of them: a gap
    above the core moved by 1-3, removed, or added (grid of 3 between neighbours)"""
    base = nbr_base()
    c = set(base)
    for s in base:
        for i in range(4, len(s)):
            for dlt in (-3, -2, -1, 1, 2, 3):
                g = s[i] + dlt
                if s[i - 1] < g <= maxgap and (i + 1 == len(s) or g < s[i + 1]):
                    c.add(s[:i] + (g,) + s[i + 1:])
            c.add(s[:i] + s[i + 1:])
        for i in range(4, len(s) + 1):
            hi = s[i] if i < len(s) else maxgap + 1
            for g in range(s[i - 1] + 2, hi - 1, 3):
                c.add(s[:i] + (g,) + s[i:])
    return sorted(c)


def batch(n, seqs, iters, seed, tag, T0="3.0"):
    path = os.path.join(RES, f"_famscan_{n}_{tag}.txt")
    with open(path, "w") as f:
        for s in seqs:
            f.write(",".join(map(str, s)) + "\n")
    out = subprocess.run([SHELLWC, "batch", str(n), path, str(iters), str(seed), T0], capture_output=True,
                         text=True).stdout
    res = {}
    for line in out.splitlines():
        w, s = line.split()
        res[tuple(int(x) for x in s.split(","))] = int(w)
    os.remove(path)
    return res


def main():
    n = int(sys.argv[1])
    maxgap = int(opt("--maxgap", str(n - 1 if n < 91 else n // 2)))
    iters = int(opt("--iters", "2000000"))
    keep1, keep2 = int(opt("--keep1", "2000")), int(opt("--keep2", "300"))
    iters2, iters3 = int(opt("--iters2", str(iters))), int(opt("--iters3", str(2 * iters)))
    T0, slack = opt("--T0", "3.0"), int(opt("--slack", "6"))
    plan = opt("--plan", str(n))
    cands = {"64": plan64, "64b": plan64b, "91": plan91, "128": plan128, "181": plan181, "256": plan256,
             "256b": plan256b, "362": plan362, "512": plan512, "nbr": plan_nbr}[plan](maxgap)
    db_path = os.path.join(RES, f"famscan_{n}" + ("" if plan == str(n) else f"_{plan}") + opt("--tag", "") + ".json")
    t = time.time()
    runs = {s: [] for s in cands}
    print(f"N={n}: {len(cands)} candidates (plan {plan}, maxgap {maxgap})", flush=True)

    def stage(seqs, reps, it, seed0, tag):
        for r in range(reps):
            res = batch(n, seqs, it, seed0 + r, f"{tag}{r}", T0)
            for s, w in res.items():
                runs[s].append(w)
            print(f"  {tag} run {r + 1}/{reps}: {len(seqs)} sequences x {it} moves ({time.time() - t:.0f}s)", flush=True)

    score = lambda s: max(runs[s])

    def survivors(seqs, keep, plen):
        """the best `keep`, plus the best of every `plen`-gap prefix family scoring within `slack` of the cut"""
        order = sorted(seqs, key=lambda s: (score(s), len(s), s))
        cut = score(order[min(keep, len(order)) - 1])
        out, seen = set(order[:keep]), set()
        for s in order:
            if s[:plen] not in seen:
                seen.add(s[:plen])
                if score(s) <= cut + slack:
                    out.add(s)
        return sorted(out)

    stage(cands, 1, iters, 101, "s1_")
    s2 = survivors(cands, keep1, 5)
    stage(s2, 3, iters2, 201, "s2_")
    s3 = survivors(s2, keep2, 4)
    stage(s3, 8, iters3, 301, "s3_")
    final = sorted(s3, key=lambda s: (score(s), len(s), s))
    fam4 = {}                       # a family's representative: its member that reached the deepest stage
    for s in sorted(cands, key=lambda s: (-len(runs[s]), score(s), s)):
        fam4.setdefault(",".join(map(str, s[:4])), []).append(s)
    db = {"N": n, "maxgap": maxgap, "iters": iters, "iters2": iters2, "iters3": iters3, "plan": plan,
          "n_candidates": len(cands), "T0": float(T0), "slack": slack,
          "engine": "pt " + os.environ["BATCH_PT"] if os.environ.get("BATCH_PT") else "anneal",
          "time_s": round(time.time() - t),
          "stage_sizes": [len(cands), len(s2), len(s3)],
          "final": [{"seq": list(s), "score": score(s), "runs": runs[s]} for s in final],
          "families4": {k: {"best": score(v[0]), "best_seq": list(v[0]), "n": len(v)} for k, v in fam4.items()},
          "all": {",".join(map(str, s)): runs[s] for s in cands}}
    if plan == "nbr":                   # record where a refinement started (a ranked list may hold earlier candidates)
        db["from"], db["base"] = opt("--from", ""), [list(s) for s in nbr_base()]
    json.dump(db, open(db_path, "w"))
    print(f"N={n}: done in {time.time() - t:.0f}s; top 25:", flush=True)
    for s in final[:25]:
        print(f"  {score(s)}  {{{','.join(map(str, s))}}}  runs={sorted(runs[s])}", flush=True)


if __name__ == "__main__":
    main()
