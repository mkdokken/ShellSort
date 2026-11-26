"""Compare our independently derived worst-case optima (results/summary.json) with the README tables.

Writes results/comparison.md and prints a compact table.
"""
import json, os, sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from readme_data import README_WORST, README_ALTS

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
RES = os.path.join(ROOT, "results")


def fmt(seq):
    return "{" + ",".join(map(str, seq)) + "}"


def main():
    with open(os.path.join(RES, "summary.json")) as f:
        summ = json.load(f)
    lines = ["| N | README best (W) | our W* | our best (avg tie-break) | ties | exact? | README tie list vs ours |",
             "|---|---|---|---|---|---|---|"]
    for key in sorted(summ, key=int):
        n = int(key)
        s = summ[key]
        ours = s["optimal"]
        best = ours[0] if ours else None
        oset = {tuple(r["seq"]) for r in ours}
        rd = README_WORST.get(n)
        rseq, rW = (rd[0], rd[1]) if rd else (None, None)
        rset = set()
        if rd:
            rset.add(tuple(rseq))
            for a in README_ALTS.get(n, []):
                rset.add(tuple(a))
        missing = sorted(oset - rset, key=lambda t: (len(t), t))      # ours, not in README
        extra = sorted(rset - oset, key=lambda t: (len(t), t))        # README, not ours
        if rd is None:
            note = "(not in README)"
        elif s["W"] != rW:
            note = f"VALUE DIFFERS: README {rW}, ours {s['W']}"
        elif not missing and not extra:
            note = "identical"
        else:
            parts = []
            if missing:
                parts.append("new ties: " + ", ".join(fmt(t) for t in missing))
            if extra:
                parts.append("README-listed but not optimal: " + ", ".join(fmt(t) for t in extra))
            note = "; ".join(parts)
        same_best = rd is not None and best is not None and tuple(best["seq"]) == tuple(rseq)
        lines.append(f"| {n} | {fmt(rseq) + ' (' + str(rW) + ')' if rd else '-'} | {s['W']} | "
                     f"{fmt(best['seq']) if best else '-'} {'(=README)' if same_best else ''} "
                     f"avg {best['avg']:.3f} | {len(ours)} | {'yes' if s['all_exact'] else 'NO'} | {note} |")
    out = "\n".join(lines)
    with open(os.path.join(RES, "comparison.md"), "w") as f:
        f.write(out + "\n")
    print(out)


if __name__ == "__main__":
    main()
