"""Exact worst-case input for a layered-certified optimum: rerun the one-byte layered DP with every level
dumped to disk, walk forward to reconstruct an attaining input, verify it in Python, cache it in
results/witness_cache.json, and delete the dump.

usage: python scripts/reconstruct.py N 1,4,9,11,21 DUMPDIR
"""
import json, os, re, shutil, subprocess, sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
RES = os.path.join(ROOT, "results")
SHELLWC = os.path.join(ROOT, "bin", "shellwc.exe")
CACHE = os.path.join(RES, "witness_cache.json")
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from witnesses import cost


def main():
    n, seq, base = int(sys.argv[1]), sys.argv[2], sys.argv[3]
    d = os.path.join(base, f"N{n}_{seq.replace(',', '_')}")
    os.makedirs(d, exist_ok=True)
    try:
        out = subprocess.run([SHELLWC, "lay", str(n), seq, "12000000000", "-1", d], capture_output=True, text=True).stdout
    finally:
        shutil.rmtree(d, ignore_errors=True)
    m = re.search(r"lay_worst=(\d+) check=(\d+).*perm=\[([\d,]+)\]", out)
    if not m:
        sys.exit(f"reconstruction failed: {out.strip()}")
    W, perm = int(m.group(1)), [int(x) for x in m.group(3).split(",")]
    c = cost(perm, [int(x) for x in seq.split(",")])
    assert c == W, (c, W)
    cache = json.load(open(CACHE)) if os.path.exists(CACHE) else {}
    cache[f"{n}:{seq}"] = perm
    json.dump(cache, open(CACHE, "w"))
    print(f"N={n} {{{seq}}}: exact {W}, attaining input reconstructed and verified")


if __name__ == "__main__":
    main()
