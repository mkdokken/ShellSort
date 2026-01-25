//
//  main.c
//  ShellSort
//
//  Created by Michael Dokken on 12/28/24.
//

#include <stdio.h> // printf
#include <stdlib.h> // qsort, srand, rand, malloc, free
#include <math.h> // pow, sqrt, erf
#include <string.h> // memcpy, strstr
#include <time.h> // time
#include <sys/time.h> // gettimeofday
#include <pthread.h> // pthread_create, pthread_join, pthread_t
#include <inttypes.h> // uint64_t, uint32_t, int64_t
#include <unistd.h> // getpid

typedef uint64_t U64;
typedef uint32_t U32;
typedef int64_t I64;

//static I64 COMPARE_COUNTER = 0;
static __thread I64 COMPARE_COUNTER = 0;// thread local variable, makes sorting 13% slower but allows each thread to have it's own compare counter

// can be used in qsort
int compare_qsort(const void* a, const void* b) {
    COMPARE_COUNTER++;
    return (*(int*)a - *(int*)b);
}

// returns a - b
static inline int compareInts(int a, int b) {
    return compare_qsort(&a, &b);
    // return a - b;
}

#define TICKS_PER_SEC 1000000llu
static inline U64 currentTime(void) {
    //return clock();
    struct timeval t;
    gettimeofday(&t, NULL);
    return ((U64)t.tv_usec) + TICKS_PER_SEC * ((U64)t.tv_sec);
}

// returns probability that a statistic is less than z
double standardNormalCdf(double z) {
    return 0.5 * (1.0 + erf(z/sqrt(2.0)));
}

// using pcg pseudo-random number generator (other simple prng alternatives include very fast (xoshiro256+), or very high quality (jsf/sfc/arbee)
// has 2^63 possible streams of pseudo-random numbers each with period of 2^64
// _rand_pcg_inc is set once and determines which of 2^63 possible sequences to use
//_rand_pcg_state is modified with each call and will repeat every 2^64 calls
// https://www.pcg-random.org/ , https://github.com/imneme/pcg-c-basic/blob/master/pcg_basic.c
// better than stdlib rand because on my windows laptop rand() defaulted to an lcg outputting a 16-bit integer with period 2^32 and on my macbook rand() defaulted to an lcg outputting a 32 bit integer with period 2^31-1
static __thread U64 _rand_pcg_state;// thread-local so every thread has it's own state
static __thread U64 _rand_pcg_inc;
U32 rand_pcg_u32(void) {
    U64 oldstate = _rand_pcg_state;
    _rand_pcg_state = oldstate * 6364136223846793005ULL + (_rand_pcg_inc | 1);
    U32 xorshifted = (U32) (((oldstate >> 18u) ^ oldstate) >> 27u);
    U32 rot = oldstate >> 59u;
    return (xorshifted >> rot) | (xorshifted << ((-rot) & 31));
}
int rand_pcg_int(void) {
    return rand_pcg_u32() & 0x7FFFFFFFu;// throw away sign bit so can fit in as a positive int
}
U64 rand_pcg_u64(void) {
    return rand_pcg_u32() ^ (((U64)rand_pcg_u32()) << 32);
}
I64 rand_pcg_int64(void) {
    return rand_pcg_u64() & 0x7FFFFFFFFFFFFFFFLLU;// throw away sign bit so can fit in as a positive int
}
void srand_pcg(U64 initState, U64 initInc) {
    _rand_pcg_state = 0;
    _rand_pcg_inc = (initInc << 1) | 1;
    (void)rand_pcg_u32();
    _rand_pcg_state += initState;
    (void)rand_pcg_u32();
}
void srand_pcg_easy(void) {
    U64 initState = currentTime() ^ (((U64)getpid()) << 48);
    U64 initInc = ((U64)&_rand_pcg_inc) ^ (((U64)rand()) << 32) ^ (((U64)rand()) << 48);
    srand_pcg(initState, initInc);
}
U32 rand_pcg_u32_bounded(U32 range) {
    // return random number in [0, range)
    // this implementation is biased but is very fast
    U32 x = rand_pcg_u32();
    U64 m = ((U64)x) * ((U64)range);
    return m >> 32;
}

static inline int randomInt(void) {
    return rand_pcg_int();
}

static inline void swapInts(int* a, int* b) {
    int temp = *a;
    *a = *b;
    *b = temp;
}

static inline void swapInts64bit(I64* a, I64* b) {
    I64 temp = *a;
    *a = *b;
    *b = temp;
}

// fisher yates shuffle
void shuffleArray(int array[], I64 length) {
    if (length < 1000000000) {
        for (int i = (int)(length-1); i > 0; i--) {
            //int j = randomInt() % (i+1);
            int j = rand_pcg_u32_bounded(i+1);
            swapInts(&array[i], &array[j]);
        }
    }
    else {
        for (I64 i = length-1; i > 0; i--) {
            I64 j = rand_pcg_int64() % (i+1);
            //int j = rand_pcg_u32_bounded(i+1);
            swapInts(&array[i], &array[j]);
        }
    }
}

void initializeArray(int array[], I64 length) {
    if (length < (1LL << 31)) {
        // initialize array with unique nonnegative integers starting at 0
        for (int i = 0; i < length; i++) {
            array[i] = i;
        }
    }
    else if (length <= (1LL << 32)) {
        // initialize array with unique integers starting below 0
        int j = (int) (-(length/2));
        for (I64 i = 0; i < length; i++) {
            array[i] = j;
            j++;
        }
    }
    else {
        // length is too big, not enough unique 32-bit integers
        printf("error 258\n");
        exit(1);
    }
}

void reverseArray(int array[], I64 length) {
    for (I64 i = 0; i < length / 2; i++) {
        swapInts(&array[i], &array[length-1-i]);
    }
}

// finds next permutation in lexicographic ordering, algorithm from wikipedia
void nextPermutation(int array[], I64 length) {
    if (length < 2) return;
    
    I64 k = length - 2;
    while (compareInts(array[k], array[k+1]) >= 0) {
        if (k == 0) {
            // we are at the last permutation
            return reverseArray(array, length);
        }
        k--;
    }
    
    I64 m = length - 1;
    while (compareInts(array[k], array[m]) >= 0) {
        if (m == 0) {
            printf("error 284\n");
            exit(1);
        }
        m--;
    }
    
    swapInts(&array[k], &array[m]);
    reverseArray(&array[k+1], length - (k+1));
}

void copyArray(int const arrayFrom[], int arrayTo[], I64 length) {
    memcpy(arrayTo, arrayFrom, length * sizeof(int));
}

void printArray(int const array[], I64 length) {
    for (I64 i = 0; i < length; i++) {
        printf("%d, ", array[i]);
    }
    printf("\n\n");
}

// assumes gap sequence ends with -1 or any negative number
void printGaps(I64 const gaps[]) {
    printf("{");
    for (I64 i = 0; ; i++) {
        printf("%lld, ", gaps[i]);
        if (gaps[i] < 0) {
            break;
        }
    }
    printf("}");
}

// assumes we are sorting ints
void insertionSort(int array[], I64 length) {
    for (I64 i = 1; i < length; i++) {// i is index of element we need to insert
        int temp = array[i];
        I64 j = i-1;
        while (1) {
            if (compareInts(array[j], temp) > 0) {
                array[j+1] = array[j];
            }
            else {
                array[j+1] = temp;
                break;
            }
            if (j == 0) {
                array[0] = temp;
                break;
            }
            j--;
        }
    }
}

// assumes first element in gaps/lastGaps is 1, last element in gaps/lastGaps is -1
void shellSortCustomWithLastGaps(int array[], I64 length, const I64 gaps[], const I64 lastGaps[]) {
    // find initial gap (largest gap that is less than length)
    I64 g = 0;
    while (lastGaps[g] < length && lastGaps[g] > 0) {
        g++;
    }
    
    for (I64 gap = lastGaps[--g]; g > 0; gap = gaps[--g]) {
        for (I64 i = gap; i < length; i++) {// i is index of element we need to insert
            int temp = array[i];
            I64 j = i-gap;
            I64 j2 = i;
            while (1) {
                if (compareInts(array[j], temp) > 0) {
                    array[j2] = array[j];
                }
                else {
                    array[j2] = temp;
                    break;
                }
                if (j < gap) {
                    array[j] = temp;
                    break;
                }
                j2 = j - gap;
                
                // swap places of j and j2 and repeat what is above
                
                if (compareInts(array[j2], temp) > 0) {
                    array[j] = array[j2];
                }
                else {
                    array[j] = temp;
                    break;
                }
                if (j2 < gap) {
                    array[j2] = temp;
                    break;
                }
                j = j2 - gap;
            }
        }
    }
    
    insertionSort(array, length);
}

// assumes first element in gaps is 1, last element in gaps is -1
void shellSortCustom(int array[], I64 length, const I64 gaps[]) {
    shellSortCustomWithLastGaps(array, length, gaps, gaps);
}

// insert element at index i, return index that it got inserted into
static I64 shellSortSingleInsert(int array[], I64 gap, I64 i) {
    int temp = array[i];
    I64 j = i-gap;
    I64 j2 = i;
    while (1) {
        if (compareInts(array[j], temp) > 0) {
            array[j2] = array[j];
        }
        else {
            array[j2] = temp;
            return j2;
        }
        if (j < gap) {
            array[j] = temp;
            return j;
        }
        j2 = j - gap;
        
        // swap places of j and j2 and repeat what is above
        
        if (compareInts(array[j2], temp) > 0) {
            array[j] = array[j2];
        }
        else {
            array[j] = temp;
            return j;
        }
        if (j2 < gap) {
            array[j2] = temp;
            return j2;
        }
        j = j2 - gap;
    }
}

static void shellSortSingleGap(int array[], I64 length, I64 gap) {
    for (I64 i = gap; i < length; i++) {// i is index of element we need to insert
        shellSortSingleInsert(array, gap, i);
    }
}

typedef struct {
    int* array;
    I64 length;
    I64 gap;
} ShellSortParams;

typedef struct {
    ShellSortParams const* params;
    I64 threadNum;
    I64 totalThreads;
    I64 compareCount;
} ShellSortThreadArg;

void* shellSortThreadFunc(void* _arg) {
    ShellSortThreadArg* arg = _arg;
    ShellSortParams const* params = arg->params;
    
    COMPARE_COUNTER = 0;
    
    I64 gap = params->gap;
    I64 length = params->length;
    int* array = params->array;
    
    I64 threadNum = arg->threadNum;
    I64 totalThreads = arg->totalThreads;
    
    for (I64 base = gap; 1; base += gap) {
        for (I64 extra = threadNum; extra < gap; extra += totalThreads) {
            I64 i = base + extra;
            if (i >= length) {
                goto doubleBreak;
            }
            shellSortSingleInsert(array, gap, i);
        }
    }
doubleBreak:
    
    arg->compareCount = COMPARE_COUNTER;
    return NULL;
}

void shellSortCustomWithLastGapsMultithreaded(int array[], I64 length, const I64 gaps[], const I64 lastGaps[], I64 maxThreads) {
    const I64 minLengthPerThread = 1 << 17;// at least 2^17 = 131072 per thread
    if (length < 2 * minLengthPerThread || maxThreads <= 1) {
        return shellSortCustomWithLastGaps(array, length, gaps, lastGaps);
        //maxThreads = 1;
    }
    if (maxThreads > 32) {
        maxThreads = 32;
    }
    
    ShellSortParams params;
    ShellSortThreadArg* threadArgs = NULL;
    pthread_t* threads = NULL;
    if (maxThreads > 1) {
        params.array = array;
        params.length = length;
        threadArgs = malloc(sizeof(ShellSortThreadArg) * maxThreads);
        for (I64 i = 0; i < maxThreads; i++) {
            threadArgs[i].params = &params;
            threadArgs[i].threadNum = i;
        }
        threads = malloc(sizeof(pthread_t) * maxThreads);
    }
    
    I64 g = 0;
    while (lastGaps[g] < length && lastGaps[g] > 0) {
        g++;
    }
    g--;
    
    I64 gap = lastGaps[g];
    do {
        I64 numThreadsToUse = maxThreads;
        if (numThreadsToUse > 1) {
            if (numThreadsToUse > gap) {
                numThreadsToUse = gap;
            }
            if (numThreadsToUse > (length - gap) / minLengthPerThread) {
                numThreadsToUse = (length - gap) / minLengthPerThread;
            }
        }
        //printf("sort gap=%d, numThreadsToUse=%d\n", gap, numThreadsToUse);
        if (numThreadsToUse > 1) {
            params.gap = gap;
            for (I64 i = 0; i < numThreadsToUse; i++) {
                threadArgs[i].totalThreads = numThreadsToUse;
                pthread_create(&threads[i], NULL, shellSortThreadFunc, (void*)&threadArgs[i]);
            }
            for (I64 i = 0; i < numThreadsToUse; i++) {
                pthread_join(threads[i], NULL);
                COMPARE_COUNTER += threadArgs[i].compareCount;
            }
        }
        else {
            shellSortSingleGap(array, length, gap);
        }
        g--;
        gap = gaps[g];
    }
    while (g > 0);
    
    if (maxThreads > 1) {
        free(threads);
        free(threadArgs);
    }
    
    insertionSort(array, length);
}

// assumes first element in gaps is 1 and second element in gaps is positive, last element in gaps is -1
void shellSortCustomAdjustLast(int array[], I64 length, I64 const gaps[]) {
    
    if (length <= gaps[1]) {
        return insertionSort(array, length);
    }
    
    // find initial gap (largest gap that is less than length)
    I64 g = 2;
    while (gaps[g] < length && gaps[g] > 0) {
        g++;
    }
    g--;
    
    I64 gap;
    gap = sqrt(gaps[g] * (double)gaps[g+1]);
    if (gap >= length) {
        g--;
        gap = sqrt(gaps[g] * (double)gaps[g+1]);
    }
    
    do {
        shellSortSingleGap(array, length, gap);
        g--;
        gap = gaps[g];
    }
    while (g > 0);
    
    insertionSort(array, length);
}


static const I64 gaps_blaazen[] = {1, 4, 10, 23, 57, 132, 301, 701, 1559, 3463, 7703, 17099, 37957, 83459, 185267, 411211, 912871, 2026567, 4498951, 9987709, 22172701, 49223393, 109275931, 242592563, 538555487, -1};
// ciura's gap sequence 1, ..., 701, extended using blaazen's prime numbers 1559, 3463, ..., 49223393 found at https://forum.lazarus.freepascal.org/index.php?topic=52551.0
// I extended further starting with 109275931, ...
// all of blaazen's gaps after 701 are prime, ratios are approx: 2.223, 2.221, 2.224, 2.2197, 2.2198, 2.198, 2.2198, 2.2195, 2.21995, 2.219992, 2.21998, 2.220008, 2.219998, 2.2199998


// computed experimentally for minimizing compares, arraySize~=lastGap*8000/301, with 2 extra gaps with randRatio in [2.2,2.8] then [2.3,3.2]
// has ratios: 4.000, 2.500, 2.300, 2.478, 2.316, 2.280, 2.329, 2.334, 2.237, 2.222, 2.229, 2.227, 2.209, 2.219, 2.242, 2.195, 2.190, 2.203, 2.241, 2.197, 2.22, 2.22, ...
static const I64 gaps_dokken5_222f[] = {1, 4, 10, 23, 57, 132, 301, 701, 1636, 3659, 8129, 18118, 40354, 89129, 197803, 443557, 973657, 2131981, 4697153, 10528127, 23135351, 51360479, 114020263, 253124983, 561937462, 1247501165, 2769452586, 6148184740, -1};
static const I64 gaps_dokken5_last[] = {1, 5, 14, 27, 80, 199, 479, 1059, 2337, 5453, 12135, 27039, 59972, 132777, 296204, 657169, 1440770, 3164528, 7032227, 15606790, 33962508, 73189213, -1};// experimentally computed for [1, 5, ..., 2337], then computed as [5453=sqrt(3659*8129), ..., 33962508=sqrt(23135351*49856687)] then computed [73189213=(49856687/23135351)^(3/2)*23135351]
static const I64 gaps_dokken5_222f_time[] = {1, 10, 57, 301, 1636, 8129, 40354, 197803, 973657, 4697153, 23135351, 114020263, 561937462, 2769452586, -1};// for minimizing time

// computed experimentally for minimizing compares, arraySize~=lastGap*8000/301, with 2 extra gaps with randRatio in [2.4,2.9] then [2.6,3.3]
// has ratios: 4.000, 2.500, 2.300, 2.478, 2.316, 2.280, 2.329, 2.198, 2.270, 2.201, 2.213, 2.220, 2.165, 2.191, 2.189, 2.185, 2.194, 2.167, 2.206, 2.197, 2.22, 2.22, 2.22, 2.22, 2.22
// experimentally computed until 19782319, then extending with constant ratio 2.22 rounded down to nearest integer
static const I64  gaps_dokken11_222f[] = {1, 4, 10, 23, 57, 132, 301, 701, 1541, 3498, 7699, 17041, 37835, 81907, 179433, 392867, 858419, 1883473, 4081849, 9002887, 19782319, 43916748, 97495180, 216439299, 480495243, 1066699439, 2368072754, 5257121513, 11670809758, -1};
static const I64  gaps_dokken11_222f_time[] = {1, 10, 57, 301, 1541, 7699, 37835, 179433, 858419, 4081849, 19782319, 97495180, 480495243, 2368072754, 11670809758, -1};// for minimizing time

// computed experimentally for minimizing compares, arraySize~=lastGap*8000/301, with 2 extra gaps with randRatio in [2.5,2.9] then [2.7,3.3]
// has ratios: 4.000, 2.500, 2.300, 2.478, 2.316, 2.280, 2.329, 2.146, 2.170, 2.205, 2.216, 2.172, 2.148, 2.177, 2.142, 2.149, 2.145, 2.154, 2.157, 2.143, 2.22, 2.22, 2.22, 2.22, 2.22
// experimentally computed until 15933053, then extending with constant ratio 2.22 rounded down to nearest integer
static const I64 gaps_dokken12_222f[] = {1, 4, 10, 23, 57, 132, 301, 701, 1504, 3263, 7196, 15948, 34644, 74428, 162005, 347077, 745919, 1599893, 3446017, 7434649, 15933053, 35371377, 78524456, 174324292, 386999928, 859139840, 1907290444, 4234184785, 9399890222, -1};
static const I64 gaps_dokken12_222f_time[] = {1, 10, 57, 301, 1504, 7196, 34644, 162005, 745919, 3446017, 15933053, 78524456, 386999928, 1907290444, 9399890222, -1};// for minimizing time

// computed experimentally for minimizing time when sorting arrays of roughly 400 or less elements
static const I64 gaps_dokken_fast4[] = {1, 27, -1};
static const I64 gaps_dokken_fast4_last[] = {1, 38, 185, -1};


static const I64 gaps_swenson[] = {1, 4, 10, 23, 57, 132, 301, 701, 1750, 4376, 10941, 27353, 68383, 170958, 427396, 1068491, 2671228, 6678071, 16695178, 41737946, 104344866, 260862166, 652155416, 1630388541, -1};
// gap sequence as implemented by Christopher Swenson at https://github.com/swenson/sort/blob/main/sort.h by extending ciura's gap sequence with ratios roughly 2.50

static const I64 gaps_ciura225odd[] = {1, 4, 10, 23, 57, 132, 301, 701, 1577, 3549, 7985, 17967, 40425, 90957, 204653, 460469, 1036055, 2331123, 5245027, 11801311, 26552949, 59744135, 134424303, 302454681, 680523033, -1};
// gap sequence suggested by stack overflow user Mark R https://stackoverflow.com/users/299425/mark-r on https://stackoverflow.com/questions/2539545/fastest-gap-sequence-for-shell-sort extending ciura's gap sequence by multiplying most recent by 2.25 and then setting last bit to 1

static const I64 gaps_skean2023[] = {1, 4, 10, 27, 72, 187, 488, 1272, 3313, 8627, 22465, 58498, 152328, 396653, 1032864, 2689522, 7003368, 18236386, 47486542, 123652334, 321983850, 838428169, -1};
// "Ours-B10000-Comp" gaps from 2023 paper by Skean, Ehrenborg, and Jaromczyk https://arxiv.org/pdf/2301.00316, generated by formula: floor(4.0816*8.5714^(i/2.2449))
// ratio approaches 2.603944785=8.5714^(1/2.2449)

static const I64 gaps_skean2023A1000Time[] = {1, 3, 7, 16, 33, 85, 179, 472, 999, 2646, 5608, 14862, 31508, 83514, 469339, 2637659, 14823528, 39292030, 83307618, 220819608, 468185426, 1240997226, -1};
// "Ours-A1000-Time" gaps from 2023 paper by Skean, Ehrenborg, and Jaromczyk https://arxiv.org/pdf/2301.00316, generated by formula: floor((2.75^floor(i/2.75)*3.7142^floor(i/2.4286))^0.7429+2)

static const I64 gaps_lee2021[] = {1, 4, 9, 20, 45, 102, 230, 516, 1158, 2599, 5831, 13082, 29351, 65853, 147748, 331490, 743735, 1668650, 3743800, 8399623, 18845471, 42281871, 94863989, 212837706, 477524607, 1071378536, -1};
// gaps from 2021 paper by Ying Wai Lee, generated by formula: ceil((y^i-1)/(y-1)) where y=2.243609061420001, https://oeis.org/A366726

static const I64 gaps_tokuda1992[] = {1, 4, 9, 20, 46, 103, 233, 525, 1182, 2660, 5985, 13467, 30301, 68178, 153401, 345152, 776591, 1747331, 3931496, 8845866, 19903198, 44782196, 100759940, 226709866, 510097200, 1147718700, -1};
// gaps by Tokuda, 1992, generated by formula: ceil((y^i-1)/(y-1)) where y=2.25

static const I64 gaps_hibbard1963[] = {1, 3, 7, 15, 31, 63, 127, 255, 511, 1023, 2047, 4095, 8191, 16383, 32767, 65535, 131071, 262143, 524287, 1048575, 2097151, 4194303, 8388607, 16777215, 33554431, 67108863, 134217727, 268435455, 536870911, 1073741823, -1};
// gaps by Hibbard, 1963, generated by formula: 2^i-1

static const I64 gaps_pratt1971[] = {1,2,3,4,6,8,9,12,16,18,24,27,32,36,48,54,64,72,81,96,108,128,144,162,192,216,243,256,288,324,384,432,486,512,576,648,729,768,864,972,1024,1152,1296,1458,1536,1728,1944,2048,2187,2304,2592,2916,3072,3456,3888,4096,4374,4608,5184,5832,6144,6561,6912,7776,8192,8748,9216,10368,11664,12288,13122,13824,15552,16384,17496,18432,19683,20736,23328,24576,26244,27648,31104,32768,34992,36864,39366,41472,46656,49152,52488,55296,59049,62208,65536,69984,73728,78732,82944,93312,98304,104976,110592,118098,124416,131072,139968,147456,157464,165888,177147,186624,196608,209952,221184,236196,248832,262144,279936,294912,314928,331776,354294,373248,393216,419904,442368,472392,497664,524288,531441,559872,589824,629856,663552,708588,746496,786432,839808,884736,944784,995328,1048576,1062882,1119744,1179648,1259712,1327104,1417176,1492992,1572864,1594323,1679616,1769472,1889568,1990656,2097152,2125764,2239488,2359296,2519424,2654208,2834352,2985984,3145728,3188646,3359232,3538944,3779136,3981312,4194304,4251528,4478976,4718592,4782969,5038848,5308416,5668704,5971968,6291456,6377292,6718464,7077888,7558272,7962624,8388608,8503056,8957952,9437184,9565938,10077696,10616832,11337408,11943936,12582912,12754584,13436928,14155776,14348907,15116544,15925248,16777216,17006112,17915904,18874368,19131876,20155392,21233664,22674816,23887872,25165824,25509168,26873856,28311552,28697814,30233088,31850496,33554432,34012224,35831808,37748736,38263752,40310784,42467328,43046721,45349632,47775744,50331648,51018336,53747712,56623104,57395628,60466176,63700992,67108864,68024448,71663616,75497472,76527504,80621568,84934656,86093442,90699264,95551488,100663296,102036672,107495424,113246208,114791256,120932352,127401984,129140163,134217728,136048896,143327232,150994944,153055008,161243136,169869312,172186884,181398528,191102976,201326592,204073344,214990848,226492416,229582512,241864704,254803968,258280326,268435456,272097792,286654464,301989888,306110016,322486272,339738624,344373768,362797056,382205952,387420489,402653184,408146688,429981696,452984832,459165024,483729408,509607936,516560652,536870912,544195584,573308928,603979776,612220032,644972544,679477248,688747536,725594112,764411904,774840978,805306368,816293376,859963392,905969664,918330048,967458816, -1};
// gaps by Pratt, 1971, 3-smooth numbers, numbers of the form 2^i*3^j with i, j >= 0, https://oeis.org/A003586

static const I64 gaps_knuth1973[] = {1, 4, 13, 40, 121, 364, 1093, 3280, 9841, 29524, 88573, 265720, 797161, 2391484, 7174453, 21523360, 64570081, 193710244, 581130733, -1};
// gaps by Knuth, 1973, generated by formula: (3^i-1)/2

static const I64 gaps_sedgewick1986[] = {1, 5, 19, 41, 109, 209, 505, 929, 2161, 3905, 8929, 16001, 36289, 64769, 146305, 260609, 587521, 1045505, 2354689, 4188161, 9427969, 16764929, 37730305, 67084289, 150958081, 268386305, 603906049, 1073643521, -1};
// gaps by Sedgewick, 1986, https://oeis.org/A033622

static const I64 gaps_sedgewick1982[] = {1, 8, 23, 77, 281, 1073, 4193, 16577, 65921, 262913, 1050113, 4197377, 16783361, 67121153, 268460033, 1073790977, -1};
// gaps by Sedgewick, 1982, https://oeis.org/A036562, generated by formula: 4^(i+1)+3*2^i+1

static const I64 gaps_incerpi1985[] = {1, 3, 7, 21, 48, 112, 336, 861, 1968, 4592, 13776, 33936, 86961, 198768, 463792, 1391376, 3402672, 8382192, 21479367, 49095696, 114556624, 343669872, 852913488, -1};
// gaps by Incerpi and Sedgewick, 1985, https://oeis.org/A036569

static const I64 gaps_baobao[] = {1, 9, 34, 182, 836, 4025, 19001, 90358, 428481, 2034035, 9651787, 45806244, 217378076, 1031612713, -1};
// gaps by Baobaobear, https://github.com/Baobaobear/sort/blob/master/sortlib.hpp
// intended to optimize for time

static const I64 gaps_aphitoriteC23601[] = {1, 3, 8, 19, 45, 107, 253, 598, 1412, 3333, 7867, 18567, 43820, 103420, 244082, 576058, 1359555, 3208686, 7572820, 17872613, 42181154, 99551742, 234952067, 554510374, -1};
// gaps by aphitorite, https://pastebin.com/u/aphitorite, https://sortingalgos.miraheze.org/wiki/Shellsort, generated by formula a(n) = ceil(2.3601 * a(n-1))

static const I64 gaps_aphitoriteC214399[] = {1, 4, 10, 23, 51, 111, 239, 514, 1104, 2368, 5078, 10889, 23347, 50057, 107323, 230101, 493336, 1057709, 2267719, 4861968, 10424012, 22348979, 47915989, 102731403, 220255102, 472224738, 1012445118, -1};
// gaps by aphitorite, https://pastebin.com/u/aphitorite, https://sortingalgos.miraheze.org/wiki/Shellsort, generated by formula a(n) = ceil(2.14399 * a(n-1) + 1)

static const I64 gaps_aphitoriteSplitRatio[] = {1, 4, 11, 28, 69, 167, 371, 825, 1838, 4096, 9131, 20358, 45391, 101207, 225662, 503161, 1121906, 2501535, 5577721, 12436754, 27730477, 61831197, 137866255, 307403144, 685422937, -1};
// gaps by aphitorite, https://pastebin.com/u/aphitorite, https://sortingalgos.miraheze.org/wiki/Shellsort, generated by formula:
// a(n) = ceil(2.4 * (a(n-1) + 1)) - 1 if a(n-1) < 167
// a(n) = ceil(2.22972 * (a(n-1) - 1)) otherwise

static const I64 gaps_aphitoriteCiura1636F22344[] = {1, 4, 10, 23, 57, 132, 301, 701, 1636, 3655, 8166, 18246, 40768, 91092, 203535, 454778, 1016155, 2270496, 5073196, 11335549, 25328150, 56593218, 126451886, 282544094, 631316523, -1};
// gaps by aphitorite, https://pastebin.com/u/aphitorite, https://sortingalgos.miraheze.org/wiki/Shellsort, extending Ciura gaps + 1636 with a(n) = floor(2.2344 * a(n-1))

static const I64 gaps_aphitoriteCiura1636F22344LDE[] = {1, 4, 10, 23, 57, 132, 301, 701, 1636, 3657, 8172, 18235, 40764, 91064, 203519, 454741, 1016156, 2270499, 5073398, 11335582, 25328324, 56518561, 126451290, 282544198, 631315018, -1};
// gaps by aphitorite, PCBoy, Control, Nov 2023, https://pastebin.com/u/aphitorite, https://sortingalgos.miraheze.org/wiki/Shellsort, Slight modification and improvement of gaps_aphitoriteCiura1636F22344, finding terms beyond 631315018 would take a large amount of computing time

static const I64 gaps_pcboyAutoLDE[] = {1, 4, 10, 23, 57, 132, 301, 701, 1524, 3385, 7343, 16277, 35245, 77641, 168356, 371037, 826601, 1801365, 3985424, 8636511, 19297925, 42608009, 93923600, 208531231, 468458525, 1019339649, -1};
// gaps by PCBoy, Dec 2023, https://pastebin.com/u/aphitorite, https://sortingalgos.miraheze.org/wiki/Shellsort, Extending Ciura's gaps with "AutoLDE X2.15-2.25 Coprime Extended gaps", finding larger terms beyond 1B takes considerable computing time

// from https://ghostproxies.com/sort-b/
// outputs gap sequence into gaps
void computeGhostProxiesGaps(I64* gaps, I64 length) {
    I64 gap = (length >> 5) + (length >> 3) + 1;
    I64 i = 0;
    while (gap > 0) {
        gaps[i] = gap;
        i++;
        if (gap > 7 || gap == 1) {
            gap = (gap >> 5) + (gap >> 3);
        }
        else {
            gap = 1;
        }
    }
    
    // reverse order of gaps
    for (I64 j = 0; j < i/2; j++) {
        swapInts64bit(&gaps[j], &gaps[i-1-j]);
    }
    
    gaps[i] = -1;
}

void shellSortGhostProxiesSortb(int array[], I64 length) {
    // gaps by GhostProxies, intended to optimize for time, https://ghostproxies.com/sort-b/
    
    I64 gaps[32];
    computeGhostProxiesGaps(gaps, length);
    shellSortCustom(array, length, gaps);
}

int isArraySorted(int array[], I64 length) {
    for (I64 i = 1; i < length; i++) {
        if (compareInts(array[i], array[i-1]) <= 0) {
            return 0;
        }
    }
    return 1;
}

I64 chooseRandomGap(I64 previousGap, double minRatio, double maxRatio) {
    I64 minGap = previousGap * minRatio;
    I64 maxGap = previousGap * maxRatio;
    //int random_number = minGap + randomInt() % (maxGap - minGap + 1);
    U64 bound = maxGap - minGap + 1;
    if (bound >= 1LLU << 32) {
        printf("error 1908\n");
        exit(1);
    }
    I64 random_number = minGap + (I64)rand_pcg_u32_bounded((U32)bound);
    return random_number;
}

typedef struct {
    I64 count;// total compare count
    I64 gap;
    I64 sampleCount;
    double mean;// average number of compares per sample, count / sampleCount
    double M2;// sum of squares of differences from the current mean, updated using welford's online algorithm
}
GapAndCount;

int compareGapAndCount(const void* a, const void* b) {
    //COMPARE_COUNTER++;
    if (((GapAndCount*)a)->count < ((GapAndCount*)b)->count) {
        return -1;
    }
    else if (((GapAndCount*)a)->count > ((GapAndCount*)b)->count) {
        return 1;
    }
    return 0;
    //return (int)(((GapAndCount*)a)->count - ((GapAndCount*)b)->count);
}

I64 gcd(I64 a, I64 b) {
    // euclid's algorithm
    while (b != 0) {
        I64 temp = a % b;
        a = b;
        b = temp;
    }
    return a;
}

int isCoprimeToAll(I64 n, const I64 gaps[], I64 gapsLength) {
    for (I64 i = 0; i < gapsLength; i++) {
        if (gcd(n, gaps[i]) != 1) {
            return 0;
        }
    }
    return 1;
}

// compute extension of gap sequence with constant ratio and rounding down to nearest integer
// output in newGaps
// assumes gaps ends in -1 or any negative number
// will calculate new gaps until a gap greater than 6bil is reached
// assumes length of newGaps is long enough to hold new extended gap sequence, but not more than 32 long
// will put -1 at end of newGaps sequence
void extendGapsWithRatioFloor(const I64 gaps[], double ratio, I64 newGaps[]) {
    I64 i = 0;
    while (1) {
        newGaps[i] = gaps[i];
        if (newGaps[i] < 0) {
            break;
        }
        i++;
    }
    
    I64 current = newGaps[i-1];
    while (current < 6000000000LL && i < 32) {
        current *= ratio;
        newGaps[i] = current;
        i++;
    }
    if (i >= 32) {
        printf("error 1960\n");
        exit(1);
    }
    newGaps[i] = -1;
}

// computes a "good" lastGaps sequence corresponding to passed in gaps
// uses precomputed good last gaps while following ciura+1504+3263 gaps 1, 4, ..., 701, 1504, 3263
// otherwise uses geometric mean
// output in lastGaps
// assumes gaps ends in -1 or any negative number
// assumes length of lastGaps is as long as input gaps, but not more than 32 long
// will put -1 at end of lastGaps sequence
void computeGoodLastGaps(const I64 gaps[], I64 lastGaps[]) {
    lastGaps[0] = 1;
    
    static const I64 precomputedGaps[] =     {1, 4, 10, 23, 57, 132, 301,  701, 1504, 3263};
    static const I64 precomputedLastGaps[] = {1, 5, 14, 27, 80, 199, 479, 1059, 2337};
    
    I64 numPrecomputed = sizeof(precomputedLastGaps) / sizeof(I64);
    
    I64 i = 1;
    while (i < numPrecomputed && gaps[i] == precomputedGaps[i] && gaps[i+1] == precomputedGaps[i+1]) {
        lastGaps[i] = precomputedLastGaps[i];
        i++;
    }
    
    while (gaps[i] >= 0 && i < 32) {
        double g;
        if (gaps[i+1] >= 0) {
            g = sqrt(gaps[i] * (double)gaps[i+1]);
        }
        else {
            g = pow(gaps[i], 1.5) / sqrt(gaps[i-1]);
        }
        
        lastGaps[i] = g;
        i++;
    }
    if (i >= 32) {
        printf("error 1255, maybe gaps are too long or do not end with -1\n");
        exit(1);
    }
    lastGaps[i] = -1;
}

// compute 3-smooth numbers for pratt gap sequence
void print3smoothNumbers(void) {
    for (int k = 1; k < 1000000000; k++) {
        int i = k;
        while (i % 2 == 0) {
            i = i / 2;
        }
        while (i % 3 == 0) {
            i = i / 3;
        }
        if (i == 1) {
            printf("%d,", k);
        }
    }
    printf("\n");
}

// test average runtime of different sorting algorithms
void testAverageRuntime(void) {
    const I64 N = 512;
    //int array[N];
    int* array = malloc(sizeof(int) * N);
    initializeArray(array, N);
    
    I64 numSamples = 1000;// 10000000;
    
    U64 totalTime = 0;
    for (I64 i = 0; i < numSamples; i++) {
        shuffleArray(array, N);
        U64 startTime = currentTime();
        
        //insertionSort(array, N);
        
        //qsort(array, N, sizeof(int), compare_qsort);
        //mergesort(array, N, sizeof(int), compare_qsort);
        //heapsort(array, N, sizeof(int), compare_qsort);
        
        shellSortCustom(array, N, gaps_dokken12_222f);
        //shellSortCustomWithLastGapsMultithreaded(array, N, gaps_dokken11_222f, gaps_dokken11_222f, 5);
        
        U64 endTime = currentTime();
        totalTime += endTime - startTime;
    }
    printf("numCompares = %lld, %g million compares\n", COMPARE_COUNTER, (COMPARE_COUNTER / 1000000.0));
    printf("time to sort = %llu microseconds, %g seconds\n", totalTime, (totalTime / (double)TICKS_PER_SEC));
    
    printf("average compares per element = %g\n", ((COMPARE_COUNTER / (double)numSamples) / N));
    
    if (!isArraySorted(array, N)) {
        printArray(array, N);
        printf("error 610\n");
        exit(1);
    }
    
    free(array);
}

// find worst case approximation using a greedy algorithm
// will not find the absolute worst case
// can be improved further with findWorstCaseWithRandomMutations
void findWorstCase(int length, const I64 gaps[]) {
    int* array = malloc(sizeof(int) * length);
    int* array2 = malloc(sizeof(int) * length);
    
    int middleValue = (length - 1) / 2;
    
    // start array with all middleValues
    for (int i = 0; i < length; i++) {
        array[i] = middleValue;
    }
    
    for (int k = 0; k < length; k++) {
        int i;
        if (k % 2 == 0) {
            i = length - 1 - k/2;
        }
        else {
            i = k/2;
        }
        
        // find optimal place for i to be
        
        I64 mostCompares = -1;
        int indexOfMostCompares = -1;
        
        for (int j = 0; j < length; j++) {
            // check if index j is optimal for i to be placed
            
            if (array[j] != middleValue) {
                // we already placed a number at index j
                continue;
            }
            
            copyArray(array, array2, length);
            array2[j] = i;
            COMPARE_COUNTER = 0;
            shellSortCustom(array2, length, gaps);
            if (COMPARE_COUNTER > mostCompares || (COMPARE_COUNTER == mostCompares && i < middleValue)) {
                mostCompares = COMPARE_COUNTER;
                indexOfMostCompares = j;
            }
        }
        
        array[indexOfMostCompares] = i;
        if (i < 10 || length - i < 10) {
            printf("set array[%d] = %d\n", indexOfMostCompares, i);
        }
    }
    
    printArray(array, length);
    copyArray(array, array2, length);
    COMPARE_COUNTER = 0;
    shellSortCustom(array2, length, gaps);
    printf("total compares = %lld\n", COMPARE_COUNTER);
    
    free(array2);
    free(array);
}

// find worst case approximation using random mutations
// mutations include random swapping of elements and a couple other types of mutations, but maybe there are other better mutations possible
// keeps all mutations that increase compares, and to avoid local maximums will keep a slightly bad mutation with some probability
void findWorstCaseWithRandomMutations(void) {
    //const I64 N = 48;
    //const I64 gapsToUse[] = {1, 3, 8, 24, -1};
    //static const int array_initial[48] = {47, 29, 20, 40, 22, 19, 39, 21, 43, 27, 10, 34, 18, 8, 31, 14, 41, 4, 2, 33, 6, 1, 30, 13, 44, 38, 12, 46, 37, 9, 45, 36, 42, 28, 11, 35, 17, 7, 32, 15, 26, 23, 0, 24, 16, 3, 25, 5,};
    //int haltOnCompares = 477;
    //const int possibleGapsForPairSwaps[] = {3, 8, 24};// i just set this to all gaps g with 1 < g < N
    //const int possibleCycles[] = {2, 3, 8, 24};// i just set this to a 2 and then all gaps g with 1 < g < N

    const I64 N = 100;
    const I64* gapsToUse = gaps_dokken12_222f;// 1, 4, 10, 23, 57, 132, 301, 701, 1504, ...
    static const int array_initial[100] = {99, 85, 93, 58, 98, 26, 89, 15, 94, 67, 96, 83, 88, 57, 59, 52, 47, 5, 33, 72, 17, 38, 76, 39, 71, 81, 2, 49, 60, 12, 3, 35, 9, 37, 40, 78, 6, 44, 50, 24, 13, 62, 14, 75, 1, 84, 22, 0, 95, 53, 48, 31, 10, 73, 30, 97, 18, 79, 42, 91, 54, 87, 61, 92, 27, 36, 74, 77, 4, 86, 56, 90, 20, 23, 8, 63, 65, 29, 66, 68, 21, 55, 80, 43, 46, 32, 11, 28, 64, 16, 19, 69, 70, 41, 82, 51, 45, 25, 34, 7, };
    int haltOnCompares = 1391;
    const int possibleGapsForPairSwaps[] = {4, 10, 23, 57};// i just set this to all gaps g with 1 < g < N
    const int possibleCycles[] = {2, 4, 10, 23, 57};// i just set this to a 2 and then all gaps g with 1 < g < N

    int useInitialArray = 1;
    int useHaltOnCompares = 1;
    I64 numSamples = 10000000;
    int granularity = 6;// set to 12 to make loss-causing changes very rare, set to 4 to make loss-causing changes more common, or 5,6,7,8 are some good medium values

    int* array = malloc(sizeof(int) * N);
    initializeArray(array, N);
    
    U64 totalTime = 0;
    int* array2 = malloc(sizeof(int) * N);
    int* array3 = malloc(sizeof(int) * N);
    copyArray(array, array2, N);
    copyArray(array, array3, N);
    I64 highestCompares = 0;
    for (I64 i = 0; i < numSamples; i++) {
        if (highestCompares == 0) {
            if (useInitialArray) {
                // start with our initial array
                copyArray(array_initial, array2, N);
            }
            else {
                // start with a random array
                shuffleArray(array2, N);
            }
        }
        else {
            // make a small change
            U32 mutationType = rand_pcg_u32_bounded(4);
            if (mutationType == 0) {
                // make a single swap
                I64 j1 = rand_pcg_u32_bounded(N);
                I64 j2 = rand_pcg_u32_bounded(N);
                swapInts(&array2[j1], &array2[j2]);
            }
            else if (mutationType == 1) {
                // make 1 or more swaps
                do {
                    I64 j1 = rand_pcg_u32_bounded(N);
                    I64 j2 = rand_pcg_u32_bounded(N);
                    swapInts(&array2[j1], &array2[j2]);
                }
                while (rand_pcg_u32() & 1);
            }
            else if (mutationType == 2) {
                // swap 2 pairs m apart
                int numGaps = sizeof(possibleGapsForPairSwaps) / sizeof(possibleGapsForPairSwaps[0]);
                int m = possibleGapsForPairSwaps[rand_pcg_u32_bounded(numGaps)];
                I64 j1 = rand_pcg_u32_bounded(N-m);
                I64 j2 = rand_pcg_u32_bounded(N-m);
                swapInts(&array2[j1], &array2[j2]);
                swapInts(&array2[j1+m], &array2[j2+m]);
            }
            else {
                // swaps a m-cycle with another m-cycle
                int numCycles = sizeof(possibleCycles) / sizeof(possibleCycles[0]);
                int m = possibleCycles[rand_pcg_u32_bounded(numCycles)];
                I64 k1;
                I64 k2;
                if (m == 2) {
                    k1 = 0;
                    k2 = 1;
                }
                else {
                    k1 = rand_pcg_u32_bounded(m);
                    k2 = rand_pcg_u32_bounded(m);
                }
                I64 k_max = k1 > k2 ? k1 : k2;
                for (I64 j = 0; j+k_max < N; j+=m) {
                    swapInts(&array2[j+k1], &array2[j+k2]);
                }
            }
        }
        
        copyArray(array2, array, N);
        
        U64 startTime = currentTime();
        
        I64 saveCompareCounter = COMPARE_COUNTER;
        shellSortCustom(array, N, gapsToUse);
        
        saveCompareCounter = COMPARE_COUNTER - saveCompareCounter;
        
        if (saveCompareCounter >= highestCompares) {
            if (saveCompareCounter > highestCompares) {
                highestCompares = saveCompareCounter;
                printf("compares = %lld\n", highestCompares);
                
                if (useHaltOnCompares && highestCompares >= haltOnCompares) {
                    printf("hit haltOnCompares = %d, stopping\n", haltOnCompares);
                    printArray(array2, N);
                    exit(1);
                }
            }
            copyArray(array2, array3, N);
        }
        else if (saveCompareCounter >= highestCompares - 30) {
            U64 mask = (1LLU << (highestCompares-saveCompareCounter+granularity)) - 1LLU;
            if ((rand_pcg_u64() & mask) == 0) {
                highestCompares = saveCompareCounter;
                printf("compares = %lld\n", highestCompares);
                copyArray(array2, array3, N);
            }
            else {
                copyArray(array3, array2, N);
            }
        }
        else {
            copyArray(array3, array2, N);
        }
        
        U64 endTime = currentTime();
        totalTime += endTime - startTime;
    }
    printArray(array3, N);
    
    printf("numCompares = %lld, %g million compares\n", COMPARE_COUNTER, (COMPARE_COUNTER / 1000000.0));
    printf("time to sort = %llu microseconds, %g seconds\n", totalTime, (totalTime / (double)TICKS_PER_SEC));
    
    printf("average compares per element = %g\n", ((COMPARE_COUNTER / (double)numSamples) / N));
    
    if (!isArraySorted(array, N)) {
        printArray(array, N);
        printf("error 610\n");
        exit(1);
    }
    
    free(array3);
    free(array2);
    free(array);
}

typedef struct {
    GapAndCount* gapAndCountArray;
    I64 startIndex;
    I64 lastIndex;
    I64 arraySize;
    I64 numSamples;
    I64* gaps;
    I64 gapIndex1;
    int* array;
    
    U64 pcgInitState;
    U64 pcgInc;
}
ThreadArg;

void* thread_runSortingSamples(void* arg_) {
    ThreadArg* arg = arg_;
    if (arg->lastIndex < 0 || arg->lastIndex < arg->startIndex) {
        return NULL;
    }
    // Reduced printing - only print on first thread
    //printf("thread search from indexes %lld to %lld\n", arg->startIndex, arg->lastIndex);
    //srand_pcg_easy(); // don't need this since we are seeding the pcg later with a specific seed
    
    GapAndCount* gapAndCountArray = arg->gapAndCountArray;
    I64 arraySize = arg->arraySize;
    I64* gaps = arg->gaps;
    I64 gapIndex1 = arg->gapIndex1;
    
    int* array = arg->array;
    
    initializeArray(array, arraySize);
    
    for (I64 i = arg->startIndex; i <= arg->lastIndex; i++) {
        I64 gap1 = gapAndCountArray[i].gap;
        
        srand_pcg(arg->pcgInitState, arg->pcgInc);// use same seed for all gap1s so that shuffle is same and gap ratios are same
        
        for (int j = 0; j < arg->numSamples; j++) {
            // choose random gap2, gap3
            I64 gap2 = chooseRandomGap(gap1, 2.5, 2.9);// 2.4, 2.9 then 2.6, 3.3// 2.2, 2.8 then 2.3, 3.2 // 2.2, 4.9 both
            I64 gap3 = chooseRandomGap(gap2, 2.7, 3.3);// 2.5, 2.9 then 2.7, 3.3
            
            // avoid using exact multiple of previous gap
            if (gap3 == 3 * gap2) {
                gap3 += 1;
            }
            
            gaps[gapIndex1] = gap1;
            gaps[gapIndex1+1] = gap2;
            gaps[gapIndex1+2] = gap3;
            
            shuffleArray(array, arraySize);
            
            COMPARE_COUNTER = 0;
            shellSortCustom(array, arraySize, gaps);
            gapAndCountArray[i].count += COMPARE_COUNTER;
            
            // update using welford's online algorithm
            gapAndCountArray[i].sampleCount += 1;
            double delta = COMPARE_COUNTER - gapAndCountArray[i].mean;
            gapAndCountArray[i].mean += delta / gapAndCountArray[i].sampleCount;
            double delta2 = COMPARE_COUNTER - gapAndCountArray[i].mean;
            gapAndCountArray[i].M2 += delta * delta2;
            
            if (!isArraySorted(array, arraySize)) {
                printArray(array, arraySize);
                printf("error 1015\n");
                exit(1);
            }
        }
        
        // Reduced printing - removed per-gap output
        //printf("gap1=%lld, total compare count = %llu\n", gap1, gapAndCountArray[i].count);
    }
    
    return NULL;
}

// Structure for sequence candidates (used in multi-branch search)
typedef struct {
    I64* fullSequence;        // Complete sequence including new gap
    int fromInitialIndex;     // Which initial sequence this came from
    I64 nextGap;             // The new gap being tested
    I64 count;               // Total compare count
    I64 sampleCount;
    double mean;
    double M2;
} SequenceCandidate;

// Threading structures and functions for sequence candidate search
typedef struct {
    SequenceCandidate* candidates;
    I64 startIndex;
    I64 lastIndex;
    I64 arraySize;
    I64 numSamples;
    int* array;
    
    U64 pcgInitState;
    U64 pcgInc;
}
SequenceThreadArg;

void* thread_runSequenceSamples(void* arg_) {
    SequenceThreadArg* arg = arg_;
    if (arg->lastIndex < 0 || arg->lastIndex < arg->startIndex) {
        return NULL;
    }
    
    SequenceCandidate* candidates = arg->candidates;
    I64 arraySize = arg->arraySize;
    int* array = arg->array;
    
    initializeArray(array, arraySize);
    
    for (I64 i = arg->startIndex; i <= arg->lastIndex; i++) {
        I64* gaps = candidates[i].fullSequence;
        
        // Find where the sequence ends (before the 0, 0, 0, -1)
        I64 seqLen = 0;
        while (gaps[seqLen] > 0) seqLen++;
        
        I64 nextGap = gaps[seqLen - 1];  // The new gap we're testing
        
        srand_pcg(arg->pcgInitState, arg->pcgInc);  // Same seed for consistent random gaps
        
        for (int j = 0; j < arg->numSamples; j++) {
            // Generate random gap2, gap3 after nextGap
            I64 gap2 = chooseRandomGap(nextGap, 2.5, 2.9);
            I64 gap3 = chooseRandomGap(gap2, 2.7, 3.3);
            
            // Avoid exact multiples
            if (gap3 == 3 * gap2) {
                gap3 += 1;
            }
            
            // Set the random gaps
            gaps[seqLen] = gap2;
            gaps[seqLen + 1] = gap3;
            gaps[seqLen + 2] = -1;
            
            shuffleArray(array, arraySize);
            
            COMPARE_COUNTER = 0;
            shellSortCustom(array, arraySize, gaps);
            candidates[i].count += COMPARE_COUNTER;
            
            // Update using Welford's online algorithm
            candidates[i].sampleCount += 1;
            double delta = COMPARE_COUNTER - candidates[i].mean;
            candidates[i].mean += delta / candidates[i].sampleCount;
            double delta2 = COMPARE_COUNTER - candidates[i].mean;
            candidates[i].M2 += delta * delta2;
            
            if (!isArraySorted(array, arraySize)) {
                printf("error in thread_runSequenceSamples\n");
                exit(1);
            }
        }
        
        // Restore original terminator
        gaps[seqLen] = 0;
        gaps[seqLen + 1] = 0;
        gaps[seqLen + 2] = -1;
    }
    
    return NULL;
}

// find optimal shellsort gap sequences
// returns the best gap found, or -1 if error
// numRemainingGaps is output parameter showing how many candidate gaps remained at the end
// minStdErrsUsed is output parameter showing the minimum stdErrs used for cutting
I64 findOptimalNextGap_parameterized(
    I64* gaps,                    // input/output: gap sequence ending with {0, 0, 0, -1}
    int gapIndex1,                // index where to insert next gap candidate
    double minRatio,
    double maxRatio,
    double numStdErrsToCutoff,
    int initialNumSamples,
    double maxRuntimeSeconds,
    int numThreads,
    I64* numRemainingGaps,        // output: how many gaps remained at end
    double* minStdErrsUsed        // output: minimum stdErrs used for cutting
) {
    U64 startTime = currentTime();
    int numSamples = initialNumSamples;
    
    I64 arraySize = round(gaps[gapIndex1-1] / 301.0 * 8000.0);
    printf("arraySize = %lld\n", arraySize);
    
    I64 gap0 = gaps[gapIndex1-1];
    I64 minGap1 = minRatio * gap0;
    I64 maxGap1 = maxRatio * gap0;
    I64 numGap1s = maxGap1 - minGap1 + 1;
    I64* gap1s = malloc(sizeof(I64) * numGap1s);
    for (I64 i = 0; i < numGap1s; i++) {
        gap1s[i] = minGap1 + i;
    }
    
    //int gap1s[] = {1636,1601,1638,1618,1599,1616,1621,1597,1640,1619,1614,1634,1561};
    //int numGap1s = sizeof(gap1s) / sizeof(I64);
    
    printf("Initial numGap1s = %lld\n", numGap1s);
    
    // Estimate time for first iteration to avoid using too much time upfront
    I64 midGap = (minGap1 + maxGap1) / 2;  // Pick middle gap for estimate
    double estimatedFirstIterTime = (numGap1s / (double)numThreads) * initialNumSamples * midGap / 1000000.0;
    double maxFirstIterTime = maxRuntimeSeconds * 0.10;  // Max 10% of total time
    
    printf("Estimated first iteration: %.1f seconds (%.0f%% of budget)\n", 
           estimatedFirstIterTime, (estimatedFirstIterTime / maxRuntimeSeconds) * 100);
    
    // Filter gaps if first iteration would take too long
    if (estimatedFirstIterTime > maxFirstIterTime) {
        printf("First iteration too slow, applying filters...\n");
        
        // Try filtering by max-gcd
        I64* filteredGaps = malloc(sizeof(I64) * numGap1s);
        I64 numFiltered = 0;
        
        // First try: max-gcd <= 6 (reduces to ~40%)
        for (I64 i = 0; i < numGap1s; i++) {
            I64 maxGcd = 1;
            for (int j = 0; j < gapIndex1; j++) {
                I64 g = gcd(gap1s[i], gaps[j]);
                if (g > maxGcd) maxGcd = g;
            }
            if (maxGcd <= 6) {
                filteredGaps[numFiltered++] = gap1s[i];
            }
        }
        
        double estimatedWithGcd6 = (numFiltered / (double)numThreads) * initialNumSamples * midGap / 1000000.0;
        printf("  Filtering max-gcd <= 6: %lld gaps (%.1fs, %.0f%%)\n", 
               numFiltered, estimatedWithGcd6, (estimatedWithGcd6 / maxRuntimeSeconds) * 100);
        
        // If still too slow, try coprime only (max-gcd <= 1)
        if (estimatedWithGcd6 > maxFirstIterTime) {
            numFiltered = 0;
            for (I64 i = 0; i < numGap1s; i++) {
                if (isCoprimeToAll(gap1s[i], gaps, gapIndex1)) {
                    filteredGaps[numFiltered++] = gap1s[i];
                }
            }
            
            double estimatedCoprime = (numFiltered / (double)numThreads) * initialNumSamples * midGap / 1000000.0;
            printf("  Filtering coprime only: %lld gaps (%.1fs, %.0f%%)\n", 
                   numFiltered, estimatedCoprime, (estimatedCoprime / maxRuntimeSeconds) * 100);
            
            // If still too slow, subsample coprime gaps evenly
            if (estimatedCoprime > maxFirstIterTime && numFiltered > numThreads) {
                double targetFraction = maxFirstIterTime / estimatedCoprime;
                I64 targetNum = (I64)(numFiltered * targetFraction);
                if (targetNum < numThreads) targetNum = numThreads;
                
                printf("  Subsampling coprime gaps: keeping %lld of %lld (every %.1f)\n", 
                       targetNum, numFiltered, numFiltered / (double)targetNum);
                
                // Evenly distribute selected gaps across the range
                I64* subsampledGaps = malloc(sizeof(I64) * targetNum);
                for (I64 i = 0; i < targetNum; i++) {
                    I64 index = (i * numFiltered) / targetNum;
                    subsampledGaps[i] = filteredGaps[index];
                }
                
                free(gap1s);
                free(filteredGaps);
                gap1s = subsampledGaps;
                numGap1s = targetNum;
            } else {
                // Use coprime filtered gaps
                free(gap1s);
                gap1s = filteredGaps;
                numGap1s = numFiltered;
            }
        } else {
            // Use max-gcd <= 6 filtered gaps
            free(gap1s);
            gap1s = filteredGaps;
            numGap1s = numFiltered;
        }
        
        printf("Final numGap1s after filtering = %lld\n", numGap1s);
    }
    
    printf("\n");
    
    GapAndCount* gapAndCountArray = malloc(sizeof(GapAndCount) * numGap1s);
    for (int i = 0; i < numGap1s; i++) {
        gapAndCountArray[i].gap = gap1s[i];
        gapAndCountArray[i].count = 0;
        gapAndCountArray[i].mean = 0;
        gapAndCountArray[i].M2 = 0;
        gapAndCountArray[i].sampleCount = 0;
    }
    
    
    int* array_for_thread[numThreads];
    I64* gaps_for_thread[numThreads];
    pthread_t threads[numThreads];
    ThreadArg threadArgs[numThreads];
    
    // Calculate size of gaps array (count until we hit -1)
    I64 gapsSize = 0;
    while (gaps[gapsSize] >= 0) {
        gapsSize++;
    }
    gapsSize++; // include the -1
    
    for (int i = 0; i < numThreads; i++) {
        array_for_thread[i] = malloc(sizeof(int) * arraySize);
        gaps_for_thread[i] = malloc(sizeof(I64) * gapsSize);
        memcpy(gaps_for_thread[i], gaps, sizeof(I64) * gapsSize);
    }
    
    // Adaptive time-based approach
    I64 initialNumGap1s = numGap1s;
    double targetHalvings = log(initialNumGap1s) / log(2.0);  // how many times we need to halve to get to 1
    double minStdErrs = 999.0;  // track minimum stdErrs we had to use for cutting
    int iterationCount = 0;
    
    printf("Starting with %lld candidate gaps, target %.1f halvings\n", initialNumGap1s, targetHalvings);
    
    while (numGap1s > 1) {
        U64 pcgInitState = rand_pcg_u64();
        U64 pcgInc = rand_pcg_u64();
        for (int i = 0; i < numThreads; i++) {
            threadArgs[i].gapAndCountArray = gapAndCountArray;
            if (i == 0) {
                threadArgs[i].startIndex = 0;
            }
            else {
                threadArgs[i].startIndex = threadArgs[i-1].lastIndex + 1;
            }
            if (i == numThreads - 1) {
                threadArgs[i].lastIndex = numGap1s - 1;
            }
            else {
                threadArgs[i].lastIndex = ((i+1) * numGap1s) / numThreads - 1;
            }
            threadArgs[i].arraySize = arraySize;
            threadArgs[i].numSamples = numSamples;
            threadArgs[i].gaps = gaps_for_thread[i];
            threadArgs[i].gapIndex1 = gapIndex1;
            threadArgs[i].array = array_for_thread[i];
            threadArgs[i].pcgInitState = pcgInitState;
            threadArgs[i].pcgInc = pcgInc;
            pthread_create(&threads[i], NULL, thread_runSortingSamples, (void*)&threadArgs[i]);
        }
        for (int i = 0; i < numThreads; i++) {
            pthread_join(threads[i], NULL);
        }
        if (COMPARE_COUNTER != 0) {
            printf("error 1577\n");
            exit(1);
        }
        
        qsort(gapAndCountArray, numGap1s, sizeof(GapAndCount), compareGapAndCount);
        
        // Calculate time-based target for number of gaps
        double elapsedTime = (currentTime() - startTime) / (double)TICKS_PER_SEC;
        double timePercent = elapsedTime / maxRuntimeSeconds;
        if (timePercent > 1.0) timePercent = 1.0;
        
        // Target: at timePercent of budget, we should have done (timePercent * targetHalvings) halvings
        double targetHalvingsDone = timePercent * targetHalvings;
        I64 targetNumGaps = (I64)(initialNumGap1s / pow(2.0, targetHalvingsDone));
        if (targetNumGaps < 1) targetNumGaps = 1;
        
        // Don't cut below numThreads during search - keep parallel resources fully utilized
        // Exception: if we naturally converge to 1, that's fine to end early
        if (targetNumGaps < numThreads && targetNumGaps > 1) {
            targetNumGaps = numThreads;
        }
        
        // Calculate statistics
        double pooled_variance = 0;
        for (I64 i = 0; i < numGap1s; i++) {
            double sample_variance = gapAndCountArray[i].M2 / (gapAndCountArray[i].sampleCount - 1);
            pooled_variance += sample_variance;
        }
        pooled_variance /= numGap1s;
        double pooledStdErr = sqrt(pooled_variance / gapAndCountArray[0].sampleCount);
        
        // Direct calculation of stdErrs needed to cut to targetNumGaps
        // Much simpler: just look at the gap at index targetNumGaps and calculate its distance from best
        
        // Clamp target to valid range
        I64 targetIndex = targetNumGaps - 1;  // 0-indexed
        if (targetIndex < 0) targetIndex = 0;
        if (targetIndex >= numGap1s) targetIndex = numGap1s - 1;
        
        // Enforce minimum of numThreads for parallel efficiency
        if (targetIndex < numThreads - 1 && targetIndex > 0 && numGap1s > numThreads) {
            targetIndex = numThreads - 1;  // Keep at least numThreads gaps
        }
        
        I64 newNumGap1s = targetIndex + 1;  // Keep gaps from index 0 to targetIndex (inclusive)
        
        // Calculate exact stdErrs: distance between best kept and first cut
        // Only meaningful when we're actually cutting
        double adaptiveNumStdErrs;
        if (newNumGap1s < numGap1s) {
            // We're cutting - show distance to first gap being cut (at index newNumGap1s)
            double meanDifference = gapAndCountArray[newNumGap1s].mean - gapAndCountArray[0].mean;
            adaptiveNumStdErrs = meanDifference / pooledStdErr;
        } else {
            // No cut this iteration - use default large value to indicate no cut
            adaptiveNumStdErrs = 10.0;
        }
        
        // Ensure non-negative (should always be positive since array is sorted, but handle edge case)
        if (adaptiveNumStdErrs < 0.0) adaptiveNumStdErrs = 0.0;
        
        // Upper bound sanity check
        if (adaptiveNumStdErrs > 10.0) adaptiveNumStdErrs = 10.0;
        
        // Apply the cut - even if newNumGap1s == numGap1s (no cut), that's fine!
        // This allows iterations where we don't cut, just gather more samples
        // Don't cut below numThreads unless converging to 1 (natural end)
        if (newNumGap1s <= numGap1s && newNumGap1s >= 1) {
            if (newNumGap1s >= numThreads || newNumGap1s == 1) {
                numGap1s = newNumGap1s;
                if (adaptiveNumStdErrs < minStdErrs) {
                    minStdErrs = adaptiveNumStdErrs;
                }
            }
            // else: skip this cut because it would go below numThreads
        }
        
        // Force cut to numThreads if we're stuck above it and target says we should be there
        // This handles cases where statistical thresholds can't distinguish well enough
        if (numGap1s > numThreads && targetNumGaps <= numThreads && targetNumGaps > 1) {
            // Just take the top numThreads gaps directly
            numGap1s = numThreads;
            if (adaptiveNumStdErrs < minStdErrs) {
                minStdErrs = adaptiveNumStdErrs;
            }
        }
        
        iterationCount++;
        
        // Print summary every few iterations or when gaps is small
        if (iterationCount % 5 == 0 || numGap1s <= 10) {
            const char* status = "";
            if (numGap1s == numThreads && targetNumGaps < numThreads) {
                status = " [holding at numThreads]";
            }
            printf("Iter %d: time %.1fs (%.0f%%), %lld gaps remain (target %lld), stdErrs=%.2f, samples=%d, best gap=%lld%s\n",
                   iterationCount, elapsedTime, timePercent * 100, numGap1s, targetNumGaps, 
                   adaptiveNumStdErrs, (int)gapAndCountArray[0].sampleCount, gapAndCountArray[0].gap, status);
        }
        
        if (elapsedTime > maxRuntimeSeconds) {
            printf("Hit max runtime. Final: %lld gaps, minStdErrs=%.2f\n", numGap1s, minStdErrs);
            break;
        }
        
        // Increase samples for next iteration
        numSamples = numSamples * 1.17 + 1;
    }
    
    printf("\n=== Search Complete ===\n");
    printf("Remaining gaps: %lld\n", numGap1s);
    printf("Minimum stdErrs used for cutting: %.2f\n", minStdErrs);
    printf("Total samples per gap: %lld\n", gapAndCountArray[0].sampleCount);
    printf("Top candidate gap(s):\n");
    
    I64 numToShow = numGap1s < 5 ? numGap1s : 5;
    for (I64 i = 0; i < numToShow; i++) {
        printf("  #%lld: gap=%lld, mean=%.1f\n", i+1, gapAndCountArray[i].gap, gapAndCountArray[i].mean);
    }
    
    I64 bestGap = gapAndCountArray[0].gap;
    *numRemainingGaps = numGap1s;
    *minStdErrsUsed = minStdErrs;
    
    if (numGap1s > 10) {
        printf("\nWARNING: %lld gaps remain - consider increasing runtime or checking parameters\n", numGap1s);
    }
    
    for (int i = 0; i < numThreads; i++) {
        free(gaps_for_thread[i]);
        free(array_for_thread[i]);
    }
    free(gapAndCountArray);
    free(gap1s);
    
    return bestGap;
}

// compute parameters based on current sequence length and available time
// currentSequenceLength is how many gaps we currently have in the sequence
void computeParametersForGap(int currentSequenceLength, double maxRuntimeSeconds,
                             double* numStdErrsToCutoff, int* initialNumSamples,
                             double* minRatio, double* maxRatio) {
    // Always start with just 3 samples - let time-based adaptive cutting do the work
    *initialNumSamples = 3;
    
    // numStdErrsToCutoff will be computed adaptively in the search function
    // based on time elapsed - this is just a placeholder
    *numStdErrsToCutoff = -1.0;  // signal to use adaptive approach
    
    // minRatio stays constant
    *minRatio = 2.08;
    
    // maxRatio decreases for later gaps to narrow the search range
    // Schedule is indexed by the position of the last gap in the current sequence
    static const double maxRatioSchedule[] = {5.00, 3.50, 3.20, 3.00, 2.82, 2.66, 2.52, 2.40, 2.32, 2.28, 2.26, 2.24, 2.23, 2.22, 2.22};
    int scheduleLength = sizeof(maxRatioSchedule) / sizeof(double);
    
    int scheduleIndex = currentSequenceLength - 1;  // Last gap is at index currentSequenceLength - 1
    if (scheduleIndex >= 0 && scheduleIndex < scheduleLength) {
        *maxRatio = maxRatioSchedule[scheduleIndex];
    } else if (scheduleIndex >= scheduleLength) {
        *maxRatio = 2.22;  // Use 2.22 for all gaps beyond the schedule
    } else {
        *maxRatio = 5.00;  // Fallback for edge case
    }
    
    printf("Sequence length %d: Using adaptive time-based approach with %.1f seconds (%.2f hours) allowed, maxRatio=%.2f\n",
           currentSequenceLength, maxRuntimeSeconds, maxRuntimeSeconds/3600.0, *maxRatio);
}

// automatically find multiple gaps in sequence
// saves results to a log file as it goes
void findMultipleGapsAutomated(
    I64* initialGaps,           // e.g., {1, 4, 10, 23, 57, 132, 301, 701}
    int numInitialGaps,         // e.g., 8
    int numGapsToFind,          // how many additional gaps to find
    double maxRuntimePerGapSeconds, // e.g., 3600.0 for 1 hour
    int numThreads              // e.g., 5
) {
    printf("=== Starting automated gap sequence search ===\n");
    printf("Starting gaps: {");
    for (int i = 0; i < numInitialGaps; i++) {
        printf("%lld, ", initialGaps[i]);
    }
    printf("}\n");
    printf("Will search for %d additional gaps\n", numGapsToFind);
    printf("Max runtime per gap: %.1f hours\n", maxRuntimePerGapSeconds / 3600.0);
    printf("Number of threads: %d\n\n", numThreads);
    
    // Open log file
    FILE* logFile = fopen("gap_search_results.txt", "a");
    if (logFile) {
        fprintf(logFile, "\n\n=== New search session started ===\n");
        fprintf(logFile, "Starting gaps: {");
        for (int i = 0; i < numInitialGaps; i++) {
            fprintf(logFile, "%lld, ", initialGaps[i]);
        }
        fprintf(logFile, "}\n");
        time_t now = time(NULL);
        fprintf(logFile, "Time: %s\n", ctime(&now));
        fflush(logFile);
    }
    
    // Allocate space for growing gap sequence
    // Need room for: initialGaps + new gaps + {0, 0, 0, -1}
    I64* gaps = malloc(sizeof(I64) * (numInitialGaps + numGapsToFind + 4));
    for (int i = 0; i < numInitialGaps; i++) {
        gaps[i] = initialGaps[i];
    }
    gaps[numInitialGaps] = 0;
    gaps[numInitialGaps + 1] = 0;
    gaps[numInitialGaps + 2] = 0;
    gaps[numInitialGaps + 3] = -1;
    
    // Find each gap in sequence
    for (int gapIdx = 0; gapIdx < numGapsToFind; gapIdx++) {
        int currentGapIndex = numInitialGaps + gapIdx;
        
        printf("\n========================================\n");
        printf("Searching for gap #%d (index %d)\n", gapIdx + 1, currentGapIndex);
        printf("Current gap sequence: {");
        for (int i = 0; i < currentGapIndex; i++) {
            printf("%lld, ", gaps[i]);
        }
        printf("?}\n");
        printf("========================================\n\n");
        
        // Compute parameters for this gap
        double numStdErrsToCutoff, minRatio, maxRatio;
        int initialNumSamples;
        computeParametersForGap(currentGapIndex, maxRuntimePerGapSeconds,
                               &numStdErrsToCutoff, &initialNumSamples,
                               &minRatio, &maxRatio);
        
        // Search for the next gap
        I64 numRemainingGaps = 0;
        double minStdErrsUsed = 0;
        U64 gapSearchStart = currentTime();
        I64 bestGap = findOptimalNextGap_parameterized(
            gaps, currentGapIndex,
            minRatio, maxRatio,
            numStdErrsToCutoff,
            initialNumSamples,
            maxRuntimePerGapSeconds,
            numThreads,
            &numRemainingGaps,
            &minStdErrsUsed
        );
        U64 gapSearchEnd = currentTime();
        double gapSearchTime = (gapSearchEnd - gapSearchStart) / (double)TICKS_PER_SEC;
        
        // Update gap sequence with the found gap
        gaps[currentGapIndex] = bestGap;
        gaps[currentGapIndex + 1] = 0;
        gaps[currentGapIndex + 2] = 0;
        gaps[currentGapIndex + 3] = 0;
        gaps[currentGapIndex + 4] = -1;
        
        printf("\n========================================\n");
        printf("Found gap #%d: %lld\n", gapIdx + 1, bestGap);
        printf("Remaining candidate gaps at end: %lld\n", numRemainingGaps);
        printf("Min stdErrs used for cutting: %.2f\n", minStdErrsUsed);
        printf("Search time: %.1f seconds (%.2f hours)\n", gapSearchTime, gapSearchTime / 3600.0);
        printf("Updated gap sequence: {");
        for (int i = 0; i <= currentGapIndex; i++) {
            printf("%lld, ", gaps[i]);
        }
        printf("}\n");
        printf("========================================\n\n");
        
        // Log to file
        if (logFile) {
            fprintf(logFile, "Gap #%d (index %d): %lld\n", gapIdx + 1, currentGapIndex, bestGap);
            fprintf(logFile, "  Remaining candidates: %lld\n", numRemainingGaps);
            fprintf(logFile, "  Min stdErrs used: %.2f\n", minStdErrsUsed);
            fprintf(logFile, "  Search time: %.1f seconds (%.2f hours)\n", gapSearchTime, gapSearchTime / 3600.0);
            fprintf(logFile, "  Current sequence: {");
            for (int i = 0; i <= currentGapIndex; i++) {
                fprintf(logFile, "%lld, ", gaps[i]);
            }
            fprintf(logFile, "}\n\n");
            fflush(logFile);
        }
        
        // Check if we need to adjust parameters for next run
        if (numRemainingGaps > 10) {
            printf("WARNING: Search ended with %lld remaining gaps.\n", numRemainingGaps);
            printf("Consider: 1) Increasing runtime, 2) Adjusting parameters\n\n");
            if (logFile) {
                fprintf(logFile, "  WARNING: Too many remaining gaps (%lld)\n", numRemainingGaps);
            }
        }
    }
    
    // Final summary
    printf("\n\n========================================\n");
    printf("=== AUTOMATED SEARCH COMPLETE ===\n");
    printf("Final gap sequence: {");
    for (int i = 0; i < numInitialGaps + numGapsToFind; i++) {
        printf("%lld, ", gaps[i]);
    }
    printf("}\n");
    printf("========================================\n\n");
    
    if (logFile) {
        fprintf(logFile, "\n=== Search session complete ===\n");
        fprintf(logFile, "Final sequence: {");
        for (int i = 0; i < numInitialGaps + numGapsToFind; i++) {
            fprintf(logFile, "%lld, ", gaps[i]);
        }
        fprintf(logFile, "}\n\n");
        fclose(logFile);
    }
    
    free(gaps);
}

// Comparison function for sorting SequenceCandidates
int compareSequenceCandidate(const void* a, const void* b) {
    const SequenceCandidate* sa = (const SequenceCandidate*)a;
    const SequenceCandidate* sb = (const SequenceCandidate*)b;
    if (sa->count < sb->count) return -1;
    if (sa->count > sb->count) return 1;
    return 0;
}

// Find best N gap sequences from M initial sequences
// Each initial sequence gets extended with candidate next gaps, all tested together
// Returns the best numBestToKeep sequences
void findMultipleBestSequences(
    I64** initialSequences,       // Array of sequences, e.g., 4 sequences
    int numInitialSequences,      // Number of initial sequences (e.g., 4)
    int sequenceLength,           // Length of each initial sequence (e.g., 8)
    int numBestToKeep,            // How many best sequences to keep (e.g., 4)
    double minRatio,
    double maxRatio,
    double maxRuntimeSeconds,
    int numThreads,
    I64** outputSequences         // Output: best sequences (caller allocates)
) {
    printf("\n=== Searching for best %d sequences from %d initial sequences ===\n", 
           numBestToKeep, numInitialSequences);
    
    // Calculate average of last gaps for consistent arraySize
    I64 sumLastGaps = 0;
    for (int i = 0; i < numInitialSequences; i++) {
        sumLastGaps += initialSequences[i][sequenceLength - 1];
    }
    I64 avgLastGap = sumLastGaps / numInitialSequences;
    I64 arraySize = round(avgLastGap / 301.0 * 8000.0);
    
    printf("Average last gap: %lld, arraySize: %lld\n", avgLastGap, arraySize);
    
    // Count total candidates
    I64 totalCandidates = 0;
    for (int i = 0; i < numInitialSequences; i++) {
        I64 lastGap = initialSequences[i][sequenceLength - 1];
        I64 minNextGap = (I64)(lastGap * minRatio);
        I64 maxNextGap = (I64)(lastGap * maxRatio);
        totalCandidates += (maxNextGap - minNextGap + 1);
    }
    
    printf("Total candidate sequences: %lld\n\n", totalCandidates);
    
    // Allocate candidate array
    SequenceCandidate* candidates = malloc(sizeof(SequenceCandidate) * totalCandidates);
    I64 candidateIdx = 0;
    
    // Generate all candidates
    for (int i = 0; i < numInitialSequences; i++) {
        I64 lastGap = initialSequences[i][sequenceLength - 1];
        I64 minNextGap = (I64)(lastGap * minRatio);
        I64 maxNextGap = (I64)(lastGap * maxRatio);
        
        for (I64 nextGap = minNextGap; nextGap <= maxNextGap; nextGap++) {
            candidates[candidateIdx].fullSequence = malloc(sizeof(I64) * (sequenceLength + 4));
            // Copy initial sequence
            for (int j = 0; j < sequenceLength; j++) {
                candidates[candidateIdx].fullSequence[j] = initialSequences[i][j];
            }
            // Add new gap and terminator
            candidates[candidateIdx].fullSequence[sequenceLength] = nextGap;
            candidates[candidateIdx].fullSequence[sequenceLength + 1] = 0;
            candidates[candidateIdx].fullSequence[sequenceLength + 2] = 0;
            candidates[candidateIdx].fullSequence[sequenceLength + 3] = -1;
            
            candidates[candidateIdx].fromInitialIndex = i;
            candidates[candidateIdx].nextGap = nextGap;
            candidates[candidateIdx].count = 0;
            candidates[candidateIdx].sampleCount = 0;
            candidates[candidateIdx].mean = 0;
            candidates[candidateIdx].M2 = 0;
            
            candidateIdx++;
        }
    }
    
    // Now search similar to findOptimalNextGap_parameterized
    // but targeting numBestToKeep sequences instead of 1
    
    U64 startTime = currentTime();
    int numSamples = 3;  // Start with 3 samples
    I64 numRemaining = totalCandidates;
    
    // Prepare threading
    int* array_for_thread[numThreads];
    pthread_t threads[numThreads];
    SequenceThreadArg threadArgs[numThreads];
    
    for (int i = 0; i < numThreads; i++) {
        array_for_thread[i] = malloc(sizeof(int) * arraySize);
    }
    
    double targetHalvings = log(totalCandidates / (double)numBestToKeep) / log(2.0);
    double minStdErrs = 999.0;
    int iterationCount = 0;
    
    printf("Starting with %lld candidates, target %.1f halvings to reach %d\n", 
           totalCandidates, targetHalvings, numBestToKeep);
    
    while (numRemaining > numBestToKeep) {
        // Run samples on all remaining candidates using threads
        U64 pcgInitState = rand_pcg_u64();
        U64 pcgInc = rand_pcg_u64();
        
        for (int i = 0; i < numThreads; i++) {
            threadArgs[i].candidates = candidates;
            if (i == 0) {
                threadArgs[i].startIndex = 0;
            }
            else {
                threadArgs[i].startIndex = threadArgs[i-1].lastIndex + 1;
            }
            if (i == numThreads - 1) {
                threadArgs[i].lastIndex = numRemaining - 1;
            }
            else {
                threadArgs[i].lastIndex = ((i+1) * numRemaining) / numThreads - 1;
            }
            threadArgs[i].arraySize = arraySize;
            threadArgs[i].numSamples = numSamples;
            threadArgs[i].array = array_for_thread[i];
            threadArgs[i].pcgInitState = pcgInitState;
            threadArgs[i].pcgInc = pcgInc;
            pthread_create(&threads[i], NULL, thread_runSequenceSamples, (void*)&threadArgs[i]);
        }
        
        for (int i = 0; i < numThreads; i++) {
            pthread_join(threads[i], NULL);
        }
        
        if (COMPARE_COUNTER != 0) {
            printf("error: COMPARE_COUNTER should be 0\n");
            exit(1);
        }
        
        // Sort by count
        qsort(candidates, numRemaining, sizeof(SequenceCandidate), compareSequenceCandidate);
        
        // Calculate time progress and target
        double elapsedTime = (currentTime() - startTime) / (double)TICKS_PER_SEC;
        double timePercent = elapsedTime / maxRuntimeSeconds;
        if (timePercent > 1.0) timePercent = 1.0;
        
        double targetHalvingsDone = timePercent * targetHalvings;
        I64 targetNum = (I64)(totalCandidates / pow(2.0, targetHalvingsDone));
        if (targetNum < numBestToKeep) targetNum = numBestToKeep;
        
        // Enforce minimum of numThreads for parallel efficiency
        if (targetNum < numThreads && targetNum > numBestToKeep) {
            targetNum = numThreads;
        }
        
        // Calculate stdErrs and cut
        I64 targetIndex = targetNum - 1;
        if (targetIndex >= numRemaining) targetIndex = numRemaining - 1;
        if (targetIndex < numBestToKeep - 1) targetIndex = numBestToKeep - 1;
        
        I64 newNumRemaining = targetIndex + 1;
        
        // Calculate stdErrs
        double pooled_variance = 0;
        for (I64 i = 0; i < numRemaining; i++) {
            double sample_variance = candidates[i].M2 / (candidates[i].sampleCount - 1);
            pooled_variance += sample_variance;
        }
        pooled_variance /= numRemaining;
        double pooledStdErr = sqrt(pooled_variance / candidates[0].sampleCount);
        
        double adaptiveNumStdErrs = 10.0;
        if (newNumRemaining < numRemaining) {
            double meanDiff = candidates[newNumRemaining].mean - candidates[0].mean;
            adaptiveNumStdErrs = meanDiff / pooledStdErr;
            if (adaptiveNumStdErrs < 0.0) adaptiveNumStdErrs = 0.0;
        }
        
        if (newNumRemaining <= numRemaining && newNumRemaining >= numBestToKeep) {
            if (newNumRemaining >= numThreads || newNumRemaining == numBestToKeep) {
                numRemaining = newNumRemaining;
                if (adaptiveNumStdErrs < minStdErrs) {
                    minStdErrs = adaptiveNumStdErrs;
                }
            }
        }
        
        // Force cut if stuck
        if (numRemaining > numThreads && targetNum <= numThreads && targetNum > numBestToKeep) {
            numRemaining = numThreads;
        }
        
        iterationCount++;
        
        if (iterationCount % 5 == 0 || numRemaining <= 10) {
            printf("Iter %d: time %.1fs (%.0f%%), %lld sequences remain (target %lld), stdErrs=%.2f, samples=%lld\n",
                   iterationCount, elapsedTime, timePercent * 100, numRemaining, targetNum,
                   adaptiveNumStdErrs, candidates[0].sampleCount);
        }
        
        if (elapsedTime > maxRuntimeSeconds) {
            printf("Hit max runtime.\n");
            break;
        }
        
        numSamples = numSamples * 1.17 + 1;
    }
    
    printf("\n=== Search Complete ===\n");
    printf("Best %lld sequences found, min stdErrs: %.2f\n", numRemaining, minStdErrs);
    
    // Copy best sequences to output
    I64 numToCopy = numRemaining < numBestToKeep ? numRemaining : numBestToKeep;
    for (I64 i = 0; i < numToCopy; i++) {
        for (int j = 0; j <= sequenceLength; j++) {  // Include the new gap
            outputSequences[i][j] = candidates[i].fullSequence[j];
        }
        printf("  #%lld: from initial[%d], next gap=%lld, mean=%.1f\n",
               i+1, candidates[i].fromInitialIndex, candidates[i].nextGap, candidates[i].mean);
    }
    
    // Cleanup
    for (I64 i = 0; i < totalCandidates; i++) {
        free(candidates[i].fullSequence);
    }
    free(candidates);
    for (int i = 0; i < numThreads; i++) {
        free(array_for_thread[i]);
    }
}

// Automated multi-branch search with iterative halving
// Starts with M sequences, expands to N, then halves down to 1 final sequence
// Time allocation doubles each iteration (1x, 2x, 4x, 8x, ...)
void findBestSequenceAutomatedMultiBranch(
    I64** initialSequences,      // Array of initial sequences
    int numInitialSequences,     // Number of initial sequences (e.g., 4)
    int initialSequenceLength,   // Length of each initial sequence (e.g., 8)
    int numBestFirstIteration,   // Target number of sequences after first iteration (e.g., 16)
    int numIterations,           // How many iterations to run (e.g., 5: 16->8->4->2->1)
    double maxRuntimePerIter,    // Max runtime for first iteration in seconds (doubles each iteration)
    int numThreads               // Number of threads
) {
    printf("\n=== Automated Multi-Branch Search ===\n");
    printf("Starting with %d sequences of length %d\n", numInitialSequences, initialSequenceLength);
    printf("Target after first iteration: %d sequences, Iterations: %d\n", numBestFirstIteration, numIterations);
    printf("Base runtime (1st iter): %.1f seconds (doubles each iteration)\n\n", maxRuntimePerIter);
    
    // Open log file
    FILE* logFile = fopen("multibranch_search_results.txt", "a");
    if (logFile) {
        fprintf(logFile, "\n\n=== New multi-branch search session ===\n");
        fprintf(logFile, "Starting sequences: %d, length: %d\n", numInitialSequences, initialSequenceLength);
        fprintf(logFile, "First iteration target: %d, Iterations: %d\n", numBestFirstIteration, numIterations);
        time_t now = time(NULL);
        fprintf(logFile, "Time: %s\n", ctime(&now));
        fflush(logFile);
    }

    // fix off by one error
    numIterations -= 1;
    
    // Calculate halving schedule: expand then halve
    int* numToKeep = malloc(sizeof(int) * (numIterations + 1));
    int currentNum = numBestFirstIteration;  // First iteration target
    numToKeep[0] = currentNum;
    for (int i = 1; i <= numIterations; i++) {
        currentNum = currentNum / 2;
        if (currentNum < 1) currentNum = 1;
        numToKeep[i] = currentNum;
    }
    
    printf("Schedule: %d initial -> ", numInitialSequences);
    for (int i = 0; i <= numIterations; i++) {
        printf("%d", numToKeep[i]);
        if (i < numIterations) printf(" -> ");
    }
    printf("\n\n");
    
    // Allocate for current and next sequences
    int maxSequences = numToKeep[0];  // Maximum we'll ever have
    int currentLength = initialSequenceLength;
    
    I64** currentSequences = malloc(sizeof(I64*) * maxSequences);
    I64** nextSequences = malloc(sizeof(I64*) * maxSequences);
    
    for (int i = 0; i < maxSequences; i++) {
        currentSequences[i] = malloc(sizeof(I64) * (initialSequenceLength + numIterations + 2));
        nextSequences[i] = malloc(sizeof(I64) * (initialSequenceLength + numIterations + 2));
    }
    
    // Copy initial sequences to currentSequences
    for (int i = 0; i < numInitialSequences; i++) {
        for (int j = 0; j < initialSequenceLength; j++) {
            currentSequences[i][j] = initialSequences[i][j];
        }
    }
    int currentCount = numInitialSequences;
    
    // Run iterations
    for (int iter = 0; iter <= numIterations; iter++) {
        int targetCount = numToKeep[iter];
        
        // Calculate time for this iteration: double each iteration
        double iterTimeAllocation = maxRuntimePerIter * (1 << iter);  // 2^iter
        
        printf("========================================\n");
        printf("Iteration %d: Searching for best %d sequences (length %d -> %d)\n", 
               iter + 1, targetCount, currentLength, currentLength + 1);
        printf("Current sequences: %d\n", currentCount);
        printf("Time allocation: %.1f seconds (%.1f minutes)\n", iterTimeAllocation, iterTimeAllocation / 60.0);
        printf("========================================\n\n");
        
        U64 iterStart = currentTime();
        
        // Calculate ratios based on current sequence length (how many gaps we have)
        double minRatio = 2.08;
        static const double maxRatioSchedule[] = {5.00, 3.50, 3.20, 3.00, 2.82, 2.66, 2.52, 2.40, 2.32, 2.28, 2.26, 2.24, 2.23, 2.22, 2.22};
        int scheduleLength = sizeof(maxRatioSchedule) / sizeof(double);
        int scheduleIndex = currentLength - 1;  // Last gap is at index currentLength - 1
        double maxRatio;
        if (scheduleIndex >= 0 && scheduleIndex < scheduleLength) {
            maxRatio = maxRatioSchedule[scheduleIndex];
        } else if (scheduleIndex >= scheduleLength) {
            maxRatio = 2.22;
        } else {
            maxRatio = 5.00;
        }
        
        printf("Using minRatio=%.2f, maxRatio=%.2f (sequence length %d)\n\n", minRatio, maxRatio, currentLength);
        
        // Run findMultipleBestSequences
        findMultipleBestSequences(
            currentSequences, currentCount, currentLength, targetCount,
            minRatio, maxRatio, iterTimeAllocation, numThreads,
            nextSequences
        );
        
        U64 iterEnd = currentTime();
        double iterTime = (iterEnd - iterStart) / (double)TICKS_PER_SEC;
        
        printf("\n========================================\n");
        printf("Iteration %d complete: %d sequences found in %.1f seconds\n", 
               iter + 1, targetCount, iterTime);
        printf("========================================\n");
        
        // Print all best sequences from this iteration
        printf("Best sequences from this iteration:\n");
        for (int i = 0; i < targetCount; i++) {
            printf("  #%d: {", i + 1);
            for (int j = 0; j <= currentLength; j++) {  // currentLength is old length, +1 for new gap
                printf("%lld", nextSequences[i][j]);
                if (j < currentLength) printf(", ");
            }
            printf("}\n");
        }
        printf("\n");
        
        // Log to file
        if (logFile) {
            fprintf(logFile, "\nIteration %d: %d -> %d sequences (length %d)\n", 
                    iter + 1, currentCount, targetCount, currentLength + 1);
            fprintf(logFile, "  Time allocated: %.1f seconds, Time used: %.1f seconds\n", iterTimeAllocation, iterTime);
            fprintf(logFile, "  Best sequences:\n");
            for (int i = 0; i < targetCount; i++) {
                fprintf(logFile, "    #%d: {", i + 1);
                for (int j = 0; j <= currentLength; j++) {
                    fprintf(logFile, "%lld", nextSequences[i][j]);
                    if (j < currentLength) fprintf(logFile, ", ");
                }
                fprintf(logFile, "}\n");
            }
            fflush(logFile);
        }
        
        // Swap current and next
        I64** temp = currentSequences;
        currentSequences = nextSequences;
        nextSequences = temp;
        
        currentCount = targetCount;
        currentLength++;
        
        // If we're down to 1, we're done
        if (currentCount == 1) {
            printf("\n=== FINAL RESULT ===\n");
            printf("Best sequence found: {");
            for (int j = 0; j < currentLength; j++) {
                printf("%lld", currentSequences[0][j]);
                if (j < currentLength - 1) printf(", ");
            }
            printf("}\n\n");
            
            if (logFile) {
                fprintf(logFile, "\n=== Final best sequence ===\n{");
                for (int j = 0; j < currentLength; j++) {
                    fprintf(logFile, "%lld", currentSequences[0][j]);
                    if (j < currentLength - 1) fprintf(logFile, ", ");
                }
                fprintf(logFile, "}\n\n");
            }
            break;
        }
    }
    
    // Cleanup
    for (int i = 0; i < maxSequences; i++) {
        free(currentSequences[i]);
        free(nextSequences[i]);
    }
    free(currentSequences);
    free(nextSequences);
    free(numToKeep);
    
    if (logFile) {
        fclose(logFile);
    }
}

int main(int argc, const char * argv[]) {
    printf("\n\n\n\n\n\n\n\n\n\n\n");
    
    U64 programStartTime = currentTime();
    
    srand((unsigned)time(NULL));
    srand_pcg_easy();
    
    // compute 3-smooth numbers for pratt gap sequence
    if (0) {
        print3smoothNumbers();
    }
    
    // test average runtime of different sorting algorithms
    if (0) {
        testAverageRuntime();
    }
    
    // find worst case approximation using a greedy algorithm
    if (0) {
        findWorstCase(512, gaps_dokken12_222f);
    }

    // find worst case approximation using random mutations
    if (0) {
        findWorstCaseWithRandomMutations();
    }
    
    // automated search for multiple gaps in sequence (single branch)
    if (0) {
        I64 startingGaps[] = {1, 4, 10, 23, 57, 132, 301, 701};
        int numStartingGaps = sizeof(startingGaps) / sizeof(I64);
        int numGapsToFind = 10;
        double maxRuntimePerGap = 120.0;  // in seconds
        int numThreads = 5;
        
        findMultipleGapsAutomated(startingGaps, numStartingGaps, numGapsToFind, maxRuntimePerGap, numThreads);
    }

    // automated search for multiple gaps in sequence (single branch)
    if (0) {
        I64 startingGaps[] = {1, 4, 10, 23, 57, 132, 301, 644, 1408, 3227, 6847, 14842, 31970, 69487, 149728};
        int numStartingGaps = sizeof(startingGaps) / sizeof(I64);
        int numGapsToFind = 6;
        double maxRuntimePerGap = 7200.0;  // in seconds
        int numThreads = 10;
        
        findMultipleGapsAutomated(startingGaps, numStartingGaps, numGapsToFind, maxRuntimePerGap, numThreads);
    }

    // Automated multi-branch search 
    if (0) {
        // Start with these sequences
        I64 seq1[] = {1, 4, 10, 23};
        I64 seq2[] = {1, 4, 10, 21};
        I64 seq3[] = {1, 4, 9, 24};
        
        I64* initialSequences[] = {seq1, seq2, seq3};
        
        int numInitialSequences = sizeof(initialSequences) / sizeof(initialSequences[0]);
        int initialSequenceLength = sizeof(seq1) / sizeof(seq1[0]);
        
        findBestSequenceAutomatedMultiBranch(
            initialSequences,
            numInitialSequences,
            initialSequenceLength,
            64,                 // target number of sequences after first iteration, then halves each iteration
            1,                 // numIterations
            60.0,              // runtime in seconds of first iteration, then doubles each iteration
            5                  // numThreads
        );
    }

    // Automated multi-branch search 
    if (0) {
        // Start with these sequences
        I64 seq1[] = {1, 4, 10, 23, 57, 132, 301};
        I64 seq2[] = {1, 4, 10, 21, 56, 125, 288};
        
        I64* initialSequences[] = {seq1, seq2};
        
        int numInitialSequences = sizeof(initialSequences) / sizeof(initialSequences[0]);
        int initialSequenceLength = sizeof(seq1) / sizeof(seq1[0]);
        
        findBestSequenceAutomatedMultiBranch(
            initialSequences,
            numInitialSequences,
            initialSequenceLength,
            16,                // target number of sequences after first iteration, then halves each iteration
            1,                 // numIterations
            60.0,              // runtime in seconds of first iteration, then doubles each iteration
            5                  // numThreads
        );
    }

    // Automated multi-branch search 
    if (1) {
        // Start with these sequences
        I64 seq1[] = {1, 4, 10, 23, 57};
        I64 seq1b[] = {1, 4, 10, 23, 61};
        I64 seq1c[] = {1, 4, 10, 23, 54};
        I64 seq1d[] = {1, 4, 10, 23, 55};
        I64 seq1e[] = {1, 4, 10, 23, 60};
        I64 seq1f[] = {1, 4, 10, 23, 61};
        I64 seq2[] = {1, 4, 10, 21, 56};
        I64 seq2b[] = {1, 4, 10, 21, 59};
        I64 seq2c[] = {1, 4, 10, 21, 57};
        I64 seq2d[] = {1, 4, 10, 21, 55};
        I64 seq2e[] = {1, 4, 10, 21, 49};
        I64 seq2f[] = {1, 4, 10, 21, 58};
        I64 seq2g[] = {1, 4, 10, 21, 60};
        I64 seq3[] = {1, 4, 9, 24, 58};
        I64 seq3b[] = {1, 4, 9, 24, 61};
        I64 seq3c[] = {1, 4, 9, 24, 59};
        I64 seq3d[] = {1, 4, 9, 24, 55};
        I64 seq3e[] = {1, 4, 9, 24, 53};
        I64 seq3f[] = {1, 4, 9, 24, 52};
        I64 seq3g[] = {1, 4, 9, 24, 56};
        I64 seq3h[] = {1, 4, 9, 24, 62};
        
        I64* initialSequences[] = {seq1, seq1b, seq1c, seq1d, seq1e, seq1f, seq2, seq2b, seq2c, seq2d, seq2e, seq2f, seq2g, seq3, seq3b, seq3c, seq3d, seq3e, seq3f, seq3g, seq3h};
        
        int numInitialSequences = sizeof(initialSequences) / sizeof(initialSequences[0]);
        int initialSequenceLength = sizeof(seq1) / sizeof(seq1[0]);
        
        findBestSequenceAutomatedMultiBranch(
            initialSequences,
            numInitialSequences,
            initialSequenceLength,
            512,                 // target number of sequences after first iteration, then halves each iteration
            10,                 // numIterations
            50.0,              // runtime in seconds of first iteration, then doubles each iteration
            10                  // numThreads
        );
    }
    
    printf("program run time = %g seconds\n", ((currentTime() - programStartTime) / (double)TICKS_PER_SEC));
    return 0;
}





