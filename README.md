# ShellSort
Finding best shell sort gap sequences to minimize comparisons.


This project contains code to attempt to find the best gap sequence for shell sort.


In early 2025 the best sequence the code found was: {1, 4, 10, 23, 57, 132, 301, 701, 1504, 3263, 7196, 15948, 34644, 74428, 162005, 347077, 745919, 1599893, 3446017, 7434649, 15933053}, which is an extension of Marcin Ciura's gaps: {1, 4, 10, 23, 57, 132, 301, 701}.
https://stackoverflow.com/questions/2539545/fastest-gap-sequence-for-shell-sort/79637484#79637484

As of January 2026, after improving the code, it finds numerous potentially better sequences such as: {1, 4, 10, 23, 57, 132, 301, 644, 1408, 3227, 6847, 14842, 31970, 69487, 149728}.


When building the code, I have always used the -O3 optimization flag. 
I have built and run it on MacOS using the default c compiler in Xcode, and I have built and run it on Windows using the default c compiler in Codeblocks. 
