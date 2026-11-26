# Large-N campaign, 2026-10-30/31: N = 256 (16 h)

Same goals as the earlier campaigns (`docs/CAMPAIGN_64_91.md`, `docs/CAMPAIGN_128_181.md`): look for
sequences, families and prefixes that earlier searches missed, and push the lower bounds of the top
candidates up with longer runs. Every value is the cost of a stored, re-simulated input
(`results/strong.json`); a sequence's true worst case can only be higher. Lower = better sequence.
Gaps ≤ 128 (reference convention N/2).

## Engines at this size

* **Throughput.** About 1.0M annealing moves per second per thread (24 threads), against 1.4M at N = 181
  and 2.5M at N = 128; 9-gap sequences are somewhat slower than 7-gap ones.
* **Screening runs** (2M moves, one thread per sequence): the best start temperature keeps rising with N.
  Mean over 24 known sequences 3965.0 / 4021.8 / 4037.5 / 4040.2 / 4035.7 for T₀ = 3 / 4.5 / 6 / 9 / 13
  (T₀ = 4.5 was best at N = 128 and 181), so the N = 256 screens anneal from T₀ = 9.
* **Long runs** (24 threads, 100M moves per thread, one seed, two sequences): Luby restarts with 4M-move base
  runs from T₀ = 4.5 gave 4179 / 4145 for {1,4,9,15,35,49,101,123} / {1,4,9,11,30,36,70,121}, 8M from T₀ = 9
  gave 4139 / 4148, and 16M from T₀ = 9 gave 4153 / 4156. No setting wins on both, so the rounds alternate
  4M-move restarts from T₀ = 4.5 and 16M-move restarts from T₀ = 9. Parallel tempering was not used: a
  10M-move ladder reached 3899 where 16M moves of Luby restarts reached 4125.

## Exploration: 80,228 sequences in three screens

* **Main screen** (57,796 sequences, 93 min; 1 run of 2M moves for all, 3 runs of 4M for the best 3,005,
  8 runs of 8M for the best 300), built on the N = 128 / 181 results: every 5-gap {1,4,9,11,·} and
  {1,4,9,15,·} and 1,000 random 6-gap ones each; {1,4,9,11,30} (the fifth gap that led at N = 91–181) with
  every 6- and 7-gap completion, 10,000 random 8-gap and 4,000 random 9-gap ones; every 8-gap completion of
  {1,4,9,11,30,36} and {1,4,9,11,30,47} (the leaders' sixth gaps at N = 181) and 2,000 random 9-gap ones each;
  300 / 350 / 150 random 7- / 8- / 9-gap completions for each other fifth gap (21–40 for {1,4,9,11}, 25–40
  for {1,4,9,15}); 600 samples each of {1,4,7,9}, {1,4,11,13} and {1,3,8,10}. The README sequence was not
  drawn.
* **{1,4,9,15} screen** (20,700 sequences, 40 min): {1,4,9,15,x,·,·,·} for x = 29…37 with 300 / 1,500 / 500
  random 7- / 8- / 9-gap completions each, so that {1,4,9,15} gets about the sampling density the main screen
  gave {1,4,9,11,30} (see Families).
* **Refinement** (2,479 sequences, 18 min, runs of 4M/8M/16M) around the 40 best main-screen finalists:
  1,951 new sequences, 26 of them among its final 61; the best finalist stays {1,4,9,11,30,47,67,95}.

**Families.** All 25 best main-screen finalists are {1,4,9,11,30,36,…} or {1,4,9,11,30,47,…} with 8 gaps (one
with 9): {1,4,9,11,30,47,67,95} 4063, {1,4,9,11,30,36,59,104} 4068, {1,4,9,11,30,47,56,93} 4075,
{1,4,9,11,30,36,70,115} 4080. Only two cores reach the last stage, and {1,4,9,15} seemed 54 behind (4117).
That gap is mostly sample size: a family's best member improves with the number of members drawn, and
{1,4,9,11,30} had 29,789 against about 800 per {1,4,9,15} fifth gap. At equal density the {1,4,9,15} screen's
best, {1,4,9,15,35,37,83,117}, scored 4075, 12 behind. Statistics that do not grow with the sample size (first
stage, one 2M-move run per sequence) put the two cores level:

| 5-gap prefix | sequences | median | 1st percentile |
|---|---:|---:|---:|
| {1,4,9,15,31} | 3,099 | 4304 | 4042 |
| {1,4,9,15,35} | 3,090 | 4308 | 4053 |
| {1,4,9,11,30} | 29,789 | 4321 | 4024 |
| {1,4,9,15,34} | 3,095 | 4328 | 4046 |
| {1,4,9,15,29} | 3,101 | 4328 | 4055 |
| {1,4,9,11,32} | 818 | 4338 | 4087 |
| {1,4,9,15,32} | 3,098 | 4340 | 4056 |
| {1,4,9,11,25} | 826 | 4373 | 4089 |
| {1,4,9,11,34} | 816 | 4398 | 4089 |
| {1,4,9,11,21} | 814 | 4409 | 4107 |
| {1,4,9,15,30}, {1,4,9,15,33}, {1,4,9,15,36} | 3,085–3,095 | 4641–4759 | 4187–4231 |

Fifth gaps that share the factor 3 with 9 and 15 (30, 33, 36, and 27 and 39 in the main screen) are much
worse for {1,4,9,15}. For {1,4,9,11}, 30 leads, and the other fifth gaps sampled (21–40) are 17–244 behind in
median. The cores that fell behind at N = 128 / 181 stay out: medians {1,4,11,13} 4509, {1,4,7,9} 4681,
{1,3,8,10} 4893, against 4361 for {1,4,9,11} and 4426 for {1,4,9,15} overall; 13 of their 1,800 members pass
the first stage and none the second.
The same statistics at N = 128 and 181 (`famscan_128.json`, `famscan_181.json`) also have {1,4,9,15,31} level
with {1,4,9,11,30} (N = 181: medians 2716 and 2730; N = 128: 1717 and 1709).

Under attack the cores separate again. In round A (100M moves per thread) the best {1,4,9,11} member gave
4115 and the best {1,4,9,15} member 4138 (+23). Both {1,4,9,15} screen leaders then rose sharply
({1,4,9,15,35,37,83,117} from 4168 to 4287 at 1G, {1,4,9,15,31,56,74,126} to 4181), and after all rounds the
lowest {1,4,9,15} bound is 4181, 10 above the leader. Best 5-gap prefixes in round A: {1,4,9,11,30} 0,
{1,4,9,15,35} +23, {1,4,9,15,31} +26, {1,4,9,11,32} +35, {1,4,9,15,37} +49, {1,4,9,11,25} +71. The best
sequences are {1,4,9,11,30,·,·,·} with 8 gaps: sixth gap 47 or 36, seventh 59–70, eighth 89–122.

## Long runs on the top candidates

All Luby restarts, 24 threads, alternating 4M-move restarts from T₀ = 4.5 (rounds A, C, E) and 16M-move
restarts from T₀ = 9 (B, D): **A** 100M moves per thread for 57 sequences (the 25 best main-screen finalists,
the 12 best of the {1,4,9,15} screen, 8 from the refinement and the 12 known sequences with stored bounds
≤ 4192), **B** 300M for the best 20 plus the README sequence and the earlier {1,4,9,11,30,36,70,121} and
{…,123}, **C** 1G for the best 9 after B, then for the 7 more within reach, **D** 2G for the best 6, **E** 4G
for the best 3. Afterwards the eight sequences that still had bounds ≤ 4171 from round A or old races alone got
round B, and the three still at or below 4171 got round C.

| sequence | A | B | C | D | E | stored best (≥) |
|---|---:|---:|---:|---:|---:|---:|
| {1,4,9,11,30,47,67,95} (screen) | 4137 | 4116 | 4171 | 4136 | 4150 | **4171** |
| {1,4,9,11,30,36,59,122} (screen) | 4161 | 4158 | 4163 | 4174 | – | 4174 |
| {1,4,9,11,30,36,62,89} (screen) | 4129 | 4166 | 4163 | 4173 | 4177 | 4177 |
| {1,4,9,11,30,36,70,109} (screen; earlier candidate) | 4151 | 4165 | 4180 | – | – | 4180 |
| {1,4,9,11,30,36,70,113} (screen) | 4152 | 4172 | 4181 | – | – | 4181 |
| {1,4,9,15,31,56,74,126} ({1,4,9,15} screen) | 4149 | 4171 | 4181 | – | – | 4181 |
| {1,4,9,11,30,36,70,123} (screen; earlier candidate) | 4125 | 4165 | 4182 | – | – | 4182 |
| {1,4,9,11,30,36,59,109} (screen) | 4135 | 4169 | 4176 | 4186 | – | 4186 |
| {1,4,9,11,30,36,70,121} (screen; earlier candidate, README alternate) | 4149 | 4163 | 4188 | – | – | 4188 |
| README {1,4,9,15,35,49,101,123} | 4143 | 4167 | 4174 | 4193 | – | 4193 |
| {1,4,9,11,30,47,67,106} (screen) | 4115 | 4146 | 4173 | 4169 | 4193 | 4193 |
| {1,4,9,11,30,36,59,104} (screen) | 4152 | 4171 | 4180 | 4193 | – | 4193 |
| {1,4,9,11,30,47,59,86} (screen) | 4167 | 4171 | 4194 | – | – | 4194 |
| {1,4,9,15,35,37,83,117} ({1,4,9,15} screen) | 4138 | 4168 | 4287 | – | – | 4287 |

* **Lowest bound at N = 256: {1,4,9,11,30,47,67,95} ≥ 4171**, a new sequence, the main screen's best
  finalist. Its five runs (7.4G moves per thread, both restart settings) gave 4116–4171. Next are
  {1,4,9,11,30,36,59,122} ≥ 4174 (3.4G) and {1,4,9,11,30,36,62,89} ≥ 4177 (7.4G), also new.
* **The README sequence {1,4,9,15,35,49,101,123} is now ≥ 4193**, 22 above the leader and 154 above its
  published 4039. Its stored 4163 was race-stopped: the engine check reached 4179 (4M-move restarts, 100M
  moves) and round D 4193; its other runs gave 4139–4174 (3.7G moves per thread in all). Nine sequences in
  the table, six of them new, have lower bounds than it after at least 1.4G moves each.
* The earlier candidates, race-stopped at 4165–4170, rose to 4180–4200: {1,4,9,11,30,36,70,109} 4180,
  {…,70,123} 4182, {…,65,103,118} 4182, {…,70,121} 4188, {1,4,9,15,34,56,80} 4200. The screen found the
  three {1,4,9,11,30,36,70,·} ones again independently, among its 25 best finalists.
* Bounds still jump at this size: the leader went 4116 → 4171 between 300M and 1G, {1,4,9,11,30,47,67,106}
  (best after round A, 4115) rose to 4193 at 4G, and {1,4,9,15,35,37,83,117} rose by 119 at 1G. The top three
  are within 6, so their order is a ranking at this effort, not a proof; the README sequence's 22-point
  deficit is the clearer result.

## What this suggests for larger N

1. **Prefix.** {1,4,9,11,30} has now been the best 5-gap prefix under attack at N = 91, 128, 181 and 256.
   At N = 256 the best sequences continue it with a sixth gap of 47 or 36, then one gap in 59–70 and one in
   89–122 (8 gaps; the best 9-gap sequence, {1,4,9,11,30,36,59,106,127}, is ≥ 4191). The README's N = 362
   sequence starts {1,4,9,11,30,36,70}. A search at N = 362 (gaps ≤ 181) can start from
   {1,4,9,11,30,47,67,·,·,(·)} and {1,4,9,11,30,36,59…70,·,·,(·)} with 9–10 gaps.
2. **{1,4,9,15} is level in the screens but behind under attack.** Its typical members score as well as those
   of {1,4,9,11,30} in short runs (medians, N = 128–256), but its best attacked members end 10–22 behind at
   N = 256, as at N = 128 and 181 (7–18). Short runs consistently flatter it, so it needs a share of the
   attack budget, not of the screen.
3. **Compare families by statistics, not by their best member.** A family's best screen score improves with
   the number of members drawn ({1,4,9,15} looked 54 behind with 30× fewer members, 12 behind at equal
   density). Draw comparable numbers per family, or compare medians and percentiles.
4. **Budgets.** At N = 256 bounds still jump by 20–40 between 300M and 4G moves per thread (and by 119 once),
   so a ranking needs ≥ 2G per sequence with both restart settings; the margins at the top (6 between the best
   two) are a ranking at this effort, not a proof. Each 4G run takes 40–45 minutes here; at N = 362 the moves
   cost about twice as much.
