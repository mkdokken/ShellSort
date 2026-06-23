# ShellSort
Finding best shell sort gap sequences to minimize comparisons for randomly shuffled lists.


This project contains code to attempt to find the best gap sequence for shell sort.


In early 2025 the best sequences the code found (for minimizing average comparisons) were:

{1, 4, 10, 23, 57, 132, 301, 701, 1504, 3263, 7196, 15948, 34644, 74428, 162005, 347077, 745919, 1599893, 3446017, 7434649, 15933053},  
{1, 4, 10, 23, 57, 132, 301, 701, 1541, 3498, 7699, 17041, 37835, 81907, 179433, 392867, 858419, 1883473, 4081849, 9002887, 19782319}, and  
{1, 4, 10, 23, 57, 132, 301, 701, 1636, 3659, 8129, 18118, 40354, 89129, 197803, 443557, 973657, 2131981, 4697153, 10528127, 23135351},

which are all extensions of Marcin Ciura's gaps: {1, 4, 10, 23, 57, 132, 301, 701}.
https://stackoverflow.com/questions/2539545/fastest-gap-sequence-for-shell-sort/79637484#79637484


But as of 2026, after improving the code, it finds some potentially better sequences with a 644 instead of a 701 following the 301.

<!-- 
For example:
{1, 4, 10, 23, 57, 132, 301, 644, 1408, 3227, 6847, 14842, 31970, 69487, 149728, 324011, 692843, 1499809, 3211613, 7025789, 14937367},  
{1, 4, 10, 23, 57, 132, 301, 644, 1408, 3227, 6847, 14917, 32910, 71651, 157678, 345119, 747533, 1631227, 3484469, 7526447, 16541333}, and  
{1, 4, 10, 23, 57, 132, 301, 644, 1445, 3165, 6913, 15349, 33794, 75251, 163395, 358349, 784009, 1722313, 3809723, 8255479, 17990407}.  
-->

It seems to me, based on the pattern in the table below, that {1, 4, 10, 23, 57, 132, 301, 644, 1408} might be the start of the optimal gap sequence (for minimizing average comparisons) for large N. The best sequences for large N not starting with {1, 4, 10, 23, 57, 132, 301} seem to start with {1, 4, 10, 21, 56, 125, 288} instead. While I can't get the {1, 4, 10, 21} sequences to perform quite as well as the {1, 4, 10, 23} sequences, I think it's worth investigating if for some large enough N they might start to become slightly better. 

My best attempt at finding optimal gap sequences (for minimizing average comparisons) for fixed size lists of various sizes are listed below. For N=16 through N=64 I believe these are optimal, since it is pretty easy to search all (reasonable) possibilities and nothing else was close. For N=128 through N=1000, they are likely optimal up to the last number which can sometimes be changed with very little effect on the average number of comparisons. For N=2k through N=100k, these sequences optimistically might be optimal up to the last 2 or 3 terms. For N=1million through N=1billion I'm sure these sequences are more than just the last 3 terms away from optimal but I don't even have a good guess as to how close/far from optimal they really are. My results for N=128 and N=1000 match Ciura's results in his 2001 paper, and my results for N=16 and N=32 match the table of optimal gap sequences found at https://sortingalgos.miraheze.org/wiki/Shellsort. 

| N | Best Sequence | Avg Comparisons | Num Random Samples |
| :---:     |    :---: |     :---: |     :---: |
| 16  |  1, 5, 14 |  54.174 +/- 0.003 | 14000000 |
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
| 20000  |  1, 4, 10, 23, 57, 132, 301, 701, 1636, 4021, 13293, 19908 |  418668 +/- 9 | 86000 |
| 30000  |  1, 4, 10, 23, 57, 132, 301, 701, 1541, 3498, 11336, 28631 |  661018 +/- 15 | 58000 |
| 50000  |  1, 4, 10, 23, 57, 132, 301, 701, 1504, 3263, 8399, 30113, 49256 |  1172020 +/- 20 | 60000 |
| 100000  |  1, 4, 10, 23, 57, 132, 301, 644, 1445, 3165, 6913, 17736, 62185, 99668 |  2535370 +/- 60 | 18000 |
| 1 million  |  1, 4, 10, 23, 57, 132, 301, 644, 1408, 3227, 6847, 14917, 32910, 71651, 171523, 606250, 989292 |  31730950 +/- 500 | 4400 |
| 10 million  |  1, 4, 10, 23, 57, 132, 301, 644, 1408, 3227, 6847, 14842, 31970, 69487, 149728, 324011, 692843, 1645254, 5934785, 9775485 |  381508422 +/- 5712 | 880 |
| 100 million  |  1, 4, 10, 23, 57, 132, 301, 644, 1408, 3227, 6847, 14842, 31970, 68467, 147869, 316034, 667787, 1442593, 3085219, 6662519, 17234807, 60001006, 98743101 |  4458427670 +/- 45026 | 232 |
| 1 billion  |  1, 4, 10, 23, 57, 132, 301, 644, 1408, 3227, 6847, 14842, 31970, 68467, 147869, 316034, 667787, 1442593, 3085219, 6662519, 14349443, 30994463, 66950617, 167899094, 600000000, 981186611 |  51029961078 +/- 597316 | 29 |

Below I have listed some close alternatives I found for N = 10 million. 

| N | Description | Sequence | Avg Comparisons | Num Random Samples |
| :---:     |    :---: |    :---: |     :---: |     :---: |
| 10 million  |  Best  |  1, 4, 10, 23, 57, 132, 301, 644, 1408, 3227, 6847, 14842, 31970, 69487, 149728, 324011, 692843, 1645254, 5934785, 9775485 |  381508422 +/- 5712 | 880 |
| 10 million  |  Best with 644, X>1408  |  1, 4, 10, 23, 57, 132, 301, 644, 1445, 3165, 6913, 14836, 32056, 69350, 147544, 318977, 700831, 1686433, 6033754, 9818957 |  381511145 +/- 4916 | 1030 |
| 10 million  |  Best with 301, X!=644  |  1, 4, 10, 23, 57, 132, 301, 701, 1504, 3263, 6895, 15253, 32518, 71096, 153575, 333031, 727381, 1719031, 6043655, 9883970 |  381513396 +/- 5518 | 736 |
| 10 million  |  Best with 644, X<1408  |  1, 4, 10, 23, 57, 132, 301, 644, 1371, 3016, 6535, 14081, 29612, 64663, 141588, 309979, 680821, 1636132, 5503249, 9657208 |  381529743 +/- 5216 | 862 |
| 10 million  |  Best with 4, 10, 21  |  1, 4, 10, 21, 56, 125, 288, 630, 1381, 3002, 6414, 13964, 30143, 65044, 142682, 307807, 697201, 1717374, 5970299, 9910633 |  381531463 +/- 5368 | 920 |


My best attempt at finding optimal gap sequences (for minimizing worst-case comparisons) for fixed size lists of various sizes are listed below. I did not include N=1 though N=5 because shell sort does not provide any improvement in the worst case over a plain insertion sort. For some sizes of N there are multiple different gap sequences all tied for the lowest worst-case and some of them are listed in the "Alternate Sequences" column (I did not include/count non-increasing gap sequences such as {1, 8, 9, 5} which would have the same 35 worst case comparisons for N=10). For N=6 though N=16 my results match the table of optimal gap sequences found at https://sortingalgos.miraheze.org/wiki/Shellsort. I can't guarentee for N>16 that this table does not have any mistakes because the numbers get too big to search exhaustively all possibilities. For example, for N=45, while the worst case that I was able to find for the gap sequence {1, 4, 9, 11, 21} used exactly 380 comparisons, I can't say with 100% certainty that there doesn't exist some ordering that requires slightly more than 380 comparisons for this particular gap sequence, and I can't say with 100% certainty that there doesn't exist some other gap sequence with a better worst case (although all other gap sequences I checked required 380 or more comparisons in the worst case I could find). For N>64 I stopped checking gap sequences containing gaps greater than N/2 to reduce the search space. 

| N | Best Sequence | Worst-Case Comparisons | Alternate Sequences |
| :---:     |    :---: |     :---: |     :---: |
| 6  |  1, 4 |  14 |  {1,3,5} and {1,4,5}  |
| 7  |  1, 4, 6 |  18 |  |
| 8  |  1, 5, 7 |  23 |  |
| 9  |  1, 4, 7 |  29 |  {1,3,4} and 3 others  |
| 10  |  1, 6, 9 |  35 |  {1,5,8,9} and {1,6,8,9}  |
| 11  |  1, 4, 5 |  41 |  {1,6,9,10}  |
| 12  |  1, 3, 7, 11 |  48 |  {1,7,10,11}  |
| 13  |  1, 5, 7 |  56 |  {1,2,5} and 10 others  |
| 14  |  1, 4, 7, 9 |  64 |  {1,3,4} and 18 others  |
| 15  |  1, 4, 7, 9 |  71 |  {1,3,7} and 6 others  |
| 16  |  1, 4, 7, 9 |  78 |  |
| 17  |  1, 4, 6, 9 |  86 |  |
| 18  |  1, 4, 6, 9, 17 |  94 |  |
| 19  |  1, 4, 7, 9 |  103 |  |
| 20  |  1, 4, 7, 9 |  113 |  {1,4,6,9} and 3 others  |
| 21  |  1, 4, 7, 9 |  121 |  {1,4,7,9,20}  |
| 22  |  1, 4, 7, 9 |  131 |  {1,3,7,8} and 2 others  |
| 23  |  1, 4, 7, 9 |  139 |  {1,3,7,12} and {1,4,7,9,22}  |
| 24  |  1, 4, 7, 9 |  148 |  |
| 25  |  1, 4, 7, 9, 24 |  158 |  |
| 26  |  1, 4, 7, 9 |  167 |  |
| 27  |  1, 4, 7, 9, 26 |  177 |  |
| 28  |  1, 4, 7, 9 |  187 |  {1,4,7,9,26}  |
| 29  |  1, 4, 7, 9 |  198 |  {1,4,7,9,28}  |
| 30  |  1, 4, 7, 9, 29 |  207 |  |
| 31  |  1, 4, 7, 9 |  220 |  |
| 32  |  1, 4, 7, 9 |  229 |  |
| 33  |  1, 4, 7, 9 |  240 |  {1,4,7,9,32}  |
| 34  |  1, 4, 7, 9, 33 |  251 |  |
| 35  |  1, 4, 7, 9 |  262 |  {1,4,7,9,33}  |
| 36  |  1, 4, 7, 9, 19, 34 |  275 |  |
| 37  |  1, 4, 7, 9 |  286 |  {1,4,7,9,35}  |
| 38  |  1, 4, 7, 9, 19 |  298 |  {1,4,9,11,21,35}  |
| 39  |  1, 4, 9, 11, 21, 38 |  310 |  |
| 40  |  1, 4, 7, 9, 28 |  321 |  |
| 41  |  1, 4, 7, 9, 28, 40 |  334 |  {1,4,9,11,21} and {1,4,9,11,21,40}  |
| 42  |  1, 4, 7, 9, 28, 41 |  345 |  {1,4,9,11} and {1,4,9,11,21,40}  |
| 43  |  1, 4, 9, 11, 21, 40 |  356 |  {1,4,9,11,21,41}  |
| 44  |  1, 4, 9, 11, 21 |  368 |  {1,4,9,11,21,41}  |
| 45  |  1, 4, 9, 11, 21 |  380 |  {1,4,9,11,21,44}  |
| 64  |  1, 4, 9, 11, 32 |  >= 630 |  ?  |
| 91  |  1, 4, 9, 11, 30, 44 |  >= 1041 |  ?  |



I have built and run the code on MacOS using the default c compiler in Xcode, and I have built and run it on Windows using the default c compiler in Codeblocks. 
When building the code, I have always used the -O3 optimization flag, and left the rest of the default compiler settings. 
