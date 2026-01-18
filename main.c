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
// can be improved further by testing random swaps repeatedly and keeping any changes that increase total number of compares
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

// compute max-gcd and ratios of each number in gap sequence
void computeMaxGcdAndRatios(void) {
    I64 gaps[] = {1, 4, 10, 23, 57, 132, 301, 701};
    int length = sizeof(gaps) / sizeof(I64);
    
    printf("ratios:\n");
    for (int i = 1; i < length; i++) {
        double ratio = gaps[i] / (double) gaps[i-1];
        printf("%.3f, ", ratio);
    }
    printf("\n\nmaxGcds:\n");
    for (int i = 1; i < length; i++) {
        I64 maxGcd = 1;
        for (int j = 0; j < i; j++) {
            I64 g = gcd(gaps[i], gaps[j]);
            if (g > maxGcd) {
                maxGcd = g;
            }
        }
        printf("%lld, ", maxGcd);
    }
    printf("\n\n");
    
    double minRatio = 2.12;
    double maxRatio = 2.36;
    int lastIndex = length - 1;
    I64 minGap = gaps[lastIndex] * minRatio;
    I64 maxGap = gaps[lastIndex] * maxRatio;
    int numTotal = 0;
    int numPrinted = 0;
    int numCoprime = 0;
    int numGcdAtMost2 = 0;
    int numGcdAtMost4 = 0;
    int numGcdAtMost6 = 0;
    int numGcdAtMost13 = 0;
    int numGcdAtMost20 = 0;
    printf("{");
    int counter = 0;
    for (I64 gap = minGap; gap < maxGap; gap++) {
        I64 maxGcd = 1;
        for (int j = 0; j <= lastIndex; j++) {
            I64 g = gcd(gap, gaps[j]);
            if (g > maxGcd) {
                maxGcd = g;
            }
        }
        if (maxGcd == 1) {
            numCoprime += 1;
        }
        if (maxGcd <= 2) {
            numGcdAtMost2 += 1;
        }
        if (maxGcd <= 4) {
            numGcdAtMost4 += 1;
        }
        if (maxGcd <= 6) {
            numGcdAtMost6 += 1;
        }
        if (maxGcd <= 13) {
            numGcdAtMost13 += 1;
        }
        if (maxGcd <= 20) {
            numGcdAtMost20 += 1;
        }
        numTotal += 1;
        if (maxGcd <= 6) {
            //if (counter % 1518 == 0) {
            printf("%lld,", gap);
            numPrinted++;
            //}
            counter = counter + 1;
        }
    }
    printf("}\n\nlength of printed array = %d\n\n", numPrinted);
    printf("percentage coprime to all earlier gaps = %g\n", numCoprime / (double)numTotal);
    printf("percentage maxGcd at most 2 to all earlier gaps = %g\n", numGcdAtMost2 / (double)numTotal);
    printf("percentage maxGcd at most 4 to all earlier gaps = %g\n", numGcdAtMost4 / (double)numTotal);
    printf("percentage maxGcd at most 6 to all earlier gaps = %g\n", numGcdAtMost6 / (double)numTotal);
    printf("percentage maxGcd at most 13 to all earlier gaps = %g\n", numGcdAtMost13 / (double)numTotal);
    printf("percentage maxGcd at most 20 to all earlier gaps = %g\n\n", numGcdAtMost20 / (double)numTotal);
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
        printf("thread not needed\n");
        return NULL;
    }
    printf("thread search from indexes %lld to %lld\n", arg->startIndex, arg->lastIndex);
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
        
        printf("gap1=%lld, total compare count = %llu\n", gap1, gapAndCountArray[i].count);
    }
    
    return NULL;
}

// find optimal shellsort gap sequences
void findOptimalNextGap(void) {
    U64 startTime = currentTime();
    
    const int numThreads = 5;
    
    const double minRatio = 2.12;
    const double maxRatio = 2.36;
    
    const double numStdErrsToCutoff = 3.5;// stop testing gap1's when this many standard errors away from best
    
    int numSamples = 50;// how many random samples to run on the first loop
    
    I64 gaps[] = {1, 4, 10, 23, 57, 132, 301, 701, 0, 0, 0, -1};
    // this gaps variable must must end with {0, 0, 0, -1}, so that it has room for a candidate next gap and then 2 random gaps
    
    int gapIndex1 = 0;
    while (gaps[gapIndex1] != 0) {
        gapIndex1++;
    }
    
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
    
    printf("numGap1s = %lld\n\n", numGap1s);
    
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
    
    for (int i = 0; i < numThreads; i++) {
        array_for_thread[i] = malloc(sizeof(int) * arraySize);
        gaps_for_thread[i] = malloc(sizeof(gaps));
        memcpy(gaps_for_thread[i], gaps, sizeof(gaps));
    }
    
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
        printf("after all threads joined, compare counter = %lld\n\n", COMPARE_COUNTER);
        if (COMPARE_COUNTER != 0) {
            printf("error 1577\n");
            exit(1);
        }
        
        qsort(gapAndCountArray, numGap1s, sizeof(GapAndCount), compareGapAndCount);
        printf("\nsorted\n");
        double cutOffValue = 0;
        double pooled_variance = 0;
        for (I64 i = numGap1s - 1; i >= 0; i--) {
            double sample_variance = gapAndCountArray[i].M2 / (gapAndCountArray[i].sampleCount - 1);
            double stdErr = sqrt(sample_variance / gapAndCountArray[i].sampleCount);
            pooled_variance += sample_variance;
            printf("compares = %lld, gap1 = %lld, mean=%g, stderr=%g\n", gapAndCountArray[i].count, gapAndCountArray[i].gap, gapAndCountArray[i].mean, stdErr);
        }
        pooled_variance /= numGap1s;
        double pooledStdErr = sqrt(pooled_variance / gapAndCountArray[0].sampleCount);
        if (gapAndCountArray[0].sampleCount != gapAndCountArray[1].sampleCount) {
            printf("error 1703\n");
            exit(1);
        }
        printf("pooledStdErr = %g\n", pooledStdErr);
        int cutoffIndex = 0;//round((numGap1s-1) * standardNormalCdf(-numStdErrsToCutoff));
        printf("using index=%d plus %g stderrs for cutoff value\n", cutoffIndex, numStdErrsToCutoff);
        cutOffValue = gapAndCountArray[cutoffIndex].mean + numStdErrsToCutoff * pooledStdErr;
        printf("cut any mean > %g\n", cutOffValue);
        for (int i = 0.6*numGap1s; i < numGap1s; i++) {// cut at most 40% of gap1's
            if (gapAndCountArray[i].mean > cutOffValue) {
                printf("cut worst %lld\n", numGap1s - i);
                numGap1s = i;
                break;
            }
        }
        printf("{");
        for (int i = 0; i <= (numGap1s-1); i++) {
            printf("%lld,", gapAndCountArray[i].gap);
        }
        printf("}\n\n");
        
        if (((currentTime() - startTime) / (double)TICKS_PER_SEC) > (4 * 3600.0)) {
            printf("hit max runtime, stopping\n");
            break;
        }
        
        //numSamples = gapAndCountArray[0].sampleCount / 6 + 3;
        numSamples = numSamples * 1.17 + 1;
        if (numGap1s > 1) {
            printf("run %d more samples on numGap1s = %lld\n\n", numSamples, numGap1s);
        }
    }
    
    printf("total samples ran = %lld\n\n", gapAndCountArray[0].sampleCount);
    
    for (int i = 0; i < numThreads; i++) {
        free(gaps_for_thread[i]);
        free(array_for_thread[i]);
    }
    free(gapAndCountArray);
    free(gap1s);
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
    
    // compute max-gcd and ratios of each number in gap sequence
    if (0) {
        computeMaxGcdAndRatios();
    }
    
    // find optimal shellsort gap sequences
    if (1) {
        findOptimalNextGap();
    }
    
    printf("program run time = %g seconds\n", ((currentTime() - programStartTime) / (double)TICKS_PER_SEC));
    return 0;
}





