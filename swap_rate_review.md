# Review of "Shell Sort analyzed by swap rate and bits-of-information per comparison"

This reviews `swap_rate_information_analysis.md` from the ShellSort repository.
I recomputed its numbers exactly where possible and wrote simulators of real Shell sort
and of its band model. Then I checked each step of the argument and re-ran its
optimization with a corrected objective.

## Summary

**The calculations hold up, but the conclusion does not, and the corrected argument
predicts the best-known sequences.**

1. **Every number I checked in §§2–9 is correct.** The whole pass-2 table checks out
   exactly in rational arithmetic. The values at the document's chosen gaps for passes 3–5
   reproduce in a direct simulation of the band model. (I did not re-run the
   continuous-k sweeps around those optima.) The one unstated hypothesis is that no gap
   divides an earlier one. When the second gap divides N/2, the numbers change; for
   example N/6 gives a 48.98% swap rate, not 52.53%.

2. **The band model is better than the document claims.** For generic gaps it is
   *exact* for the early passes, not a heuristic. If no gap is a small integer
   combination of earlier gaps, each chain's elements really are independent draws from
   their bands (Theorem 1). Real Shell sort follows the model to 3–4 digits until gaps
   fall to a few thousand. For the README's best 10⁷ sequence, every pass with
   gap ≥ 31,719 matches the model to within 0.07%.

3. **"Bits per comparison" computed from band laws is not information gained.** This
   refutes §11.2. Even in the idealized model, with no redundancy at all, these bits add
   up to about **1.84 × log₂N!** over a whole sort. So nearly half of them are facts that
   earlier passes had already implied. As a result, maximizing bits/comparison is not the
   same as minimizing comparisons.

4. **The correct objective is the cost per e-fold of gap reduction,
   κ(ρ) = c(ρ)/ln ρ.** Here c(ρ) is the steady-state number of comparisons per element
   per pass. In the document's own model, κ is minimized at **ρ\* ≈ 2.11, with
   κ\* ≈ 2.795**. The minimum is broad: within 0.3% for 2.05 ≤ ρ ≤ 2.2.
   - The document's optimum, exact ratio 2, costs 2.855, which is 2.1% more.
   - Exact integer ratios ("cusps") cost 1–2% *more* than nearby generic ratios, even
     though they maximize the document's bits.
   - Alternating ratios do not beat a constant ratio.

5. **This matches the README's best-known sequences.** Their middle ratios are
   2.11–2.17, and stable at ≈ 2.15 from N = 10⁷ to 10⁹ rather than drifting toward 2.
   Their measured growth rate d(C/N)/d(ln N) is 2.788, 2.795 and 2.799 over successive
   decades, converging to ≈ 2.80.
   - Run on the README's own sequences, the band model reproduces that growth: 2.802
     against the real 2.795 between 10⁷ and 10⁸.
   - Real Shell sort beats the model by a *constant* 2.05 comparisons per element,
     almost all of it in the small-gap passes.
   - So **E[C] ≈ 2.80·N·ln N − 6.9·N ≈ 1.94·N·log₂N**.
   - The best sequences therefore extract about **0.52 bits per comparison**
     asymptotically.
   - A pure N ln N law fits the README data better than the README's
     N·ln N·(ln ln N)^d law.

6. **The redundancy mechanism of §11.4 is wrong, and redundancy can't be what sets the
   middle ratio.**
   - The m-th comparison of an insertion is not against the original occupant of
     position i − m·g. So "2g in the semigroup ⇒ the second probe never swaps" is false:
     in real runs, 45% to over 90% of such probes swap.
   - The correct criterion uses the compared element's position *before* the pass, and
     it can only ever affect the stopping comparison.
   - Measured redundancy is essentially zero (at most about 1%) in every pass except the
     last 2–3.

7. **Practical warning about the recommended sequence N/2, N/6, N/12, ….**
   - Taken literally, with N chosen so that each gap divides the previous one, it is
     catastrophic: about 520 comparisons/element at N = 786,432, against 31 for good
     sequences.
   - With generic rounding it costs 33.4 at N = 10⁶, about 5% worse than the best known
     (31.7).
   - Ratios near 2 also make real Shell sort leave the band model early, at pass 5 in the
     example of §2.3.

### Verdict by section of the original document

| original section | verdict |
|------------------|---------|
| §§1–2 (model, first pass) | correct |
| §§3–5 (second pass) | correct for gaps that don't divide N/2; different numbers otherwise (§1.2) |
| §6 (bits/comparison) | values correct; flawed as an objective (§3) |
| §7 (cusp at k = 6) | true for bits; cusps are *worse* for cost (§4.2) |
| §§8–9 (band model, passes 3–5) | values correct (§2.2); the model is exact for generic gaps (Theorem 1) |
| §10 (conclusions) | true as statements about bits; item 6 ("halving is optimal") fails for cost (§4) |
| §11.1 (ratio exactly 2) | refuted: ρ\* ≈ 2.11 under the correct objective (§4) |
| §11.2 (bits ⇔ comparisons) | refuted: off by a factor of ≈ 1.84 with zero redundancy (§3) |
| §11.3 (steep below 2, flat above) | true for bits; κ(ρ) is roughly symmetric in ln ρ around 2.11 (§4.1) |
| §11.4 (probe redundancy) | refuted; corrected criterion in §6.2 |
| §§11.5–11.6 (redundancy pushes the ratio above 2) | not the mechanism: redundancy ≈ 0 in the middle passes (§6.3) |
| §11.7 (drift toward 2) | an averaging artifact; the middle ratio is stable at ≈ 2.15 (§5.1) |
| §11.8 (limit 2 from above) | not supported; the model gives ≈ 2.11, flat over 2.05–2.2 |

---

## 0. Conventions and tools

- **Input and cost.** The input is a uniformly random permutation. Comparisons are
  counted as in the repository's `main.c`. An insertion that shifts the key past s
  elements costs s + 1 comparisons, or s comparisons if the key reaches the front of its
  chain.
- **`analysis/exact_pass2.py`** gives exact rational results for pass 2, using
  polynomial integration over the loser/winner densities f_L = 2 − 2t and f_W = 2t.
- **`analysis/shellsim.c`** runs in three modes:
  - **real**: Shell sort itself.
  - **mf**: the band model. M copies of the array are sorted side by side, and before
    every pass the values at each position are randomly permuted across the copies. This
    keeps every position's exact marginal law and destroys correlations between
    positions, which is exactly the band model with position-dependent laws.
    - Caveat: a 1/M fraction of correlation survives each shuffle. For sequences with
      exact or near-exact small-integer relations between gaps, this residue is
      amplified. I therefore exclude ratios like exactly 2 or ≈ 13/6 from the mf results.
  - **bs**: the band model for divisible "cusp" sequences, in which every gap divides
    the previous one. Values are shuffled within each block [r·h_prev, (r+1)·h_prev)
    before each pass. This is exactly the document's model with no redundancy at all.

---

## 1. Passes 1–2 (§§2, 4–7): verified, with one hidden assumption

### 1.1 The numbers are exact

Second pass after a first pass with gap N/2. The second gap N/k does not divide N/2;
for odd k, chains are a 50/50 mix of the two compositions.

| gap  | swap rate (exact)        | document bits/cmp | chain-entropy bits/cmp | cmp/element |
|------|--------------------------|-------------------|------------------------|-------------|
| N/2  | 1/6 = 16.67%             | 0.6500            | 0.6500                 | 0.500       |
| N/3  | 25/72 = 34.72%           | 0.8609            | 0.8554                 | 0.800       |
| N/4  | 700/1697 = 41.25%        | 0.9312            | 0.9237                 | 1.010       |
| N/5  | 3780/7823 = 48.32%       | 0.9468            | 0.9395                 | 1.242       |
| N/6  | 62370/118729 = 52.53%    | 0.9537            | 0.9466                 | 1.428       |
| N/7  | 56.79%                   | 0.9394            | 0.9331                 | 1.635       |
| N/8  | 59.77%                   | 0.9284            | 0.9225                 | 1.813       |
| N/10 | 64.95%                   | 0.8907            | 0.8859                 | 2.181       |
| N/12 | 68.88%                   | 0.8509            | 0.8469                 | 2.541       |

Every number in the document's §5–6 tables is reproduced. The **chain-entropy** column
is H(sorted order of a chain | pass-1 outcome) / E[comparisons]. It is lower than the
document's figure because the document sums each insertion's entropy separately, and the
insertion outcomes within one chain are dependent: the elements are independent but not
identically distributed. Even the chain entropy is only an upper bound on what pass 2
actually learns. Each pass-1 pair sits in two different pass-2 chains, which couples
those chains.

### 1.2 Hidden assumption: the second gap must not divide N/2

If h₂ divides N/2 exactly (for example N = 6m and h₂ = N/6), every pass-2 chain consists
of k/2 complete first-pass pairs. Those elements are not independent, and the answers
change:

| gap (divides N/2) | E[swaps] | E[comparisons] | swap rate | true bits/cmp |
|-------------------|----------|----------------|-----------|---------------|
| N/4               | 4/3      | 23/6           | 34.78%    | 0.674         |
| N/6               | 4        | 49/6           | 48.98%    | 0.795         |
| N/8               | 8        | 167/12         | 57.49%    | 0.812         |

Here the chain entropy *is* the true information of pass 2, because every pair lies
inside a single chain, so chains are independent.

So the cusp at "exactly k = 6" in §7 is a limit of gaps *near* N/6 that do not divide
N/2, such as N/6 + 1. That distinction matters for every later section, because the
recommended sequence N/2, N/6, N/12, … is divisible at every step if taken literally.

---

## 2. Sections 8–9 (band model): exact for early passes when gaps are generic

### 2.1 Exactness theorem

**Ancestry.** Let A₀(p) = {p}. Define A_j(p) as the union of A_{j−1}(q) over all
q ≡ p (mod h_j). By induction, the key at position p after pass j is a function of the
input keys at A_j(p). Also, A_j(p) ⊆ { p + Σ_{i≤j} e_i h_i : |e_i| < N/h_i }.

**Lemma.** Suppose that for every pass j ≤ K and every 0 < m < N/h_j, the number m·h_j
is *not* of the form Σ_{i<j} f_i h_i with integers |f_i| < 2N/h_i. Then:

- in each of the first K passes, the keys entering every chain have pairwise disjoint
  ancestries, so they are mutually independent;
- the band recursion therefore gives the exact law of every position, and the exact
  expected comparisons and swaps of each pass. Each position's law is the r-th order
  statistic of its chain's independent inputs.

**Theorem 1.** Fix K and reals 1 < α₁ < … < α_K, and let h_j = N/α_j + O(1). Assume no
relation m/α_j = Σ_{i<j} f_i/α_i holds with integers 0 < |m| ≤ α_j and |f_i| ≤ 2α_i.
Then for all sufficiently large N the lemma's hypothesis holds. Consequently, the swap
rates, comparison counts and "bits" of the first K passes converge to the band-model
values as N → ∞.

*Proof.* If m·h_j = Σ f_i h_i, then N·(m/α_j − Σ f_i/α_i) = O(|m| + Σ|f_i|) = O(1).
Only finitely many coefficient vectors are allowed, and each non-vanishing linear form
is bounded away from 0, so the relation fails for large N. ∎

Remarks:

- **Divisible gaps** violate the hypothesis at the first opportunity (m = h_{j−1}/h_j).
- **Hibbard-type gaps 2^k − 1** satisfy 2h_{k+1} = 3h_k − h_{k−1}, a relation with tiny
  coefficients. The model therefore fails from pass 3 on. At N = 10⁶ real Shell sort
  costs 63.9 comparisons/element with Hibbard and 63.4 with 2^k + 1, against 33.6 for a
  generic ratio-2 sequence.
- **The document's exact ratios** (α = 2, 6, 12, 24, …) also violate the hypothesis,
  since 2/12 = 1/6. Exactness then depends on the O(1) rounding. In the example of §2.3,
  rounding that avoids divisibility keeps passes 2–4 within 0.02% of the model. Later
  passes eventually hit an exact relation among the small "defects" 2h_j − h_{j−1}.
- **Long runs of passes.** A fixed K can always be handled by taking N large. For a given
  N, though, a counting heuristic says relations with allowed coefficients become
  unavoidable after about √(2·log_ρ N) passes. From then on the band model is an
  approximation. The resulting dependence travels through long ancestry paths and,
  empirically, stays below 0.1% until gaps reach a few thousand.

### 2.2 Checks against simulation

**The document's own sequence in its own model.** In bs mode at N = 3·2²⁰ (divisible,
so this is exactly §§8–9's model), passes 2–5 give:

| pass | swap rate / bits per cmp | document's value |
|------|--------------------------|------------------|
| 2    | 52.54% / 0.9536          | 52.53% / 0.9537  |
| 3    | 46.49% / 0.9664          | 46.49% / 0.96635 |
| 4    | 46.80% / 0.9679          | 46.78% / 0.96781 |
| 5    | 47.52% / 0.9714          | 47.5% / 0.9713   |

Continuing with ratio 2, the band model settles at:

- **0.978 bits/comparison** (the document's figure appears to converge, as it says);
- **c = 1.979 comparisons/element/pass;**
- **a 49.5% swap rate.**

**Real Shell sort, generic ratio 2.2.** Per-pass cost of real Shell sort minus the
band-model cost, for the same gaps:

| N    | passes within 0.1%      | first visible departures      | last pass (gap 1) |
|------|-------------------------|-------------------------------|-------------------|
| 10⁵  | gaps ≥ 2140             | −0.2% at 957, then −0.6% to +14% | −32%           |
| 10⁶  | gaps ≥ 4378             | −0.2% at 2019, −1.3% at 409   | −33%              |
| 10⁷  | gaps ≥ 9089             | −0.3% at 4124, then +0.8% to +3.3% at gaps 1903–394 | −37% |

The model is quantitatively exact for the large-gap passes, and the point where it stops
moves to larger gaps as N grows, but slowly. Below that point, real Shell sort can be
either cheaper or more expensive than the model, depending on the arithmetic of the gaps.
It is much cheaper in the last 2–3 passes. This is why the smallest gaps must be tuned
by hand, Ciura-style; no smooth model can choose them.

### 2.3 Ratios near 2 leave the model early

Take the document's sequence with generic rounding at N = 10⁶:
500000, 166684, 83295, 41687, 20851, 10372, ….

- Real Shell sort matches the model to 0.02% for passes 1–4.
- At pass 5 it departs: real 1.794 cmp/element, against 1.835 in the mean-field model
  and 1.840 in the exact cusp model. Passes 6–11 then cost **1.7–4.8% more** than the
  model predicts.
- No comparison in those passes is semigroup-redundant. The cause is an exact
  small-coefficient relation among the gaps: h₂ = h₃ + h₄ + 2h₅
  (166684 = 83295 + 41687 + 2·20851). It gives pass-5 chain elements that are 2h₅ apart
  a shared ancestry.
- With ratio ≈ 2, the defects 2h_j − h_{j−1} are small numbers, so such relations
  appear after a few passes. With a ratio like 2.2, the nearest small-coefficient
  relations have large coefficients, so the model holds much deeper.

---

## 3. §11.2: what "bits per comparison" does and does not measure

### 3.1 The exact identity

Number the comparisons of the whole sort t = 1, 2, …. Let q_t be the probability that
comparison t swaps, **given the outcomes of all previous comparisons, in all earlier
passes**. The full outcome record determines the input permutation, so by the chain rule:

  log₂ N! = E[ Σ_t h(q_t) ],  and hence  E[C] = log₂ N! + E[ Σ_t (1 − h(q_t)) ].

Here h is the binary entropy function. This is the correct version of §11.2, and it has
no separate redundancy term: a redundant comparison is simply one with h(q_t) = 0.

### 3.2 The document's bits are an upper bound, not the information

Let D_k be the document's bits for pass k: the entropy of the band-law distribution of
the insertion distance g, summed over insertions. Let T_k be the pass-k outcome record.
Then

  D_k ≥ H(T_k) ≥ H(T_k | T₁, …, T_{k−1}) = I_k,  with  Σ_k I_k = log₂ N!.

- The first inequality follows from subadditivity of entropy and concavity (pooling
  insertions into one class can only raise the entropy).
- The second is an equality only if pass k's outcomes are independent of the earlier
  passes. They are strongly dependent.

### 3.3 How large the overcount is

The document's idealized world (bs mode, ratio 2: independent bands, zero redundancy)
answers this directly:

- **Per pass, in the steady state:** 1.979 comparisons × 0.978 bits ≈ **1.93 "bits" per
  element per pass.**
- **What a pass can deliver on average:** about **1 bit per element.** A whole sort makes
  log₂N + O(1) passes and needs only log₂N! ≈ N·(log₂N − 1.44) bits.
- **Totalled over a sort of N = 2²³ keys in this model:**
  - the document's bits add up to ≈ 39.7·N (measured for passes 1–11, with passes 12–23
    extrapolated at 0.978 bits/comparison);
  - log₂N! is only 21.6·N;
  - the model's actual cost is **40.76·N comparisons.** The §11.2 formula
    log₂N!/bits ≈ 21.6/0.975 ≈ 22.1·N is wrong by a factor of 1.84, with no redundancy
    anywhere.

In words, each comparison is nearly a fair coin flip *given the band laws*, but not given
what the earlier passes revealed. About 45–50% of what the document counts is
re-learning. It is not certain re-learning (that would be semigroup redundancy, which is
zero here); it is statistical re-learning, invisible to per-position marginals.

The band model can still predict **costs** exactly while getting **information** wrong.
Costs depend only on the joint law of the inputs to each chain, which the model gets
right for generic gaps. Information depends on the joint law of all passes' outcomes
together, which the model does not represent.

### 3.4 The right objective

In the scale-invariant middle of the sequence, every pass with ratio ρ costs about c(ρ)
comparisons per element, and there are (ln N)/(ln ρ) such passes. Hence

  **E[C] ≈ κ(ρ)·N·ln N + O(N),  with  κ(ρ) = c(ρ)/ln ρ.**

Equivalently, maximize the **true** bits per comparison, log₂ρ / c(ρ) = 1/(κ·ln 2). The
document's per-pass bits/comparison leaves out the fact that a smaller ratio needs more
passes. That omission is why it lands on ρ = 2.

---

## 4. Re-optimizing the document's own model

### 4.1 κ(ρ) on the steady-state plateau

These are mf runs at N = 10⁷ with M = 8 copies, averaged over 6 phase shifts of each
geometric sequence. κ = Σc_k / Σln(h_{k−1}/h_k) over passes k ≥ 12 with gap ≥ 100.

The window matters. c_k approaches its plateau slowly, about 12 passes below the top: at
pass 9 it is still 1.3% low. Windows that start earlier bias κ down by about 0.4%.

| ρ     | κ(ρ)              | c(ρ) = κ·ln ρ | swap rate |
|-------|-------------------|---------------|-----------|
| 1.95  | 2.817             | 1.881         | 46.9%     |
| 2.05  | 2.801             | 2.011         | 50.3%     |
| 2.08  | 2.795             | 2.047         | 51.2%     |
| 2.11  | **2.794**         | 2.086         | 52.2%     |
| 2.12  | 2.796             | 2.101         | 52.4%     |
| 2.14  | 2.795             | 2.127         | 53.0%     |
| 2.20  | 2.802             | 2.209         | 54.8%     |
| 2.25  | 2.814             | 2.282         | 56.2%     |
| 2.30  | 2.824             | 2.352         | 57.6%     |

A parabola in ln ρ fits these points with χ²/dof = 0.9 and gives:

- **ρ\* = 2.11 ± 0.01;**
- **κ\* = 2.795 ± 0.002** statistical, ±0.002 from the choice of window;
- curvature d²κ/d(ln ρ)² ≈ 7.4.

Ratios ≈ 2.0 and ≈ 2.17 (next to 13/6) are left out because of the mean-field
near-resonance artifact described in §0. Using M = 32 copies instead of 8 gives
κ(2.11) = 2.791 ± 0.004, so the finite-M effect on the plateau is below the noise.

The curve is close to symmetric in ln ρ, if anything slightly steeper above the
optimum: κ(1.95) is 0.8% above κ\* and κ(2.3) is 1.1% above. §11.3's "steep below 2,
flat above" is a property of bits/comparison and does not carry over to the cost.

### 4.2 Exact cusps are worse, not better

Comparing the bs plateau at an exact integer ratio with the smooth curve through
nearby generic ratios:

| ratio | exact cusp κ (bs) | generic κ near that ratio      | cusp penalty |
|-------|-------------------|--------------------------------|--------------|
| 2     | 2.855             | 2.805 (parabola at ρ = 2)      | +1.8%        |
| 3     | 3.228             | 3.195 (between 3.120 at 2.9 and 3.270 at 3.1) | +1.0% |
| 4     | 4.098             | ≈ 4.06 (between 3.91 at 3.85 and 4.21 at 4.15) | +0.9% |

The cusps that maximize the document's bits/comparison cost *more* comparisons per pass
than gaps that just miss them. The exact ratio-2 schedule also sits on the wrong side of
the true optimum: 2.855 against 2.795, i.e. 2.1% more comparisons in the middle passes.

### 4.3 Non-geometric middles don't help

I ran periodic ratio patterns with the same geometric mean (2.12), using every rotation
of each pattern and windows made of whole periods:

| pattern         | κ (plateau)      |
|-----------------|------------------|
| 2.12 (constant) | **2.796**        |
| 1.98 : 2.27     | 2.798            |
| 1.9 : 2.37      | 2.800            |
| 1.8 : 2.50      | 2.804            |
| 1.7 : 2.64      | 2.812            |
| 2.05 : 2.05 : 2.27 | 2.798         |

The penalty grows like 0.33·a², where a is the half-amplitude of ln ρ. A constant ratio
is locally optimal, so the ±1.5% ratio jitter in the README sequences costs nothing
measurable. (A first estimator that ignored pattern alignment suggested the opposite;
that was an artifact.)

### 4.4 Swap rate

The steady-state swap rate at the optimum is about **52%**: 51.2–53.0% for
ρ = 2.08–2.14, against 49.5% at the exact ratio-2 cusp. "Aim for about 50% swaps" is a
good rule of thumb, but the optimum is not defined by it.

---

## 5. Comparison with the best-known sequences (README)

### 5.1 The middle ratio stabilizes at about 2.15; it is not drifting to 2

| N   | document-style middle (drop 3 smallest and 3 largest gaps) | window: 3227 ≤ gap ≤ N/12 |
|-----|------------------------------------------------------------|---------------------------|
| 10⁶ | 2.238                                                      | 2.178 (4 ratios)          |
| 10⁷ | 2.213                                                      | 2.157 (7 ratios)          |
| 10⁸ | 2.195                                                      | 2.145 (10 ratios)         |
| 10⁹ | 2.189                                                      | 2.148 (13 ratios)         |

The downward drift in §11.7 comes from the fixed small-gap prefix
(…, 132, 301, 644, 1408, with ratios 2.3–2.5) becoming a smaller share of a longer
sequence. Over a fixed window the middle ratio stops moving at about 2.15, inside the
model's flat optimum. The model's κ at 2.15 is ≈ 2.796, only 0.05% above κ\*.

### 5.2 Growth law

Local slopes d(C/N)/d(ln N) between successive README points:

| interval         | slope  |
|------------------|--------|
| 10⁵ → 2·10⁵      | 2.767  |
| 5·10⁵ → 10⁶      | 2.777  |
| 10⁶ → 10⁷        | 2.788  |
| 10⁷ → 10⁸        | 2.795  |
| 10⁸ → 10⁹        | 2.799  |

The increments roughly halve each decade, which extrapolates to a limit of about
2.80–2.805. The band model also explains why the slope approaches its limit from below:
per pass, c_k/ln ρ rises toward the plateau, and at moderate N most passes are still in
that rising transient.

Two-parameter fits to the README data for N ≥ 10⁵:

- **C/N = κ·ln N − A** gives κ = 2.787 ± 0.003 and A = 6.76, with rms residual 0.017.
- **The README's law C/N = a·ln N·(ln ln N)^d** gives rms residual 0.077.
- The README's law also predicts slopes of 2.83 and 2.87 for the last two decades,
  against the observed 2.795 and 2.799.

### 5.3 The band model reproduces the README sequences' own growth

I also ran the band model (mf mode) on the README's best sequences themselves:

| N   | real (README) | band model | real saving (band − real) |
|-----|---------------|------------|---------------------------|
| 10⁶ | 31.729        | 33.928     | 2.199                     |
| 10⁷ | 38.148        | 40.189     | 2.041                     |
| 10⁸ | 44.584        | 46.640     | 2.056                     |

Growth rate d(C/N)/d(ln N) for the same sequences:

- **10⁶ → 10⁷:** real 2.788, band model 2.719.
- **10⁷ → 10⁸:** real 2.795, band model 2.802.

Per pass at N = 10⁷, real Shell sort matches the model to within 0.07% for every gap
≥ 31,719, and is 0.1–0.17% cheaper for gaps 14,842–3,227. Its entire saving comes from
passes with gap ≤ 1408, where it is 0.5% (gap 1408) to 31% (gap 1) cheaper. The last
three passes (gaps 10, 4, 1) account for 1.69 of the 2.04 saving, and they are the only
passes with any semigroup redundancy (0.4%, 5.5% and 12.6%).

So for the best-known sequences, real Shell sort is the band model plus a **constant**
saving of about 2.05 comparisons per element from the tuned small-gap prefix, constant
from 10⁷ onward. Their growth rate is therefore a band-model quantity.

### 5.4 What this gives for Shell sort

Putting this together: the corrected band model predicts that the cost per e-fold of the
best sequences tends to κ at their middle ratio, κ(2.15) ≈ 2.796. That is within 0.05% of
the optimum κ\* ≈ 2.795, and within 0.2–0.3% of the README's measured slope of about
2.80. The predicted optimal middle ratio matches as well. The resulting conjecture:

  **E[C] = κ\*·N·ln N − A·N + o(N),  with κ\* ≈ 2.80 (≈ 1.94·N·log₂N) and A ≈ 6.9,**

for the best sequences. With κ fixed at 2.795, the README's 10⁸ and 10⁹ points both give
A ≈ 6.9. The free two-parameter fit of §5.2 (κ = 2.787, A = 6.76) is pulled down by the
smaller N, where the slope is still rising.

The fraction log₂N!/E[C] then tends to 1/(κ\*·ln 2) ≈ **0.516**. For comparison, it is
0.583 at N = 10⁶ and 0.558 at N = 10⁹ in the README data.

Context: Jiang–Li–Vitányi show that any p-pass Shell sort averages Ω(p·N^{1+1/p})
comparisons. With p = Θ(log N) that bound is Ω(N log N), so the form is the only one
possible for logarithmically many passes. Whether some sequence achieves O(N log N) on
average is still open. This analysis says yes, at about 2.8·N·ln N, but it is a model
prediction, not a proof (§8). I'm not aware of a published model-based derivation of
this constant.

---

## 6. Redundancy (§§11.4–11.6)

### 6.1 The probe claim is false

In the gap-g insertion of the key at position i, the m-th comparison is against the m-th
largest element of the already sorted prefix. That is generally *not* the element that
started the pass at position i − m·g. Counterexample: a chain that is 2-sorted, as it is
whenever 2g was an earlier gap, such as x = (5, 1, 6, 2, 7, 3). Inserting 3 into the
sorted prefix [1, 2, 5, 6, 7] compares against 7 (swap), 6 (swap), 5 (swap) and 2
(stop). The second probe swaps.

Over all 2-sorted arrangements of length n, the fraction of second probes that swap is
25% (n = 4), 32% (n = 6), 38% (n = 8) and 42% (n = 10). In real runs of the document's
exactly divisible sequence, where 2g is always the previous gap, 45% of second probes
swap in pass 3, 58% in pass 4, and over 90% from gap 1024 down.

### 6.2 The correct criterion

Before the gap-g pass, the array is d-sorted for every d in the numerical semigroup S
generated by the earlier gaps. Suppose the key x (pre-pass position i) is compared with
a prefix element y whose pre-pass position was q. Then **the outcome is already implied
by S-sortedness if and only if i − q ∈ S.**

- *If:* the array was (i − q)-sorted, so y ≤ x.
- *Only if:* the S-sorted constraints form a poset in which q precedes i exactly when
  i − q ∈ S. If they are incomparable, some linear extension has y > x.

Consequently, **semigroup-redundant comparisons are always stopping comparisons, never
swaps.** Each insertion has at most one, and it depends on where the stopping element
started, not on the probe index m.

### 6.3 How much redundancy there is

These are measured fractions of redundant comparisons, using the exact criterion, in
real Shell sort with generic ratio 2.2.

| N   | passes with gap ≥ ~10 | last 2–3 passes           |
|-----|-----------------------|---------------------------|
| 10⁵ | 0                     | 5.3% (gap 8), 15.6% (gap 2), 38.5% (gap 1) |
| 10⁶ | 0                     | 3.7% (gap 3), 13.2% (gap 2), 40.0% (gap 1) |
| 10⁷ | 0, except 0.96% at gap 394 | 1.9% (gap 2), 37.9% (gap 1) |

§11.6 is right that the smallest gaps are dominated by redundancy and correlation.
Redundancy is nevertheless zero where the middle ratio is set, so it cannot be what keeps
that ratio above 2. The ratio is above 2 because κ(ρ) is minimized at 2.11 even in the
redundancy-free model.

---

## 7. Practical comparison (real Shell sort, N = 10⁶, 12 trials)

| sequence                                         | comparisons / N |
|--------------------------------------------------|-----------------|
| README best for 10⁶                              | **31.73**       |
| Ciura, extended by ×2.25                         | 31.96           |
| Tokuda                                           | 32.08           |
| geometric 2.2, generic rounding to the bottom    | 33.05           |
| document's N/2, N/6, N/12, …, generic rounding   | 33.36           |
| geometric 2.1, generic rounding                  | 33.56           |
| geometric 2.0, generic rounding                  | 33.63           |
| Sedgewick 1986                                   | 40.37           |
| Papernov–Stasevich 2^k + 1                       | 63.41           |
| Hibbard 2^k − 1                                  | 63.87           |
| document's sequence, exactly divisible (N = 786,432) | ≈ 520 (491–557 over 4 seeds) |

At N = 10⁶, more than half the cost is in the correlated small-gap passes, where tuned
prefixes such as 1, 4, 10, 23, 57, 132, 301, 644, … matter more than the middle ratio.
The middle ratio becomes the dominant term only as N grows.

---

## 8. Status of the claims

**Proven here.**
- Exact pass-2 values, for both generic and divisible second gaps.
- Theorem 1: exactness of the band model for generic gaps over any fixed number of
  passes.
- The information identities and inequalities in §3.1–3.2.
- The redundancy criterion and the "stopping comparisons only" property (§6.2).
- The counterexample to §11.4.

**Numerical, with high confidence.**
- The overcount factor of about 1.84 in the idealized model.
- The shape of κ(ρ), with ρ\* ≈ 2.11 and κ\* ≈ 2.795, in the band model.
- The cusp penalties.
- The pattern result.
- The agreement between real Shell sort and the band model for large gaps.
- The band model reproducing the README sequences' growth, with a constant
  correlation saving (§5.3).

**Conjectural.**
- E[C] ≈ κ\*·N·ln N − A·N for the best sequences, with the README's slope converging to
  κ\* of the band model. This needs two things, and the evidence for each is good but
  not a proof:
  - (a) The correlated small-gap regime must contribute only O(N). For the README
    sequences the saving is 2.04 at 10⁷ and 2.06 at 10⁸ comparisons per element (§5.3).
  - (b) Real Shell sort must keep following the band model in the middle passes. At 10⁷
    it does to within 0.07% for every gap ≥ 31,719. The point where it departs grows
    slowly with N, roughly doubling per decade (§2.2). Whether that point eventually
    takes in a fixed fraction of all passes is unresolved.

**Open.**
- A rigorous treatment of the correlated regime (gaps ≲ 10³): when correlations help (the
  last passes) and when they hurt (near-resonant gaps, §2.3).
- A rigorous existence and stability proof for the band model's steady state at a
  non-integer ratio, and an exact computation of κ\* from it, without simulation.

---
