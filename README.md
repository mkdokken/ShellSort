# ShellSort
Finding best shell sort gap sequences to minimize comparisons.


This project contains code to attempt to find the best gap sequence for shell sort.


In early 2025 the best sequences the code found were:

{1, 4, 10, 23, 57, 132, 301, 701, 1504, 3263, 7196, 15948, 34644, 74428, 162005, 347077, 745919, 1599893, 3446017, 7434649, 15933053},  
{1, 4, 10, 23, 57, 132, 301, 701, 1541, 3498, 7699, 17041, 37835, 81907, 179433, 392867, 858419, 1883473, 4081849, 9002887, 19782319}, and  
{1, 4, 10, 23, 57, 132, 301, 701, 1636, 3659, 8129, 18118, 40354, 89129, 197803, 443557, 973657, 2131981, 4697153, 10528127, 23135351},

which are all extensions of Marcin Ciura's gaps: {1, 4, 10, 23, 57, 132, 301, 701}.
https://stackoverflow.com/questions/2539545/fastest-gap-sequence-for-shell-sort/79637484#79637484


As of 2026, after improving the code, it finds some potentially better sequences with a 644 instead of a 701 following the 301. For example:

{1, 4, 10, 23, 57, 132, 301, 644, 1408, 3227, 6847, 14842, 31970, 69487, 149728, 324011, 692843, 1499809, 3211613, 7025789, 14937367},  
{1, 4, 10, 23, 57, 132, 301, 644, 1408, 3227, 6847, 14917, 32910, 71651, 157678, 345119, 747533, 1631227, 3484469, 7526447, 16541333}, and  
{1, 4, 10, 23, 57, 132, 301, 644, 1445, 3165, 6913, 15349, 33794, 75251, 163395, 358349, 784009, 1722313, 3809723, 8255479, 17990407}.  

It seems likely to me now, based on the pattern in the table below, that {1, 4, 10, 23, 57, 132, 301, 644, 1408} could be the start of the optimal gap sequence for large N.

The best sequences for large N not starting with {1, 4, 10, 23, 57, 132, 301} seem to start with {1, 4, 10, 21, 56, 125, 288} instead, but they don't seem to perform quite as well. 

My best attempt at finding optimal gap sequences for fixed size lists of various sizes are listed below. For N=32 and N=64 I believe these are optimal, since it is pretty easy to search all (reasonable) possibilities and nothing else was close. For N=128 through N=1k, they are likely optimal up to the last number which can often be changed with very little effect on the average number of comparisons. For N>1k, these sequences are just the best that I could find with limited computing power, and the larger N gets the further from optimal these will likely be. My results for N=128 and N=1000 match Ciura's results in his 2001 paper, and my results for N=32 matches the N=32 row of the table of optimal gap sequences found at https://sortingalgos.miraheze.org/wiki/Shellsort. 

| N | Best Sequence | Avg Comparisons | Num Random Samples |
| :---:     |    :---: |     :---: |     :---: |
| 32  |  1, 4, 13 |  152.055 +/- 0.005 | 22000000 |
| 64  |  1, 4, 9, 38, 62 |  399.111 +/- 0.01 | 12000000 |
| 128  |  1, 4, 9, 24, 85, 126 |  1002.25 +/- 0.01 | 32000000 |
| 256  |  1, 4, 10, 27, 89, 238 |  2428.52 +/- 0.05 | 8000000 |
| 512  |  1, 4, 10, 23, 57, 189, 483 |  5734.95 +/- 0.1 | 3000000 |
| 1000  |  1, 4, 10, 23, 57, 156, 409, 996 |  12929.4 +/- 0.3 | 1000000 |
| 2000  |  1, 4, 10, 23, 57, 132, 347, 1208, 1979 |  29484.4 +/- 0.6 | 620000 |
| 3000  |  1, 4, 10, 23, 57, 132, 313, 1044, 2778 |  47449.4 +/- 1 | 600000 |
| 5000  |  1, 4, 10, 23, 57, 132, 301, 701, 1937, 4921 |  85889.1 +/- 2 | 250000 |
| 10000  |  1, 4, 10, 23, 57, 132, 301, 701, 1733, 6085, 9941 |  190466 +/- 5 | 128000 |
| 30000  |  1, 4, 10, 23, 57, 132, 301, 701, 1541, 3498, 11336, 28631 |  661018 +/- 15 | 58000 |
| 50000  |  1, 4, 10, 23, 57, 132, 301, 644, 1408, 3227, 7792, 28599, 47932 |  1172130 +/- 25 | 33000 |
| 100000  |  1, 4, 10, 23, 57, 132, 301, 644, 1445, 3165, 6913, 17736, 62185, 99668 |  2535370 +/- 60 | 18000 |
| 1000000  |  1, 4, 10, 23, 57, 132, 301, 644, 1408, 3227, 6847, 14917, 32910, 71651, 171523, 606250, 989292 |  31730200 +/- 1000 | 1100 |



I have built and run the code on MacOS using the default c compiler in Xcode, and I have built and run it on Windows using the default c compiler in Codeblocks. 
When building the code, I have always used the -O3 optimization flag, and left the rest of the default compiler settings. 
