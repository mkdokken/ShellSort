# Large-N campaign, 2026-10-30: N = 128 (8 h) and N = 181 (12 h)

*A record of the campaign as run. "README" and "reference" here mean the README's worst-case table at revision
[e75ab4a](https://github.com/mkdokken/ShellSort/blob/e75ab4a058fcaaa775a16ff71a2510981993245e/README.md), the
version current when this work began; the README has since been updated with the results found here.*

Same goal as the first campaign (`docs/CAMPAIGN_64_91.md`): look for sequences, families and prefixes that
earlier searches missed, and push the lower bounds of the top candidates up with longer runs. Every value is
the cost of a stored, re-simulated input (`results/strong.json`); a sequence's true worst case can only be
higher. Lower = better sequence. Gaps ≤ N/2 (reference convention for N ≥ 91).

## Engines at these sizes

* **Throughput.** About 2.5M annealing moves per second per thread at N = 128 and 1.4M at N = 181 (24 threads),
  against 3.3M at N = 64.
* **Screening runs** (one thread per sequence, `batch`): plain annealing beats parallel tempering at equal
  moves at N = 128 (best of 4 runs for the README sequence: 1616 vs 1573 at 2M moves, 1645 vs 1611 at 8M),
  and a hotter start helps: mean over 24 known sequences 1622.5 / 1645.5 / 1659.4 / 1663.7 / 1663.2 for
  T₀ = 1.5 / 2 / 3 / 4.5 / 6. The N = 128 screens use annealing with T₀ = 4.5.
* **Long runs** (24 threads, 100M moves per thread, two seeds, two sequences): Luby restarts gave
  1652–1658 for the README sequence and 1666–1675 for {1,4,9,11,30,47,48}; parallel tempering (R = 16,
  T ∈ [0.3, 3.0] or [0.3, 4.5]) gave 1651–1652 and 1659–1664. So parallel tempering, clearly best at N = 64
  and level at N = 91, falls behind at N = 128, and all N = 128 rounds use Luby restarts, alternating the
  base length and start temperature (2M moves at T₀ = 3, or 4M at T₀ = 4.5).

## N = 128 (gaps ≤ 64)

### Exploration: 79,651 sequences in two screens

* **Screen** (79,092 sequences, 53 min; 1 run of 2M moves for all, 3 runs of 4M for the best 3,049, 8 runs
  of 8M for the best 304): every {1,4,9,11,·} and {1,4,9,15,·} with up to three more gaps (all 5- to 7-gap
  sequences of the two cores); {1,4,7,9}, {1,4,11,13}, {1,4,10,15} and {1,3,8,10} complete up to 6 gaps plus
  2,500 random 7-gap members each; 16 further cores (second gap 3, 4 or 5) with samples; 8-gap samples
  {1,4,9,11,x,·,·,·} (x = 18…40) and {1,4,9,15,x,·,·,·} (x = 22…40).
* **Neighbourhood refinement** (1,367 sequences, 6 min, runs of 4M/8M/16M): the 40 best finalists and every
  sequence one step away (an upper gap moved by 1–3, removed, or a gap inserted). It mainly re-ranks the
  finalists with longer runs: the first screen already contained every 7-gap neighbour, and of the 559 new
  sequences (mostly 8-gap insertions) only two reached its final 61, neither competitive under attack (≥ 1682).

**Families.** Best 12-run screen score per 4-gap core, as excess over the best core at the same N (the screens
used different engines at different N, so only the excesses compare), and the best value under the first
fixed-budget attack (100M Luby moves per thread):

| core | N = 64 | N = 91 | N = 128 | N = 128 attacked (100M) |
|---|---:|---:|---:|---|
| {1,4,9,11} | 0 | 0 | 0 | 1653 {1,4,9,11,30,36,59} |
| {1,4,9,15} | +12 | +9 | +12 | +18 {1,4,9,15,25,38,57} |
| {1,4,7,9} | +12 | +13 | +14 | +34 {1,4,7,9,25,28,58} |
| {1,4,11,13} | +14 | +12 | +15 | +32 {1,4,11,13,29,62} |
| {1,3,8,10} | +19 | +19 | +16 | +27 {1,3,8,10,27,43,58} |
| {1,4,10,15} | +22 | +18 | +43 | +25 {1,4,10,15,17,33,59} |

Short screens flatter most other cores: {1,4,9,15}, {1,4,7,9}, {1,4,11,13} and {1,3,8,10} are within 12–16
in the screen but 18–34 behind once attacked ({1,4,10,15}, with only two members attacked, is the exception).
{1,4,9,15} is again the closest, and it is the core most inflated by the screen (1,125 of the 3,049
stage-2 survivors were {1,4,9,15} sequences, and its best screen member {1,4,9,15,31,56}, 1657, reached 1710
under attack).

**Fifth gap.** Under attack (rounds A and A2, 100M moves per thread, 123 sequences) the best 5-gap prefixes
are {1,4,9,11,30} 1653, {1,4,9,11,21} +5, {1,4,9,11,34} +8, then {·,25} +16, {·,26} +18, {·,19} +19,
{·,32} +21 and {·,28} +25. In the screens, {·,30} moves up with N (excess +6 at N = 64, +3 at 91, 0 at 128),
{·,21} stays close (+4, +6, +3) and {·,32}, best at N = 64 and 91 in the screens, drops to +8. The best
sequences have 7 gaps; no 6- or 8-gap sequence was competitive under attack ({1,4,9,11,30,58} ≥ 1679,
{1,4,9,11,30,36,59,62} ≥ 1684).

### Long runs on the top candidates

All Luby restarts, 24 threads: **A** 100M moves per thread for 58 sequences (the 41 best screen and
refinement finalists + 17 known sequences from earlier searches with stored bounds ≤ 1690), **A2** 100M for
the next 65 finalists, **B** 300M for the best 25, **C** 1G for the best 10, **D** 2G for the best 6 (one
from A2), **E** 4G for the best 4.

| sequence | A | B | C | D | E | stored best (≥) |
|---|---:|---:|---:|---:|---:|---:|
| README {1,4,9,11,30,36,59} | 1653 | 1656 | 1656 | 1657 | 1658 | **1661** |
| {1,4,9,11,30,47,59} | 1661 | 1665 | 1666 | 1667 | 1667 | 1667 |
| {1,4,9,11,21,48,56} (new) | 1661 | 1667 | 1664 | 1670 | 1668 | 1670 |
| {1,4,9,11,34,43,62} | 1661 | 1666 | 1667 | 1668 | 1671 | 1671 |
| {1,4,9,11,30,42,58} (new) | 1667 (A2) | – | – | 1673 | – | 1673 |
| {1,4,9,11,21,45,60} (new) | 1666 | 1668 | 1673 | – | – | 1673 |
| {1,4,9,11,30,42,47} (new) | 1665 | 1672 | 1673 | – | – | 1673 |
| {1,4,9,11,21,49,57} (new) | 1658 | 1661 | 1668 | 1674 | – | 1674 |
| {1,4,9,11,30,47,56} (new) | 1664 | 1669 | 1676 | – | – | 1676 |
| {1,4,9,11,30,58} (new) | 1671 | 1672 | 1679 | – | – | 1679 |
| {1,4,9,11,21,37,61} (new) | 1670 | 1673 | 1681 | – | – | 1681 |

* The README sequence stays the **sole leader at N = 128**: 1651–1658 in all 13 of its equal-budget runs
  (8.2G moves per thread in total, including the engine benchmark), never reaching its stored 1661 (an input
  from the earlier 6,120-restart race). Every rival was pushed to ≥ 1667; in each round the best rival is
  ≥ 5 above the README sequence (≥ 8 from 1G moves on).
* The two rivals known before (both race-stopped at 1662) rose to 1667 and 1671. The best new sequences are
  {1,4,9,11,21,48,56} ≥ 1670 and {1,4,9,11,30,42,58}, {1,4,9,11,21,45,60}, {1,4,9,11,30,42,47} ≥ 1673.
  {1,4,9,11,21,49,57} looked closest after 300M moves (1661) but rose to 1668 and 1674.
* From round A to round E the rivals' values rose by 6–16 and the README sequence's by 5, so longer runs
  widened the gap (5 at 100M–300M, 8–10 from 1G on) instead of closing it.

## N = 181 (gaps ≤ 90)

### Exploration: 81,728 sequences in three screens

* **Engines.** Annealing from T₀ = 4.5 again gave the highest screening scores (mean over 24 known sequences
  2564.2 / 2584.1 / 2579.0 / 2583.1 for T₀ = 3 / 4.5 / 6 / 9). In the long-run check (100M moves per thread)
  Luby restarts beat parallel tempering by 8–32 (README sequence 2628 and 2633 against 2620;
  {1,4,9,11,30,58,81} 2657 and 2663 against 2631), and 4M-move restarts from T₀ = 4.5 beat 2M from T₀ = 3
  by 5–6, so the N = 181 rounds use Luby restarts with 4M (or 8M) base runs from T₀ = 4.5.
* **Screen** (80,111 sequences, 76 min; 2M / 4M / 8M moves; 4,015 and 301 survivors): {1,4,9,11} with every
  5- and 6-gap completion, every 7-gap one for the fifth gaps 19, 21, 23, 25, 26, 28, 30, 32, 34, 36 and 400
  random ones for each other fifth gap 17–48, 8-gap samples (500 per fifth gap 19–40, 3,000 for 21, 30 and 34);
  {1,4,9,15}: every 6-gap, 400 random 7-gap per fifth gap 20–44 and 200 random 8-gap per fifth gap 22–40;
  {1,4,7,9}: every 6-gap, 3,000 7-gap and 1,500 8-gap samples; {1,4,11,13} and {1,3,8,10}: samples. The
  earlier candidates were not seeded: most of the known 8-gap ones (the README's included) were not drawn.
* **Refinement 1** (1,832 sequences, 10 min) around the 40 best screen finalists, as at N = 128; here it
  matters more because 7- and 8-gap families were only sampled: 1,285 of its sequences were new, 25 of them
  among its final 60. **Refinement 2** (566 sequences, 7 min) around the 12 best sequences after round B
  (attack ranking, so it includes the earlier candidates): its nine best scores all belong to the starting
  sequences, and the best new neighbours (2645–2651 in that screen) gave 2657–2684 under attack (round A2).

**Families.** Only two cores reach the last screen stage close together: {1,4,9,11} 2606 and {1,4,9,15} 2610
(+4; +12 at N = 128); {1,4,11,13} (+61) and {1,4,7,9} (+63) fall far behind, and {1,3,8,10} drops out after
the first stage (7 of its 2,880 members survive it). As at N = 128 the screen flatters some sequences — its
best, {1,4,9,11,30,58,81} (2606), gave 2657–2667 under attack, and {1,4,9,11,30,47,59} (prefix of the README
sequence) 2670 — so the attacks below decide.

Under the first attack (rounds A and A2, 100M moves per thread, 92 sequences) the best member of {1,4,9,11}
gave 2623 ({1,4,9,11,30,47,65,75}) and of {1,4,9,15} 2630 (+7, {1,4,9,15,31,44,81}; +18 at N = 128). Best
5-gap prefixes: {1,4,9,11,30} 0, {1,4,9,15,31} +7, {1,4,9,11,25} +16, {1,4,9,15,25} +20, {1,4,9,15,32} +23,
{1,4,9,11,32} +27, {·,34} +29, {·,28} +30, {·,21} +33, {·,36} +38. The fifth gap 30 now dominates: {·,21},
5 behind at N = 128, falls 33 behind. Of the seven best {1,4,9,11} sequences after all rounds, six continue
{30,36,59,…} or {30,47,…}.

### Long runs on the top candidates

All Luby restarts, 24 threads, start temperature 4.5: **A** 100M moves per thread (4M-move base runs) for 59
sequences (the 30 best screen finalists, 17 from refinement 1 and the 12 known sequences with stored bounds
≤ 2660), **A2** 100M for 33 more (screen ranks 31–58, refinements 1 and 2), **B** 300M (4M base) for the best
25, **C** 1G (8M base) for the best 10, **D** 2G (4M base) for the best 6, **E** 4G (8M base) for the best 3,
**F** 4G (4M base) for the best 2, and in the remaining time **G** 4G (8M base, another seed) for the leader
and rounds C and D for {1,4,9,11,25,37,58,81}. Bounds stored before this campaign (2637–2641 for most
sequences below) came from races that stopped at their target or from short runs.

| sequence | A | B | C | D | E | F | G | stored best (≥) |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| README {1,4,9,11,30,47,59,86} | 2628 | 2642 | 2642 | 2646 | 2642 | 2643 | 2644 | **2646** |
| {1,4,9,11,30,36,59} (screen) | 2633 | 2640 | 2651 | 2652 | – | – | – | 2652 |
| {1,4,9,11,30,36,59,87} | 2636 | 2628 | 2641 | 2652 | – | – | – | 2652 |
| {1,4,9,11,30,47,65,75} | 2623 | 2651 | 2649 | 2646 | 2651 | 2654 | – | 2654 |
| {1,4,9,11,30,47,84} | 2638 | 2649 | 2654 | – | – | – | – | 2654 |
| {1,4,9,15,31,44,81} | 2630 | 2638 | 2645 | 2655 | – | – | – | 2655 |
| {1,4,9,11,30,36,59,86} | 2632 | 2636 | 2646 | 2642 | 2656 | – | – | 2656 |
| {1,4,9,15,32,43,89} | 2646 | 2650 | 2656 | – | – | – | – | 2656 |
| {1,4,9,11,25,37,58,81} (refinement) | 2639 | 2652 | 2655 | 2657 | – | – | – | 2657 |
| {1,4,9,15,31,44,82} | 2651 | 2649 | 2659 | – | – | – | – | 2659 |
| {1,4,9,11,30,47,59,79} (refinement) | 2643 | 2647 | 2664 | – | – | – | – | 2664 |
| {1,4,9,11,30,47,59} | 2670 | – | – | – | – | – | – | 2670 |

* The README sequence has the **lowest bound at N = 181, 2646**, 6 below the next sequences (2652), after
  15.4G moves per thread (rounds A–G) plus the engine check; its 10 runs gave 2620–2646, the three 4G runs
  2642–2644. Its earlier stored 2637 was low: round B reached 2642 and round D 2646.
* The lead changed hands along the way. After round B four sequences shared 2640 and the README sequence was
  at 2642; after round C {1,4,9,11,30,36,59,87} led with 2641; after round D the README sequence and
  {1,4,9,11,30,36,59,86} shared 2646, until round E pushed the latter to 2656. Single runs at 1–4G still
  raised leaders by 10–11, so this lead is less settled than the one at N = 128, where the gap grew steadily
  with the budget.
* The race-stopped bounds of the earlier candidates rose by 12–30: {1,4,9,11,30,47,65,75} 2640 → 2654,
  {1,4,9,11,30,36,59,87} 2640 → 2652, {1,4,9,15,31,44,81} 2640 → 2655, {1,4,9,11,30,36,59,86} 2640 → 2656,
  {1,4,9,11,30,47,84} 2641 → 2654, {1,4,9,11,30,47,59} 2640 → 2670.
* A new sequence from the screen shares the second rank: {1,4,9,11,30,36,59} (the README's N = 128 sequence,
  found by the screen, not seeded) ≥ 2652, with {1,4,9,11,30,36,59,87}. {1,4,9,11,25,37,58,81}, found by
  refinement 1, was at 2652 after round B and rose to 2657 in rounds C and D.

## What this suggests for larger N

1. **Two cores.** {1,4,9,11} leads at both sizes. The only other core within reach is {1,4,9,15}, and it
   closes in: in the first attack round (100M moves) its best member is 18 behind at N = 128 and 7 behind at
   N = 181 (9 after the long rounds: {1,4,9,15,31,44,81} ≥ 2655 against 2646). It is also the core of the
   README's N = 256 sequence, so a search at N = 256 needs both cores in full. {1,4,7,9}, {1,4,11,13}, {1,3,8,10} and {1,4,10,15} are 25–34 behind at
   N = 128; in the N = 181 screen the first two fall more than 60 behind and {1,3,8,10} does not get past the
   first stage. They can be dropped.
2. **Prefixes.** The fifth gap settles at 30 for {1,4,9,11} (best at N = 91, 128 and 181; {·,21} and {·,32},
   the N = 64 leaders, fall away) and at 31 or 35 for {1,4,9,15} ({1,4,9,15,31,44,81} at N = 181, the README's
   {1,4,9,15,35,49,…} at N = 256). The leaders continue {30,36,59,…} (the README's N = 128 sequence, and at
   N = 181 with a top gap of 86) or {30,47,…} ({30,47,59,86}, {30,47,65,75}); the README's N = 362 sequence
   starts {1,4,9,11,30,36,70}. A search at N = 256 can start from {1,4,9,11,30,36,·,·,·},
   {1,4,9,11,30,47,·,·,·} and {1,4,9,15,31…35,·,·,·}, with 8–9 gaps.
3. **Number of gaps.** 7 gaps at N = 128 (no 6- or 8-gap sequence competitive), 7 and 8 gaps side by side at
   N = 181 (the leader has 8; the two sequences at 2652 have 7 and 8).
4. **How to rank.** Short screens get less reliable as N grows: their best sequence at N = 181,
   {1,4,9,11,30,58,81}, gave 2657–2667 at 100M moves, where the eventual leaders gave 2623–2633. Use them to
   choose a broad field (90+ sequences, attacked at 100M moves) and grow the budget only on the leaders. Use
   Luby restarts with long base runs (4–8M moves, T₀ = 4.5) at N ≥ 128; parallel tempering is 8–32 worse at
   N = 181. Rank only after ≥ 2G moves per sequence, and expect changes even then: at N = 181 single rare
   inputs moved leaders by 10–11 at 1–4G ({1,4,9,11,30,36,59} 2640 → 2651, {1,4,9,11,30,36,59,87}
   2641 → 2652, {1,4,9,11,30,36,59,86} 2646 → 2656).
