"""Cross-evaluation: every stored worst-case input of size N is re-simulated on every other stored sequence of
the same N (inputs found for one sequence are often strong for its neighbours).  Any improvement is stored in
results/strong.json with mode "crosseval".

usage: python scripts/crosseval.py [N ...]
"""
import json, os, sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
RES = os.path.join(ROOT, "results")
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from witnesses import cost


def main():
    ns = [int(x) for x in sys.argv[1:]] or [64, 91, 128, 181, 256, 362]
    sp = os.path.join(RES, "strong.json")
    st = json.load(open(sp))
    for n in ns:
        recs = {k: v for k, v in st.items() if int(k.split(":")[0]) == n}
        inputs = [(k, v["witness"]) for k, v in recs.items() if v.get("witness")]
        better = 0
        for k, v in recs.items():
            seq = [int(x) for x in k.split(":")[1].split(",")]
            best, src = v["W_lower"], None
            for k2, perm in inputs:
                if k2 != k:
                    c = cost(perm, seq)
                    if c > best:
                        best, src, bp = c, k2, perm
            if src:
                print(f"N={n} {{{k.split(':')[1]}}}: {v['W_lower']} -> {best} (input of {{{src.split(':')[1]}}})", flush=True)
                v["runs"].append({"mode": "crosseval", "W": best, "from": src})
                v["W_lower"], v["witness"] = best, bp
                better += 1
        print(f"N={n}: {len(recs)} sequences, {len(inputs)} inputs, {better} improved", flush=True)
    st2 = json.load(open(sp))          # merge into the current file (a race may have written meanwhile)
    for k, v in st.items():
        if k in st2 and v["W_lower"] > st2[k]["W_lower"]:
            st2[k]["W_lower"], st2[k]["witness"] = v["W_lower"], v["witness"]
            st2[k]["runs"].append(v["runs"][-1])
    json.dump(st2, open(sp, "w"), indent=1)


if __name__ == "__main__":
    main()
