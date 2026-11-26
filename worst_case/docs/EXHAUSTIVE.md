# How the exhaustive search reaches N = 46

This note explains, without code, how the worst-case table was proved exhaustively for N = 6 … 45 (optimum and
every tie) and for the optimum at N = 46. The full technical description is in [METHODS.md](METHODS.md); the
programs are `src/seqsearch.c` (search over gap tables), `src/exact.h`, `src/exact2.h` and `src/layered.h`
(exact worst case of one table), driven by `scripts/run_search.py`.

## The two walls

Proving the table at size N means answering two questions, and both are exponential:

1. **The worst case of one gap table** is a maximum over all N! inputs (46! ≈ 5.5·10⁵⁷).
2. **The best table** must be found among all 2^(N−2) tables (gap 1 is compulsory, and every gap from 2 to
   N−1 is either in or out): 2,097,152 tables at N = 23, 17.6 trillion (1.76·10¹³) at N = 46.

The direct exhaustive method treats the two walls separately. For (1) it runs a dynamic program over the
2^N subsets of positions, and for (2) it checks every table one by one. At N = 23 that means 8.4 million DP
states and 2.1 million tables, which is easy. At N = 46 it would mean 70 trillion states per DP and 17.6
trillion tables, which is hopeless. That is why the direct method stops in the low twenties.

The search here gets through each wall with its own idea:

* **Wall 2: eliminate whole families of tables with one input (§2).** At N = 46 the search visits 4.5 million
  families and tables instead of checking 17.6 trillion tables.
* **Wall 1: make the exact computation small (§1).** Only the few tables that survive the elimination need
  their exact worst case. For them the DP runs on a much smaller state space: 3.3·10¹⁰ states for the N = 46
  winner, stored at one byte per state.

## 1. The worst case of one table

### 1.1 The worst input is a heaviest path

Build an input by placing the values 0, 1, …, N−1 one after another into positions. When value v goes into
position p, the comparisons v will cost over all passes are already fixed. They depend only on the set S of
positions holding the smaller values 0 … v−1 and on p. They do not depend on how the smaller values are ordered
among themselves, nor on the larger values placed later.

The reason is that in each pass, v costs one comparison for every larger element it moves past in its chain,
plus one more if a smaller element stops it. Where v sits before each pass is fixed by the 0/1 pattern "smaller
than v / not smaller than v". Sorting a chain of 0s and 1s just moves its 0s to the front, so that pattern after
every pass follows from S and p alone.

So every input is a path ∅ → {p₀} → {p₀, p₁} → … → {all positions}. Its cost is the sum of the step costs, and
the worst case is the heaviest such path. That path is found by a DP over the 2^N subsets. This is the standard
formulation, also used in the paper attached to issue #2.

### 1.2 Pay for the first pass in advance

The first pass (largest gap h₁) sorts each of its h₁ chains separately, and two facts follow:

* On a chain of m elements this pass costs at most m(m−1)/2. It reaches that maximum when the chain is in
  decreasing order, whatever values the chain holds.
* The later passes only see the output of the first pass, in which every chain is sorted. So they depend only
  on which values each chain received.

Hence W = Σ m(m−1)/2 + (the worst case of the remaining passes over all h₁-sorted arrays). The DP state only
needs to record how many of the values placed so far went into each first-gap chain, since they fill a sorted
chain from its start.

For {1,4,9,11,21} at N = 46, the 21 chains hold 3 or 2 elements, so the state is 21 small counters:
4⁴ · 3¹⁷ = 3.3·10¹⁰ states instead of 2⁴⁶ = 7.0·10¹³. This reduction goes back to Lee's search for N ≤ 16 and
is also in the issue #2 paper.

### 1.3 Skip over "pair passes"

A gap h ≥ N/2 has chains of at most two elements. Its pass is N − h compare-exchanges and always costs exactly
N − h. As a first pass it would make §1.2 useless, because almost every chain has one or two elements: for
{1,4,9,11,21,45} at N = 46 the counters of the 45-pass give 3 · 2⁴⁴ = 5.3·10¹³ states.

Instead the DP adds the constant N − h and pays the *next* pass in advance. The arrays such a pass can output
are exactly those with x[i] < x[i+h] for each of its pairs, so the DP also tracks which of those pairs already
hold their values. An exchange argument (the "back-fill lemma", METHODS §1.3) keeps that extra state small, and
{1,4,9,11,21,45} at N = 46 runs on 5.6·10¹⁰ states.

A refinement, "front pair rules", covers pair passes whose pairs join the start of one chain to the end of
another. They need no extra state at all. This is how {1,4,9,11,21,44} and {1,4,9,11,21,44,45} were certified at
N = 46.

### 1.4 One byte per state, two levels at a time

Each step places one value, so the DP runs level by level (level L = L values placed) and keeps only two levels
in memory. In every pass, each value still to be placed costs at least one comparison unless it lands on a chain
head. That gives a floor for the remaining cost at each level, and the difference between the best cost and the
floor fits in one byte.

For {1,4,9,11,21} at N = 46 that comes to 3.3·10¹⁰ states, 6.6 GB of memory and 15 minutes on 24 threads. The
run can also write its levels to disk, so that an input attaining the maximum can be reconstructed afterwards.
Every such input is then re-simulated by an independent Python Shellsort.

## 2. The best table: eliminating whole families

### 2.1 The family lemma

Group the tables by their *smallest* gaps, the ones used in the last passes. For a set P = {1 = p₁ < … < p_m},
the family F(P) consists of P together with any set of extra gaps larger than p_m. That is 2^(N−1−p_m) tables.

Call an input **p_m-local** if every inversion in it spans at most p_m positions, that is, any two elements more
than p_m apart are already in order. Feed such an input to any table of F(P):

* The passes with gaps larger than p_m come first.
* In each of those passes, consecutive chain elements are more than p_m apart, so every chain is already
  sorted. The pass makes exactly N − h comparisons and moves nothing.
* The passes of P then run on the untouched input.

So every table G = P ∪ Q in the family costs

    cost_G(x) = cost_P(x) + Σ_{h ∈ Q} (N − h)  ≥  cost_P(x).

If cost_P(x) is larger than a worst case W already proved by DP for some table, every table of F(P) is worse
than that table. The whole family is eliminated by one input and one short simulation.

**Example (N = 10, where the best value is 35, proved for {1,6,9}).** F({1,2,3,4}) is the 32 tables whose four
smallest gaps are 1, 2, 3 and 4. The input x = [0,1,3,2,6,8,9,4,5,7] has no inversion spanning more than 4
positions, and the passes 4, 3, 2, 1 cost 39 on it. Extra gaps from 5 … 9 only find sorted chains:

* {1,2,3,4,9} costs 39 + 1 = 40 on x;
* {1,2,3,4,5} costs 39 + 5 = 44;
* {1,2,…,9} costs 39 + 15 = 54.

All 32 tables exceed 35, so all are eliminated at once.

### 2.2 The search tree

The search walks a tree of these families. The root is {1}, and the children of P are P ∪ {g} for every
g > p_m. At each node the search does two things:

1. It tries to eliminate the whole family with a p_m-local input of cost greater than T, where T is the best
   value proved so far.
2. Otherwise it checks the table P itself (with no extra gaps), which any input of cost greater than T
   eliminates, and then descends to the children.

Two effects make most families die near the root:

* **Inheritance.** A p_m-local input is also g-local for every g > p_m, and on the child its cost grows by
  N − g: cost_{P∪{g}}(x) = cost_P(x) + N − g. So a child whose new gap g is small is often eliminated by its
  parent's input before any new search.
  At N = 10, for example, the best 3-local input for {1,2,3} costs 31. It eliminates {1,2,3,4} and {1,2,3,5}
  immediately, because 31 + 6 = 37 and 31 + 5 = 36 are both above 35.
* **Reuse.** Every eliminating input is kept in a pool and tried first on later nodes.

Inputs are found by greedy climbs and by annealing restricted to local inputs. Every elimination is confirmed
by a plain re-simulation of its input.

### 2.3 Why the result is rigorous

T only ever drops to a value that an exact DP has proved for some table. A table or family is eliminated only by
a concrete input whose cost is strictly above T. So when the tree is exhausted, every table that was not
eliminated is either certified by an exact DP or still listed as open.

The tables tied at the optimum survive by construction, because no input pushes them above it. The search
starts from a value T₀ at or slightly above the optimum and is re-run whenever a better value is certified
(`scripts/run_search.py`), so the list of tables attaining the optimum is complete.

### 2.4 How much this saves

From `results/summary.json` (final search round at each size):

| N | tables 2^(N−2) | tree nodes visited | families eliminated whole | tables eliminated by an input | exact DPs in the search | search time |
|---:|---:|---:|---:|---:|---:|---:|
| 17 | 32,768 | 1,259 | 1,044 | 164 | 51 | < 0.1 s |
| 20 | 262,144 | 3,912 | 3,380 | 454 | 78 | 0.1 s |
| 23 | 2,097,152 | 8,344 | 7,412 | 846 | 86 | 0.5 s |
| 30 | 268,435,456 | 64,927 | 58,067 | 6,381 | 479 | 170 s |
| 35 | 8.6·10⁹ | 186,022 | 164,633 | 21,368 | 21 | 19 s |
| 40 | 2.7·10¹¹ | 829,261 | 706,207 | 123,047 | 3 | 294 s |
| 45 | 8.8·10¹² | 2,915,067 | 2,469,804 | 445,248 | 2 | 1,046 s |
| 46 | 1.8·10¹³ | 4,474,424 | 3,743,222 | 731,186 | 0 | 1,858 s |

At N = 23 the search handles 8,344 nodes instead of 2.1 million tables. At N = 46 it handles 4.5 million
instead of 17.6 trillion. Tables whose exact DP is too large to run inside the search go to a separate
certification step: large-memory DPs, or targeted searches for an input above the optimum
(`scripts/run_search.py`, `scripts/manual_cert.py`). Search times are not monotone in N: at N = 25–31 several
hundred exact DPs run inside the search itself.

### 2.5 What stops it at N = 46

* Some surviving tables need DPs of 17–61 GB. Three possible ties at N = 46 — {1,4,7,9,26}, {1,4,7,9,26,41}
  and {1,4,7,9,26,45} — stay open for that reason. Each reaches 394, but none could be computed exactly within
  12 GB.
* At N = 64 even the best tables need 10¹³–10¹⁷ DP states (1.5 TB to 14 PB of memory).
* The family bound also weakens at N = 64: families built from small gaps, such as {1,3,…} or {1,4,5,…}, stay
  open for hours.

## 3. What is trusted, and what can be re-checked

* **The C code is not formally verified.** The DP was checked against brute force over all N! inputs, on
  random tables up to N = 12 and with spot checks at N = 13. The faster DP variants were checked against the
  plain DP (METHODS §1.6).
* **Every optimum has a stored input that attains it.** These inputs are re-simulated by a separate
  pure-Python Shellsort (`scripts/verify.py`: 842 stored inputs across all sizes, no contradictions).
* **The eliminating inputs are not saved.** They are found during the search and used immediately, so
  there is no certificate file to replay. To re-check the result, re-run the search: minutes up to N = 40,
  and 1.6–2.3 hours with up to 11 GB of memory at N = 45–46. The search could be extended to write such a
  certificate, with one record per tree node: about 8,300 records at N = 23, instead of the 2.1 million rows
  needed when every table is listed.
* **The DP tables themselves are not stored** (tens of GB each). The maxima are as trustworthy as the DP
  code that computed them.

## 4. Relation to the certificates of issue #2

Both approaches use the path formulation (§1.1) and the first-pass reduction (§1.2). The certificates of
issue #2 list every table individually: a witness or a baseline bound for each losing table, and an exact DP
with a potential function for each winner. That keeps the checker simple, and a checker proved sound in Lean
can replay them. The cost is that a certificate is as large as the number of tables, 2^(N−2).

The family lemma (§2.1) would let a certificate exclude a whole family with one record: 1,259 records instead of
32,768 at N = 17, and 8,344 instead of 2,097,152 at N = 23. The lemma should be easy to formalize on top of an
operational Shellsort model, since all it needs is this fact: a pass over chains that are already sorted makes
exactly N − h comparisons and changes nothing.

The winners' upper bounds are a separate cost. A potential over all 2^N subsets stays large unless the
first-pass reduction is formalized too, which would bring it down to the counter states of §1.2.

## 5. Reproducing

Build with `powershell -File build.ps1` (Windows, MinGW-w64 gcc 8, OpenMP; 24 threads were used). Then:

```
python scripts\run_search.py 6 44         # search + certification, results/summary.json
bin\shellwc.exe lay 45 1,4,9,11,21        # the N = 45 winner: 2.5·10¹⁰ states, 5 GB, about 11 min
python scripts\verify.py                  # re-simulate every stored worst-case input
```

`scripts/manual_cert.py` (`run` and `rules`) handles the tables whose DP exceeds the driver's state cap
(N = 43–46); `results/manual_cert.json` lists what it certified or excluded.

| N | wall-clock per size (24 threads) |
|---|---|
| ≤ 35 | seconds to a few minutes |
| 36–40 | 8–19 min |
| 41–44 | 35–82 min |
| 45 | 1.6 h (about 19 core-hours), peak memory 8.6 GB |
| 46 | 2.3 h (about 28 core-hours), peak memory 11 GB, plus 11–12 min for each front-pair-rules certification |

Every DP refuses to use more than 12 GB.
