# Targeted Luby-restart attacks on the N=46 sequences the driver left as possible ties (worst case >= W* = 394,
# exact DP too large).  An input reaching 395 proves the sequence is not optimal; it is stored in
# results/strong.json (challenge.py re-simulates it) and recorded as an exclusion via manual_cert.py.
#   powershell -File scripts\ties46.ps1 [-Iters 1000000] [-Reps 255] [-Seed 4646]
param([int]$Iters = 1000000, [int]$Reps = 255, [int]$Seed = 4646)
$open = (python -c "import json; s=json.load(open('results/summary.json'))['46']; print(' '.join(','.join(map(str, r['seq'])) for r in s['unresolved']))").Split(' ')
foreach ($s in $open) {
    if (-not $s) { continue }
    python -u scripts\challenge.py 46 $s 395 --iters $Iters --reps $Reps --luby --seed $Seed
    $w = python -c "import json; print(json.load(open('results/strong.json'))['46:$s']['W_lower'])"
    if ([int]$w -ge 395) { python scripts\manual_cert.py kill 46 $s }
}
python scripts\manual_cert.py merge
