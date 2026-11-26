# Large-N campaign, 2026-10-29: N = 64 (4 h) and N = 91 (6 h)

Goal set by the reference author: look for sequences, families and prefixes that earlier searches may have
missed (to guide the search at larger N), and push the lower bounds of the top candidates up with longer runs.
Every value below is the cost of a stored, re-simulated input (`results/strong.json`); a sequence's true
worst case can only be higher. Lower = better sequence.

## New tools

* **Parallel tempering** (`fsa_pt`, mode `pt`, `BATCH_PT`; METHODS §2.3). At equal moves it beat the
  Luby-restarted annealer of the earlier races in all 10 benchmark runs at N = 64 (by 1–5 comparisons) and
  adds ≈ 2 comparisons per 2M-move screening run, so all N = 64 attacks use it. At N = 91 the Luby restarts
  were as good or better (1044–1047 against 1042–1045 at equal moves), so N = 91 rounds alternate the two.
* **Family screen** (`scripts/famscan.py`): complete sequences from family patterns, successive halving
  (1 run of 2M moves for everyone; the best ≈ 2,000 plus the best of each 5-gap prefix get 3 more; the best
  ≈ 300 plus the best of each 4-gap core get 8 runs of 4M). Score = worst input found over all runs.
* **Fixed-budget attack** (`scripts/maxout.py`): every candidate gets the same number of moves per thread
  (24 threads) and no target, so the resulting bounds are comparable across sequences; earlier races stopped
  at a target, which left many bounds artificially low.
* **Beam search over the exact DP** (`src/beam.h`; METHODS §2.4): exact at N = 46 with B = 10⁶, but at
  N = 64 it stalls at 627 for the README sequence, so it was not used for bounds.

## N = 64 (gaps ≤ 63)

### Exploration: 85,675 sequences in two screens

* Screen 1 (annealing engine, 66,370 sequences, 43 min): {1,4,9,c,x[,y]} for every c ∈ {10, 12…20};
  {1,4,7,9,x[,y]}; {1,4,10,c,x[,y]} for c ∈ 12…20; all 7-gap {1,4,9,11,x,y,z} with x ∈ 14…36; coarse grids
  (upper gaps every 3rd–4th value) over the cores {1,3,b,d}, {1,5,b,d} (b ∈ 7…13) and {1,4,b,d}
  (b ∈ {5,6,8,11,12,13}). The 6-gap {1,4,9,11,x,y} family had been scanned completely before (`scan64`).
* Screen 2 (parallel tempering, 19,305 sequences, 19 min): 8-gap {1,4,9,11,x,y,z,w} with x ∈ {21,23,30,32,34},
  y,z,w ≥ 36, and 7-gap {1,4,9,11,x,y,z} with x ≥ 37.

**Families.** The {1,4,9,11} core is far ahead. Best screen score per 4-gap core (worst input over 12 runs,
annealing engine):

| core | best member | score |
|---|---|---:|
| {1,4,9,11} | {1,4,9,11,32,62} | 623 |
| {1,4,7,9} | {1,4,7,9,35,45} | 635 |
| {1,4,9,15} | {1,4,9,15,22} | 635 |
| {1,4,11,13} | {1,4,11,13,38} | 637 |
| {1,3,8,10} | {1,3,8,10,31,55} | 642 |
| {1,5,7,13} | {1,5,7,13,42,58} | 643 |
| {1,4,10,15} | {1,4,10,15,17,33} | 645 |
| {1,3,7,17} | {1,3,7,17,26} | 646 |

All other 277 cores were eliminated in the first two stages. Inside {1,4,9,11} the screen ranks the fifth
gap {·,32} 623, {·,21} 627, {·,30} and {·,34} 629, {·,23} 632, {·,36} 633; the longer runs below reverse
the first two (the {·,21} sequences end lower), a reminder that short screens only separate families
that differ by several comparisons. A 7th or 8th gap (extra pair passes at the top, e.g. {…,32,61,63} or
{…,32,61,62,63}) costs only N − h each and keeps up with the best 6-gap sequences.

### Long runs on the top candidates

Fixed budgets (parallel tempering, 24 threads): **Round A** 100M moves per thread for 66 sequences (the 36
stored N = 64 sequences + the 30 best new ones), **Round B** 300M for the 25 best, **Round C** 1G for the
six still at the lowest level.

* Most old bounds of 631–632 were artefacts of races that stopped at their target. Round A pushed, e.g.,
  {1,4,9,11,32,44} 631 → 644, {1,4,9,11,34,44} 631 → 642, {1,4,9,11,30,36} 632 → 641,
  {1,4,9,11,14,35} 632 → 646, {1,4,9,11,32,60} 631 → 634.
* Six sequences were still at 631 after rounds A and B: README {1,4,9,11,21,61}, {1,4,9,11,21,57},
  {1,4,9,11,21,59}, {1,4,9,11,21,60}, {1,4,9,11,21,62} and the new 7-gap {1,4,9,11,32,61,63} (average
  424.09 ± 0.01, against 433.8 for the README sequence). Round C pushed {1,4,9,11,32,61,63} to 632.
* **Best known at N = 64: 631** for {1,4,9,11,21,57}, {1,4,9,11,21,59}, {1,4,9,11,21,60} and
  {1,4,9,11,21,62}; none of them was pushed to 632 by 1.4G moves per thread (rounds A + B + C). The README
  sequence also gives 631 in all three rounds; its stored 632 comes from one earlier long run and was never
  re-found, so the five cannot be ranked: README ≥ 632 is a rare input, not a measured difference.
* 632 after the rounds: {1,4,9,11,21}, {1,4,9,11,21,58}, {1,4,9,11,32}, {1,4,9,11,32,61}, {1,4,9,11,32,62},
  {1,4,9,11,32,61,63}, {1,4,9,11,21,57,63}, {1,4,9,11,21,58,62}, {1,4,9,11,21,59,62}, {1,4,9,11,21,60,62},
  {1,4,9,11,21,60,63}. Everything else is ≥ 633.

**Prefixes for larger N (from N = 64).** {1,4,9,11} is the only competitive 4-gap core (≥ 12 ahead of
{1,4,7,9} and {1,4,9,15} in the screen). The best fifth gap is 21 (four of the five sequences at the lowest
level), with a single large pair-pass gap 57–62; {1,4,9,11,32,·} is one comparison behind and keeps a ≈ 10
lower average. {1,4,9,11,30,·}, the README's prefix for N ≥ 91, is ≥ 633 at N = 64.

## N = 91 (gaps ≤ 45, reference convention)

### Exploration: 44,573 sequences

Screen (parallel tempering, 21 min): every {1,4,9,c,x[,y]} for c ∈ 10…20 (the complete 6-gap families
{1,4,9,11,·,·} and {1,4,9,15,·,·} included); all 7-gap {1,4,9,11,x,y,z} and {1,4,9,15,x,y,z} with
x ≤ 40; {1,4,7,9} with up to 3 more gaps; {1,4,10,c,x[,y]} for c ∈ 12…20; grids (every 2nd–3rd value) over
the cores {1,3,b,d}, {1,5,b,d} (b ∈ 7…13) and {1,4,b,d} (b ∈ {5,6,8,11,12,13}).

**Families, and how they move from N = 64 to N = 91** (excess of each core's best screen score over the
best core at the same N; 12-run maxima; the N = 64 screen used the annealing engine, the N = 91 screen
parallel tempering, so only the excesses are comparable):

| core | N = 64 | N = 91 | best member at N = 91 |
|---|---:|---:|---|
| {1,4,9,11} | 0 | 0 | {1,4,9,11,32,44} |
| {1,4,9,15} | +12 | +9 | {1,4,9,15,25,38} |
| {1,4,11,13} | +14 | +12 | {1,4,11,13,40} (screen artefact, see below) |
| {1,4,7,9} | +12 | +13 | {1,4,7,9,19,35,43} |
| {1,4,10,15} | +22 | +18 | {1,4,10,15,17,33} |
| {1,3,8,10} | +19 | +19 | {1,3,8,10,29,35} |
| {1,3,7,17} | +23 | +22 | {1,3,7,17,36,45} |

The new {1,4,11,13} core looked third-best in both screens, but its members are easy to under-attack:
{1,4,11,13,40} scored 1045 in the screen and 1074 under the first fixed-budget attack, {1,4,11,13,34,40}
1073, {1,4,11,13,38} 1054. It is not competitive. Inside {1,4,9,11} the screen's best fifth gaps were
32, 30, 27, 21, 23, 25 and 31; the attacks below keep 30 and 32 at the top, and {1,4,9,11,23,43} (new)
is the best sequence outside the earlier candidate lists.

### Long runs on the top candidates

Rounds at N = 91 alternate the two engines (METHODS §2.3): **A** parallel tempering, 100M moves per thread,
50 sequences (the 27 known ones below 1060 + 30 new from the screen, deduplicated); **B** Luby restarts,
300M, the best 25; **C** hot parallel tempering (R = 16, T ∈ [0.3, 3.0]), 1G, the best 9; **D** Luby, 2G,
the best 6; **E** 4G of hot tempering and 4G of Luby for the best two; **F** hot tempering, 2G, the next six.

| sequence | A | B | C | D | E (PT / Luby) | F | stored best (≥) |
|---|---:|---:|---:|---:|---:|---:|---:|
| README {1,4,9,11,30,44} | 1042 | 1045 | 1045 | 1045 | 1045 / 1045 | – | **1045** |
| {1,4,9,11,32,44} | 1045 | 1047 | 1047 | 1047 | 1047 / 1047 | – | 1047 |
| {1,4,9,11,30,36} | 1045 | 1045 | 1049 | 1049 | – | 1049 | 1049 |
| {1,4,9,11,30,42} | 1048 | 1047 | 1049 | 1047 | – | 1049 | 1049 |
| {1,4,9,11,34,42} | 1047 | 1046 | 1049 | 1048 | – | 1049 | 1049 |
| {1,4,9,11,23,43} (new) | 1048 | 1047 | 1050 | 1050 | – | 1050 | 1050 |
| {1,4,9,11,21,38} | 1048 | 1045 | 1051 | – | – | 1051 | 1051 |
| {1,4,9,11,30,43} | 1051 | 1050 | 1052 | – | – | – | 1052 |
| {1,4,9,15,35,37} | 1048 | 1051 | 1052 | – | – | 1052 | 1052 |

* The README sequence stays the **sole leader at N = 91**: 1045 in every round from 300M moves on
  (11.4G moves per thread in total, both engines), while every rival was pushed to 1047 or more. The seven
  that the earlier races had left at 1046 now stand at 1047–1052 ({1,4,9,11,32,38}, not in the table, at 1052).
* The ranking stopped moving: rounds D–F (2–4G moves per thread) did not change any stored value, so the
  order above is stable at this level of effort. The closest rival is {1,4,9,11,32,44}, 2 behind.
* Values move by ± 2–4 between rounds of the same budget class, so a single round never ranks two
  sequences that are within a few comparisons; only the maxima over all rounds are used.

## What this suggests for larger N

1. **Keep the {1,4,9,11} core; carry {1,4,9,15} along.** At N = 64 and N = 91 no other 4-gap core comes
   within 9 comparisons of it. The only one closing in is {1,4,9,15} (screen excess +12 → +9; best attacked
   member {1,4,9,15,35,37} ≥ 1052 at N = 91, 7 above the README sequence), which is also the core of the
   README's N = 256 sequence. {1,4,7,9} (the N ≤ 42 core) stays ≈ 12 behind. All other cores tried (≈ 280:
   second gap 3 or 5, other third and fourth gaps) were eliminated by the screens or ended ≥ 17 behind, and
   {1,4,11,13} is a screen artefact (N = 91 section). These can be dropped.
2. **The fifth gap grows with N.** At N = 64 the leaders are {1,4,9,11,21,·} with one large pair-pass gap
   57–62 ({·,32,·} one comparison behind). At N = 91 they are {1,4,9,11,30,44} and {1,4,9,11,32,44}, then
   {·,30,36}, {·,30,42}, {·,34,42}, {·,23,43} and {·,21,38} within 4–6. So a search at N = 128 can start from
   {1,4,9,11,g₅,g₆,·} with g₅ ∈ [21, 34] and g₆ ∈ [36, 45], plus the {1,4,9,15,25…35,·,·} family. The README's
   own N = 128 sequence {1,4,9,11,30,36,59} is of this form.
3. **Extra gaps.** At N = 64 extra pair passes on top (gaps ≥ N/2, cost N − h each) keep up with the best
   6-gap sequences ({1,4,9,11,32,61,63} ≥ 632). Under the N ≥ 91 convention (gaps ≤ N/2) no 7-gap sequence
   came within 9 of the leader at N = 91 ({1,4,9,11,21,37,45} ≥ 1054, {1,4,9,11,30,36,39} ≥ 1061).
4. **How to rank at larger N.** Short screens only separate families that differ by several comparisons, and
   they are fooled by sequences whose worst inputs are hard to find ({1,4,11,13,40}: 1045 in the screen,
   1074 under attack). The reliable order came from successive halving (≥ 12 short runs per finalist)
   followed by fixed-budget attacks of ≥ 100M moves per thread on ≈ 50 candidates and growing budgets on
   the leaders, with both engines (parallel tempering was clearly better at N = 64, Luby restarts at least
   as good at N = 91).
