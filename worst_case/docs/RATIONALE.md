# Why these sequences are optimal: structure of worst-case-optimal gap sequences

The certified results (N = 6 … 45) show three regimes. The explanations below are backed by exact
computations (per-pass anatomy of the certified worst-case inputs, `results/anatomy.md`; exact core
comparisons, `results/core_crossover.txt`).

## 1. What a gap has to buy

For a pass with gap h, an element at chain index t with a smaller elements before it in its chain costs
t − a + [a > 0] comparisons. A pass therefore costs at least N − h, plus one comparison per inversion
that is still present inside its chains. After passes with gaps H, the array is d-sorted for every d
in the numerical semigroup ⟨H⟩ (sums of earlier gaps), so pass h can only meet inversions at chain
distances d·h that are not in ⟨H⟩. A good worst-case sequence balances

* the *fixed* price of each pass (N − h, i.e. small gaps are expensive to run), against
* how many distances its gap removes from the set of "still invertible" distances of later passes
  (and how badly an adversary can exploit those that remain).

Exact anatomy of the optimal sequences shows the adversary keeps every pass busy: e.g. at N = 32 the
worst input for {1,4,7,9} costs 42 + 44 + 57 + 86 comparisons in the 9-, 7-, 4- and 1-passes.

## 2. Regime N ≤ 15: two or three passes, top gap near N

With few elements, every extra pass costs a large fraction of the total. Optimal sequences use one or two
large gaps close to N (pair passes, 2h ≥ N, cost exactly N − h) plus a medium gap, e.g. {1,6,9} at N = 10,
{1,7,11,13} at N = 14. Ties are numerous (up to 23 at N = 14) because many gap sets leave the final
insertion pass the same adversarial freedom.

## 3. Regime 16 ≤ N ≤ 37: the {1,4,7,9} core

{1,4,7,9} (possibly plus pair passes) is optimal at 19 of these 22 sizes; the exceptions are N = 17, 18
({1,4,6,9}, {1,4,6,9,17}) and N = 20 ({1,5,7,11}). At N = 22 it ties with {1,3,5}, {1,3,7,8}, {1,4,5,7}
and {1,3,7,8,21}.

* ⟨4,7,9⟩ leaves only six distances {1,2,3,5,6,10} that can still be inverted before the final pass;
  the final pass then costs about 2.5–3 comparisons per element in the worst case. (Denser cores such
  as ⟨4,5,6⟩ leave fewer distances but their own passes cost more than they save.)
* 7 and 9 are coprime to 4 and to each other, so the 9-, 7- and 4-passes do not duplicate work.
* A **pair pass** (one gap h ≥ N/2) is added exactly when it pays: it costs N − h comparisons but
  forbids the most damaging long-range inversions (x[i] > x[i+h]). Exact DP shows when the trade is
  positive: at N = 25, 27, 30, 33, 34, 35, 37 (gaps 24, 26, 29, 32, 33, 33, 35), and two of them
  ({19, 34}) at N = 36.

## 4. Transition 38 ≤ N ≤ 42, then the {1,4,9,11} core (43 ≤ N ≤ 45)

Between N = 38 and 42 both cores appear among the optima: {1,4,9,11,21,35} and {1,4,7,9,19} tie at 38,
{1,4,9,11,21,38} wins at 39, {1,4,7,9,28} at 40, and at 41 and 42 sequences of both families tie
({1,4,7,9,28,40} / {1,4,9,11,21}, {1,4,9,11} / {1,4,7,9,28,41}). From N = 43 on only {1,4,9,11,21,…}
is optimal. Exact worst cases of the bare cores (no extra gaps):

| N | {1,4,7,9} | {1,4,9,11} |
|---|---|---|
| 30 | 209 | 214 |
| 35 | 262 | 265 |
| 36 | 277 | 276 |
| 38 | 301 | 300 |
| 40 | 326 | 323 |
| 42 | 349 | 345 |
| 45 | 393 | 382 |

The {4,7,9} core leaves fewer unsorted distances for the final pass, but its 9- and 7-passes become
too expensive as N grows (chains get long and the 9-pass is decoupled, i.e. it pays its full worst case
Σ m(m−1)/2). {4,9,11} shifts work from the long, expensive top pass to cheaper passes and wins from
N ≈ 40 on, by 11 comparisons at N = 45. The added gap 21 lies inside ⟨4,9⟩ (21 = 3·4 + 9), so it does not
change what the final pass may see. It pays for itself in the 4-pass: with 21 present, distances such as
21, 32 = 11 + 21 and 42 are already sorted before that pass, which cuts the 4-pass's potentially inverted
chain pairs at N = 45 from 191 to 178 (distances below N still unsorted: 30 → 24), for a fixed price of
only N − 21 comparisons plus the 21-pass's own worst case.

## 5. Why holistic tuning matters

Changing one gap changes which distances every later pass must handle, so the cost landscape over
gap sequences is extremely rugged. Exact values at N = 36 around the optimum {1,4,7,9,19,34} = 275:

| change | sequence | exact W |
|---|---|---|
| (optimum) | {1,4,7,9,19,34} | 275 |
| 34 → 33 | {1,4,7,9,19,33} | 276 |
| 34 → 35 | {1,4,7,9,19,35} | 277 |
| drop 34 | {1,4,7,9,19} | 277 |
| drop 19 | {1,4,7,9,34} | 278 |
| 19 → 18 | {1,4,7,9,18,34} | **285** |
| 19 → 20 | {1,4,7,9,20,34} | **288** |

Moving the gap 19 by ±1 costs 10–13 comparisons although removing it entirely costs only 3: the value
of a gap depends on its exact arithmetic relation to the other gaps, not on its size. A search that
perturbs one gap at a time sees a landscape of isolated spikes and steps; the method here therefore never
does that. It enumerates complete sequences family by family, prunes whole families at once with
certified witnesses, and (at large N) re-optimises the top gaps jointly.

## 6. Average versus worst case

The README also lists the sequence with the lowest *average* for every N, with its worst case. All 40
of those worst cases for N ≤ 45 are reproduced exactly here (`scripts/avgopt.py`), and for N = 46 the
README author supplied the average-optimal sequence {1,4,13,41}, whose exact worst case is 494. So the
trade-off between the two goals can be read off exact values. The two optima coincide only at N = 6 and
10, and the trade-off is asymmetric:

* **The average-optimal sequence pays 18–27% in the worst case for 26 ≤ N ≤ 44**, against at most 9% for
  N ≤ 17. For 19 ≤ N ≤ 44 its shape is {1,4,13} or {1,4,14} plus at most one pair pass. Without a 7 or 9,
  every distance outside ⟨4,13⟩ (up to 35) can still be inverted in the final pass, and the worst input
  concentrates there. At N = 43, {1,4,13,37} costs 451 (W* = 356), and 233 of them, 52%, fall in the final
  pass, against 40% (143 of 356) for the worst-case optimum. At N = 45 the average optimum switches to
  {1,4,9,33}, and its worst case drops to 433, only 14% above W*. At N = 46 it is {1,4,13,41} again
  (average ≈ 254.76, sampled), and the worst case returns to 494, 25% above W* = 394.
* **For N ≥ 64 the price stays as high.** Only lower bounds exist there, but with equal annealing effort on both
  sides (`docs/CAMPAIGN_AVG.md`) the average-optimal sequences stay 15% (N = 64) to 33% (N = 256) above the best
  worst-case sequences, and 28% at N = 181, 362 and 512.
* **The worst-case-optimal sequence pays only about 10% on average (6–13%) for N ≥ 16.** The cores
  {1,4,7,9} and {1,4,9,11} cut the long, rare chains of inversions that dominate the worst case. That
  costs some extra passes in the typical case, but not many.

Optimising the worst case is therefore the cheaper insurance: it gives up about a tenth of the average,
while the average optimum gives up a fifth to a quarter of the worst case.
