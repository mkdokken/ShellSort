typedef struct {
    I64* sequence;
    double mean;
    double M2;
    I64 sampleCount;
    I64 count;
    double score;  // mean + 2*stdErr, calculated before sorting
}
FixedSizeCandidate;

typedef struct {
    FixedSizeCandidate* candidates;
    I64 startIndex;
    I64 lastIndex;
    I64 arraySize;
    I64 numSamples;
    int* array;
    U64 pcgInitState;
    U64 pcgInc;
}
FixedSizeThreadArg;

int compare_fixedSizeCandidates(const void* a, const void* b) {
    FixedSizeCandidate* candidateA = (FixedSizeCandidate*)a;
    FixedSizeCandidate* candidateB = (FixedSizeCandidate*)b;
    
    // Sort by pre-calculated score (mean + 2*stdErr)
    if (candidateA->score < candidateB->score) {
        return -1;
    }
    else if (candidateA->score > candidateB->score) {
        return 1;
    }
    return 0;
}

void* thread_runFixedSizeSamples(void* arg_) {
    FixedSizeThreadArg* arg = arg_;
    if (arg->lastIndex < 0 || arg->lastIndex < arg->startIndex) {
        return NULL;
    }
    
    FixedSizeCandidate* candidates = arg->candidates;
    I64 arraySize = arg->arraySize;
    int* array = arg->array;
    
    initializeArray(array, arraySize);
    
    for (I64 i = arg->startIndex; i <= arg->lastIndex; i++) {
        I64* sequence = candidates[i].sequence;
        srand_pcg(arg->pcgInitState, arg->pcgInc);

        for (I64 j = 0; j < arg->numSamples; j++) {
            shuffleArray(array, arraySize);
            COMPARE_COUNTER = 0;
            shellSortCustom(array, arraySize, sequence);
            candidates[i].count += COMPARE_COUNTER;
            
            // Update using Welford's online algorithm
            candidates[i].sampleCount += 1;
            double delta = COMPARE_COUNTER - candidates[i].mean;
            candidates[i].mean += delta / candidates[i].sampleCount;
            double delta2 = COMPARE_COUNTER - candidates[i].mean;
            candidates[i].M2 += delta * delta2;
        }

        if (!isArraySorted(array, arraySize)) {
            printf("error in thread_runFixedSizeSamples\n");
            exit(1);
        }
    }
    
    return NULL;
}

// Helper function to mutate a number within bounds
// 50% chance to increase, 50% to decrease
// 1/3 of the time: full range mutation
// 1/3 of the time: at most sqrt(range) away
// 1/3 of the time: at most range*0.10 away
I64 mutateNumber(I64 currentNumber, I64 lowerBound, I64 upperBound) {
    // Decide whether to increase or decrease (50/50)
    int shouldIncrease = rand_pcg_u32_bounded(2);
    
    I64 numPossibleNumbers;
    
    if (shouldIncrease) {
        numPossibleNumbers = upperBound - currentNumber - 1;
    } else {
        numPossibleNumbers = currentNumber - lowerBound - 1;
    }
    
    if (numPossibleNumbers <= 0) {
        // Can't go in that direction, try the opposite
        shouldIncrease = !shouldIncrease;
        if (shouldIncrease) {
            numPossibleNumbers = upperBound - currentNumber - 1;
        } else {
            numPossibleNumbers = currentNumber - lowerBound - 1;
        }
    }
    
    if (numPossibleNumbers <= 0) {
        // No mutations possible
        return currentNumber;
    }
    
    // Calculate sqrt and range*0.10
    I64 sqrtRange = (I64)sqrt((double)numPossibleNumbers);
    //I64 sqrtSqrtRange = (I64)sqrt((double)sqrtRange);
    I64 range10 = (I64)(numPossibleNumbers * 0.10);
    if (sqrtRange < 1) sqrtRange = 1;
    if (range10 < 1) range10 = 1;
    
    // Decide which range to use (1/3 for each)
    int rangeChoice = rand_pcg_u32_bounded(3);// 3 for full range, sqrt range, range*0.10
    
    I64 maxDistance;
    if (rangeChoice == 0) {
        // Full range
        maxDistance = numPossibleNumbers;
    } else if (rangeChoice == 1) {
        // sqrt range
        maxDistance = sqrtRange;
    } else {
        // range*0.10
        maxDistance = range10;
    }
    
    // Make sure maxDistance doesn't exceed numPossibleNumbers
    if (maxDistance > numPossibleNumbers) {
        maxDistance = numPossibleNumbers;
    }
    
    // Generate a random offset (1 to maxDistance)
    I64 offset = rand_pcg_u32_bounded(maxDistance) + 1;
    
    I64 newValue;
    if (shouldIncrease) {
        newValue = currentNumber + offset;
    } else {
        newValue = currentNumber - offset;
    }
    
    return newValue;
}

// mutate sequenceFrom into sequenceTo, by changing some of the last numGapsToChange gaps
void mutateSequence(I64 const* sequenceFrom, I64* sequenceTo, int sequenceLength, int numGapsToChange, I64 arraySize) {
    // first, copy the entire sequenceFrom to sequenceTo
    memcpy(sequenceTo, sequenceFrom, sizeof(I64) * (sequenceLength + 1));
    
    // add this line to avoid changing the largest gap, and instead change the 2nd largest//
    //sequenceLength -= 1;
    
    int choice = rand_pcg_u32_bounded(10);
    
    if (choice < 4) {
        // 40% chance to change all gaps randomly
        I64 minGap = sequenceFrom[sequenceLength - 1 - numGapsToChange] + 1;
        I64 maxGap = arraySize - 1;
        for (int i = sequenceLength - numGapsToChange; i < sequenceLength; i++) {
            U64 bound = maxGap - minGap + 1;
            if (bound >= 1LLU << 32) {
                printf("error 2185\n");
                exit(1);
            }
            sequenceTo[i] = minGap + (I64)rand_pcg_u32_bounded((U32)bound);

            // insert into correct position in gaps
            for (int j = i; j > sequenceLength - numGapsToChange; j--) {
                if (sequenceTo[j] < sequenceTo[j-1]) {
                    swapInts64bit(&sequenceTo[j], &sequenceTo[j-1]);
                }
                else {
                    break;
                }
            }
        }
    }
    else if (choice < 5) {
        // 10% chance to change all gaps using mutateNumber, from largest to smallest
        for (int i = sequenceLength - 1; i >= sequenceLength - numGapsToChange; i--) {
            I64 currentGap = sequenceFrom[i];
            I64 lowerBound = sequenceFrom[i - 1];  // Gap before this one
            I64 upperBound;
            
            if (i == sequenceLength - 1) {
                // Largest gap
                upperBound = arraySize;
            } else {
                // Use the already-mutated next larger gap
                upperBound = sequenceTo[i + 1];
            }
            
            sequenceTo[i] = mutateNumber(currentGap, lowerBound, upperBound);
        }
    }
    else {
        // 50% chance to change one gap
        int gapIndex = rand_pcg_u32_bounded(numGapsToChange);
        I64 minGap = sequenceFrom[sequenceLength - 2 - gapIndex] + 1;// used to be bug here (sequenceLength - 1 - gapIndex) instead of (sequenceLength - 2 - gapIndex)
        I64 maxGap = arraySize - 1;
        if (gapIndex != 0) {
            maxGap = sequenceFrom[sequenceLength - gapIndex] - 1;
        }
        
        // Use mutateNumber with exclusive bounds
        I64 currentGap = sequenceFrom[sequenceLength - 1 - gapIndex];
        sequenceTo[sequenceLength - 1 - gapIndex] = mutateNumber(currentGap, minGap - 1, maxGap + 1);
    }
}

void findOptimalSequenceForFixedSize(
    I64* initialSequence, // initial sequence to start with, must end with -1
    I64 arraySize,
    int numGapsToChange,
    double maxRuntime, // in seconds
    int numThreads,
    FixedSizeCandidate* result // if non-NULL, fill with best candidate info
) {
    int initialSequenceLength = 0;
    while (initialSequence[initialSequenceLength] >= 0) {
        initialSequenceLength++;
    }

    printf("Finding optimal sequence for fixed size %lld with %d gaps to change\n", arraySize, numGapsToChange);
    printf("Initial sequence: {");
    for (int i = 0; i < initialSequenceLength; i++) {
        printf("%lld", initialSequence[i]);
        if (i < initialSequenceLength - 1) printf(", ");
    }
    printf("}\n");

    const int numCandidates = 20;
    const double numStdErrs = 1.6; // Number of standard errors for score calculation and cutoff

    FixedSizeCandidate* candidates = malloc(sizeof(FixedSizeCandidate) * numCandidates);
    for (int i = 0; i < numCandidates; i++) {
        candidates[i].sequence = malloc(sizeof(I64) * (initialSequenceLength + 1));
        candidates[i].count = 0;
        candidates[i].sampleCount = 0;
        candidates[i].mean = 0;
        candidates[i].M2 = 0;
        candidates[i].score = 0;
    }
    
    // Initialize first candidate with the initial sequence
    memcpy(candidates[0].sequence, initialSequence, sizeof(I64) * (initialSequenceLength + 1));
    
    // Initialize all other candidates with mutations of the initial sequence
    for (int i = 1; i < numCandidates; i++) {
        mutateSequence(initialSequence, candidates[i].sequence, initialSequenceLength, numGapsToChange, arraySize);
    }

    U64 startTime = currentTime();
    U64 lastPrintTime = startTime;
    const int numSamples = 20;

    pthread_t threads[numThreads];
    FixedSizeThreadArg threadArgs[numThreads];
    int* array_for_thread[numThreads];

    for (int i = 0; i < numThreads; i++) {
        array_for_thread[i] = malloc(sizeof(int) * arraySize);
    }
    for (int i = 0; i < numThreads; i++) {
        if (i == 0) {
            threadArgs[i].startIndex = 0;
        }
        else {
            threadArgs[i].startIndex = threadArgs[i-1].lastIndex + 1;
        }
        if (i == numThreads - 1) {
            threadArgs[i].lastIndex = numCandidates - 1;
        }
        else {
            threadArgs[i].lastIndex = ((i+1) * numCandidates) / numThreads - 1;
        }
        threadArgs[i].candidates = candidates;
        threadArgs[i].arraySize = arraySize;
        threadArgs[i].numSamples = numSamples;
        threadArgs[i].array = array_for_thread[i];
    }

    while (currentTime() - startTime < maxRuntime * TICKS_PER_SEC) {
        U64 pcgInitState = rand_pcg_u64();
        U64 pcgInc = rand_pcg_u64();
        for (int i = 0; i < numThreads; i++) {
            threadArgs[i].pcgInitState = pcgInitState;
            threadArgs[i].pcgInc = pcgInc;
            pthread_create(&threads[i], NULL, thread_runFixedSizeSamples, (void*)&threadArgs[i]);
        }
        for (int i = 0; i < numThreads; i++) {
            pthread_join(threads[i], NULL);
        }

        if (COMPARE_COUNTER != 0) {
            printf("error: COMPARE_COUNTER should be 0\n");
            exit(1);
        }
        
        // Calculate score (mean + numStdErrs*stdErr) for all candidates before sorting
        for (int i = 0; i < numCandidates; i++) {
            double variance = candidates[i].M2 / (candidates[i].sampleCount - 1);
            double stdErr = sqrt(variance / candidates[i].sampleCount);
            candidates[i].score = candidates[i].mean + numStdErrs * stdErr;
        }
        
        // Sort by score (mean + 2*stdErr)
        qsort(candidates, numCandidates, sizeof(FixedSizeCandidate), compare_fixedSizeCandidates);

        // The best candidate is now at index 0 (lowest score)
        double bestVariance = candidates[0].M2 / (candidates[0].sampleCount - 1);
        double bestStdErr = sqrt(bestVariance / candidates[0].sampleCount);
        double bestScore = candidates[0].score;  // Already calculated
        
        // Check each other candidate and replace if statistically significantly worse
        for (int i = 1; i < numCandidates; i++) {
            double candidateVariance = candidates[i].M2 / (candidates[i].sampleCount - 1);
            double candidateStdErr = sqrt(candidateVariance / candidates[i].sampleCount);
            
            // Calculate standard error of the difference
            double stdErrOfDiff = sqrt(bestVariance / candidates[0].sampleCount + 
                                      candidateVariance / candidates[i].sampleCount);
            
            double meanDiff = candidates[i].mean - candidates[0].mean;
            
            // If this candidate is more than numStdErrs * stdErrOfDiff worse, replace it
            // BUT don't replace if it has more than 5x the samples of the best candidate
            if (meanDiff > numStdErrs * stdErrOfDiff && 
                candidates[i].sampleCount <= 5 * candidates[0].sampleCount) {
                // Choose a random candidate to mutate from
                int sourceIdx = rand_pcg_u32_bounded(i);
                mutateSequence(candidates[sourceIdx].sequence, candidates[i].sequence, 
                              initialSequenceLength, numGapsToChange, arraySize);
                
                // Reset statistics for this candidate
                candidates[i].count = 0;
                candidates[i].sampleCount = 0;
                candidates[i].mean = 0;
                candidates[i].M2 = 0;
                candidates[i].score = 0;
            }
        }

        // once every 5 seconds print the current list of candidates
        if (currentTime() - lastPrintTime > 5 * TICKS_PER_SEC) {
            lastPrintTime = currentTime();
            printf("Best candidate (sorted by mean+2*stderr): mean = %g, stderr = %g, mean+2*stderr = %g\n",
                   candidates[0].mean, bestStdErr, bestScore);
            // Calculate stdErrs and print mean, stderr, sample count, and full sequence for each candidate
            for (I64 i = numCandidates-1; i >= 0; i--) {
                double sample_variance = candidates[i].M2 / (candidates[i].sampleCount - 1);
                double stdErr = sqrt(sample_variance / candidates[i].sampleCount);
                double score = candidates[i].mean + numStdErrs * stdErr;
                printf("candidate %lld: mean = %g, stderr = %g, mean+%.1f*stderr = %g, sample count = %lld\n", 
                       i, candidates[i].mean, stdErr, numStdErrs, score, candidates[i].sampleCount);
                printf("{");
                for (int j = 0; j < initialSequenceLength; j++) {
                    printf("%lld", candidates[i].sequence[j]);
                    if (j < initialSequenceLength - 1) printf(", ");
                }
                printf("}\n");
            }
            printf("\n");
        }
        
        // Check all consecutive pairs for duplicates, and mutate the one with fewer samples
        for (int i = 0; i < numCandidates - 1; i++) {
            int areDuplicates = 1;
            for (int j = 0; j < initialSequenceLength; j++) {
                if (candidates[i].sequence[j] != candidates[i+1].sequence[j]) {
                    areDuplicates = 0;
                    break;
                }
            }
            
            if (areDuplicates) {
                // Mutate and reset the one with fewer samples
                int toReset = (candidates[i].sampleCount < candidates[i+1].sampleCount) ? i : i+1;
                int sourceIdx = rand_pcg_u32_bounded(numCandidates);
                mutateSequence(candidates[sourceIdx].sequence, candidates[toReset].sequence, 
                              initialSequenceLength, numGapsToChange, arraySize);
                candidates[toReset].count = 0;
                candidates[toReset].sampleCount = 0;
                candidates[toReset].mean = 0;
                candidates[toReset].M2 = 0;
                candidates[toReset].score = 0;
            }
        }
    }
    
    // Final sort by score
    for (int i = 0; i < numCandidates; i++) {
        if (candidates[i].sampleCount > 1) {
            double variance = candidates[i].M2 / (candidates[i].sampleCount - 1);
            double stdErr = sqrt(variance / candidates[i].sampleCount);
            candidates[i].score = candidates[i].mean + numStdErrs * stdErr;
        } else {
            candidates[i].score = candidates[i].mean + 1e9;
        }
    }
    qsort(candidates, numCandidates, sizeof(FixedSizeCandidate), compare_fixedSizeCandidates);
    
    // Fill result if requested
    if (result != NULL) {
        I64 maxSamples = 0;
        for (int i = 0; i < numCandidates; i++) {
            if (candidates[i].sampleCount > maxSamples) {
                maxSamples = candidates[i].sampleCount;
            }
        }
        
        int bestIdx = -1;
        for (int i = 0; i < numCandidates; i++) {
            if (candidates[i].sampleCount >= maxSamples / 2) {
                bestIdx = i;
                break;
            }
        }
        if (bestIdx < 0) bestIdx = 0;
        
        memcpy(result->sequence, candidates[bestIdx].sequence, sizeof(I64) * (initialSequenceLength + 1));
        result->mean = candidates[bestIdx].mean;
        result->M2 = candidates[bestIdx].M2;
        result->sampleCount = candidates[bestIdx].sampleCount;
        result->score = candidates[bestIdx].score;
    }
    
    // Cleanup
    for (int i = 0; i < numThreads; i++) {
        free(array_for_thread[i]);
    }
    for (int i = 0; i < numCandidates; i++) {
        free(candidates[i].sequence);
    }
    free(candidates);
}

// Run additional sorting samples on multiple candidates to refine their statistics.
// Each candidate's sampleCount/mean/M2/score fields are updated in place.
void runAdditionalSamples(
    FixedSizeCandidate* candidates, int numCandidates,
    I64 arraySize, double maxRuntime, int numThreads
) {
    const int numSamples = 20;
    
    pthread_t threads[numThreads];
    FixedSizeThreadArg threadArgs[numThreads];
    int* array_for_thread[numThreads];
    
    for (int i = 0; i < numThreads; i++) {
        array_for_thread[i] = malloc(sizeof(int) * arraySize);
    }
    for (int i = 0; i < numThreads; i++) {
        if (i == 0) {
            threadArgs[i].startIndex = 0;
        } else {
            threadArgs[i].startIndex = threadArgs[i-1].lastIndex + 1;
        }
        if (i == numThreads - 1) {
            threadArgs[i].lastIndex = numCandidates - 1;
        } else {
            threadArgs[i].lastIndex = ((i+1) * numCandidates) / numThreads - 1;
        }
        threadArgs[i].candidates = candidates;
        threadArgs[i].arraySize = arraySize;
        threadArgs[i].numSamples = numSamples;
        threadArgs[i].array = array_for_thread[i];
    }
    
    U64 startTime = currentTime();
    while (currentTime() - startTime < maxRuntime * TICKS_PER_SEC) {
        U64 pcgInitState = rand_pcg_u64();
        U64 pcgInc = rand_pcg_u64();
        for (int i = 0; i < numThreads; i++) {
            threadArgs[i].pcgInitState = pcgInitState;
            threadArgs[i].pcgInc = pcgInc;
            pthread_create(&threads[i], NULL, thread_runFixedSizeSamples, (void*)&threadArgs[i]);
        }
        for (int i = 0; i < numThreads; i++) {
            pthread_join(threads[i], NULL);
        }
    }
    
    // Update scores
    for (int i = 0; i < numCandidates; i++) {
        if (candidates[i].sampleCount > 1) {
            double variance = candidates[i].M2 / (candidates[i].sampleCount - 1);
            double stdErr = sqrt(variance / candidates[i].sampleCount);
            candidates[i].score = candidates[i].mean + 1.6 * stdErr;
        } else {
            candidates[i].score = candidates[i].mean + 1e9;
        }
    }
    
    for (int i = 0; i < numThreads; i++) {
        free(array_for_thread[i]);
    }
}

static void printFixedSizeResult(const char* label, FixedSizeCandidate* r) {
    int seqLen = 0;
    while (r->sequence[seqLen] >= 0) seqLen++;
    double stdErr = 0;
    if (r->sampleCount > 1) {
        double variance = r->M2 / (r->sampleCount - 1);
        stdErr = sqrt(variance / r->sampleCount);
    }
    printf("  %s: mean=%.2f, stdErr=%.2f, samples=%lld  {",
           label, r->mean, stdErr, r->sampleCount);
    for (int i = 0; i < seqLen; i++) {
        printf("%lld", r->sequence[i]);
        if (i < seqLen - 1) printf(", ");
    }
    printf("}\n");
}

static double crossRunScore(FixedSizeCandidate* r) {
    if (r->sampleCount > 1) {
        double variance = r->M2 / (r->sampleCount - 1);
        double stdErr = sqrt(variance / r->sampleCount);
        return r->mean + 3.2 * stdErr;
    }
    return r->mean + 1e9;
}

void findOptimalSequenceForFixedSizeAutomated(
    I64* initialSequence, // must end with -1
    I64 arraySize,
    double maxRuntime, // in seconds
    int numThreads
) {
    int seqLen = 0;
    while (initialSequence[seqLen] >= 0) {
        seqLen++;
    }
    
    printf("\n=== Automated Fixed-Size Optimizer ===\n");
    printf("Array size: %lld, sequence length: %d, max runtime: %.1f seconds\n", arraySize, seqLen, maxRuntime);
    printf("Initial sequence: {");
    for (int i = 0; i < seqLen; i++) {
        printf("%lld", initialSequence[i]);
        if (i < seqLen - 1) printf(", ");
    }
    printf("}\n\n");
    
    // Allocate working sequence buffer (reused for constructing modified initial sequences)
    // seqLen+2 to accommodate longer sequence (seqLen+1 gaps + terminator)
    I64* workSeq = malloc(sizeof(I64) * (seqLen + 2));
    
    // Allocate result structs with sequence buffers
    // We track up to 10 results (some phases produce multiple)
    const int MAX_RESULTS = 10;
    FixedSizeCandidate results[MAX_RESULTS];
    for (int i = 0; i < MAX_RESULTS; i++) {
        results[i].sequence = malloc(sizeof(I64) * (seqLen + 2));
        results[i].sequence[0] = -1;
        results[i].mean = 0;
        results[i].M2 = 0;
        results[i].count = 0;
        results[i].sampleCount = 0;
        results[i].score = 1e18;
    }
    // result indices:
    // 0 = A1 (1 gap changed)
    // 1 = A2 (2 gaps changed)
    // 2 = A3 (best of 3 runs with 3 gaps changed)
    // 3 = B1 (2 gaps changed, refined 3rd-to-last from A3)
    // 4 = B2 (1 gap changed, refined 2nd+3rd-to-last from B1)
    // 5 = C4 (1 gap changed, refined from 4-gap search)
    // 6 = C1 (4 gaps changed directly)
    // 7 = D  (best from shorter sequence)
    // 8 = F  (best from longer sequence)
    // 9 = scratch
    
    int numResults = 0;
    
    // Temporary result for intermediate phases
    FixedSizeCandidate tempResult;
    tempResult.sequence = malloc(sizeof(I64) * (seqLen + 2));
    tempResult.sequence[0] = -1;
    
    FixedSizeCandidate tempResult2;
    tempResult2.sequence = malloc(sizeof(I64) * (seqLen + 2));
    tempResult2.sequence[0] = -1;
    
    // ===== Phase A: Search with numGapsToChange = 1, 2, 3 =====
    printf("--- Phase A1: numGapsToChange=1 (1%% of time) ---\n");
    memcpy(workSeq, initialSequence, sizeof(I64) * (seqLen + 1));
    findOptimalSequenceForFixedSize(workSeq, arraySize, 1, maxRuntime * 0.01, numThreads, &results[0]);
    printFixedSizeResult("A1 result", &results[0]);
    numResults = 1;
    
    printf("\n--- Phase A2: numGapsToChange=2 (4%% of time) ---\n");
    memcpy(workSeq, initialSequence, sizeof(I64) * (seqLen + 1));
    findOptimalSequenceForFixedSize(workSeq, arraySize, 2, maxRuntime * 0.04, numThreads, &results[1]);
    printFixedSizeResult("A2 result", &results[1]);
    numResults = 2;
    
    printf("\n--- Phase A3: numGapsToChange=3 (4 x 7.5%% of time) ---\n");
    // Run 4 times, keep the best using cross-run score (mean + 3.2*stdErr)
    memcpy(workSeq, initialSequence, sizeof(I64) * (seqLen + 1));
    findOptimalSequenceForFixedSize(workSeq, arraySize, 3, maxRuntime * 0.075, numThreads, &results[2]);
    printFixedSizeResult("A3 run1", &results[2]);
    
    for (int run = 1; run < 4; run++) {
        memcpy(workSeq, initialSequence, sizeof(I64) * (seqLen + 1));
        findOptimalSequenceForFixedSize(workSeq, arraySize, 3, maxRuntime * 0.075, numThreads, &tempResult);
        printf("  A3 run%d: crossRunScore=%.2f", run + 1, crossRunScore(&tempResult));
        printFixedSizeResult("", &tempResult);
        
        if (crossRunScore(&tempResult) < crossRunScore(&results[2])) {
            memcpy(results[2].sequence, tempResult.sequence, sizeof(I64) * (seqLen + 1));
            results[2].mean = tempResult.mean;
            results[2].M2 = tempResult.M2;
            results[2].sampleCount = tempResult.sampleCount;
            results[2].score = tempResult.score;
        }
    }
    printFixedSizeResult("A3 best", &results[2]);
    numResults = 3;
    
    // ===== Phase B: Refine from result3 =====
    printf("\n--- Phase B1: numGapsToChange=2, refined 3rd-to-last from A3 (2 x 10%% of time) ---\n");
    memcpy(workSeq, initialSequence, sizeof(I64) * (seqLen + 1));
    workSeq[seqLen - 3] = results[2].sequence[seqLen - 3]; // set 3rd-to-last from A3
    findOptimalSequenceForFixedSize(workSeq, arraySize, 2, maxRuntime * 0.10, numThreads, &results[3]);
    printFixedSizeResult("B1 run1", &results[3]);
    
    memcpy(workSeq, initialSequence, sizeof(I64) * (seqLen + 1));
    workSeq[seqLen - 3] = results[2].sequence[seqLen - 3];
    findOptimalSequenceForFixedSize(workSeq, arraySize, 2, maxRuntime * 0.10, numThreads, &tempResult);
    printFixedSizeResult("B1 run2", &tempResult);
    
    if (crossRunScore(&tempResult) < crossRunScore(&results[3])) {
        memcpy(results[3].sequence, tempResult.sequence, sizeof(I64) * (seqLen + 1));
        results[3].mean = tempResult.mean;
        results[3].M2 = tempResult.M2;
        results[3].sampleCount = tempResult.sampleCount;
        results[3].score = tempResult.score;
    }
    printFixedSizeResult("B1 best", &results[3]);
    numResults = 4;
    
    printf("\n--- Phase B2: numGapsToChange=1, refined 2nd+3rd-to-last from B1 (10%% of time) ---\n");
    memcpy(workSeq, initialSequence, sizeof(I64) * (seqLen + 1));
    workSeq[seqLen - 3] = results[3].sequence[seqLen - 3];
    workSeq[seqLen - 2] = results[3].sequence[seqLen - 2];
    findOptimalSequenceForFixedSize(workSeq, arraySize, 1, maxRuntime * 0.10, numThreads, &results[4]);
    printFixedSizeResult("B2 result", &results[4]);
    numResults = 5;
    
    // ===== Phase C: Search with numGapsToChange=4 and refine =====
    printf("\n--- Phase C1: numGapsToChange=4 (4%% of time) ---\n");
    memcpy(workSeq, initialSequence, sizeof(I64) * (seqLen + 1));
    findOptimalSequenceForFixedSize(workSeq, arraySize, 4, maxRuntime * 0.04, numThreads, &results[6]);
    printFixedSizeResult("C1 result", &results[6]);
    
    printf("\n--- Phase C2: numGapsToChange=3, refined 4th-to-last from C1 (4%% of time) ---\n");
    memcpy(workSeq, initialSequence, sizeof(I64) * (seqLen + 1));
    workSeq[seqLen - 4] = results[6].sequence[seqLen - 4];
    findOptimalSequenceForFixedSize(workSeq, arraySize, 3, maxRuntime * 0.04, numThreads, &tempResult2);
    printFixedSizeResult("C2 result", &tempResult2);
    
    printf("\n--- Phase C3: numGapsToChange=2, refined 4th+3rd-to-last from C2 (4%% of time) ---\n");
    memcpy(workSeq, initialSequence, sizeof(I64) * (seqLen + 1));
    workSeq[seqLen - 4] = tempResult2.sequence[seqLen - 4];
    workSeq[seqLen - 3] = tempResult2.sequence[seqLen - 3];
    findOptimalSequenceForFixedSize(workSeq, arraySize, 2, maxRuntime * 0.04, numThreads, &tempResult);
    printFixedSizeResult("C3 result", &tempResult);
    
    printf("\n--- Phase C4: numGapsToChange=1, refined 4th+3rd+2nd-to-last from C3 (4%% of time) ---\n");
    memcpy(workSeq, initialSequence, sizeof(I64) * (seqLen + 1));
    workSeq[seqLen - 4] = tempResult.sequence[seqLen - 4];
    workSeq[seqLen - 3] = tempResult.sequence[seqLen - 3];
    workSeq[seqLen - 2] = tempResult.sequence[seqLen - 2];
    findOptimalSequenceForFixedSize(workSeq, arraySize, 1, maxRuntime * 0.04, numThreads, &results[5]);
    printFixedSizeResult("C4 result", &results[5]);
    numResults = 7;
    
    // ===== Phase D: Verify shorter sequence =====
    printf("\n--- Phase D: Verify removing last gap (~5%% of time) ---\n");
    // Build shortened sequence: remove last gap, so seqLen-1 gaps, terminated with -1
    I64* shortSeq = malloc(sizeof(I64) * (seqLen + 1));
    memcpy(shortSeq, initialSequence, sizeof(I64) * (seqLen - 1));
    shortSeq[seqLen - 1] = -1;
    int shortLen = seqLen - 1;
    
    FixedSizeCandidate shortResult;
    shortResult.sequence = malloc(sizeof(I64) * (seqLen + 2));
    shortResult.sequence[0] = -1;
    shortResult.score = 1e18;
    shortResult.sampleCount = 0;
    shortResult.mean = 0;
    shortResult.M2 = 0;
    shortResult.count = 0;
    
    double dTime = maxRuntime * 0.05 / 3.0;
    
    // D1: numGapsToChange=1
    memcpy(workSeq, shortSeq, sizeof(I64) * (shortLen + 1));
    findOptimalSequenceForFixedSize(workSeq, arraySize, 1, dTime, numThreads, &shortResult);
    printFixedSizeResult("D1 (short, 1 gap)", &shortResult);
    
    // D2: numGapsToChange=2
    FixedSizeCandidate shortTemp;
    shortTemp.sequence = malloc(sizeof(I64) * (seqLen + 2));
    shortTemp.sequence[0] = -1;
    memcpy(workSeq, shortSeq, sizeof(I64) * (shortLen + 1));
    findOptimalSequenceForFixedSize(workSeq, arraySize, 2, dTime, numThreads, &shortTemp);
    printFixedSizeResult("D2 (short, 2 gaps)", &shortTemp);
    if (crossRunScore(&shortTemp) < crossRunScore(&shortResult)) {
        memcpy(shortResult.sequence, shortTemp.sequence, sizeof(I64) * (shortLen + 1));
        shortResult.mean = shortTemp.mean;
        shortResult.M2 = shortTemp.M2;
        shortResult.sampleCount = shortTemp.sampleCount;
        shortResult.score = shortTemp.score;
    }
    
    // D3: numGapsToChange=3
    memcpy(workSeq, shortSeq, sizeof(I64) * (shortLen + 1));
    findOptimalSequenceForFixedSize(workSeq, arraySize, 3, dTime, numThreads, &shortTemp);
    printFixedSizeResult("D3 (short, 3 gaps)", &shortTemp);
    if (crossRunScore(&shortTemp) < crossRunScore(&shortResult)) {
        memcpy(shortResult.sequence, shortTemp.sequence, sizeof(I64) * (shortLen + 1));
        shortResult.mean = shortTemp.mean;
        shortResult.M2 = shortTemp.M2;
        shortResult.sampleCount = shortTemp.sampleCount;
        shortResult.score = shortTemp.score;
    }
    
    // Copy best short result to results[7]
    memcpy(results[7].sequence, shortResult.sequence, sizeof(I64) * (seqLen + 1));
    results[7].mean = shortResult.mean;
    results[7].M2 = shortResult.M2;
    results[7].sampleCount = shortResult.sampleCount;
    results[7].score = shortResult.score;
    printFixedSizeResult("D best (short)", &results[7]);
    numResults = 8;
    
    // ===== Phase F: Verify longer sequence (add 1 extra gap) =====
    printf("\n--- Phase F: Verify adding one extra gap (~6%% of time) ---\n");
    // Build lengthened sequence: original + one extra gap initialized to arraySize-2, terminated with -1
    I64* longSeq = malloc(sizeof(I64) * (seqLen + 2));
    memcpy(longSeq, initialSequence, sizeof(I64) * seqLen);
    longSeq[seqLen] = arraySize - 2;
    longSeq[seqLen + 1] = -1;
    int longLen = seqLen + 1;
    
    FixedSizeCandidate longResult;
    longResult.sequence = malloc(sizeof(I64) * (seqLen + 2));
    longResult.sequence[0] = -1;
    longResult.score = 1e18;
    longResult.sampleCount = 0;
    longResult.mean = 0;
    longResult.M2 = 0;
    longResult.count = 0;
    
    double fTime = maxRuntime * 0.06 / 4.0;
    
    // F1: numGapsToChange=1, using B2 result with extra gap appended
    int b2Len = 0;
    while (results[4].sequence[b2Len] >= 0) b2Len++;
    memcpy(workSeq, results[4].sequence, sizeof(I64) * b2Len);
    workSeq[b2Len] = arraySize - 2;
    workSeq[b2Len + 1] = -1;
    findOptimalSequenceForFixedSize(workSeq, arraySize, 1, fTime, numThreads, &longResult);
    printFixedSizeResult("F1 (B2+extra, 1 gap)", &longResult);
    
    // F2: numGapsToChange=2
    FixedSizeCandidate longTemp;
    longTemp.sequence = malloc(sizeof(I64) * (seqLen + 2));
    longTemp.sequence[0] = -1;
    memcpy(workSeq, longSeq, sizeof(I64) * (longLen + 1));
    findOptimalSequenceForFixedSize(workSeq, arraySize, 2, fTime, numThreads, &longTemp);
    printFixedSizeResult("F2 (long, 2 gaps)", &longTemp);
    if (crossRunScore(&longTemp) < crossRunScore(&longResult)) {
        memcpy(longResult.sequence, longTemp.sequence, sizeof(I64) * (longLen + 1));
        longResult.mean = longTemp.mean;
        longResult.M2 = longTemp.M2;
        longResult.sampleCount = longTemp.sampleCount;
        longResult.score = longTemp.score;
    }
    
    // F3: numGapsToChange=3
    memcpy(workSeq, longSeq, sizeof(I64) * (longLen + 1));
    findOptimalSequenceForFixedSize(workSeq, arraySize, 3, fTime, numThreads, &longTemp);
    printFixedSizeResult("F3 (long, 3 gaps)", &longTemp);
    if (crossRunScore(&longTemp) < crossRunScore(&longResult)) {
        memcpy(longResult.sequence, longTemp.sequence, sizeof(I64) * (longLen + 1));
        longResult.mean = longTemp.mean;
        longResult.M2 = longTemp.M2;
        longResult.sampleCount = longTemp.sampleCount;
        longResult.score = longTemp.score;
    }
    
    // F4: numGapsToChange=4
    memcpy(workSeq, longSeq, sizeof(I64) * (longLen + 1));
    findOptimalSequenceForFixedSize(workSeq, arraySize, 4, fTime, numThreads, &longTemp);
    printFixedSizeResult("F4 (long, 4 gaps)", &longTemp);
    if (crossRunScore(&longTemp) < crossRunScore(&longResult)) {
        memcpy(longResult.sequence, longTemp.sequence, sizeof(I64) * (longLen + 1));
        longResult.mean = longTemp.mean;
        longResult.M2 = longTemp.M2;
        longResult.sampleCount = longTemp.sampleCount;
        longResult.score = longTemp.score;
    }
    
    // Copy best long result to results[8]
    memcpy(results[8].sequence, longResult.sequence, sizeof(I64) * (longLen + 1));
    results[8].mean = longResult.mean;
    results[8].M2 = longResult.M2;
    results[8].sampleCount = longResult.sampleCount;
    results[8].score = longResult.score;
    printFixedSizeResult("F best (long)", &results[8]);
    numResults = 9;
    
    // ===== Final validation with additional samples =====
    printf("\n--- Running additional samples on %d best results (8%% of time) ---\n", numResults);
    runAdditionalSamples(results, numResults, arraySize, maxRuntime * 0.08, numThreads);
    
    // ===== Final comparison =====
    printf("\n=== Final Results (after additional samples) ===\n");
    const char* phaseNames[] = {"A1(1gap)", "A2(2gap)", "A3(3gap)", "B1(ref2)", "B2(ref1)", "C4(ref4)", "C1(4gap)", "D(short)", "F(long)"};
    
    int bestIdx = 0;
    for (int i = 0; i < numResults; i++) {
        printFixedSizeResult(phaseNames[i], &results[i]);
        if (results[i].score < results[bestIdx].score) {
            bestIdx = i;
        }
    }
    
    printf("\n=== Overall Best: %s ===\n", phaseNames[bestIdx]);
    printFixedSizeResult("BEST", &results[bestIdx]);
    
    // Cleanup
    free(workSeq);
    free(shortSeq);
    free(shortResult.sequence);
    free(shortTemp.sequence);
    free(longSeq);
    free(longResult.sequence);
    free(longTemp.sequence);
    free(tempResult.sequence);
    free(tempResult2.sequence);
    for (int i = 0; i < MAX_RESULTS; i++) {
        free(results[i].sequence);
    }
}
