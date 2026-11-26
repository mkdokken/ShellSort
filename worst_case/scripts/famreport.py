"""Summarise a family screen (results/famscan_N.json) next to the stored lower bounds (results/strong.json).

Prints the finalists, the best member of every 4-gap core and 5-gap prefix that reached the last stage,
and the finalists that are new (no stored input yet) -- the input list for maxout.py.

usage: python scripts/famreport.py N [--top 40] [--new 30]
"""
import json, os, sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
RES = os.path.join(ROOT, "results")


def opt(k, d):
    return sys.argv[sys.argv.index(k) + 1] if k in sys.argv else d


def main():
    n = int(sys.argv[1])
    top, nnew = int(opt("--top", "40")), int(opt("--new", "30"))
    db = json.load(open(os.path.join(RES, f"famscan_{n}.json")))
    st = json.load(open(os.path.join(RES, "strong.json")))
    known = {k.split(":")[1]: v["W_lower"] for k, v in st.items() if k.startswith(f"{n}:")}
    allr = db["all"]
    deep = max(len(v) for v in allr.values())
    fin = [(max(v), k) for k, v in allr.items() if len(v) == deep]
    fin.sort(key=lambda x: (x[0], len(x[1].split(",")), x[1]))
    print(f"N={n}: {db['n_candidates']} candidates, stages {db['stage_sizes']}, {db['time_s']}s; "
          f"finalists have {deep} runs")
    for w, s in fin[:top]:
        print(f"  {w}  {{{s}}}" + (f"   stored {known[s]}" if s in known else ""))
    for plen in (4, 5):
        fam = {}
        for w, s in fin:
            key = ",".join(s.split(",")[:plen])
            if key not in fam:
                fam[key] = (w, s)
        print(f"best finalist per {plen}-gap prefix ({len(fam)} prefixes):")
        for key, (w, s) in sorted(fam.items(), key=lambda kv: kv[1])[:25]:
            print(f"  {{{key}}}: {w} by {{{s}}}")
    new = [s for w, s in fin if s not in known][:nnew]
    print("NEW:" + ";".join(new))


if __name__ == "__main__":
    main()
