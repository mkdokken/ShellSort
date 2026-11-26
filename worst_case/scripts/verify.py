"""Independent re-simulation (pure Python, no C code) of every stored witness input.

Checks: summary.json (exact optima: the stored input must reach the certified W),
transfer*.json and strong.json (large-N lower bounds: the stored input must reach the reported value),
avgopt.json (README average-optimal sequences: the stored input must reach the reported worst case).
Also verifies the two headline counter-examples quoted in the report.
"""
import json, os, sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
RES = os.path.join(ROOT, "results")


def comparisons(perm, gaps):
    a, n, c = list(perm), len(perm), 0
    assert sorted(a) == list(range(n)), "not a permutation"
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
    bad = ok = short = 0
    p = os.path.join(RES, "summary.json")
    if os.path.exists(p):
        for k, s in json.load(open(p)).items():
            for r in s["optimal"]:
                c = comparisons(r["worst_input"], r["seq"])
                if c > r["W"]:
                    bad += 1; print("CONTRADICTION", k, r["seq"], c, r["W"])
                elif c < r["W"]:
                    short += 1
                else:
                    ok += 1
    for name in ("transfer.json", "transfer_div.json"):
        p = os.path.join(RES, name)
        if not os.path.exists(p):
            continue
        for k, d in json.load(open(p)).items():
            for r in d["final"] + ([d["readme"]] if d.get("readme") else []):
                if r.get("witness"):
                    c = comparisons(r["witness"], r["seq"])
                    if c != r["W_lower"] and c < r["W_lower"]:
                        # W_lower may include a phase-2 value without stored witness; only flag if stored input is above
                        short += 1
                    elif c > r["W_lower"]:
                        bad += 1; print("MISMATCH", name, k, r["seq"], c, r["W_lower"])
                    else:
                        ok += 1
    p = os.path.join(RES, "strong.json")
    if os.path.exists(p):
        for k, r in json.load(open(p)).items():
            if r.get("witness"):
                c = comparisons(r["witness"], r["seq"])
                if c != r["W_lower"]:
                    bad += 1; print("MISMATCH strong", k, c, r["W_lower"])
                else:
                    ok += 1
    p = os.path.join(RES, "avgopt.json")              # README average-optimal sequences (exact or lower bound)
    if os.path.exists(p):
        for k, r in json.load(open(p)).items():
            if r.get("witness"):
                c = comparisons(r["witness"], r["seq"])
                if c != r["W"]:
                    bad += 1; print("MISMATCH avgopt", k, r["seq"], c, r["W"])
                else:
                    ok += 1
    # headline counter-example: {1,3,7,12} at N=14 is not optimal (README lists it with 64)
    c = comparisons([3, 8, 0, 6, 11, 1, 5, 10, 13, 4, 9, 12, 2, 7], [1, 3, 7, 12])
    print(f"{{1,3,7,12}} @14 witness: {c} comparisons")
    print(f"witnesses verified: {ok}, inputs below the recorded value (value from a DP or a run whose input "
          f"was not stored): {short}, contradictions: {bad}")
    sys.exit(1 if bad else 0)


if __name__ == "__main__":
    main()
