"""Copy the files meant for the public `worst_case/` folder of the GitHub repository into DEST and check sizes.

usage: python scripts/export_worst_case.py [DEST] [--replace]
       (default DEST: github_upload/worst_case inside this project)

Copied: README.md, REPORT.md, report.html, build.ps1 and the folders src/, scripts/, docs/, results/, logs/.
Left out: build outputs (bin/; build.ps1 recreates them), initial_instructions.txt, scratch files in results/
whose names start with '_', empty *.err files, and temporary files. The export fails if any file reaches 50 MB
(GitHub warns at 50 MB and rejects files of 100 MB) or the folder reaches 200 MB. DEST must be empty or missing
unless --replace is given, in which case it is deleted first.
"""
import os, shutil, sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
FILES = ["README.md", "REPORT.md", "report.html", "build.ps1"]
DIRS = ["src", "scripts", "docs", "results", "logs"]
FILE_LIMIT, TOTAL_LIMIT = 50 * 2 ** 20, 200 * 2 ** 20


def wanted(rel):
    name = os.path.basename(rel)
    if "__pycache__" in rel.split(os.sep) or name.endswith((".pyc", ".tmp")):
        return False
    if rel.startswith("results" + os.sep) and name.startswith("_"):
        return False
    if name.endswith(".err") and os.path.getsize(os.path.join(ROOT, rel)) == 0:
        return False
    return True


def main():
    args = [a for a in sys.argv[1:] if not a.startswith("--")]
    dest = os.path.abspath(args[0]) if args else os.path.join(ROOT, "github_upload", "worst_case")
    if os.path.isdir(dest) and os.listdir(dest):
        if "--replace" not in sys.argv:
            sys.exit(f"{dest} is not empty; pass --replace to delete and rewrite it")
        shutil.rmtree(dest)
    rels = [f for f in FILES if os.path.isfile(os.path.join(ROOT, f))]
    for d in DIRS:
        for base, subdirs, names in os.walk(os.path.join(ROOT, d)):
            subdirs[:] = sorted(s for s in subdirs if s != "__pycache__")
            rels += [os.path.relpath(os.path.join(base, n), ROOT) for n in sorted(names)]
    rels = [r for r in rels if wanted(r)]
    sizes = {r: os.path.getsize(os.path.join(ROOT, r)) for r in rels}
    big = [r for r, s in sizes.items() if s >= FILE_LIMIT]
    if big:
        sys.exit("files of 50 MB or more: " + ", ".join(big))
    total = sum(sizes.values())
    if total >= TOTAL_LIMIT:
        sys.exit(f"total {total / 2 ** 20:.1f} MB reaches the {TOTAL_LIMIT // 2 ** 20} MB limit")
    for r in rels:
        os.makedirs(os.path.dirname(os.path.join(dest, r)) or dest, exist_ok=True)
        shutil.copy2(os.path.join(ROOT, r), os.path.join(dest, r))
    per = {}
    for r, s in sizes.items():
        top = r.split(os.sep)[0] if os.sep in r else "(top level)"
        n, b = per.get(top, (0, 0))
        per[top] = (n + 1, b + s)
    print(f"exported {len(rels)} files, {total / 2 ** 20:.2f} MB, to {dest}")
    for top, (n, b) in sorted(per.items(), key=lambda kv: -kv[1][1]):
        print(f"  {top:12s} {n:4d} files {b / 2 ** 20:7.2f} MB")
    r, s = max(sizes.items(), key=lambda kv: kv[1])
    print(f"largest file: {r} ({s / 2 ** 20:.2f} MB)")


if __name__ == "__main__":
    main()
