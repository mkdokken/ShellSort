"""Attach to every certified optimum a concrete input that attains the certified worst case.

The layered DP proves exact values without reconstructing an input; for those entries a targeted search
(incremental annealing that stops when the certified value is reached) looks for an attaining input,
re-simulated here in Python.  Found inputs are cached in results/witness_cache.json so they can be
re-applied after summary.json is rewritten.  Entries for which no attaining input is found keep their
exact (DP-proved) value and are marked "witness": "not reconstructed".
"""
import json, os, re, subprocess, sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
RES = os.path.join(ROOT, "results")
SHELLWC = os.path.join(ROOT, "bin", "shellwc.exe")
CACHE = os.path.join(RES, "witness_cache.json")


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
    assert a == sorted(a)
    return c


def main():
    search = "--apply-only" not in sys.argv
    path = os.path.join(RES, "summary.json")
    summ = json.load(open(path))
    cache = json.load(open(CACHE)) if os.path.exists(CACHE) else {}
    changed = missing = 0
    for key in sorted(summ, key=int):
        for r in summ[key]["optimal"]:
            n, seq, W = summ[key]["N"], r["seq"], r["W"]
            ck = f"{n}:{','.join(map(str, seq))}"
            if cost(r["worst_input"], seq) == W:
                r.pop("witness_status", None)
                continue
            if ck in cache and cost(cache[ck], seq) == W:
                r["worst_input"] = cache[ck]; r.pop("witness_status", None); changed += 1
                continue
            found = False
            if search:
                for attempt in range(2):
                    out = subprocess.run([SHELLWC, "fwc", str(n), ",".join(map(str, seq)), "4000000", str(31 + attempt),
                                          "3.0", "0.05", "10", str(W), "100"], capture_output=True, text=True).stdout
                    m = re.search(r"perm=\[([\d,]+)\]", out)
                    perm = [int(x) for x in m.group(1).split(",")]
                    c = cost(perm, seq)
                    if c > W:
                        sys.exit(f"CONTRADICTION: input of cost {c} > certified {W} for N={n} {seq}")
                    if c == W:
                        r["worst_input"] = perm; cache[ck] = perm; found = True; changed += 1
                        json.dump(cache, open(CACHE, "w"))
                        print(f"N={n} {{{','.join(map(str, seq))}}}: attaining input found ({W})", flush=True)
                        break
            if not found:
                r["witness_status"] = "exact by DP; attaining input not reconstructed"
                missing += 1
                print(f"N={n} {{{','.join(map(str, seq))}}}: exact {W} by DP, no attaining input reconstructed", flush=True)
    json.dump(summ, open(path, "w"), indent=1)
    print(f"{changed} witnesses attached, {missing} without reconstructed input")


if __name__ == "__main__":
    main()
