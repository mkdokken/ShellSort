# Worst-case-optimal Shellsort gap sequences: independent derivation and certification

Goal: find gap sequences minimising the **worst-case** number of comparisons of Shellsort, after first replicating the worst-case table of the README (github.com/mkdokken/ShellSort) independently.

**Which README.** Comparisons in this report are with the README's worst-case table as it stood when this work began: revision [e75ab4a](https://github.com/mkdokken/ShellSort/blob/e75ab4a058fcaaa775a16ff71a2510981993245e/README.md) (last edited 2026-10-23), transcribed in `scripts/readme_data.py`. The README has since been updated with the corrections, the N = 46 row and the large-N bounds found here. So where this report says a README value was too low or a tie was missing, the current README already has the corrected entry. "The original README" always means revision e75ab4a.

## Summary

* **Replication (N = 6 … 45).** An exhaustive, parallel branch-and-bound over *all* gap sets reproduces the original README's optimum value at 40 of 40 sizes, with the same reported sequence or a tie of it. For 37 sizes the complete tie list is identical as well.
* **Certified.** Every optimum (and every tie) is proved by exact dynamic programming over threshold sets; the original README's values for N ≤ 45 were then only believed exact (hill-climbing lower bounds). 40 of 40 sizes are fully certified (no unresolved candidate).
* **Beyond the original README (N = 46).** W*(46) = 394 (proved), attained by {1,4,9,11,21}, {1,4,9,11,21,45}, {1,4,9,11,21,44}, {1,4,9,11,21,44,45}; best by average: {1,4,9,11,21} (277.86, sampled). {1,4,9,11,21,44} and {1,4,9,11,21,44,45} were certified with front pair rules (METHODS §1.3), a DP on the state space of the sequence without its leading pair passes; targeted annealing alone had stalled at 394 for them. 1 candidate too large for the exact DP here ({1,4,7,9,26,41,45}) was excluded by inputs costing more than 394, found by targeted annealing and re-simulated. 3 further sequences ({1,4,7,9,26}, {1,4,7,9,26,41}, {1,4,7,9,26,45}) have worst case ≥ 394; targeted Luby-restart annealing (6,120 restarts each) never pushed them to 395, but they could not be checked for equality (the exact DP needs more than this machine's 31 GB); all have higher averages, so the choice is unaffected. N = 45 is therefore the largest size with a complete, certified tie list.
* **Corrections to the original README (since made in the README).** The search returns all optimal sequences: it adds ties the original tie lists missed and removes one that is not optimal ({1,3,7,12} at N = 14 has worst case 67, input `[3,8,0,6,11,1,5,10,13,4,9,12,2,7]`).
* **Better tie-break at N = 22.** {1,3,5} has the same optimal worst case (131) as the original README's {1,4,7,9} but a lower exact average (94.507 vs 97.469); the README now lists {1,3,5}.
* **Large N: the original README's worst cases are too low.** Exact evaluation is impossible, so values are lower bounds attained by stored, re-simulated inputs. For the original README's *own* sequences this work finds inputs costing more than their published worst case at N = 64, 91, 128, 181, 256, 362 (largest: N = 362, ≥ 6576 vs 6186, +6.3%). The README now lists the bounds found here.
* **Large N: comparisons under equal budgets.** The independent beam search (seeded only from this work's own optima) rediscovers the original README's sequences at N = 91 and N = 128. Candidates from this work and from the README author's earlier searches were then compared in equal-budget races (see `docs/LARGE_N.md`):
  * N = 64: lowest lower bound among the raced sequences is 631; raced above it and never pushed there: {1,4,9,11,21,57}, {1,4,9,11,21,59}, {1,4,9,11,21,60}, {1,4,9,11,21,62} (long annealing runs (≤ 3 × 200M moves/thread) to 632; long annealing runs (≤ 6 × 200M moves/thread) to 632). Pushed higher: {1,4,9,11,21} ≥ 632, {1,4,9,11,21,61} ≥ 632, {1,4,9,11,32} ≥ 632, {1,4,9,11,32,61} ≥ 632, {1,4,9,11,32,62} ≥ 632, {1,4,9,11,21,56} ≥ 633, {1,4,9,11,21,63} ≥ 633, {1,4,9,11,30,51} ≥ 633, {1,4,9,11,30,63} ≥ 633, {1,4,9,11,32,58} ≥ 633, {1,4,9,11,32,63} ≥ 633, {1,4,9,11,21,49} ≥ 634 and 20 more. Original README sequence {1,4,9,11,21,61} ≥ 632. **Calibration failed:** {1,4,9,11,21,61} (known ≥ 632) was not pushed there either when attacked afresh with the same budgets, so these survivals do not rank the sequences. Should their worst cases be equal, the lowest average among them is {1,4,9,11,21,60} (433.73 vs 433.84 for the original README sequence).
  * N = 91: lowest lower bound among the raced sequences is 1045; raced above it and never pushed there: {1,4,9,11,30,44} (Luby restarts (≤ 127 per thread, lengths 1,1,2,1,1,2,4,… × 2M moves) to 1046; Luby restarts (≤ 255 per thread, lengths 1,1,2,1,1,2,4,… × 2M moves) to 1046). Pushed higher: {1,4,9,11,32,44} ≥ 1047, {1,4,9,11,30,36} ≥ 1049, {1,4,9,11,30,42} ≥ 1049, {1,4,9,11,34,42} ≥ 1049, {1,4,9,11,21,38} ≥ 1051, {1,4,9,11,32,38} ≥ 1052, {1,4,9,15,35,37} ≥ 1052. Calibrated: the same budget reached the target for {1,4,9,11,21,38} (360 restarts, 16 s), {1,4,9,11,30,36} (2255 restarts, 134 s), {1,4,9,11,30,42} (379 restarts, 18 s), {1,4,9,11,32,38} (24 restarts, 1 s), {1,4,9,11,32,44} (1488 restarts, 87 s), {1,4,9,11,34,42} (72 restarts, 3 s), {1,4,9,15,35,37} (277 restarts, 10 s).
  * N = 128: lowest lower bound among the raced sequences is 1661; raced above it and never pushed there: {1,4,9,11,30,36,59} (Luby restarts (≤ 127 per thread, lengths 1,1,2,1,1,2,4,… × 2M moves) to 1662). Pushed higher: {1,4,9,11,30,47,59} ≥ 1667, {1,4,9,11,34,43,62} ≥ 1671. Calibrated: the same budget reached the target for {1,4,9,11,30,47,59} (547 restarts, 36 s), {1,4,9,11,34,43,62} (719 restarts, 57 s).
  * N = 181: lowest lower bound among the raced sequences is 2646; at 2646 but not raced higher: {1,4,9,11,30,47,59,86}. Pushed higher: {1,4,9,11,30,36,59,87} ≥ 2652, {1,4,9,11,30,47,65,75} ≥ 2654, {1,4,9,11,30,47,84} ≥ 2654, {1,4,9,15,31,44,81} ≥ 2655, {1,4,9,11,30,36,59,86} ≥ 2656, {1,4,9,11,30,47,59} ≥ 2670.
  * N = 256: lowest lower bound among the raced sequences is 4180; at 4180 but not raced higher: {1,4,9,11,30,36,70,109}. Pushed higher: {1,4,9,11,30,36,65,103,118} ≥ 4182, {1,4,9,11,30,36,70,121} ≥ 4188, {1,4,9,15,35,49,101,123} ≥ 4193, {1,4,9,15,34,56,80} ≥ 4200. Original README sequence {1,4,9,15,35,49,101,123} ≥ 4193.
  * N = 362: lowest lower bound among the raced sequences is 6517; at 6517 but not raced higher: {1,4,9,11,30,36,70,113,175}. Pushed higher: {1,4,9,11,30,47,59,127,150} ≥ 6560, {1,4,9,11,30,47,70,131,179} ≥ 6560, {1,4,9,11,30,36,70,107,163} ≥ 6576. Original README sequence {1,4,9,11,30,36,70,107,163} ≥ 6576.
* **Large N: family screens and fixed-budget attacks (N = 64 … 512; `docs/CAMPAIGN_64_91.md`, `docs/CAMPAIGN_128_181.md`, `docs/CAMPAIGN_256.md`, `docs/CAMPAIGN_362.md`, `docs/CAMPAIGN_512.md`).** Family screens over complete sequences (counts below) found no 4-gap core that competes with {1,4,9,11}; only {1,4,9,15} comes close (18 behind at N = 128, 7 at N = 181, 23 at N = 256, 31 at N = 362 and 113 at N = 512 in the first fixed-budget round). Every candidate then got the same number of moves with no target, which pushed many earlier race-stopped bounds up, by as much as 30 at N = 181 and 110 at N = 362:
  * N = 64: 85,675 sequences screened; fixed-budget rounds of 100M, 300M, 1G moves per thread (66, 25, 6 sequences). Lowest stored bound 631: {1,4,9,11,21,57}, {1,4,9,11,21,59}, {1,4,9,11,21,60}, {1,4,9,11,21,62} (1.4G moves per thread in total); original README sequence {1,4,9,11,21,61} ≥ 632 (631/631/631 in the equal-budget rounds). Pushed higher: {1,4,9,11,32,61,63} ≥ 632.
  * N = 91: 44,573 sequences screened; fixed-budget rounds of 100M, 300M, 1G, 2G, 2G, 4G, 4G moves per thread (50, 25, 9, 6, 6, 2, 2 sequences). Lowest stored bound 1045: {1,4,9,11,30,44} (11.4G moves per thread in total). Pushed higher: {1,4,9,11,32,44} ≥ 1047, {1,4,9,11,30,36} ≥ 1049, {1,4,9,11,30,42} ≥ 1049, {1,4,9,11,34,42} ≥ 1049, {1,4,9,11,23,43} ≥ 1050, {1,4,9,11,21,38} ≥ 1051, {1,4,9,11,30,43} ≥ 1052, {1,4,9,15,35,37} ≥ 1052.
  * N = 128: 79,651 sequences screened; fixed-budget rounds of 100M, 100M, 300M, 1G, 2G, 4G moves per thread (58, 65, 25, 10, 6, 4 sequences). Lowest stored bound 1661: {1,4,9,11,30,36,59} (7.4G moves per thread in total). Pushed higher: {1,4,9,11,30,47,59} ≥ 1667, {1,4,9,11,21,48,56} ≥ 1670, {1,4,9,11,34,43,62} ≥ 1671, {1,4,9,11,21,45,60} ≥ 1673, {1,4,9,11,30,42,47} ≥ 1673, {1,4,9,11,30,42,58} ≥ 1673, {1,4,9,11,21,49,57} ≥ 1674, {1,4,9,11,30,47,56} ≥ 1676, {1,4,9,11,30,58} ≥ 1679, {1,4,9,11,21,37,61} ≥ 1681.
  * N = 181: 81,728 sequences screened; fixed-budget rounds of 100M, 100M, 300M, 1G, 2G, 4G, 4G, 4G moves per thread (59, 33, 25, 11, 7, 3, 2, 1 sequences). Lowest stored bound 2646: {1,4,9,11,30,47,59,86} (15.4G moves per thread in total). Pushed higher: {1,4,9,11,30,36,59} ≥ 2652, {1,4,9,11,30,36,59,87} ≥ 2652, {1,4,9,11,30,47,65,75} ≥ 2654, {1,4,9,11,30,47,84} ≥ 2654, {1,4,9,15,31,44,81} ≥ 2655, {1,4,9,11,30,36,59,86} ≥ 2656, {1,4,9,15,32,43,89} ≥ 2656, {1,4,9,11,25,37,58,81} ≥ 2657, {1,4,9,15,31,44,82} ≥ 2659, {1,4,9,11,30,47,59,79} ≥ 2664.
  * N = 256: 80,228 sequences screened; fixed-budget rounds of 100M, 300M, 1G, 2G, 4G moves per thread (57, 31, 19, 7, 3 sequences). Lowest stored bound 4171: {1,4,9,11,30,47,67,95} (7.4G moves per thread in total); original README sequence {1,4,9,15,35,49,101,123} ≥ 4193 (4143/4167/4174/4193 in the equal-budget rounds). Pushed higher: {1,4,9,11,30,36,59,122} ≥ 4174, {1,4,9,11,30,36,62,89} ≥ 4177, {1,4,9,11,30,36,70,109} ≥ 4180, {1,4,9,11,30,36,70,113} ≥ 4181, {1,4,9,15,31,56,74,126} ≥ 4181, {1,4,9,11,30,36,70,123} ≥ 4182, {1,4,9,11,30,36,59,109} ≥ 4186, {1,4,9,11,32,36,62,119} ≥ 4187, {1,4,9,11,30,36,70,121} ≥ 4188, {1,4,9,11,30,36,59,106,127} ≥ 4191, {1,4,9,11,30,36,70,117} ≥ 4191, {1,4,9,11,30,47,75,87} ≥ 4191, {1,4,9,11,30,36,59,104} ≥ 4193, {1,4,9,11,30,47,67,106} ≥ 4193, {1,4,9,11,30,47,59,86} ≥ 4194, {1,4,9,11,30,47,67,96} ≥ 4204, {1,4,9,15,35,37,83,117} ≥ 4287.
  * N = 362: 40,509 sequences screened; fixed-budget rounds of 128M, 128M, 128M, 512M, 1G, 2G, 2G, 4G moves per thread (34, 27, 3, 13, 7, 3, 1, 2 sequences). Lowest stored bound 6521: {1,4,9,11,30,36,70,109,171} (7.6G moves per thread in total); original README sequence {1,4,9,11,30,36,70,107,163} ≥ 6576 (6508/6540/6556/6576 in the equal-budget rounds). Pushed higher: {1,4,9,11,30,36,70,117,143} ≥ 6532, {1,4,9,11,30,36,62,121,161} ≥ 6537, {1,4,9,15,31,56,74,126,177} ≥ 6544, {1,4,9,11,30,47,67,95,160} ≥ 6597, {1,4,9,11,30,47,67,95,152} ≥ 6652.
  * N = 512: 15,990 sequences screened; fixed-budget rounds of 128M, 128M, 128M, 128M, 512M, 1G, 1G, 2G, 2G, 4G moves per thread (32, 15, 8, 8, 12, 7, 1, 3, 1, 1 sequences). Lowest stored bound 10119: {1,4,9,11,30,36,59,98,162,213} (8.6G moves per thread in total) (no original README sequence at this size). Pushed higher: {1,4,9,11,30,36,70,109,195,248} ≥ 10150, {1,4,9,11,30,36,70,109,191,207} ≥ 10186, {1,4,9,11,30,36,59,98,162,217} ≥ 10189, {1,4,9,11,30,36,70,117,143,254} ≥ 10196, {1,4,9,11,30,36,59,98,162,253} ≥ 10205, {1,4,9,15,31,56,74,126,177,241} ≥ 10214.
* **Large N: lower-bound races must be calibrated.** Which restart length finds a sequence's worst inputs depends on the sequence, so single-method races mislead. Long runs left {1,4,9,11,21,60} and {1,4,9,11,21,62} at 630 while 4M-move restarts reached 631 in seconds; an earlier lone survival of the original README sequence at N = 91 and its 10-comparison lead at N = 256 also disappeared under Luby restarts. Every verdict above therefore comes from a race that also re-attacks a contender already known to reach the target. At N = 64 even that check fails for 632, so N = 64 stays open (METHODS §2.2).
* **README's average-optimal sequences (§3).** The worst cases the original README lists for its average-optimal sequences are confirmed exactly at 40 of 40 sizes N ≤ 45. At N = 46 (not in the original README table) the README author's average-optimal {1,4,13,41} has exact worst case 494, against W* = 394. For N ≥ 64 (lower bounds only) the searches here reproduce the listed value exactly at N = 64 (723), N = 91 (1244); find worse inputs than listed at N = 128 (2014 → ≥ 2017), N = 181 (3372 → ≥ 3390), N = 256 (5462 → ≥ 5532), N = 362 (8006 → ≥ 8335). The README now lists the higher values. At N = 512 (no worst case in the original README) the README author's average-optimal {1,4,10,23,57,189,483} has worst case ≥ 12973, about 28% above the lowest bound found here for any sequence (≥ 10119). Optimising the average costs up to 27% more comparisons in the worst case (N = 43, exact).

Details: `docs/METHODS.md` (algorithms, proofs, validation), `docs/RATIONALE.md` (why the optima look the way they do).

## 1. Exact optima for N = 6 … 46

W* = min over gap sets of max over all N! inputs of the number of comparisons (convention: every evaluation of `a[j-h] > t` in gapped insertion sort; gaps ≥ N are no-ops). † = some tie candidate could not be certified (see notes).

| N | W* | optimal sequence (lowest average among ties) | average (≈ sampled) | ties | vs original README |
|---:|---:|---|---:|---:|---|
| 6 | 14 | {1,4} | 10.633 | 3 | identical |
| 7 | 18 | {1,4,6} | 14.390 | 1 | identical |
| 8 | 23 | {1,5,7} | 17.765 | 1 | identical |
| 9 | 29 | {1,5,8} | 21.445 | 5 | identical |
| 10 | 35 | {1,6,9} | 25.512 | 3 | identical |
| 11 | 41 | {1,6,9,10} | 30.681 | 2 | identical |
| 12 | 48 | {1,7,10,11} | 35.431 | 2 | identical |
| 13 | 56 | {1,6,10} | 39.547 | 12 | identical |
| 14 | 64 | {1,7,11,13} | 44.898 | 23 | same optimum, tie list differs |
| 15 | 71 | {1,4,9,14} | 50.582 | 9 | same optimum, tie list differs |
| 16 | 78 | {1,4,7,9} | 60.612 | 1 | identical |
| 17 | 86 | {1,4,6,9} | 66.573 | 1 | identical |
| 18 | 94 | {1,4,6,9,17} | 72.924 | 1 | identical |
| 19 | 103 | {1,4,7,9} | 78.219 | 1 | identical |
| 20 | 112 | {1,5,7,11} | 83.511 | 1 | identical |
| 21 | 121 | {1,4,7,9} | 90.861 | 2 | identical |
| 22 | 131 | {1,3,5} | 94.507 | 5 | same W; lower-average tie than the original README's {1,4,7,9} (97.469) |
| 23 | 139 | {1,4,7,9} | 103.934 | 2 | identical |
| 24 | 148 | {1,4,7,9} | 110.626 | 1 | identical |
| 25 | 158 | {1,4,7,9,24} | 117.657 | 1 | identical |
| 26 | 167 | {1,4,7,9} | 124.178 | 1 | identical |
| 27 | 177 | {1,4,7,9,26} | ≈ 131.24 | 1 | identical |
| 28 | 187 | {1,4,7,9} | 137.882 | 2 | identical |
| 29 | 198 | {1,4,7,9} | 144.997 | 2 | identical |
| 30 | 207 | {1,4,7,9,29} | ≈ 152.35 | 1 | identical |
| 31 | 220 | {1,4,7,9} | 159.429 | 1 | identical |
| 32 | 229 | {1,4,7,9} | 166.705 | 1 | identical |
| 33 | 240 | {1,4,7,9,32} | ≈ 174.10 | 2 | identical |
| 34 | 251 | {1,4,7,9,33} | ≈ 181.58 | 1 | identical |
| 35 | 262 | {1,4,7,9,33} | ≈ 189.11 | 2 | identical |
| 36 | 275 | {1,4,7,9,19,34} | ≈ 203.06 | 1 | identical |
| 37 | 286 | {1,4,7,9,35} | ≈ 204.36 | 2 | identical |
| 38 | 298 | {1,4,9,11,21,35} | ≈ 215.42 | 2 | identical |
| 39 | 310 | {1,4,9,11,21,38} | ≈ 221.76 | 1 | identical |
| 40 | 321 | {1,4,7,9,28} | ≈ 229.17 | 1 | identical |
| 41 | 334 | {1,4,7,9,28,40} | ≈ 237.53 | 3 | identical |
| 42 | 345 | {1,4,9,11} | 237.684 | 3 | identical |
| 43 | 356 | {1,4,9,11,21,41} | ≈ 254.53 | 2 | identical |
| 44 | 368 | {1,4,9,11,21} | ≈ 261.65 | 2 | identical |
| 45 | 380 | {1,4,9,11,21} | ≈ 269.68 | 2 | identical |
| 46 | 394 † | {1,4,9,11,21} | ≈ 277.86 | 4 | – |

### Corrections to the original README's tie lists (since made in the README; all values exact; every non-optimal entry has a re-simulated witness input)

* **N = 14** (W* = 64) — additional optimal sequence(s): {1,3,7,8}, {1,3,7,11}, {1,4,9,11}, {1,4,5,7,12}; listed in the original README but **not optimal**: {1,3,7,12}
* **N = 15** (W* = 71) — additional optimal sequence(s): {1,5,6,8,14}
* **N = 22** (W* = 131) — additional optimal sequence(s): {1,3,5}

## 2. Large N (certified lower bounds)

Exact DP needs 10¹³–10¹⁷ states per sequence at N = 64 (1.5 TB to ≈ 14 PB of memory, against 11 GB and 2.3 h for all of N = 46; METHODS §1.7), so here every value is attained by a concrete input (re-simulated); a sequence's true worst case can only be higher. The search is a prefix-transfer beam (complete candidate sequences = this work's best sequences from the previous size, truncated by ≤ 2 gaps, plus up to 3 jointly chosen new gaps; gaps ≤ N/2 for N ≥ 91 as in the README; a second, diversified chain starts from the whole {1,4,9,·,·,·}, {1,4,7,9,·,·} and {1,4,10,·,·,·} families at N = 91), followed by strong evaluation of the finalists and of the original README sequence.

**Winner's curse.** Picking the sequence with the lowest *lower bound* favours sequences whose worst inputs are merely hard to find. This happened here: at N = 64 the beam's apparent winner {1,4,9,11,32,62} (≥ 627 after one strong run) was shown by the README author's hill-climber to reach 631, which a targeted search here reproduced in 16 s; the next apparent winner {1,4,9,11,32,63} survived an 80-restart targeted attack at 630 but was also reported at 631 by the README author. Large-N verdicts below therefore only use *adversarial* comparisons: a candidate counts as better only if a targeted search cannot push it up to the best-known value of the original README sequence. The comparisons must also be *calibrated*: the same budget has to re-find a value that some contender is known to reach, and restarts of every length must be tried (Luby schedule), because the best restart length differs between sequences (METHODS §2.2).

| N | original README sequence | its README value | input found here for it (≥) | best other sequence found here | its value (≥) | verdict |
|---:|---|---:|---:|---|---:|---|
| 64 | {1,4,9,11,21,61} | 630 | **632** | {1,4,9,11,21,57} | 631 | other sequence lower (lower bounds only, not a proof) |
| 91 | {1,4,9,11,30,44} | 1043 | **1045** | {1,4,9,11,32,44} | 1047 | original README sequence remains best known |
| 128 | {1,4,9,11,30,36,59} | 1645 | **1661** | {1,4,9,11,30,47,59} | 1667 | original README sequence remains best known |
| 181 | {1,4,9,11,30,47,59,86} | 2601 | **2646** | {1,4,9,11,30,36,59} | 2652 | original README sequence remains best known |
| 256 | {1,4,9,15,35,49,101,123} | 4039 | **4193** | {1,4,9,11,30,47,67,95} | 4171 | other sequence lower (lower bounds only, not a proof) |
| 362 | {1,4,9,11,30,36,70,107,163} | 6186 | **6576** | {1,4,9,11,30,36,70,109,171} | 6521 | other sequence lower (lower bounds only, not a proof) |
| 512 | – (the original README table ends at N = 362) | – | – | {1,4,9,11,30,36,59,98,162,213} | 10119 | no original README sequence at this size |

### Equal-budget races

Each contender is attacked with the same maximum budget (targeted annealing restarts, all stored inputs re-simulated); a contender that reaches the target is at least that bad, a survivor has not been pushed there.

* **N = 64, target 631**, long annealing runs (≤ 2 × 100M moves/thread): reached by 29 — {1,4,9,11,32,62}, {1,4,9,11,32,49}, {1,4,9,11,21,61}, {1,4,9,11,32,51}, {1,4,9,11,32,59}, {1,4,9,11,32}, {1,4,9,11,32,63}, {1,4,9,11,14,35}, {1,4,9,11,21,57}, {1,4,9,11,30,62}, {1,4,9,11,30,63}, {1,4,9,11,32,44}, {1,4,9,11,32,55}, {1,4,9,11,34,44}, {1,4,9,11,30}, {1,4,9,11,30,49}, {1,4,9,11,30,51}, {1,4,9,11,21,39}, {1,4,9,11,21,49}, {1,4,9,11,21,54}, {1,4,9,11,21,56}, {1,4,9,11,32,58}, {1,4,9,11,36,39}, {1,4,9,11,21,48}, {1,4,9,11,21,63}, {1,4,9,11,30,36}, {1,4,9,11,30,40}, {1,4,9,11,32,42}, {1,4,9,11,21}; **survived (1)** — {1,4,9,11,21,62} (≥ 630)
* **N = 64, target 631**, short restarts (≤ 400 × 4M moves/thread): reached by 7 — {1,4,9,11,21,60}, {1,4,9,11,21,62}, {1,4,9,11,21,57}, {1,4,9,11,21,59}, {1,4,9,11,32,58}, {1,4,9,11,32,60}, {1,4,9,11,21,61}; **survived (0)** — none
* **N = 64, target 632**, long annealing runs (≤ 3 × 200M moves/thread): reached by 8 — {1,4,9,11,21,61}, {1,4,9,11,32,63}, {1,4,9,11,32,61}, {1,4,9,11,32,57}, {1,4,9,11,30,58}, {1,4,9,11,32}, {1,4,9,11,21}, {1,4,9,11,32,51}; **survived (7)** — {1,4,9,11,21,62} (≥ 630), {1,4,9,11,21,60} (≥ 630), {1,4,9,11,21,59} (≥ 631), {1,4,9,11,21,57} (≥ 631), {1,4,9,11,32,58} (≥ 631), {1,4,9,11,32,62} (≥ 631), {1,4,9,11,32,60} (≥ 631)
* **N = 64, target 632**, long annealing runs (≤ 6 × 200M moves/thread): reached by 0 — ; **survived (8)** — {1,4,9,11,21,57} (≥ 631), {1,4,9,11,21,59} (≥ 631), {1,4,9,11,21,60} (≥ 630), {1,4,9,11,21,62} (≥ 630), {1,4,9,11,32,58} (≥ 631), {1,4,9,11,32,60} (≥ 631), {1,4,9,11,32,62} (≥ 631), {1,4,9,11,21,61} (≥ 632) *known ≥ target: missed by this budget*
* **N = 64, target 632**, Luby restarts (≤ 127 per thread, lengths 1,1,2,1,1,2,4,… × 2M moves): reached by 0 — ; **survived (1)** — {1,4,9,11,21,61} (≥ 632) *known ≥ target: missed by this budget*
* **N = 64, target 632**, seeded restarts (≤ 40 × 2M moves/thread): reached by 0 — ; **survived (1)** — {1,4,9,11,21,61} (≥ 632) *known ≥ target: missed by this budget*
* **N = 91, target 1045**, long annealing runs (≤ 3 × 200M moves/thread): reached by 3 — {1,4,9,11,32,38}, {1,4,9,11,32,44}, {1,4,9,11,34,42}; **survived (1)** — {1,4,9,11,30,44} (≥ 1044)
* **N = 91, target 1045**, Luby restarts (≤ 127 per thread, lengths 1,1,2,1,1,2,4,… × 2M moves): reached by 3 — {1,4,9,11,30,42}, {1,4,9,11,32,44}, {1,4,9,11,30,44}; **survived (0)** — none
* **N = 91, target 1046**, Luby restarts (≤ 127 per thread, lengths 1,1,2,1,1,2,4,… × 2M moves): reached by 7 — {1,4,9,15,35,37}, {1,4,9,11,30,42}, {1,4,9,11,32,44}, {1,4,9,11,21,38}, {1,4,9,11,30,36}, {1,4,9,11,32,38}, {1,4,9,11,34,42}; **survived (1)** — {1,4,9,11,30,44} (≥ 1045)
* **N = 91, target 1046**, Luby restarts (≤ 255 per thread, lengths 1,1,2,1,1,2,4,… × 2M moves): reached by 0 — ; **survived (1)** — {1,4,9,11,30,44} (≥ 1045)
* **N = 128, target 1661**, Luby restarts (≤ 127 per thread, lengths 1,1,2,1,1,2,4,… × 2M moves): reached by 1 — {1,4,9,11,30,47,59}; **survived (1)** — {1,4,9,11,30,36,59} (≥ 1655)
* **N = 128, target 1661**, Luby restarts (≤ 255 per thread, lengths 1,1,2,1,1,2,4,… × 2M moves): reached by 1 — {1,4,9,11,30,36,59}; **survived (0)** — none
* **N = 128, target 1662**, Luby restarts (≤ 127 per thread, lengths 1,1,2,1,1,2,4,… × 2M moves): reached by 2 — {1,4,9,11,34,43,62}, {1,4,9,11,30,47,59}; **survived (1)** — {1,4,9,11,30,36,59} (≥ 1661)
* **N = 181, target 2630**, long annealing runs (≤ 3 × 200M moves/thread): reached by 3 — {1,4,9,11,30,36,59,87}, {1,4,9,11,30,36,59,86}, {1,4,9,15,31,44,81}; **survived (0)** — none
* **N = 181, target 2631**, long annealing runs (≤ 3 × 200M moves/thread): reached by 4 — {1,4,9,11,30,47,59,86}, {1,4,9,11,30,36,59,87}, {1,4,9,11,30,36,59,86}, {1,4,9,15,31,44,81}; **survived (0)** — none
* **N = 181, target 2632**, Luby restarts (≤ 63 per thread, lengths 1,1,2,1,1,2,4,… × 4M moves): reached by 4 — {1,4,9,11,30,47,59,86}, {1,4,9,11,30,36,59,86}, {1,4,9,11,30,36,59,87}, {1,4,9,15,31,44,81}; **survived (0)** — none
* **N = 181, target 2635**, Luby restarts (≤ 63 per thread, lengths 1,1,2,1,1,2,4,… × 4M moves): reached by 5 — {1,4,9,11,30,47,65,75}, {1,4,9,11,30,47,59,86}, {1,4,9,11,30,36,59,86}, {1,4,9,11,30,36,59,87}, {1,4,9,15,31,44,81}; **survived (0)** — none
* **N = 181, target 2637**, Luby restarts (≤ 63 per thread, lengths 1,1,2,1,1,2,4,… × 4M moves): reached by 5 — {1,4,9,11,30,47,59}, {1,4,9,11,30,47,59,86}, {1,4,9,11,30,36,59,86}, {1,4,9,11,30,36,59,87}, {1,4,9,15,31,44,81}; **survived (0)** — none
* **N = 181, target 2640**, Luby restarts (≤ 63 per thread, lengths 1,1,2,1,1,2,4,… × 4M moves): reached by 6 — {1,4,9,11,30,47,84}, {1,4,9,11,30,36,59,86}, {1,4,9,11,30,36,59,87}, {1,4,9,15,31,44,81}, {1,4,9,11,30,47,59}, {1,4,9,11,30,47,65,75}; **survived (1)** — {1,4,9,11,30,47,59,86} (≥ 2637)
* **N = 256, target 4152**, Luby restarts (≤ 63 per thread, lengths 1,1,2,1,1,2,4,… × 4M moves): reached by 2 — {1,4,9,11,30,36,65,103,118}, {1,4,9,15,35,49,101,123}; **survived (0)** — none
* **N = 256, target 4160**, Luby restarts (≤ 63 per thread, lengths 1,1,2,1,1,2,4,… × 4M moves): reached by 3 — {1,4,9,15,34,56,80}, {1,4,9,15,35,49,101,123}, {1,4,9,11,30,36,65,103,118}; **survived (0)** — none
* **N = 256, target 4165**, Luby restarts (≤ 63 per thread, lengths 1,1,2,1,1,2,4,… × 4M moves): reached by 4 — {1,4,9,11,30,36,70,121}, {1,4,9,11,30,36,65,103,118}, {1,4,9,15,34,56,80}, {1,4,9,11,30,36,70,109}; **survived (1)** — {1,4,9,15,35,49,101,123} (≥ 4163)
* **N = 362, target 6425**, long annealing runs (≤ 3 × 200M moves/thread): reached by 3 — {1,4,9,11,30,36,70,107,163}, {1,4,9,11,30,47,59,127,150}, {1,4,9,11,30,47,70,131,179}; **survived (0)** — none
* **N = 362, target 6429**, Luby restarts (≤ 63 per thread, lengths 1,1,2,1,1,2,4,… × 4M moves): reached by 3 — {1,4,9,11,30,36,70,107,163}, {1,4,9,11,30,47,70,131,179}, {1,4,9,11,30,47,59,127,150}; **survived (0)** — none
* **N = 362, target 6440**, Luby restarts (≤ 63 per thread, lengths 1,1,2,1,1,2,4,… × 4M moves): reached by 4 — {1,4,9,11,30,36,70,113,175}, {1,4,9,11,30,36,70,107,163}, {1,4,9,11,30,47,70,131,179}, {1,4,9,11,30,47,59,127,150}; **survived (0)** — none
* **N = 362, target 6450**, Luby restarts (≤ 63 per thread, lengths 1,1,2,1,1,2,4,… × 4M moves): reached by 4 — {1,4,9,11,30,36,70,113,175}, {1,4,9,11,30,36,70,107,163}, {1,4,9,11,30,47,70,131,179}, {1,4,9,11,30,47,59,127,150}; **survived (0)** — none

Gaps above N/2 are excluded for N ≥ 91 (README convention). One side check at N = 91: adding a single top gap h ∈ [79, 90] to {1,4,9,11,30,44} looked better in screening, but the best such sequence reached the base value 1044 under a targeted attack, so no improvement is distinguishable; candidates of this kind differ by too little for lower-bound comparisons to rank reliably.

## 3. Worst case of the README's average-optimal sequences

The original README also lists, for each N, the sequence with the lowest *average* and that sequence's worst case, found by hill-climbing (explicitly lower bounds for N ≥ 91). These worst cases are recomputed here: exactly by the same DP wherever it fits in 12 GB (every N ≤ 46), otherwise by annealing, which gives lower bounds. For N ≥ 64 each sequence received as many annealing moves per thread as the best worst-case sequence of its size, in runs of the same kinds (`docs/CAMPAIGN_AVG.md`). N = 46 is not in the original README table; its average-optimal sequence {1,4,13,41} was supplied by the README author, and its average was sampled here. At N = 512 the original README names {1,4,10,23,57,189,484} as the best average, without a worst case; its author has since found with more samples that {1,4,10,23,57,189,483} is slightly better, so only that sequence is attacked (the README now lists it). Every value is attained by a stored, re-simulated input (`scripts/avgopt.py`, `results/avgopt.json`). The last column is the worst-case price of optimising the average: the excess over the optimal worst case W* (for N ≥ 64 over the best worst-case sequence found here, i.e. the lowest bound after long attack, which rests on the same effort; both are lower bounds there, hence ≈).

The worst cases the original README lists for its average-optimal sequences are confirmed exactly at 40 of 40 sizes N ≤ 45. At N = 46 (not in the original README table) the README author's average-optimal {1,4,13,41} has exact worst case 494, against W* = 394. For N ≥ 64 (lower bounds only) the searches here reproduce the listed value exactly at N = 64 (723), N = 91 (1244); find worse inputs than listed at N = 128 (2014 → ≥ 2017), N = 181 (3372 → ≥ 3390), N = 256 (5462 → ≥ 5532), N = 362 (8006 → ≥ 8335). The README now lists the higher values. At N = 512 (no worst case in the original README) the README author's average-optimal {1,4,10,23,57,189,483} has worst case ≥ 12973, about 28% above the lowest bound found here for any sequence (≥ 10119). Optimising the average costs up to 27% more comparisons in the worst case (N = 43, exact).

N = 45 and 46 needed the 16-bit layered DP (METHODS §1.4). For N ≥ 64 the attacks were matched to the best worst-case sequence of each size, from 3.8G moves per thread at N = 64 to 17.6G at N = 181. At N = 64 and 91 no run ever exceeded the README value: parallel tempering, Luby restarts and fixed restarts reach 723 and 1244 but never more, after 3.8G and 15.0G moves per thread in all. That makes both plausible true worst cases, though not proved. At N = 128 and 181 the long runs added 1 and 3 (2017, 3390; three of the four runs at N = 181 stop at exactly 3390). At N = 256, 362 and 512 the value still grows with effort (N = 362: 8260, 8312 and 8335 after 0.55G, 2G and 4G moves per thread; N = 512: 12782 after 128M, 12960 after 1G, 12973 after 4G), so the true worst cases there are probably higher still. At equal effort the average-optimal sequences stay 15–33% above the best worst-case sequences.

| N | README best-average sequence | README average | original README worst case | worst case here | status vs original README | vs W* |
|---:|---|---:|---:|---:|---|---:|
| 6 | {1,4} | 10.633 | 14 | 14 | exact, matches | +0.0% |
| 7 | {1,5} | 13.824 | 19 | 19 | exact, matches | +5.6% |
| 8 | {1,5} | 17.399 | 24 | 24 | exact, matches | +4.3% |
| 9 | {1,6} | 21.288 | 30 | 30 | exact, matches | +3.4% |
| 10 | {1,6,9} | 25.512 | 35 | 35 | exact, matches | +0.0% |
| 11 | {1,6,10} | 29.868 | 43 | 43 | exact, matches | +4.9% |
| 12 | {1,5} | 34.217 | 52 | 52 | exact, matches | +8.3% |
| 13 | {1,5,12} | 38.907 | 57 | 57 | exact, matches | +1.8% |
| 14 | {1,5,13} | 43.73 | 66 | 66 | exact, matches | +3.1% |
| 15 | {1,5,13} | 48.881 | 76 | 76 | exact, matches | +7.0% |
| 16 | {1,5,14} | 54.174 | 85 | 85 | exact, matches | +9.0% |
| 17 | {1,6,15} | 59.763 | 94 | 94 | exact, matches | +9.3% |
| 18 | {1,5,16} | 65.131 | 107 | 107 | exact, matches | +13.8% |
| 19 | {1,4,13} | 70.629 | 112 | 112 | exact, matches | +8.7% |
| 20 | {1,4,13} | 76.439 | 127 | 127 | exact, matches | +13.4% |
| 21 | {1,4,14} | 82.117 | 139 | 139 | exact, matches | +14.9% |
| 22 | {1,4,14,21} | 88.115 | 150 | 150 | exact, matches | +14.5% |
| 23 | {1,4,14} | 94.082 | 162 | 162 | exact, matches | +16.5% |
| 24 | {1,4,13,23} | 100.179 | 168 | 168 | exact, matches | +13.5% |
| 25 | {1,4,13,23} | 106.469 | 179 | 179 | exact, matches | +13.3% |
| 26 | {1,4,14,25} | 112.837 | 201 | 201 | exact, matches | +20.4% |
| 27 | {1,4,14,25} | 119.322 | 212 | 212 | exact, matches | +19.8% |
| 28 | {1,4,13} | 125.705 | 221 | 221 | exact, matches | +18.2% |
| 29 | {1,4,13} | 132.327 | 238 | 238 | exact, matches | +20.2% |
| 30 | {1,4,13,29} | 139.062 | 252 | 252 | exact, matches | +21.7% |
| 31 | {1,4,13} | 145.635 | 264 | 264 | exact, matches | +20.0% |
| 32 | {1,4,13} | 152.055 | 272 | 272 | exact, matches | +18.8% |
| 33 | {1,4,13,32} | 158.81 | 290 | 290 | exact, matches | +20.8% |
| 34 | {1,4,13,32} | 165.71 | 307 | 307 | exact, matches | +22.3% |
| 35 | {1,4,13,33} | 172.83 | 322 | 322 | exact, matches | +22.9% |
| 36 | {1,4,13,33} | 180.23 | 334 | 334 | exact, matches | +21.5% |
| 37 | {1,4,13,35} | 187.315 | 345 | 345 | exact, matches | +20.6% |
| 38 | {1,4,13,36} | 194.36 | 373 | 373 | exact, matches | +25.2% |
| 39 | {1,4,13,36} | 201.71 | 387 | 387 | exact, matches | +24.8% |
| 40 | {1,4,13,36} | 209.3 | 401 | 401 | exact, matches | +24.9% |
| 41 | {1,4,13,36} | 216.64 | 411 | 411 | exact, matches | +23.1% |
| 42 | {1,4,13,36} | 224.11 | 425 | 425 | exact, matches | +23.2% |
| 43 | {1,4,13,37} | 231.89 | 451 | 451 | exact, matches | +26.7% |
| 44 | {1,4,13,41} | 239.63 | 463 | 463 | exact, matches | +25.8% |
| 45 | {1,4,9,33} | 247.27 | 433 | 433 | exact, matches | +13.9% |
| 46 | {1,4,13,41} | ≈ 254.76 (here) | – (not listed) | 494 | exact, no value listed | +25.4% |
| 64 | {1,4,9,38,62} | 399.11 | 723 | ≥ 723 | value reached | ≈ +14.6% |
| 91 | {1,4,9,33,86} | 639.33 | ≥ 1244 | ≥ 1244 | value reached | ≈ +19.0% |
| 128 | {1,4,9,24,85} | 1002.25 | ≥ 2014 | ≥ 2017 | ≥ 2014 improved by +3 | ≈ +21.4% |
| 181 | {1,4,10,21,70,176} | 1564.88 | ≥ 3372 | ≥ 3390 | ≥ 3372 improved by +18 | ≈ +28.1% |
| 256 | {1,4,10,27,89,238} | 2428.52 | ≥ 5462 | ≥ 5532 | ≥ 5462 improved by +70 | ≈ +32.6% |
| 362 | {1,4,10,23,67,236,355} | 3742.34 | ≥ 8006 | ≥ 8335 | ≥ 8006 improved by +329 | ≈ +27.8% |
| 512 | {1,4,10,23,57,189,483} | – | – (not listed) | ≥ 12973 | lower bound, no value listed | ≈ +28.2% |

## 4. Why these sequences (summary of `docs/RATIONALE.md`)

* A pass with gap h costs at least N − h, plus one comparison per inversion its chains still contain; before pass h the array is already d-sorted for every d in the numerical semigroup generated by the earlier (larger) gaps. Each gap is worth what it removes from later passes minus its own fixed price.
* **16 ≤ N ≤ 37: core {1,4,7,9}** (optimal at 19 of 22 sizes; exceptions N = 17, 18: {1,4,6,9}…, N = 20: {1,5,7,11}). After the 9-, 7- and 4-passes only six distances {1,2,3,5,6,10} can still be inverted, so the final insertion pass costs ≈ 3 comparisons per element. A single pair pass (gap h ≥ N/2, cost exactly N − h) is added exactly when it saves more than it costs (N = 25, 27, 30, 33–35, 37; two of them at N = 36).
* **38 ≤ N ≤ 42: transition** (both cores appear among the optima); **43 ≤ N ≤ 45: core {1,4,9,11} plus 21.** The decoupled top pass of {4,7,9} (cost Σ m(m−1)/2 over 9-chains) grows too fast; the bare {4,9,11} core overtakes it from N ≈ 40 (exact: 382 vs 393 at N = 45). 21 = 3·4 + 9 lies inside ⟨4,9⟩, so it does not help the final pass; it pays in the 4-pass, where distances 21, 32 = 11 + 21 and 42 become pre-sorted (potentially inverted chain pairs 191 → 178 at N = 45).
* **Gaps interact sharply.** At N = 36, moving the gap 19 of the optimum {1,4,7,9,19,34} by ±1 costs +10 / +13 comparisons, while deleting it costs +2. Single-gap local search sees a landscape of isolated spikes; the method therefore only ever evaluates complete sequences and prunes whole families.

## 5. Method (summary of `docs/METHODS.md`)

1. **Exact worst case of a sequence:** threshold-set DP (a longest path in a lattice of sets), with the first pass decoupled exactly, pair passes absorbed as compare-exchange constraints, free positions back-filled (exchange argument), parallel level sweeps, and a two-level/one-byte layered variant that certifies 2.5·10¹⁰-state instances in ≈ 5 GB.
2. **Lower bounds at any size:** simulated annealing with *incremental* evaluation (swapping values x < y only changes the threshold sets between them) plus an exact DP that optimally re-orders windows of consecutive values; every value is attained by a re-simulated input.
3. **Continuous pruning:** branch-and-bound over gap prefixes. A permutation that every larger gap leaves untouched (it is *local*) lower-bounds the whole family of extensions, so one witness prunes a whole subtree; witnesses are pooled and reused across the tree; the threshold only falls when a value is proved exactly; staged evaluation (cheap → expensive) with a certification loop.
4. **Holistic candidates:** only complete sequences are ever evaluated, generated family by family (exact range) or by joint re-selection of the top gaps (large N); no step perturbs one gap in isolation.
5. **Parallel:** subtrees, candidate batches and DP levels are independent units (OpenMP, 24 threads).

## 6. Validation

* DP = brute force over all N! inputs on 115 random sequences (N ≤ 12, 479M inputs each at N = 12) and N = 13 checks.
* Three independent DP formulations (plain, pair-absorbing, layered one-byte) agree on hundreds of random cases.
* Incremental annealing totals = full re-simulation on 2.9M random moves (N = 12 … 362); kills inside the search are re-simulated before acceptance.
* Exact averages = full enumeration (N ≤ 10) and match the README's sampled averages to its precision.

## 7. Anatomy of the certified worst cases

Comparisons per pass on the certified worst-case input of each optimum (count@gap), and the distances that are still unsorted before the final insertion pass.

| N | optimal sequence | W | comparisons per pass on the worst input (count@gap) | distances still unsorted before the final pass |
|---|---|---|---|---|
| 6 | {1,4} | 14 | 2@4 + 12@1 | 4: {1,2,3,5} |
| 7 | {1,4,6} | 18 | 1@6 + 3@4 + 14@1 | 4: {1,2,3,5} |
| 8 | {1,5,7} | 23 | 1@7 + 3@5 + 19@1 | 5: {1,2,3,4,6} |
| 9 | {1,5,8} | 29 | 1@8 + 4@5 + 24@1 | 6: {1,2,3,4,6,7} |
| 10 | {1,6,9} | 35 | 1@9 + 4@6 + 30@1 | 7: {1,2,3,4,5,7,8} |
| 11 | {1,6,9,10} | 41 | 1@10 + 2@9 + 5@6 + 33@1 | 7: {1,2,3,4,5,7,8} |
| 12 | {1,7,10,11} | 48 | 1@11 + 2@10 + 5@7 + 40@1 | 8: {1,2,3,4,5,6,8,9} |
| 13 | {1,6,10} | 56 | 3@10 + 8@6 + 45@1 | 9: {1,2,3,4,5,7,8,9,11} |
| 14 | {1,7,11,13} | 64 | 1@13 + 3@11 + 7@7 + 53@1 | 10: {1,2,3,4,5,6,8,9,10,12} |
| 15 | {1,4,9,14} | 71 | 1@14 + 6@9 + 19@4 + 45@1 | 8: {1,2,3,5,6,7,10,11} |
| 16 | {1,4,7,9} | 78 | 7@9 + 10@7 + 18@4 + 43@1 | 6: {1,2,3,5,6,10} |
| 17 | {1,4,6,9} | 86 | 8@9 + 14@6 + 19@4 + 45@1 | 6: {1,2,3,5,7,11} |
| 18 | {1,4,6,9,17} | 94 | 1@17 + 9@9 + 16@6 + 20@4 + 48@1 | 6: {1,2,3,5,7,11} |
| 19 | {1,4,7,9} | 103 | 11@9 + 15@7 + 27@4 + 50@1 | 6: {1,2,3,5,6,10} |
| 20 | {1,5,7,11} | 112 | 9@11 + 18@7 + 22@5 + 63@1 | 8: {1,2,3,4,6,8,9,13} |
| 21 | {1,4,7,9} | 121 | 15@9 + 18@7 + 31@4 + 57@1 | 6: {1,2,3,5,6,10} |
| 22 | {1,3,5} | 131 | 38@5 + 46@3 + 47@1 | 4: {1,2,4,7} |
| 23 | {1,4,7,9} | 139 | 19@9 + 23@7 + 36@4 + 61@1 | 6: {1,2,3,5,6,10} |
| 24 | {1,4,7,9} | 148 | 21@9 + 26@7 + 39@4 + 62@1 | 6: {1,2,3,5,6,10} |
| 25 | {1,4,7,9,24} | 158 | 1@24 + 23@9 + 28@7 + 40@4 + 66@1 | 6: {1,2,3,5,6,10} |
| 26 | {1,4,7,9} | 167 | 25@9 + 30@7 + 41@4 + 71@1 | 6: {1,2,3,5,6,10} |
| 27 | {1,4,7,9,26} | 177 | 1@26 + 27@9 + 31@7 + 46@4 + 72@1 | 6: {1,2,3,5,6,10} |
| 28 | {1,4,7,9} | 187 | 30@9 + 34@7 + 45@4 + 78@1 | 6: {1,2,3,5,6,10} |
| 29 | {1,4,7,9} | 198 | 33@9 + 36@7 + 49@4 + 80@1 | 6: {1,2,3,5,6,10} |
| 30 | {1,4,7,9,29} | 207 | 1@29 + 36@9 + 39@7 + 50@4 + 81@1 | 6: {1,2,3,5,6,10} |
| 31 | {1,4,7,9} | 220 | 39@9 + 44@7 + 56@4 + 81@1 | 6: {1,2,3,5,6,10} |
| 32 | {1,4,7,9} | 229 | 42@9 + 44@7 + 57@4 + 86@1 | 6: {1,2,3,5,6,10} |
| 33 | {1,4,7,9,32} | 240 | 1@32 + 45@9 + 51@7 + 61@4 + 82@1 | 6: {1,2,3,5,6,10} |
| 34 | {1,4,7,9,33} | 251 | 1@33 + 48@9 + 50@7 + 59@4 + 93@1 | 6: {1,2,3,5,6,10} |
| 35 | {1,4,7,9,33} | 262 | 2@33 + 51@9 + 57@7 + 62@4 + 90@1 | 6: {1,2,3,5,6,10} |
| 36 | {1,4,7,9,19,34} | 275 | 2@34 + 17@19 + 49@9 + 54@7 + 64@4 + 89@1 | 6: {1,2,3,5,6,10} |
| 37 | {1,4,7,9,35} | 286 | 2@35 + 58@9 + 67@7 + 64@4 + 95@1 | 6: {1,2,3,5,6,10} |
| 38 | {1,4,9,11,21,35} | 298 | 3@35 + 17@21 + 43@11 + 44@9 + 68@4 + 123@1 | 8: {1,2,3,5,6,7,10,14} |
| 39 | {1,4,9,11,21,38} | 310 | 1@38 + 18@21 + 45@11 + 52@9 + 76@4 + 118@1 | 8: {1,2,3,5,6,7,10,14} |
| 40 | {1,4,7,9,28} | 321 | 12@28 + 66@9 + 64@7 + 67@4 + 112@1 | 6: {1,2,3,5,6,10} |
| 41 | {1,4,7,9,28,40} | 334 | 1@40 + 13@28 + 68@9 + 68@7 + 70@4 + 114@1 | 6: {1,2,3,5,6,10} |
| 42 | {1,4,9,11} | 345 | 60@11 + 54@9 + 90@4 + 141@1 | 8: {1,2,3,5,6,7,10,14} |
| 43 | {1,4,9,11,21,41} | 356 | 2@41 + 23@21 + 54@11 + 57@9 + 77@4 + 143@1 | 8: {1,2,3,5,6,7,10,14} |
| 44 | {1,4,9,11,21} | 368 | 25@21 + 57@11 + 58@9 + 79@4 + 149@1 | 8: {1,2,3,5,6,7,10,14} |
| 45 | {1,4,9,11,21} | 380 | 27@21 + 62@11 + 58@9 + 88@4 + 145@1 | 8: {1,2,3,5,6,7,10,14} |
| 46 | {1,4,9,11,21} | 394 | 29@21 + 66@11 + 63@9 + 83@4 + 153@1 | 8: {1,2,3,5,6,7,10,14} |

## 8. Limitations

* N ≥ 64: values are lower bounds; the sequence ranking can change with more search effort (equal effort was used for the comparisons above).
* An exhaustive branch-and-bound at N = 64 (all gaps) was attempted and abandoned: the locality-based family bound is weak for prefixes made of small gaps, so the tree is too wide for the available time.

## 9. Reproduce

```
powershell -File build.ps1
python scripts/run_search.py 6 45        # exhaustive search + certification (hours)
python scripts/transfer.py 91 64 ...      # large-N beam (see script header)
python scripts/final_eval.py              # equal-effort comparison
python scripts/analyze.py; python scripts/make_report.py; python scripts/make_html.py
```
