# Average-optimal sequences under matched effort, 2026-11-03/05: N = 64 … 512

The README lists for each N the sequence with the lowest *average* and a worst case for it (lower bounds for
N ≥ 91). `scripts/avgopt.py` had already re-attacked these sequences for N = 64 … 362 with Luby restarts
(REPORT §3), but with only 9–70% of the effort the best worst-case sequences got in the races and fixed-budget
rounds. Here each average-optimal sequence gets as many annealing moves per thread as the best worst-case
sequence of its size received in total, so that the two lower bounds rest on the same effort.

**N = 512.** The README's best-average sequence there is {1,4,10,23,57,189,484}. Its author has since found
with more samples that {1,4,10,23,57,189,483} has the slightly lower average, so only that sequence is attacked
(`readme_data.USER_AVGSEQ`). The README gives no worst case for it.

**Result.** At equal effort the average-optimal sequences stay 15–33% above the best worst-case sequences in the
worst case (+14.6% at N = 64 rising to +32.6% at N = 256, and about +28% at N = 181, 362 and 512). Their bounds rose
at four sizes (N = 128: 2016 → 2017, 181: 3387 → 3390, 256: 5479 → 5532, 362: 8248 → 8335). At N = 64 and 91 they
stay at the README values 723 and 1244, and N = 512 gets its first bound, ≥ 12973.

## Effort accounting

Effort is counted in annealing moves per thread (every run in this project used all 24 threads), summed over
every run on the sequence: fixed-budget rounds (`maxout.py`), calibrated races and attacks, and the earlier
`avgopt.py` runs. Runs are counted by the moves they actually made:

* a Luby schedule ends with the last restart that fits the budget, so a nominal 4G run with base 4M makes
  254 restarts and 3.584G moves;
* a race stops all threads when one reaches the target: it is counted by the restarts it ran, or, for the
  two long-run races that stopped early without a restart count, by its run time at the sequence's speed in
  its fixed-budget rounds.

Screens and beam searches (well under 0.1G per thread per sequence) are not counted. The best worst-case
sequence of a size is the one with the lowest stored bound among the sequences of fixed-budget rounds of ≥ 1G
moves per thread (as in the reports). At N = 64 four sequences are tied at 631, and the most attacked of them
sets the target. `python scripts/avgopt.py effort` prints the table; before this campaign it read:

| N | best worst-case sequence | its bound | its moves/thread | average-optimal sequence | its bound | its moves/thread | still to run |
|---:|---|---:|---:|---|---:|---:|---:|
| 64 | {1,4,9,11,21,57} (+3 tied) | 631 | 3.785G | {1,4,9,38,62} | 723 | 2.648G | 1.137G |
| 91 | {1,4,9,11,30,44} | 1045 | 15.012G | {1,4,9,33,86} | 1244 | 2.448G | 12.564G |
| 128 | {1,4,9,11,30,36,59} | 1661 | 11.828G | {1,4,9,24,85} | 2016 | 2.944G | 8.884G |
| 181 | {1,4,9,11,30,47,59,86} | 2646 | 17.554G | {1,4,10,21,70,176} | 3387 | 1.792G | 15.762G |
| 256 | {1,4,9,11,30,47,67,95} | 4171 | 6.960G | {1,4,10,27,89,238} | 5479 | 0.640G | 6.320G |
| 362 | {1,4,9,11,30,36,70,109,171} | 6521 | 7.640G | {1,4,10,23,67,236,355} | 8248 | 1.088G | 6.552G |
| 512 | {1,4,9,11,30,36,59,98,162,213} | 10119 | 8.640G | {1,4,10,23,57,189,483} | – | 0.000G | 8.640G |

The fixed-budget rounds alone come to 1.4G, 11.0G, 8.0G, 15.2G, 7.0G, 7.6G and 8.6G for the best worst-case
sequences; the races add 2.4G at N = 64, 4.0G at 91, 3.8G at 128 and 2.3G at 181 (none at 256 … 512, whose best
sequences came from the screens). After the campaign the last column reads 0.000G everywhere except at N = 181,
which ends 2M moves per thread (0.01%) short because Luby schedules end on a restart boundary.

## Runs

`python scripts/avgopt.py fixed N MOVES …` runs the attack of `maxout.py` (no target; every thread anneals the
whole budget) and records it in `results/avgopt.json`. Each size repeats the long rounds its best worst-case
sequence received, with the same method and settings, and the last run is cut so that the totals match:

* N = 64: one parallel-tempering run as in round C (R = 16, T ∈ [0.25, 2.5]), cut to 1.103G; two smoke tests
  (34M moves) are counted too.
* N = 91: both runs of round E (parallel tempering 4G, R = 16, T ∈ [0.3, 3.0]; Luby 4G, base 2M, T₀ = 3) and a
  third, tempering run of 4.724G.
* N = 128: rounds E (Luby 4G, base 2M, T₀ = 3), D (2G, base 4M, T₀ = 4.5) and C (1G, base 2M, T₀ = 3), and a
  second D cut to 2.048G.
* N = 181: rounds E, F and G (Luby 4G with base 8M, 4M and 8M, T₀ = 4.5) and a fourth run of 4.176G with base 4M.
* N = 256: rounds E (Luby 4G, base 4M, T₀ = 4.5) and D (2G, base 16M, T₀ = 9), and a second D cut to 0.736G.
* N = 362: rounds E and D (one anneal per thread of 4G and 2G moves from T₀ = 13) and a third cut to 0.552G.
* N = 512: all six rounds A–F (one anneal per thread of 128M, 512M, 1G, 2G, 4G and 1G moves from T₀ = 13).

The 24 runs took 32 hours, one after another (`logs/avgopt_large.log`):

| N | run | method | moves/thread | seed | time | moves/s/thread | bound (≥) |
|---:|---|---|---:|---:|---:|---:|---:|
| 64 | avgA | PT (R = 16, T ∈ [0.25, 2.5]) | 1.103G | 111 | 7 min | 2.68M | 723 |
| 91 | avgA | PT (R = 16, T ∈ [0.3, 3]) | 4.000G | 121 | 33 min | 2.02M | 1244 |
| 91 | avgB | Luby, 509 restarts of base 2M, T₀ = 3 | 3.840G | 122 | 34 min | 1.89M | 1244 |
| 91 | avgC | PT (R = 16, T ∈ [0.3, 3]) | 4.724G | 123 | 39 min | 2.02M | 1244 |
| 128 | avgA | Luby, 509 restarts of base 2M, T₀ = 3 | 3.840G | 131 | 27 min | 2.37M | 2017 |
| 128 | avgB | Luby, 155 restarts of base 4M, T₀ = 4.5 | 2.000G | 132 | 14 min | 2.39M | 2016 |
| 128 | avgC | Luby, 155 restarts of base 2M, T₀ = 3 | 1.000G | 133 | 7 min | 2.37M | 2015 |
| 128 | avgD | Luby, 157 restarts of base 4M, T₀ = 4.5 | 2.048G | 134 | 14 min | 2.38M | 2016 |
| 181 | avgA | Luby, 155 restarts of base 8M, T₀ = 4.5 | 4.000G | 141 | 82 min | 0.82M | 3390 |
| 181 | avgB | Luby, 254 restarts of base 4M, T₀ = 4.5 | 3.584G | 142 | 74 min | 0.81M | 3390 |
| 181 | avgC | Luby, 155 restarts of base 8M, T₀ = 4.5 | 4.000G | 143 | 82 min | 0.81M | 3388 |
| 181 | avgD | Luby, 268 restarts of base 4M, T₀ = 4.5 | 4.176G | 144 | 86 min | 0.81M | 3390 |
| 256 | avgA | Luby, 254 restarts of base 4M, T₀ = 4.5 | 3.584G | 151 | 1.7 h | 0.58M | 5532 |
| 256 | avgB | Luby, 54 restarts of base 16M, T₀ = 9 | 2.000G | 152 | 57 min | 0.59M | 5524 |
| 256 | avgC | Luby, 24 restarts of base 16M, T₀ = 9 | 0.736G | 153 | 21 min | 0.58M | 5514 |
| 362 | avgA | one anneal from T₀ = 13 | 4.000G | 161 | 4.2 h | 0.26M | 8335 |
| 362 | avgB | one anneal from T₀ = 13 | 2.000G | 162 | 2.1 h | 0.26M | 8312 |
| 362 | avgC | one anneal from T₀ = 13 | 0.552G | 163 | 35 min | 0.27M | 8260 |
| 512 | avgA | one anneal from T₀ = 13 | 0.128G | 101 | 13 min | 0.17M | 12782 |
| 512 | avgB | one anneal from T₀ = 13 | 0.512G | 102 | 49 min | 0.17M | 12926 |
| 512 | avgC | one anneal from T₀ = 13 | 1.000G | 103 | 1.6 h | 0.17M | 12960 |
| 512 | avgD | one anneal from T₀ = 13 | 2.000G | 104 | 3.2 h | 0.17M | 12959 |
| 512 | avgE | one anneal from T₀ = 13 | 4.000G | 105 | 6.4 h | 0.17M | 12973 |
| 512 | avgF | one anneal from T₀ = 13 | 1.000G | 106 | 1.6 h | 0.17M | 12934 |

## Results

| N | average-optimal sequence | README value | before (≥) | new runs | now (≥) | moves/thread | best worst-case sequence (≥) | moves/thread | excess |
|---:|---|---:|---:|---|---:|---:|---:|---:|---:|
| 64 | {1,4,9,38,62} | 723 | 723 | 723 | **723** | 3.79G | 631 | 3.78G | +14.6% |
| 91 | {1,4,9,33,86} | ≥ 1244 | 1244 | 1244, 1244, 1244 | **1244** | 15.01G | 1045 | 15.01G | +19.0% |
| 128 | {1,4,9,24,85} | ≥ 2014 | 2016 | 2017, 2016, 2015, 2016 | **2017** | 11.83G | 1661 | 11.83G | +21.4% |
| 181 | {1,4,10,21,70,176} | ≥ 3372 | 3387 | 3390, 3390, 3388, 3390 | **3390** | 17.55G | 2646 | 17.55G | +28.1% |
| 256 | {1,4,10,27,89,238} | ≥ 5462 | 5479 | 5532, 5524, 5514 | **5532** | 6.96G | 4171 | 6.96G | +32.6% |
| 362 | {1,4,10,23,67,236,355} | ≥ 8006 | 8248 | 8335, 8312, 8260 | **8335** | 7.64G | 6521 | 7.64G | +27.8% |
| 512 | {1,4,10,23,57,189,483} | – | – | 12782, 12926, 12960, 12959, 12973, 12934 | **12973** | 8.64G | 10119 | 8.64G | +28.2% |

"Before" is the best bound of the earlier `avgopt.py` runs, "new runs" lists each new run's bound in the order
of the runs table, and "excess" compares the bound now with that of the best worst-case sequence, which rests on
the same effort.

* **N = 64 and 91: no change.** Every new run stops exactly at the README value: 723 after 1.1G more moves of
  parallel tempering at N = 64, and 1244 in all three runs at N = 91 (tempering and Luby, 3.8–4.7G each).
  Together with the earlier Luby and fixed-restart runs, 3.8G and 15.0G moves per thread never found a worse
  input, so 723 and 1244 are very likely the true worst cases (not proved).
* **N = 128 and 181: +1 and +3.** The bounds are now 2017 and 3390. Three of the four runs at N = 181 stop at
  exactly 3390 and the fourth at 3388, and at N = 128 the four runs spread over 2015–2017. Both values look close to
  the true worst cases.
* **N = 256 and 362: +53 and +87.** Even the shortest new run at N = 256 (0.74G) beat the old 5479. At N = 362
  the single anneals reach 8260, 8312 and 8335 after 0.55G, 2G and 4G moves per thread, so the bound still grows
  with effort, as the best worst-case sequence's does there (6503 after 2G, 6521 after 4G). The true worst cases
  are probably higher still.
* **N = 512: first bound, ≥ 12973.** The six single anneals reach 12782, 12926, 12960, 12959, 12973 and 12934
  after 128M, 512M, 1G, 2G, 4G and 1G moves per thread. The best worst-case sequence drew 9982, 10027, 10089,
  10091, 10119 and 10066 in the same rounds. On both sides the bound rises by about 1.5% from 128M to 4G moves
  (+191 and +137), and the two 1G runs differ by about 25 (26 and 23). The two lower bounds behave alike under
  the same attack.
* **The worst-case price of the average, at equal effort**, is +14.6% at N = 64, +19.0% at 91, +21.4% at 128,
  +28.1% at 181, +32.6% at 256, +27.8% at 362 and +28.2% at 512. Before this campaign, when the average-optimal
  sequences had had 9–70% of the effort (none at N = 512), the same comparison read +14.6, +19.0, +21.4, +28.0,
  +31.4 and +26.5% for N = 64 … 362. Matching the effort widened the gap by about one percentage point at N = 256
  and 362 and by at most 0.1 elsewhere. For N ≤ 46, where both values are exact, the price is 0–27% (REPORT §3).
* **Equal moves, not equal seconds.** Per move the average-optimal sequences ran 1.9–2.7× slower than the best
  worst-case sequences at N = 91, 181, 256, 362 and 512, 1.1× slower at N = 128 and 6% faster at N = 64. The best
  worst-case sequences made 2.5, 3.7, 2.6, 1.6, 1.6, 0.66 and 0.42M moves per second per thread in their
  fixed-budget rounds. Matching moves therefore gave the average-optimal sequences more wall-clock time at most
  sizes: 13.8 h against 5.8 h at N = 512, for example. Every bound is attained by a stored input that
  `scripts/verify.py` re-simulates.
