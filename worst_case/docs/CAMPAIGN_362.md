# Large-N campaign, 2026-10-31/11-01: N = 362 (20 h)

*A record of the campaign as run. "README" and "reference" here mean the README's worst-case table at revision
[e75ab4a](https://github.com/mkdokken/ShellSort/blob/e75ab4a058fcaaa775a16ff71a2510981993245e/README.md), the
version current when this work began; the README has since been updated with the results found here.*

Same goals as the earlier campaigns (`docs/CAMPAIGN_64_91.md`, `docs/CAMPAIGN_128_181.md`,
`docs/CAMPAIGN_256.md`): look for sequences, families and prefixes that earlier searches missed, and push the
lower bounds of the top candidates up with longer runs. Every value is the cost of a stored, re-simulated input
(`results/strong.json`); a sequence's true worst case can only be higher. Lower = better sequence.
Gaps ≤ 181 (reference convention N/2).

## Engines at this size

* **Throughput.** About 0.62M annealing moves per second per thread (24 threads), against 1.0M at N = 256.
  At 128M moves per thread an 8-gap sequence took 96–104 s, a 9-gap one 160–243 s and a 10-gap one 240–289 s.
* **Screening runs** (2M moves, one thread per sequence; 24 known sequences, 4 seeds): mean 6122.0 / 6197.9 /
  6222.9 / 6224.2 / 6220.3 / 6205.9 for T₀ = 4.5 / 6 / 9 / 13 / 18 / 25. T₀ = 9–18 are level; the screens
  anneal from T₀ = 13.
* **Long runs** (24 threads, two sequences: the README sequence {1,4,9,11,30,36,70,107,163} and
  {1,4,9,11,30,47,59,127,150}). At 100M moves per thread, Luby restarts with 4M-move base runs from T₀ = 6
  gave 6437 / 6453, 8M from T₀ = 9 6443 / 6485, 16M from T₀ = 13 6459 / 6492 and 32M from T₀ = 13 6483 / 6502.
  At 128M moves per thread, restarts of 32M + 32M + 64M gave 6484 / 6499, two runs of 64M 6516 / 6541, and one
  anneal of 128M 6521 / 6560. Longer anneals won at every step, so every round below gives each thread one
  anneal of its whole budget (`maxout.py --base` = `--moves`), from T₀ = 13. Both sequences were race-stopped
  at 6450 before (a tie, METHODS §2.2); 128M moves per thread already put them 71 and 110 higher.
* **Run-to-run spread.** 24 single-thread anneals of {1,4,9,11,30,47,59,127,150} gave 6229–6424 at 16M moves
  (median 6328) and 6334–6516 at 48M (median 6451). One run says little about a sequence, and two runs only
  separate sequences that differ by more than about 100.

## Exploration: 40,509 sequences in two screens, then a long-run rescreen

* **Main screen** (37,304 sequences, 156 min; 1 run of 2M moves for all, 3 runs of 4M for the best 2,500,
  8 runs of 8M for the best 300), built on the N = 256 attacks, not on the README: the four best N = 256
  seven-gap prefixes ({1,4,9,11,30,47,67}, {…,36,59}, {…,36,62}, {…,36,70}) with 2,000 random 9-gap and 400
  random 10-gap completions each; {1,4,9,11,30,36} and {1,4,9,11,30,47} with 800 / 3,000 / 1,000 random 8- /
  9- / 10-gap completions; {1,4,9,11,30} with 600 / 3,000 / 600; the 14 non-README N = 256 sequences with
  bounds ≤ 4193 after at least 1G moves, each extended by every possible ninth gap; {1,4,9,11,x} for 13 other
  fifth gaps (21–40) with 80 / 250 / 80; {1,4,9,15,x} for x = 29, 31, 32, 34, 35, 37 with 100 / 500 / 100
  (200 / 1,200 / 300 for 31 and 35) and {1,4,9,15,31,56,74} (the best N = 256 member of that core) with 800
  9-gap completions; 300 random 9-gap members each of {1,4,7,9}, {1,4,11,13} and {1,3,8,10}. The README
  sequence was drawn by chance among the {1,4,9,11,30,36,70} completions.
* **Refinement** (3,481 sequences, 38 min, runs of 4M / 8M / 16M) around the 40 best main-screen finalists
  (gaps moved by ±1–3, one removed, or one inserted on a grid of 3): 3,205 new sequences, 22 of them among its
  60 finalists.
* **Long-run rescreen** (new, `scripts/rescreen.py`, 19 min): the 150 best finalists of each screen (178
  distinct sequences) got two single-thread anneals of 48M moves each, to pick the field for the long runs.

**Families.** First-stage statistics (one 2M-move run per sequence, which do not grow with the sample size):

| prefix (9-gap members only) | sequences | median | 1st percentile |
|---|---:|---:|---:|
| {1,4,9,15,31} | 2,045 | 6502 | 6132 |
| {1,4,9,11,30} | 17,514 | 6525 | 6110 |
| {1,4,9,15,35} | 1,200 | 6684 | 6258 |
| {1,4,9,15,29} | 500 | 6698 | 6274 |
| {1,4,9,11,32} | 312 | 6736 | 6173 |
| {1,4,9,15,34}, {…,32}, {…,37} | 500 each | 6759–6781 | 6260–6274 |
| {1,4,9,11,x}, x = 21, 25, 28, 34 | 250 each | 6788–6890 | 6336–6413 |

* **Cores.** {1,4,9,11} and {1,4,9,15} again lead by far (medians 6633 and 6682 over all members);
  {1,4,11,13} 7025, {1,4,7,9} 7321, {1,3,8,10} 7583. None of their 900 members reached the second stage.
* **Fifth gap.** {1,4,9,15,31} and {1,4,9,11,30} are level once more (as at N = 128–256); every other fifth
  gap is at least 150 behind in median.
* **Number of gaps.** Nine: for {1,4,9,11,30} the medians are 6826 / 6525 / 6552 for 8 / 9 / 10 gaps, for
  {1,4,9,15} 6786 / 6652 / 6744.
* **Sixth and seventh gaps.** Among 9-gap members of {1,4,9,11,30}, sixth gaps 36 (median 6427) and 47
  (6533) lead; the other sixth gaps (50–90 members each) have medians of 6587 or more. The best seven-gap
  prefixes are {1,4,9,15,31,56,74} (median 6323), {1,4,9,11,30,36,70} (6333), {…,47,75} (6344), {…,36,62}
  (6369), {…,36,59} (6380) and {…,47,67} (6410).
* **Finalists.** {1,4,9,11,30,47,67,·,·} dominates both screens' finals: best main-screen finalists
  {1,4,9,11,30,47,67,132,180} 6243 and {…,113,169} 6245, then the README sequence 6274 (third of 300).

**Short runs mislead at this size.** The rescreen's best ({1,4,9,11,30,47,67,113,169} and {…,115,174}, 6325)
came out among the worst of round A (6604 and 6611), and the README sequence, 88th of 178 in the rescreen
(6455), came out 10th of 34. Over the 32 round-A sequences that were rescreened, the rank correlation between
the rescreen and round A is −0.38: among sequences that pass a short screen, the best-looking ones tend to be
those whose bad inputs are hard to find, so their bounds rise most under attack. Round A therefore also took
in sequences by family rather than by screen score (round A2 below). Across that wider field the screen
still carries information. Over the 58 sequences of rounds A and A2 that the screens scored, the rank
correlation with the 128M value is +0.44 for the screen score (the best of up to 12 short runs) and +0.24 for
the family median, but only +0.04 for the rescreen's two 48M runs and −0.03 for a single 2M run. The screen
separates good sequences from bad ones; it cannot rank the finalists among themselves.

## Long runs on the top candidates

Every round gives each of the 24 threads one anneal of its whole budget from T₀ = 13, with no target
(`maxout.py --method luby` with `--base` = `--moves`):

* **A**: 128M moves per thread for 34 sequences. These are 31 of the rescreen's 42 best (all of its best 27,
  its two best with 10 gaps and three {1,4,9,15} members in all), the README sequence (88th in the rescreen),
  and two earlier race candidates that missed the screens' finals, {1,4,9,11,30,47,70,131,179} and
  {…,36,70,97,159}.
* **A2**: 128M for 27 sequences picked by family instead of by screen score. These are eleven other ninth gaps
  after {1,4,9,11,30,47,67,95} (the prefix of round A's leader), three other {…,47,67,·,·} pairs, six
  members of {1,4,9,11,30,36,70,·,·}, three of {1,4,9,15,31,56,74,·,·} and four of {…,36,59,·,·} and
  {…,36,62,·,·}.
* **A3**: 128M for the three best sequences known only from the old transfer searches.
* **B**: 512M for the best 12 of A–A3 and the README sequence.
* **C**: 1G for the best 6 after B and the README sequence.
* **D**: 2G for the best 3 after C (and, after E, for the README sequence as well).
* **E**: 4G for the best 2 after D.

Column A holds the round A or A2 value (128M moves per thread).

| sequence | A | B | C | D | E | stored best (≥) |
|---|---:|---:|---:|---:|---:|---:|
| {1,4,9,11,30,36,70,109,171} (screen) | 6481 | 6480 | 6517 | 6503 | 6521 | **6521** |
| {1,4,9,11,30,36,70,117,143} (screen) | 6493 | 6518 | 6523 | 6523 | 6532 | 6532 |
| {1,4,9,11,30,36,62,121,161} (screen) | 6506 | 6512 | 6515 | 6537 | – | 6537 |
| {1,4,9,11,30,36,70,97,159} (earlier candidate) | 6494 | 6539 | – | – | – | 6539 |
| {1,4,9,11,30,47,67,132,180} (screen) | 6497 | 6540 | – | – | – | 6540 |
| {1,4,9,11,30,47,67,95,169} (screen) | 6505 | 6544 | – | – | – | 6544 |
| {1,4,9,15,31,56,74,126,177} (screen) | 6496 | 6532 | 6544 | – | – | 6544 |
| {1,4,9,11,30,36,59,98,162} (screen) | 6513 | 6545 | – | – | – | 6545 |
| README {1,4,9,11,30,36,70,107,163} (also drawn by the screen) | 6508 | 6540 | 6556 | 6576 | – | 6576 |
| {1,4,9,11,30,36,70,133,159} (screen; earlier candidate) | 6512 | 6585 | – | – | – | 6585 |
| {1,4,9,11,30,47,67,95,160} (screen) | 6489 | 6536 | 6597 | – | – | 6597 |
| {1,4,9,11,30,47,67,95,172} (screen) | 6465 | 6636 | – | – | – | 6636 |
| {1,4,9,11,30,47,67,95,152} (screen) | 6479 | 6511 | 6652 | – | – | 6652 |

* **Lowest bound at N = 362: {1,4,9,11,30,36,70,109,171} ≥ 6521.** This is a new sequence, the main
  screen's 8th finalist. It extends {1,4,9,11,30,36,70,109} (≥ 4180, fourth at N = 256) by one gap. Its five
  runs (7.6G moves per thread in all) gave 6480–6521, so its bound rose by only 40 from 128M to 4G. Next come
  {…,36,70,117,143} ≥ 6532 (7.6G) and {…,36,62,121,161} ≥ 6537 (3.6G), both also new.
* **The README sequence {1,4,9,11,30,36,70,107,163} is now ≥ 6576**, 55 above the leader and 390 above
  its published 6186. Its stored 6450 was race-stopped, a tie with every contender (METHODS §2.2). The
  engine checks then reached 6437–6521 (100M–128M moves per thread), and rounds A–D reached 6508, 6540, 6556
  and 6576 (3.6G moves per thread in all, as much as the third sequence).
* **The ranking.** The top three are within 16 of each other after at least 3.6G moves each, so their order
  is a ranking at this effort, not a proof. The README sequence's 55-point deficit is the clearer result. Four
  sequences have lower bounds than it after at least 1.6G moves each and four more after 0.64G. Many more do
  after one 128M round only, but those bounds are not comparable (128M bounds rose by up to 173 when
  attacked longer). The
  README sequence's bound rose by 68 between 128M and 2G moves per thread, about twice as much as the
  leaders' (31–40).
* **The family that dominated the screens collapsed under long attack.** {1,4,9,11,30,47,67,·,·} supplied
  the best finalists of both screens and round A's three best sequences: {…,95,172} 6465, {…,95,152} 6479
  and {…,95,160} 6489. By 1G (512M for the first) these stood at 6636, 6652 and 6597, 108–173 higher. Over
  the same range the three best {1,4,9,11,30,36,·,·,·} sequences rose by only 9–36. Round A2's other ninth gaps after
  {…,47,67,95} all came out ≥ 6521 at 128M. Moving the eighth gap from 95 to 96 gave 6615, and to 94 gave
  7218. Because 94 = 2 × 47, every pass after the 47-pass sees the same sorted distances with or without
  the 94-pass, so that pass is nearly wasted.
* **{1,4,9,15} stays behind.** Its best member, {1,4,9,15,31,56,74,126,177}, was 31 behind after round A,
  51 after B and 29 after C (6496, 6532, 6544). It ends 23 behind the leader's final 6521, with 1.6G moves
  per thread against the leader's 7.6G.
* **Earlier bounds were far too low.** The earlier race-stopped candidates rose by 31–110 in their first
  128M-move runs: {1,4,9,11,30,47,59,127,150} and {…,47,70,131,179} went 6450 → 6560,
  {…,36,70,113,175} 6453 → 6517, {…,36,70,133,159} 6458 → 6512 (then 6585 at 512M) and {…,36,70,97,159}
  6463 → 6494 (then 6539 at 512M). Round A3's three transfer-search sequences rose by 129–202:
  {1,4,9,11,30,47,78,135} went 6485 → 6687, {1,4,9,15,34,56,125,141} 6507 → 6636 and
  {1,4,9,15,25,53,82,146} 6524 → 6705.

## What this suggests for larger N

1. **Prefix.** {1,4,9,11,30} has now been the best 5-gap prefix under attack at N = 91, 128, 181, 256 and
   362. At N = 362 the sequences that hold up under long attack continue it with a sixth gap of 36, then 62
   or 70, and two more gaps (9 gaps in all). The leader extends {1,4,9,11,30,36,70,109} (≥ 4180, fourth at
   N = 256) by one gap, and its last two ratios are both about 1.57 (109/70, 171/109). The README's N = 362
   sequence also starts {1,4,9,11,30,36,70}. A search at N = 512 (gaps ≤ 256) can start from
   {1,4,9,11,30,36,62|70,·,·,(·)} with 9–10 gaps, for example the N = 362 leaders with their last gaps
   moved or a tenth gap added near 250.
2. **Screens only nominate; long attacks select.** The screens' favourite family, {1,4,9,11,30,47,67,·,·}
   (the prefix of the N = 256 leader), also led after round A, but its members rose by 108–173 by 1G moves
   per thread. The rank correlation between the 48M-move rescreen and the 128M round was −0.38. Short runs
   select for sequences whose bad inputs are rare, so at N ≥ 362 a screen can only nominate a broad field.
   Take the long-attack field by family (several members of each leading prefix), and give at least a dozen
   sequences ≥ 512M moves per thread before ranking them.
3. **{1,4,9,15} is not closing the gap.** It is level with {1,4,9,11,30} in the screens' medians, but its best
   attacked member is 31 behind after the first round at N = 362 and 23 behind at the end (with less
   attack). At N = 128–256 the first-round gap was 7–23.
4. **Budgets.** One anneal of the whole budget per thread beat restarts at every length tested. At N = 362 a
   1G-move anneal of one 9-gap sequence takes 21–30 minutes on 24 threads and a 4G one 1.5–1.7 hours. The
   three leaders' bounds still rose by 31–40 between 128M and their last round (2G or 4G), and other
   sequences' by up to 173.
   Throughput fell from 1.0M to 0.62M moves per second per thread between N = 256 and 362 (about N^1.4).
   At N = 512 that predicts about 0.38M: 45 minutes per 1G-move anneal and about 3 hours per 4G. A campaign
   of this shape would need well over 20 hours there, mostly for the long attacks.
