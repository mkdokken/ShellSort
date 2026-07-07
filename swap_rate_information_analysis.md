# Shell Sort analyzed by swap rate and bits-of-information per comparison

An information-theoretic angle on choosing Shell-sort gaps. The guiding idea:
a comparison is most useful when its two outcomes are equally likely, i.e. when
its probability of causing a swap is close to 50%. Then each comparison carries
close to 1 bit of information. Too many gaps -> swap rate far below 50% (wasted
comparisons, like Pratt's 3-smooth gaps); too few gaps -> swap rate far above
50% (also wasteful, tending toward plain insertion sort).

All numbers below are derived analytically (order-statistic densities), not by
simulation. The continuous-k optimization and the entropy values are evaluated
numerically from those exact densities (see "Computational check").


## 1. The comparison / swap model

A gap-g pass runs insertion sort independently on each of the g chains (indices
i, i+g, i+2g, ...). Inserting a key, we repeatedly test "is the element to the
left greater than the key?":

- each test = one comparison;
- a "true" result = one shift (a swap); a "false" result stops the insertion
  (still counted as a comparison).

swap rate  r = (total swaps) / (total comparisons) = E[S] / E[C].

For the information view, the value of a single comparison whose swap
probability (conditioned on it being executed) is q is the binary entropy

    H(q) = -q*log2(q) - (1-q)*log2(1-q)      (H(1/2) = 1 bit).

average bits per comparison = E[I] / E[C], where
E[I] = sum over executed comparisons of H(q_of_that_comparison).


## 2. Warm-up: swap rate of the first pass

Input: a uniformly random permutation of N distinct keys. A gap-g first pass
sorts g chains, each a random permutation of its elements.

- Gap >= N/2: every chain has length 1 or 2. Each length-2 chain does exactly
  one comparison, which swaps with probability 1/2. So the swap rate is exactly
  50% for ANY gap >= N/2.

- Gap = N/3: every chain has length 3. Enumerating the 3! = 6 orderings of a
  chain gives total comparisons 16 and total swaps 9, so the swap rate is
  9/16 = 56.25%.

General trend: shorter first gaps (longer chains) raise the swap rate above 50%
toward the pure-insertion-sort limit; gap >= N/2 is the boundary where each
comparison is a genuine 50/50 (a full bit), because it only ever compares two
never-before-compared elements.


## 3. The CDF/PDF propagation method

Model element values as uniform(0,1) (only relative order matters). After a pass,
each position's element has a known value distribution, tracked by its CDF/PDF:

- max of independent variables:  CDF_max = product of the CDFs
  (a first-pass "winner" = max of 2 uniforms: CDF = t*t = t^2, PDF = 2t).
- min of independent variables:  survival_min = product of the survivals
  (a first-pass "loser" = min of 2 uniforms: survival = (1-t)^2,
   CDF = 1-(1-t)^2 = 2t-t^2, PDF = 2(1-t)).

Any comparison probability is then an overlap integral of these, e.g.
P(X > Y) = integral f_Y(t) (1 - F_X(t)) dt. For example
P(loser > winner) = integral 2t (1-t)^2 dt = 1/6.

This method composes across passes (feed each pass's updated distributions into
the next). Caveat: it assumes independence, so it is exact only for elements
that have NOT been directly compared before ("no redundant comparisons"); once
two elements share a chain, that pair becomes dependent.


## 4. Second pass after a first pass of N/2

The first pass (gap N/2) sorts each of the N/2 pairs, so:
- Section 1 (first half of the array) holds the smaller of each pair = "loser"
  (min of 2 uniforms).
- Section 2 (second half) holds the larger = "winner" (max of 2 uniforms).

A second pass with gap N/k produces chains of length k whose first part lies in
section 1 (losers) and second part in section 2 (winners). For a chain starting
at array fraction s (uniform on [0, 1/k)):

    a = ceil(k*(1/2 - s))            (number of losers, then)
    L = ceil(k*(1 - s)),  b = L - a  (number of winners)

so the chain is [L^a, W^b] (a losers followed by b winners). For integer k every
chain is identical with a = b = k/2; for non-integer k the chains are a mixture
of a few (a,b) compositions.

Per-chain formulas (loser-loser and winner-winner pairs invert with prob 1/2,
loser-winner pairs with prob 1/6):

    E[S] = [a(a-1) + b(b-1)]/4 + a*b/6
    E[C] = E[S] + k - H_a - sum_{w=1..b} q(w),   H_a = 1+1/2+...+1/a
    q(w) = integral 2t (1-t)^(2a) (1 - t^2)^(w-1) dt
         = P(the w-th winner is the minimum of everything placed so far)

For even k with m = k/2:  E[S] = m(4m - 3)/6.

### Worked example, gap N/4 (chain [L,L,W,W])
Confirmed two independent ways (inversions+stops, and slot-by-slot probabilities):
    E[S] = 5/3,  E[C] = 1697/420,  swap rate = 700/1697 = 41.25%.


## 5. Second-pass swap rates (first pass = N/2)

    gap   k     swap rate
    N/2   2     1/6            = 16.67%
    N/3   3     25/72          = 34.72%
    N/4   4     700/1697       = 41.25%
    N/5   5     3780/7823      = 48.32%
    N/6   6     62370/118729   = 52.53%
    N/8   8     ~              = 59.77%

The second-pass swap rate increases monotonically as the gap shrinks, from
16.67% (gap N/2: nearly-sorted [loser, winner] pairs) up toward 100% (plain
insertion sort). NOTE: gap N/2 on the second pass gives 16.67%, NOT 50% -- the
50% figure only applies to the FIRST pass. The swap rate crosses 50% between N/5
and N/6, at roughly gap N/5.4 (k ~ 5.4).


## 6. Bits of information per comparison (the better objective)

A 50% AVERAGE swap rate is not the real goal: one comparison at 75% plus one at
25% averages to 50% swaps but carries less than 1 bit each. The correct objective
is average entropy per comparison, using each comparison's conditional swap
probability.

For inserting an element at position p (prefix size s = p-1), let g = number of
prefix elements greater than the key. Comparison j (j = 1..s):
    reached with probability   P(g >= j-1)
    conditional swap prob       q_j = P(g >= j) / P(g >= j-1)
    contributes                 P(g >= j-1) * H(q_j)  bits.

Results (first pass = N/2):

    gap    swap rate   bits/comparison
    N/2    16.67%      0.6500
    N/3    34.72%      0.8609
    N/4    41.25%      0.9312
    N/5    48.32%      0.9468
    N/6    52.53%      0.9537   <- maximum
    N/7    56.79%      0.9394
    N/8    59.77%      0.9284
    N/10   64.95%      0.8907
    N/12   68.88%      0.8509


## 7. Optimizing over continuous k

Treating k as a real number (chains are then a mixture of lengths/compositions,
per the s-integration in Section 4), the bits/comparison curve peaks EXACTLY at
k = 6 (gap N/6):

    N/5.8   bits 0.95260
    N/5.9   bits 0.95315
    N/6.0   bits 0.95366   <- maximum (cusp)
    N/6.1   bits 0.95182
    N/6.5   bits 0.94551

The maximum is a cusp at the integer k = 6. Reason: at k = 6 every chain has the
identical clean composition [L,L,L,W,W,W]. Moving k off 6 mixes in length-7
chains (less informative, ~0.939 bits) above, or length-5 chains (~0.947 bits)
below, both of which lower the average. So the local optima sit at integer k, and
k = 6 is the best of them. Allowing any real k does not move the optimum off N/6.


## 8. Third pass after N/2 then N/6

To push the analysis one pass further we need the value distribution of every
position after the first two passes.

- After N/2: section 1 = losers (min of 2 uniforms), section 2 = winners.
- After N/6: each length-6 chain [L,L,L,W,W,W] gets sorted. Because gap N/6
  places chain-position r (r = 0..5) at array indices [r*N/6, (r+1)*N/6), the
  array splits into 6 EQUAL contiguous "bands". Band r holds the r-th smallest
  of a mixed 6-element set {3 losers, 3 winners}, i.e. an order statistic D_r.

So after two passes the array is 6 monotone bands D_0 <= ... <= D_5 (in
distribution). D_r's CDF is P(at least r+1 of the 6 mixed elements are <= t),
a Poisson-binomial with 3 probs F_L = 2t-t^2 and 3 probs F_W = t^2.

A third pass with gap N/m reads a chain across these bands. Under the
no-redundant-comparisons assumption the chain's elements come from distinct
2nd-pass chains, so they are INDEPENDENT draws from their bands' D_r. A chain
starting at fraction s has element i (i = 0..L-1) in band r_i = floor(6*(s+i/m)),
a non-decreasing list; we insertion-sort that chain of independent order
statistics and average over s (uniform on [0, 1/m)).

Result (1st gap N/2, 2nd gap N/6), sweeping the third gap:

    gap      swap rate   bits/comparison
    N/11.0   44.35%      0.95498
    N/11.5   45.49%      0.96105
    N/11.75  46.00%      0.96379
    N/12.0   46.49%      0.96635   <- maximum (cusp)
    N/12.25  47.27%      0.96604
    N/13.0   49.31%      0.96524
    N/14.0   51.50%      0.96448

The information-optimal third gap is N/12 (~0.966 bits/comparison, swap rate
~46.5%). As before it is a cusp exactly at an integer chain length: at m = 12
every chain has the uniform composition of 2 elements from each of the 6 bands,
[D0,D0,D1,D1,D2,D2,D3,D3,D4,D4,D5,D5]. Any deviation samples the bands unevenly
and lowers the average entropy.

Notice the peak bits/comparison actually RISES pass to pass (1.000 -> 0.954 ->
0.966): each pass leaves the data more structured, so with the right gap the next
pass's comparisons cluster even closer to 50/50.

(Script: info_rate3.py in the ShellSort directory.)


## 9. Fourth pass, and the real pattern

The band model composes: each cusp-optimal pass turns the current nb bands into
new bands that are the order statistics of c independent copies of each current
band (c = chain_length / nb). Starting from a uniform distribution:

    uniform --(x2)--> 2 bands (loser, winner)              [pass 1, gap N/2]
            --(x3)--> 6 bands                              [pass 2, gap N/6]
            --(x2)--> 12 bands                             [pass 3, gap N/12]
            --(x2)--> 24 bands                             [pass 4, gap N/24]

Sweeping the fourth gap N/p over the 12 bands left by the first three passes:

    gap      swap rate   bits/comparison
    N/22.0   44.51%      0.95611
    N/23.0   45.72%      0.96235
    N/23.5   46.27%      0.96517
    N/24.0   46.78%      0.96781   <- maximum (cusp)
    N/24.5   47.56%      0.96748
    N/25.0   48.29%      0.96716
    N/26.0   49.61%      0.96660

So the information-optimal fourth gap is N/24 (~0.968 bits/comparison, swap rate
~46.8%), again a cusp at the chain length (24 = 2*12) that samples all 12 bands
uniformly.

### The real pattern in the optimal gaps

    pass k    optimal gap    denominator    ratio to previous gap
    1         N/2            2              -
    2         N/6            6              3
    3         N/12           12             2
    4         N/24           24             2
    5         N/48           48             2

The gap is multiplied by 3 on the first step and then by 2 (i.e. simply halved)
on every step after. Equivalently

    gap_1 = N/2,   gap_k = N / (3 * 2^(k-1))  for k >= 2,

giving denominators 2, 6, 12, 24, 48, 96, ....

Fifth-pass check (over the 24 bands left after N/2, N/6, N/12, N/24; only the
integer-multiple candidates can be cusps, so only three chains need testing):

    candidate   chain len   swap rate   bits/comparison
    N/24 (c=1)     24        24.7%       0.7978   (degenerate: 1 elt/band)
    N/48 (c=2)     48        47.5%       0.9713   <- maximum
    N/72 (c=3)     72        59.6%       0.9486   (over-swapping)

N/48 wins, confirming the halving continues. The reason the multiplier settles
at 2: each cusp sits at a chain length that is an integer multiple c of the
current band count nb (so every band contributes c elements). c = 1 just
reproduces the previous gap (degenerate/fully redundant), so the smallest useful
choice is c = 2 -- halving the gap. Only the very first refinement (2 coarse
bands that are far apart in value) can profitably afford c = 3.

The peak bits/comparison keeps creeping up and appears to converge:
1.000 -> 0.9537 -> 0.9663 -> 0.9678 -> 0.9713 (passes 1-5), with the optimal
swap rate settling near ~46-48%.

(Scripts: info_rate3.py [3rd pass], info_rate4.py [general band builder + 4th
pass], info_rate5.py [5th-pass candidates], in the ShellSort directory.)


## 10. Key conclusions

1. First pass: any gap >= N/2 gives exactly a 50% swap rate (a full bit per
   comparison); gap N/3 gives 56.25%.

2. Second pass (after N/2): swap rate rises monotonically as the gap shrinks,
   from 16.67% at N/2 toward 100%. It hits 50% near gap N/5.4.

3. The information-optimal second gap is N/6 (~0.954 bits/comparison, swap rate
   ~52.5%), NOT the 50%-swap-rate gap (~N/5.4). Maximizing entropy per
   comparison favors a slightly smaller gap (slightly longer chains) than the
   "aim for 50% swaps" rule of thumb, because it rewards keeping every individual
   comparison near 50/50 rather than merely the average.

4. The information-optimal third gap (after N/2, N/6) is N/12 (~0.966
   bits/comparison, swap rate ~46.5%), again a cusp at the integer chain length
   whose chains sample all 6 bands uniformly.

5. The information-optimal fourth gap (after N/2, N/6, N/12) is N/24 (~0.968
   bits/comparison, swap rate ~46.8%).

6. Each optimum is a cusp at a chain length that samples every current band
   uniformly. The optimal gap denominators are 2, 6, 12, 24, ...: multiply by 3
   on the first refinement, then halve (multiply by 2) every pass after, i.e.
   gap_k = N/(3*2^(k-1)) for k >= 2. The halving was
   reconfirmed at pass 5 (N/48 beats N/24 and N/72).

7. A later pass never reaches exactly 1 bit/comparison, because a pass
   necessarily contains a spread of comparison probabilities, not all 1/2;
   but the achievable maximum drifts upward pass to pass as structure builds
   (1.000 -> 0.954 -> 0.966 -> 0.968 -> 0.971), and the optimal swap rate
   settles near ~46-48%.


## 11. Why optimal gap ratios should approach ~2 (from above) as N grows

This section argues that the ratio between consecutive gaps in an average-case-
optimal sequence should tend toward 2 as N -> infinity -- approaching it from
ABOVE (i.e. staying > 2), and possibly settling at a constant slightly larger
than 2 rather than reaching 2 exactly. The whole argument is heuristic; the last
subsection lists where it can fail.

### 11.1 The idealized result: ratio exactly 2

Sections 6-9 solved the "no redundant comparisons" problem exactly. Under that
idealization the information-optimal schedule halves the gap every pass:
N/2, N/6, N/12, N/24, N/48, ..., i.e. a gap ratio of exactly 2 for every pass
after the first. So in a world with no redundancy, ratio 2 is optimal. Everything
below is about what happens when we put redundancy back in.

### 11.2 Maximizing bits/comparison IS minimizing comparisons (mostly)

A natural worry: is "push bits/comparison toward 1" really the right objective, or
just a proxy? In the idealized model it is exactly the right objective. Sorting a
random permutation requires extracting log2(N!) bits of order information. If
every executed comparison were non-redundant and independent, it would deliver
H(q) fresh bits, and the total number of comparisons needed to reach log2(N!)
bits is

    comparisons  =  log2(N!) / (average bits per comparison).

So with the numerator fixed, maximizing average bits per comparison is IDENTICAL
to minimizing the comparison count. This is why the entropy objective is not just
a proxy -- in the no-redundancy world it is the objective. The one and only thing
that breaks this identity is redundancy: a redundant comparison is still counted
in the cost but delivers zero fresh bits (its outcome is already determined), so
it inflates the denominator's cost without advancing the log2(N!) budget. Hence
in the real world:

    total comparisons  ~=  log2(N!)/bits_per_comp   +   (redundant comparisons).

Everything that makes real gap sequences deviate from the idealized ratio-2
schedule is an attempt to shrink that second term.

### 11.3 The curve is steep below 2 and nearly flat above 2

The sweeps in Sections 7-9 are strongly ASYMMETRIC around the optimum. Taking the
4th-pass sweep (previous gap N/12, so ratio = p/12) as representative:

    ratio 1.5 (N/18):  0.9208 bits     <- far below the peak
    ratio 2.0 (N/24):  0.9678 bits     <- peak
    ratio 2.17 (N/26): 0.9666 bits     <- barely below peak
    ratio 2.33 (N/28): 0.9658 bits     <- still barely below peak

Dropping the ratio to 1.5 costs ~0.047 bits/comparison; raising it to 2.33 costs
only ~0.002. The penalty for being a little ABOVE 2 is almost nothing; the penalty
for being below 2 is large. This asymmetry is the key structural fact: any force
that nudges the optimum off 2 will meet almost no resistance on the high side and
a steep wall on the low side, so the optimum lands slightly above 2.

### 11.4 What actually causes a redundant comparison

Passes run from large gaps down to small gaps. By the classic Shell-sort lemma,
once the array has been a-sorted and b-sorted it is automatically c-sorted for
every c in the numerical semigroup generated by a and b (every nonnegative
integer combination i*a + j*b). More generally, before a gap-g pass the array is
already sorted with respect to every distance in the semigroup generated by all
the LARGER (previously used) gaps.

In the gap-g insertion at position i we probe distances g, 2g, 3g, ... (i vs i-g,
i-2g, ...). A probe at distance m*g is a guaranteed no-swap -- a redundant,
zero-information "confirmation" -- exactly when m*g lies in the semigroup of the
larger gaps. Because each pass runs on nearly-sorted data, insertion walks back
only a small expected number of steps, so only small m matter:

- m = 1 (distance g): never redundant. Every larger gap exceeds g, so g itself is
  never a nonnegative combination of them. The primary probe is always live.
- m = 2 (distance 2g): redundant iff 2g is a nonnegative combination of larger
  gaps -- most obviously when some larger gap equals exactly 2g (ratio exactly 2),
  but also when 2g = a + b, or 2g = 2a, etc.
- m = 3, 4, ...: redundant iff 3g, 4g, ... are representable.

So "a gap equals a small-coefficient linear combination of larger gaps" is
precisely the condition for frequent redundant comparisons -- exactly the
situations to avoid (g = 2*(next gap), g = (next) + 2*(next-next), and so on).

### 11.5 Why redundancy pushes the ratio ABOVE 2, never below

Combine 11.3 and 11.4:

1. Ratio exactly 2 is the single worst case for the m=2 probe: the next larger
   gap equals 2g, so the second step of every insertion is a guaranteed no-op
   (redundant). Ratio 2 maximizes bits/comparison in the idealized model but
   simultaneously maximizes this particular redundancy.

2. To dodge it we perturb the ratio off 2. Perturbing UPWARD (ratio a bit > 2)
   removes the 2g = a coincidence and, by 11.3, costs almost no idealized
   bits/comparison. Perturbing DOWNWARD (ratio a bit < 2) also removes the
   coincidence, but lands on the steep side of the bits curve AND packs the gaps
   more densely -- more generators, spaced closer, whose semigroup covers 2g, 3g,
   ... far more readily -- so it INCREASES redundancy. Downward loses on both
   counts.

3. Therefore both terms in the cost (idealized bits and redundancy) prefer "a
   little above 2" to "a little below 2." The optimum is squeezed to just above 2,
   and 2 acts as a floor it approaches but should not cross.

### 11.6 Why the redundancy shrinks for large gaps at large N

The redundancy of a gap-g pass is governed by whether the small multiples
2g, 3g, ... fall in the semigroup of the larger gaps. Two regimes:

- Small gaps (late passes): by the time g is small, MANY larger gaps have already
  been used; their semigroup is dense (its Frobenius number is tiny compared to
  g), so 2g, 3g, ... are almost surely representable. Redundancy here is large and
  essentially unavoidable -- you cannot make a small gap that dodges a dense
  semigroup. This is why the smallest gaps carry the largest ratios (see 11.7:
  the sequences start 1, 4, 10, ... with ratios 4.0, 2.5 before settling down),
  and why this whole regime is "small-N-like."

- Large gaps (early passes): only a few, large, sparse gaps sit above them; their
  semigroup has enormous holes, so it is easy to pick g with 2g, 3g, ... (and the
  handful of larger multiples that a short insertion walk could reach) all lying
  in those holes. Redundancy here can be made very small.

As N grows, the sequence gains more large-gap passes of the second kind, and for
those the redundancy term becomes negligible, so their ratios are free to relax
toward the idealized value 2. The irreducibly redundant small gaps form a roughly
fixed prefix (1, 4, 10, 23, 57, 132, 301, 644, 1408, 3227, ... -- note how the
low end of the empirically optimal sequences literally stabilizes and stops
depending on N). That fixed prefix is a vanishing fraction of the log(N) passes
as N -> infinity, so the ASYMPTOTIC (middle-of-sequence) ratio is dominated by
the low-redundancy large-gap regime and should approach 2 from above.

### 11.7 What the best-known sequences show

The empirically optimized sequences behave exactly as this picture predicts.

Within a single large-N sequence the ratio starts high at the small gaps and
decays toward the low-2s in the middle. For N = 100 million the consecutive
ratios are:

    4.00, 2.50, 2.30, 2.48, 2.32, 2.28, 2.14, 2.19, 2.29, 2.12,
    2.17, 2.15, 2.14, 2.16, 2.14, 2.11, 2.16, 2.14, 2.16, [2.59, 3.48, 1.65]

The bracketed tail is boundary effect (the top 2-3 gaps must reach N and are the
least trustworthy / least "asymptotic"); the meaningful middle sits around
2.11-2.16.

Across N, the middle ratio drifts DOWN as N grows. The geometric-mean ratio over
the middle of the sequence (dropping the 3 smallest gaps and 3 top boundary gaps):

    N = 10^6 :  ~2.235
    N = 10^7 :  ~2.211
    N = 10^8 :  ~2.195

Both trends -- decreasing as we move up within a sequence, and decreasing as N
increases -- are consistent with a limit at (or just above) 2, with redundancy
supplying the positive offset that keeps it from reaching 2 at any finite N.

### 11.8 The claim, and where it could be wrong

Claim: for average-case-optimal gap sequences, the ratio between consecutive
"middle" gaps tends to 2 from above as N -> infinity, and in particular never
settles below 2; the residual excess above 2 is the fingerprint of unavoidable
redundant comparisons, which thin out for large gaps but never quite reach zero
at any finite N.

Possible flaws and open points:

1. Idealization. The exact ratio-2 result comes from a model that assumes
   independent draws and a clean order-statistic band structure. Real chains are
   correlated and the band model is approximate, so "ratio 2" as the redundancy-
   free optimum is itself only approximate.

2. The accounting identity in 11.2 (max bits = min comparisons) holds only in the
   no-redundancy world. Once redundancy is present, minimizing comparisons is NOT
   literally the same as maximizing per-pass entropy, and the precise trade-off is
   exactly the thing we have not solved.

3. "Redundancy -> 0 for large gaps" is not proven. The number of predecessor gaps
   grows like log(N), and their semigroup grows denser, so even large gaps might
   retain a bounded, nonzero number of redundant probes per insertion forever. If
   so, the limit is a constant strictly greater than 2 (the empirical ~2.1-2.2,
   perhaps drifting to ~2.05, not to 2.0). The data cannot yet distinguish "-> 2"
   from "-> 2 + epsilon."

4. Expected back-step > 1. Insertion always walks back more than one step some of
   the time, so the m = 2, 3 probes are always exercised; if 2g or 3g stays
   representable, a residual redundancy per insertion persists and keeps the ratio
   bounded above 2.

5. Objective sensitivity. This is average-case comparison count with distinct,
   uniformly random keys. Counting moves/swaps instead, or accounting for cache
   locality, worst case, or duplicate keys, could change the limiting constant.

6. Data quality. The best-known sequences are optimal only through N=362;
   for larger N (and especially N >= 10^6) they are heuristic and may sit above
   the true optimum. If the true optima have smaller ratios than we have found,
   the downward drift toward 2 could be stronger (or, conceivably, weaker) than
   the table suggests.

7. Greedy-vs-global. Dodging redundancy at one pass constrains neighboring gaps,
   so the optimal sequence is a global object; a single clean "limiting ratio"
   may not exist, only an average behavior.

8. Extrapolated asymmetry. The steep-below/flat-above shape (11.3) was measured
   for the first few passes; we assume it persists for very large gaps. That is
   plausible (it is a generic order-statistic effect) but unverified at scale.

Net: the argument gives a strong reason to expect ratios bounded below by ~2 and
trending toward 2 from above, with the honest caveat that the true limit could be
a constant slightly larger than 2, and that we lack a proof that the large-gap
redundancy genuinely vanishes.


## Computational check

The exact per-slot swap probabilities come from order-statistic integrals
(Beta-function values). Entropies and the continuous-k optimization were
evaluated numerically from those exact densities (Simpson integration of the
uniform/min/max densities; Poisson-binomial DP for the count g). As a
correctness check, the numerics reproduce the exact rationals, e.g.
E[C] for N/4 = 1697/420 = 4.04048 and swap rate 700/1697 = 0.41249.
(Scripts used during the analysis: info_rate.py [integer gaps], info_rate2.py
[continuous-k 2nd-pass sweep], and info_rate3.py [3rd-pass band model], in the
ShellSort directory.)
