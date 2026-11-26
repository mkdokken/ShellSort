# Worst-case-optimal Shellsort gap sequences: code and data

Code, results and write-ups behind the worst-case table in the main [README](../README.md): the gap
sequences that minimise the **worst-case** number of comparisons of Shellsort (gapped insertion sort) on N
distinct keys.

* **N = 6 … 45: exact.** An exhaustive branch-and-bound over all 2^(N−2) gap tables, with exact dynamic
  programming over threshold sets, proves the optimum and the complete list of tied tables at every size.
* **N = 46: optimum proved, tie list incomplete.** The optimum is 394, attained by {1,4,9,11,21} and three
  certified ties. Three further tables ({1,4,7,9,26}, {1,4,7,9,26,41}, {1,4,7,9,26,45}) also reach 394, but
  their exact DP needs 45–61 GB, so whether they tie is open.
* **N = 64 … 512: lower bounds only**, from long annealing attacks on the best candidates (every value is
  attained by a stored input).

**Start here:** [docs/EXHAUSTIVE.md](docs/EXHAUSTIVE.md) explains in plain terms how the exhaustive search gets
to N = 46, where the direct approach stops in the low twenties. [REPORT.md](REPORT.md) (also as
[report.html](report.html)) has all results. [docs/METHODS.md](docs/METHODS.md) has the algorithms and proofs,
[docs/RATIONALE.md](docs/RATIONALE.md) the structure of the optima, and [docs/LARGE_N.md](docs/LARGE_N.md) with
`docs/CAMPAIGN_*.md` the large-N searches.

Where these documents compare with "the README" (or "the reference"), they mean its worst-case table at revision
[e75ab4a](https://github.com/mkdokken/ShellSort/blob/e75ab4a058fcaaa775a16ff71a2510981993245e/README.md), the
version current when this work began on 2026-10-27. The corrections and bounds found here have since been
added to the README, so they are no longer differences from the current table.

## What is proved, and what to trust

* Every optimum and every tie for N ≤ 45 (and the optimum at N = 46) is proved under the correctness of the C
  programs here. They are not formally verified. The DP was checked against brute force over all N! inputs up
  to N = 12, with spot checks at N = 13 (METHODS §1.6).
* Every reported value, exact or lower bound, is attained by a stored input permutation. `scripts/verify.py`
  re-simulates all of them in pure Python: 842 inputs, no contradictions.
* The inputs that eliminate the losing tables during the search are not archived. To re-check the
  exhaustive part, re-run the search (see below).

## Layout

| path | contents |
|---|---|
| `src/core.h` | comparison-counting Shellsort, sequences, RNG |
| `src/exact.h` | exact worst case: threshold-set DP with the first pass decoupled (+ exact average) |
| `src/exact2.h` | pair-pass absorption + back-fill lemma; parallel dense DP (`dp2p`) |
| `src/layered.h` | two-level, one-byte layered DP with memory planning and disk-backed input reconstruction |
| `src/anneal.h`, `src/fastsa.h`, `src/vlns.h` | lower bounds: annealing, parallel tempering (`fsa_pt`), incremental evaluation, exact window re-ordering |
| `src/beam.h` | beam search over the threshold-set DP (cross-check; exact at N = 46, weak at N = 64) |
| `src/shellwc.c` | CLI: `eval brute dp dp2 dp2p lay layr xavg avg sa wc fwc pt beam ils batch semi size fsatest` (`BATCH_PT` env: tempering in `batch`) |
| `src/seqsearch.c` | parallel branch-and-bound over gap sequences with witness-certified pruning |
| `scripts/run_search.py` | exhaustive search + certification loop for a range of N (exact) |
| `scripts/manual_cert.py`, `witnesses.py`, `reconstruct.py` | settle leftover candidates (layered DP, front pair rules, or a witness input); attach / rebuild worst-case inputs |
| `scripts/ties46.ps1`, `test_rules.py` | Luby-restart attacks on the N = 46 possible ties; validation of the front pair rules against the general DP |
| `scripts/transfer.py`, `tournament.py`, `race.py`, `challenge.py`, `attack_all.py`, `longrun.py` | large-N search and adversarial comparisons |
| `scripts/round3.py`, `crosseval.py`, `scan64.py` | calibrated Luby-restart races; cross-evaluation of stored inputs; N = 64 family scan |
| `scripts/famscan.py`, `famreport.py`, `rescreen.py`, `maxout.py`, `prefixes.py` | family screens (successive halving; `--plan nbr` refines around finalists or a ranked list) → `results/famscan_N*.json`; long single-thread reruns of screen finalists → `results/rescreen_N.json`; fixed-budget attacks (parallel tempering, Luby restarts, or one long anneal per thread) → `results/maxout.json`; cross-N prefix table ([docs/CAMPAIGN_64_91.md](docs/CAMPAIGN_64_91.md), [docs/CAMPAIGN_128_181.md](docs/CAMPAIGN_128_181.md), [docs/CAMPAIGN_256.md](docs/CAMPAIGN_256.md), [docs/CAMPAIGN_362.md](docs/CAMPAIGN_362.md), [docs/CAMPAIGN_512.md](docs/CAMPAIGN_512.md)) |
| `scripts/avgopt.py` | worst case of the README's average-optimal sequences (exact for N ≤ 46, lower bounds above; `fixed` runs a fixed-budget attack as in `maxout.py`, `effort` compares the moves spent with those on the best worst-case sequence of each size, [docs/CAMPAIGN_AVG.md](docs/CAMPAIGN_AVG.md)) → `results/avgopt.json` |
| `scripts/verify.py` | independent pure-Python re-simulation of every stored worst-case input |
| `scripts/make_report.py`, `make_html.py`, `analyze.py`, `compare.py` | reports |
| `scripts/export_worst_case.py` | copies the publishable files into a folder and checks their sizes |
| `results/summary.json` | exact optima N = 6 … 46 with ties, averages, worst-case inputs and search statistics |
| `results/bb_N*.txt` | branch-and-bound output per N: search statistics and the surviving tables |
| `results/manual_cert.json` | tables settled after the search (N = 43 … 46): exact DP values or inputs above the optimum |
| `results/strong.json`, `transfer*.json`, `race.json`, `maxout.json`, `famscan_*.json` | large-N lower bounds (every value has a stored input) |

## Build and run

Built and run on Windows with MinGW-w64 gcc 8 (from Code::Blocks) and Python 3; the Python scripts use only the
standard library. Other platforms are untested. `src/layered.h` uses the Windows call `_fseeki64` (`fseeko`
elsewhere).

```
powershell -File build.ps1
bin\shellwc.exe dp2 36 1,4,7,9,19,34                  # exact worst case 275 + an input attaining it
bin\shellwc.exe lay 45 1,4,9,11,21                     # 2.5e10-state certification (5 GB): 380
bin\seqsearch.exe 24                                   # all optimal sequences for N = 24
python scripts\run_search.py 6 30                      # exhaustive search + certification
python scripts\verify.py                               # re-check every stored worst-case input
python scripts\race.py 128 1661 "1,4,9,11,30,47,59;1,4,9,11,30,36,59" --iters 2000000 --reps 127 --fresh --tag luby
```

The race command is a calibrated race: `--tag luby` selects the Luby restart schedule, and `--fresh` re-attacks
the runner-up, whose 1661 is already known, so a survival of the README sequence means something.

Running times on 24 threads (Ryzen 9 7845HX): N ≤ 35 takes seconds to minutes per size, N = 40 about 15
minutes, N = 45 1.6 hours (8.6 GB) and N = 46 2.3 hours (11 GB). Compile with `-mno-avx512f` (gcc 8 + Zen 4
SEH bug). All DPs refuse to use more than 12 GB.

## How this was made

The code, searches and write-ups in this folder were produced with Claude (Anthropic), an AI assistant, working
under my direction: I set the goals and reviewed the results.
