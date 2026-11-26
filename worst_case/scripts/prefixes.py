"""Cross-N prefix table: for every 5-gap (or --len L) prefix, the best stored lower bound of any sequence
with that prefix at each large N (stored, re-simulated inputs in results/strong.json; gaps <= N/2 for N >= 91),
and its excess over the best value at that N.  A prefix that stays close to the best at several N is a
candidate to extend at larger N.

usage: python scripts/prefixes.py [--len 5] [--ns 64,91] [--top 20]
"""
import json, os, sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))


def opt(k, d):
    return sys.argv[sys.argv.index(k) + 1] if k in sys.argv else d


def main():
    plen, top = int(opt("--len", "5")), int(opt("--top", "20"))
    ns = [int(x) for x in opt("--ns", "64,91").split(",")]
    st = json.load(open(os.path.join(ROOT, "results", "strong.json")))
    best = {n: {} for n in ns}
    for k, v in st.items():
        n, s = k.split(":")
        n = int(n)
        if n not in best:
            continue
        g = [int(x) for x in s.split(",")]
        if n >= 91 and max(g) > n // 2:
            continue
        p = tuple(g[:plen])
        if len(g) < plen:
            continue
        if p not in best[n] or v["W_lower"] < best[n][p][0]:
            best[n][p] = (v["W_lower"], s)
    lo = {n: min(w for w, _ in best[n].values()) for n in ns if best[n]}
    prefs = sorted(set().union(*[best[n].keys() for n in ns]),
                   key=lambda p: (sum(best[n][p][0] - lo[n] if p in best[n] else 99 for n in ns), p))
    print("prefix | " + " | ".join(f"N={n}: best (excess) by" for n in ns))
    for p in prefs[:top]:
        cells = []
        for n in ns:
            if p in best[n]:
                w, s = best[n][p]
                cells.append(f"{w} (+{w - lo[n]}) {{{s}}}")
            else:
                cells.append("–")
        print("{" + ",".join(map(str, p)) + "} | " + " | ".join(cells))


if __name__ == "__main__":
    main()
