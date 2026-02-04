# ShellSort
Finding best shell sort gap sequences to minimize comparisons.


This project contains code to attempt to find the best gap sequence for shell sort.


In early 2025 the best sequences the code found were:
{1, 4, 10, 23, 57, 132, 301, 701, 1504, 3263, 7196, 15948, 34644, 74428, 162005, 347077, 745919, 1599893, 3446017, 7434649, 15933053},
{1, 4, 10, 23, 57, 132, 301, 701, 1541, 3498, 7699, 17041, 37835, 81907, 179433, 392867, 858419, 1883473, 4081849, 9002887, 19782319}, and
{1, 4, 10, 23, 57, 132, 301, 701, 1636, 3659, 8129, 18118, 40354, 89129, 197803, 443557, 973657, 2131981, 4697153, 10528127, 23135351},

which are all extensions of Marcin Ciura's gaps: {1, 4, 10, 23, 57, 132, 301, 701}.
https://stackoverflow.com/questions/2539545/fastest-gap-sequence-for-shell-sort/79637484#79637484


As of 2026, after improving the code, it finds some very good sequences with a 644 instead of a 701 following the 301. For example:
{1, 4, 10, 23, 57, 132, 301, 644, 1408, 3227, 6847, 14842, 31970, 69487, 149728, 324011, 692843, 1499809, 3211613, 7025789, 14937367},
{1, 4, 10, 23, 57, 132, 301, 644, 1408, 3227, 6847, 14917, 32910, 71651, 157678, 345119, 747533, 1631227, 3484469, 7526447, 16541333}, and
{1, 4, 10, 23, 57, 132, 301, 644, 1445, 3165, 6913, 15349, 33794, 75251, 163395, 358349, 784009, 1722313, 3809723, 8255479, 17990407}.

It is unclear to me what the best extension of {1, 4, 10, 23, 57, 132, 301} is when N grows large. It seems it might be one of {701, 1541}, {701, 1504}, {644, 1445}, or {644, 1408}.

The best sequences not starting with {1, 4, 10, 23, 57, 132, 301} seem to start with {1, 4, 10, 21, 56, 125, 288, 661}, but they don't seem to perform as well for large N. 


When building the code, I have always used the -O3 optimization flag. 
I have built and run it on MacOS using the default c compiler in Xcode, and I have built and run it on Windows using the default c compiler in Codeblocks. 
