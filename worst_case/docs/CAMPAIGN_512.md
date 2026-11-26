# Large-N campaign, 2026-11-01/02: N = 512 (28 h)

*A record of the campaign as run. "README" and "reference" here mean the README's worst-case table at revision
[e75ab4a](https://github.com/mkdokken/ShellSort/blob/e75ab4a058fcaaa775a16ff71a2510981993245e/README.md), the
version current when this work began, which ended at N = 362; the README has since been updated with the results
found here, including an N = 512 row.*

Same goals as the earlier campaigns (`docs/CAMPAIGN_64_91.md`, `docs/CAMPAIGN_128_181.md`,
`docs/CAMPAIGN_256.md`, `docs/CAMPAIGN_362.md`): look for good sequences, families and prefixes, and push the
lower bounds of the top candidates up with longer runs. Every value is the cost of a stored, re-simulated input
(`results/strong.json`); a sequence's true worst case can only be higher. Lower = better sequence. Gaps ≤ 256
(reference convention N/2).

**No reference sequence.** The README table ends at N = 362, so this size has no published sequence or worst
case. The candidates are compared among themselves, and every search here starts from this work's own N = 362
results (`famscan.py` `N362_BEST`: the sequences found by this work with bounds ≤ 6545 after ≥ 512M moves per
thread; the README sequence and the author's candidates excluded).

## Engines at this size

* **Throughput.** 0.29–0.44M annealing moves per second per thread (24 threads) for 10-gap sequences
  (0.34–0.70M for 9 gaps), against 0.62M at N = 362 and the 0.38M that campaign predicted. A 128M-move run
  took 3–6 minutes per sequence with 9 gaps, 5–7 with 10 and 8 with 11; with 10 gaps a 1G one took 39–53
  minutes and a 2G one 76–105.
* **Screening runs** (4M moves, one thread per sequence; 24 sequences built from the N = 362 leaders, 2 seeds):
  mean 9715.6 / 9857.7 / 9908.6 / 9889.9 / 9869.5 / 9867.8 / 9817.8 for T₀ = 4.5 / 6 / 9 / 13 / 18 / 25 / 35.
  The screen anneals from T₀ = 9.
* **Longer runs** (32M moves, one run per sequence): mean 10358.6 / 10292.2 / 10356.3 for T₀ = 6 / 9 / 13,
  level within the noise of one run per sequence and about 450 above the 4M-move runs. The rounds keep
  T₀ = 13 and give each thread one anneal of its whole budget, as at N = 362 (where longer anneals won at
  every length tested).
* **No rescreen.** At N = 362 the screen score (the best of up to 12 short runs) predicted the 128M-round
  values with rank correlation +0.44 over 58 sequences, while two extra 48M-move runs per finalist predicted
  nothing (+0.04; `docs/CAMPAIGN_362.md`). This campaign spends that time on the fixed-budget rounds instead,
  and picks their field from the screen score with a cap per family.

## Exploration: 15,990 sequences in one screen

* **Screen** (15,990 sequences, 117 min; 1 run of 2M moves for all, 3 runs of 4M for the best 1,201, 8 runs of
  8M for the best 150; `famscan.py 512`), built on this work's N = 362 results, not on the README:
  * the seven N = 362 leaders, each extended by every possible tenth gap and with its last gap replaced by
    every possible value;
  * every ninth gap and 500 random 10-gap completions after {1,4,9,11,30,36,70,109}, {…,36,70,117} and
    {…,36,62,121};
  * {1,4,9,11,30,36,70} and {…,36,62} with 500 / 1,500 / 400 random 9- / 10- / 11-gap completions;
    {1,4,9,11,30,36} with 400 / 1,500 / 500; {1,4,9,11,30,47} with 200 / 700 / 200 and
    {1,4,9,11,30,47,67} with 300 10-gap completions; {1,4,9,11,30} with 300 / 1,000 / 300;
  * {1,4,9,15,31} with 200 / 600 / 200, {1,4,9,15,31,56,74} with 300 / 500 9- / 10-gap completions and
    {1,4,9,15,35} with 200 10-gap completions;
  * {1,4,9,11,x} for x = 25, 28, 32, 34 with 100 10-gap completions each, and 100 random 10-gap members each
    of {1,4,7,9}, {1,4,11,13} and {1,3,8,10}.

  The cut-offs were 9430 for the second stage and 9568 (best of four runs) for the final.

**Families.** First-stage values (one 2M-move run per sequence). The 3,148 sequences next to the N = 362
leaders (extensions, last-gap replacements and the completions of the three 8-gap prefixes) are counted
apart from the 12,842 random completions:

* **Cores.** {1,4,9,11} (median 9920) and {1,4,9,15} (9973) lead by far; {1,4,11,13} 11080, {1,4,7,9} 11483,
  {1,3,8,10} 12111. One of their 300 members reached the second stage, none the final.
* **Fifth gap.** {1,4,9,15,31} and {1,4,9,11,30} are level once more (random 10-gap completions: medians
  9973 and 10044); {1,4,9,15,35} and {1,4,9,11,x} for x = 25, 28, 32, 34 are 215–675 behind.
* **Number of gaps.** Nine or ten. Random completions of the same seven-gap prefix differ little between 9
  and 10 gaps ({…,36,70}: medians 9823 / 9802; {…,36,62}: 9953 / 9866; {1,4,9,15,31,56,74}: 9603 / 9683).
  Eleven gaps are worse ({…,36,70} 9879, {…,36,62} 9924): only 4 of the 2,000 11-gap sequences reached the
  second stage, and none the final.
* **Sixth and seventh gaps.** Sixth gap 36 beats 47 among random members of {1,4,9,11,30} (medians 9944 vs
  10085 with 10 gaps, 10001 vs 10317 with 9). The best seven-gap prefixes (random completions) are
  {1,4,9,15,31,56,74} (9603 with 9 gaps), {1,4,9,11,30,36,70} (9802 with 10), {…,47,67} (9837 with 10) and
  {…,36,62} (9866 with 10); every other seventh gap after 36 has a median of 9876 or more.
* **Near the N = 362 leaders.** Extending a leader by one gap, or replacing its last gap, gives medians of
  9347–9562, 200–490 below random completions of the same prefixes. The best of these neighbourhoods are the
  extensions of {1,4,9,11,30,47,67,132,180} (median 9348; 27 of its 76 members reached the final) and the
  last-gap replacements of {1,4,9,15,31,56,74,126,177} (9397) and {…,47,67,95,169} (9415; 25 finalists).
  The leaders themselves are not better than their neighbours: {1,4,9,11,30,36,70,109,171}, the best
  sequence at N = 362, drew 9472 and missed the second stage.
* **Finalists.** 135 of the 150 finalists (28 of the best 30) are neighbours of the N = 362 leaders.
  {1,4,9,11,30,47,67,…} supplies 69 finalists and 18 of the best 25, led by
  {1,4,9,11,30,47,67,132,180,248} (9543) and four more tenth gaps after {…,132,180} within 93; then
  {…,47,67,95,221} 9619, {1,4,9,15,31,56,74,170,209} 9656, {1,4,9,11,30,36,59,98,162,213} 9663,
  {…,36,70,109,191,207} 9672 and {…,36,62,121,161,211} 9684.

**Picking the field.** At N = 362 the screens' favourite family, {1,4,9,11,30,47,67,·,·}, collapsed under long
attack after leading at 128M. Round A therefore takes the best finalists of every seven-gap prefix that
reached the final rather than the overall best 32, and adds the N = 362 leader itself, its best extension and
the two best 11-gap sequences.

**The screen does not rank the finalists.** Over the 32 round-A sequences, the rank correlation between the
screen score and the 128M value is −0.13 (−0.07 over the 28 finalists; +0.04 for the median of their eight
8M runs). The screen's favourite family, {…,47,67,…}, came out in the middle of round A (its best 10126,
against 9982 for the leader), and round A's leader comes from {…,36,59}, the prefix with the fewest
finalists (14, all of them next to the N = 362 sequence {1,4,9,11,30,36,59,98,162}).

## Long runs on the top candidates

Every round gives each of the 24 threads one anneal of its whole budget from T₀ = 13, with no target
(`maxout.py --method luby` with `--base` = `--moves`):

* **A**: 128M moves per thread for 32 sequences: 7 / 5 / 3 / 10 / 5 of the best finalists with seven-gap
  prefixes {…,47,67}, {1,4,9,15,31,56,74}, {…,36,59}, {…,36,70} and {…,36,62} (including the N = 362 leader
  {1,4,9,11,30,36,70,109,171} itself, its best extension {…,171,251} and the N = 362 sequence
  {…,47,67,132,180}), and the two best 11-gap sequences.
* **A2**: 128M for 15 neighbours of round A's leader {1,4,9,11,30,36,59,98,162,213}: six other tenth gaps
  (197–253), four other ninth gaps (156–168), four other eighth gaps (94–102), and {…,98,162} without a
  tenth gap.
* **A3** (run after D): 128M for eight more neighbours of the leader: sixth gap 34, 35, 37 or 38, and
  seventh gap 57, 58, 61 or 62.
* **A4** (after A3): 128M for eight neighbours of {…,36,70,109,195,248}, whose last two gaps come from the
  screen's random completions: ninth gap 191, 193, 197 or 199, and tenth gap 244, 246, 250 or 254.
* **B**: 512M for the best 12 of A and A2. They span six eight-gap prefixes ({…,36,59,98} three times,
  {…,36,70,109}, {…,36,62,121}, {1,4,9,15,31,56,74,126} and {…,47,67,132} twice each, {…,36,70,117} once),
  so no family cap was needed.
* **C**: 1G for the best 6 after B and {1,4,9,11,30,36,70,117,143,254} (eighth after B, 10196), the
  extension of the N = 362 runner-up.
* **D**: 2G for the best 3 after C: {…,36,59,98,162,213}, {…,36,59,98,162,217} and {…,36,70,109,195,248}.
* **E** (after A4): 4G for the leader.
* **D2** (after E): a second 2G run for {…,36,59,98,162,217}.
* **F** (after D2): a second 1G run for the leader.

Column A holds the round A, A2, A3 or A4 value (128M moves per thread). The second value in column C is
round F's, and the second in column D is round D2's.

| sequence | A | B | C | D | E | stored best (≥) | moves/thread |
|---|---:|---:|---:|---:|---:|---:|---:|
| {1,4,9,11,30,36,59,98,162,213} | 9982 | 10027 | 10089/10066 | 10091 | 10119 | 10119 | 8.64G |
| {1,4,9,11,30,36,70,109,195,248} | 10058 | 10147 | 10150 | 10125 | – | 10150 | 3.64G |
| {1,4,9,15,31,56,74,126,177,250} | 10120 | 10179 | – | – | – | 10179 | 0.64G |
| {1,4,9,11,30,36,70,109,191,207} | 10070 | 10169 | 10186 | – | – | 10186 | 1.64G |
| {1,4,9,11,30,36,59,98,162,217} | 10071 | 10130 | 10111 | 10135/10189 | – | 10189 | 5.64G |
| {1,4,9,11,30,36,70,117,143,254} | 10114 | 10196 | 10184 | – | – | 10196 | 1.64G |
| {1,4,9,11,30,36,59,98,162,253} | 10100 | 10164 | 10205 | – | – | 10205 | 1.64G |
| {1,4,9,11,30,36,62,121,161,211} | 10077 | 10210 | – | – | – | 10210 | 0.64G |
| {1,4,9,11,30,36,62,121,197} | 10135 | 10211 | – | – | – | 10211 | 0.64G |
| {1,4,9,15,31,56,74,126,177,241} | 10095 | 10156 | 10214 | – | – | 10214 | 1.64G |
| {1,4,9,11,30,47,67,132,180,248} | 10147 | 10253 | – | – | – | 10253 | 0.64G |
| {1,4,9,11,30,47,67,132,180} | 10126 | 10282 | – | – | – | 10282 | 0.64G |

* **Lowest bound after long attack at N = 512: {1,4,9,11,30,36,59,98,162,213} ≥ 10119.** This is a new sequence, the 9th of
  the screen's 150 finalists. It extends {1,4,9,11,30,36,59,98,162} (≥ 6545 at N = 362, the last of the
  seven sequences this search started from) by a tenth gap. Its runs of 128M, 512M, 1G, 2G and 4G moves per
  thread gave 9982, 10027, 10089, 10091 and 10119: its bound rose by 137 from 128M to 4G, and by 28 in the
  last doubling, against 40 from 128M to 4G for the N = 362 leader. A second 1G run (F) gave 10066, so the
  bound stands at 10119 after 8.6G moves per thread in all. Next come {…,36,70,109,195,248} ≥ 10150 (3.6G),
  also new, and {1,4,9,15,31,56,74,126,177,250} ≥ 10179 after only 0.64G. {…,36,59,98,162,217}, second
  after E with 10135, reached 10189 in its second 2G run (D2) and is now fifth.
* **The ranking.** The leader's margin in stored bounds fell from 103 after B to 41 after C, 44 after D and
  16 after E, partly because it had twice the attack of the next two. D2 then lifted {…,162,217} from 10135
  to 10189, which leaves {…,195,248} second, 31 behind. In each equal-budget round, though, the leader's run
  came out lowest: 9982 against 10058 at 128M, 10027 against 10130 at 512M, 10089 (and 10066 in F) against
  10111 at 1G and 10091 against 10125 at 2G. Runs of one sequence differ by tens: the two 2G runs of
  {…,162,217} were 54 apart and the leader's two 1G runs 23, and three of the nine 1G and 2G runs of other
  sequences ended below an earlier, shorter run of the same sequence ({…,162,217} 10111 at 1G after 10130 at
  512M, {…,117,143,254} 10184 after 10196, {…,195,248} 10125 at 2G after 10150 at 1G). So the order is a
  ranking at this effort, not a proof. The other ten sequences of round B are 60–163 behind the leader after
  0.64–5.64G moves per thread each. Three neighbours of {…,195,248} from round A4 have lower stored bounds
  (10076, 10107, 10114), but after 128M moves per thread only, where the leader drew 9982.

* **A narrow optimum.** Rounds A2 and A3 moved each of the leader's last five gaps by a few steps, one at a
  time, and no neighbour came within 89 at 128M (A: tenth gap 217, 10071). The earlier the gap, the more a
  move costs:
  * sixth gap 34, 35, 37 or 38 instead of 36: 1001–1582 (11330, 10983, 11564, 11126);
  * seventh gap 57, 58, 61 or 62 instead of 59: 775–1076 (10865, 10937, 10757, 11058);
  * eighth gap 94, 96, 100 or 102 instead of 98: 296–1001 (10278, 10755, 10565, 10983);
  * ninth gap 156, 159, 165 or 168 instead of 162: 216–363 (10198, 10276, 10229, 10345);
  * other tenth gaps: 89–320 (217 10071, 253 10100, 205 10178, 243 10188, 209 10215, 197 10220,
    229 10265, 221 10302); no tenth gap: 183 ({…,98,162} 10165).

  The screen had drawn only the tenth-gap neighbours and {…,98,162}; the others were new.
* **The second family's optimum is flatter.** In round A4, other ninth gaps (191, 193, 197, 199) and
  other tenth gaps (244, 246, 250, 254) for {…,36,70,109,195,248} cost 18–230 at 128M (10076, 10199, 10288,
  10233; 10197, 10114, 10263, 10107; against 10058). None of the eight beat it or came near the leader.
* **The screens' favourite family collapsed again.** {1,4,9,11,30,47,67,…} supplied 69 of the 150
  finalists and 18 of the best 25. Its seven members in round A drew 10126–10355. Its best two rose by 156
  and 106 in round B, the largest and third-largest rises of the twelve, to the bottom two places (10282 for
  {…,47,67,132,180}, 10253 for {…,132,180,248}).
* **The N = 362 leader does not carry over as it is.** {1,4,9,11,30,36,70,109,171} drew 10230 at 128M
  (21st of the 32 in round A), and its best extension {…,171,251} 10178. Its eight-gap prefix does better with
  two new gaps: {…,36,70,109,195,248} and {…,36,70,109,191,207}, two of the screen's random completions of
  {…,36,70,109}, were second and third in round A (10058, 10070).
* **Ten gaps.** The best nine-gap sequences, {…,47,67,132,180} and {…,36,62,121,197}, drew 10126 and 10135
  in round A and 10282 and 10211 at 512M, and the leader without its tenth gap drew 10165. The two 11-gap
  sequences drew 10242 and 10377.
* **{1,4,9,15} stays behind.** Its best member, {1,4,9,15,31,56,74,126,177,241}, was 113 behind the leader
  in round A, 129 in round B and 125 after C (10095, 10156, 10214). Its sibling {…,126,177,250} is at
  ≥ 10179 after 512M moves per thread only.

## What this suggests for larger N

1. **Prefix.** {1,4,9,11,30} has now been the best 5-gap prefix under attack at every size from N = 91 to
   512, and {1,4,9,11,30,36} at N = 362 and 512. After that the leaders differ: {…,36,70,109,171} at
   N = 362, {…,36,59,98,162,213} at N = 512. The N = 512 leader extends {1,4,9,11,30,36,59,98,162}, the
   last of the seven N = 362 sequences this search started from (≥ 6545 there after 0.64G moves per
   thread), by one gap. Its ratios after 36 are 1.64, 1.66 and 1.65, then 1.31 to the last gap. The second
   family, {…,36,70,109,·,·}, continues the N = 362 leader's eight-gap prefix with two new gaps. A search at
   N = 724 (gaps ≤ 362) can start from these sequences with an eleventh gap added or the last gap moved up,
   and from random 10- and 11-gap completions of {…,36,59,98,162} and {…,36,70,109}.
2. **Screens only nominate; long attacks select.** At N = 512 the screen did not rank its finalists (rank
   correlation −0.07 with the 128M values over round A's 28 finalists), the leader came from the prefix with
   the fewest finalists, and the screens' favourite family, {…,47,67,…}, collapsed under long attack for the
   second size running. A screen still separates cores and five-gap prefixes, whose medians differ by
   hundreds. But the leader's optimum is narrow: moving one of its last five gaps by 1–4 cost 89–1582 at
   128M, so random completions sample it too sparsely. At N = 724 the 128M rounds need a field picked by
   prefix, and single-gap neighbourhood scans around each good sequence.
3. **{1,4,9,15} falls further behind.** Its best member was 113 behind the leader after the first round at
   N = 512 and 125 after 1G, against 7–31 after the first round at N = 128–362.
4. **Budgets.** Throughput fell from 0.62M to 0.29–0.44M moves per second per thread between N = 362 and
   512, about as predicted. The screen took 2 hours and the rounds about 25, the leader's 4G anneal
   alone 2.6. The leader's bound still rose by 28 between its 2G and 4G runs, and the second 2G run of
   {…,162,217} found 54 more than the first, so even 4G per thread does not settle a bound at this size. The N^1.4 trend gives about 0.2–0.27M at N = 724: 8–11 minutes per sequence
   for 128M, 60–85 minutes for 1G and 4–5.5 hours for 4G. A campaign of this shape would need about 40
   hours there, or a smaller field.
