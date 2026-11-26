/*
 * shellwc.c -- command-line tool for the Shellsort worst-case study.
 *
 *   shellwc eval  N gaps p0 p1 ...     comparisons for one input permutation
 *   shellwc brute N gaps               exhaustive worst case over all N! inputs (N <= 12)
 *   shellwc dp    N gaps [maxstates]   exact worst case by threshold-set DP (N <= 64)
 *   shellwc sa    N gaps [iters] [restarts] [seed]   annealing lower bound
 *   shellwc avg   N gaps [samples] [seed]            average comparisons (exact if N <= 10)
 *   shellwc cmp   N gaps               dp vs sa (heuristic validation)
 *
 * gaps are given smallest-first, comma separated, e.g. 1,4,9,11,21
 */
#include <time.h>
#include <omp.h>
#include "core.h"
#include "exact.h"
#include "exact2.h"
#include "layered.h"
#include "anneal.h"
#include "vlns.h"
#include "fastsa.h"
#include "beam.h"

static double now(void) { return omp_get_wtime(); }

static void read_seq(seq_t *s, int n, const char *str) {
    int gaps[MAXK]; int cnt = parse_gaps(str, gaps);
    seq_make(s, n, gaps, cnt);
}

static void print_perm(const int *x, int n) {
    printf("[");
    for (int i = 0; i < n; i++) printf(i ? ",%d" : "%d", x[i]);
    printf("]\n");
}

/* ---------------- brute force ---------------- */
static long brute_worst(const seq_t *s, int *wperm) {
    int n = s->n;
    long best = -1;
    int bestp[MAXN];
    /* parallelise over the first two positions */
    #pragma omp parallel
    {
        long lbest = -1; int lp[MAXN];
        #pragma omp for schedule(dynamic)
        for (int ab = 0; ab < n * n; ab++) {
            int a0 = ab / n, a1 = ab % n;
            if (a0 == a1) continue;
            int rest[MAXN], m = 0;
            for (int v = 0; v < n; v++) if (v != a0 && v != a1) rest[m++] = v;
            /* iterate permutations of rest in lexicographic order */
            int x[MAXN];
            for (;;) {
                x[0] = a0; x[1] = a1;
                memcpy(x + 2, rest, sizeof(int) * m);
                long c = shell_cost(x, s);
                if (c > lbest) { lbest = c; memcpy(lp, x, sizeof(int) * n); }
                int i = m - 2;
                while (i >= 0 && rest[i] > rest[i + 1]) i--;
                if (i < 0) break;
                int j = m - 1;
                while (rest[j] < rest[i]) j--;
                int t = rest[i]; rest[i] = rest[j]; rest[j] = t;
                for (int l = i + 1, r = m - 1; l < r; l++, r--) { t = rest[l]; rest[l] = rest[r]; rest[r] = t; }
            }
        }
        #pragma omp critical
        { if (lbest > best) { best = lbest; memcpy(bestp, lp, sizeof(int) * n); } }
    }
    if (wperm) memcpy(wperm, bestp, sizeof(int) * n);
    return best;
}

/* ---------------- average ---------------- */
static double avg_cost(const seq_t *s, long samples, uint64_t seed, double *stderr_out) {
    int n = s->n;
    double sum = 0, sum2 = 0; long cnt = 0;
    if (n <= 10) {
        /* exact enumeration */
        int x[MAXN]; for (int i = 0; i < n; i++) x[i] = i;
        for (;;) {
            long c = shell_cost(x, s); sum += c; sum2 += (double)c * c; cnt++;
            int i = n - 2; while (i >= 0 && x[i] > x[i + 1]) i--;
            if (i < 0) break;
            int j = n - 1; while (x[j] < x[i]) j--;
            int t = x[i]; x[i] = x[j]; x[j] = t;
            for (int l = i + 1, r = n - 1; l < r; l++, r--) { t = x[l]; x[l] = x[r]; x[r] = t; }
        }
        if (stderr_out) *stderr_out = 0;
        return sum / cnt;
    }
    #pragma omp parallel reduction(+:sum,sum2,cnt)
    {
        rng_t r; rng_seed(&r, seed * 1000003ULL + omp_get_thread_num());
        int x[MAXN];
        #pragma omp for schedule(static)
        for (long i = 0; i < samples; i++) {
            random_perm(x, n, &r);
            long c = shell_cost(x, s); sum += c; sum2 += (double)c * c; cnt++;
        }
    }
    double mean = sum / cnt, var = sum2 / cnt - mean * mean;
    if (stderr_out) *stderr_out = sqrt(var / cnt);
    return mean;
}

/* Luby et al. universal restart sequence 1,1,2,1,1,2,4,1,1,2,... (i >= 1): within a log factor of the best
   fixed restart length for any unknown run-time distribution */
static long long luby(long long i) {
    long long k = 1;
    while ((1LL << k) - 1 < i) k++;
    return (1LL << k) - 1 == i ? 1LL << (k - 1) : luby(i - (1LL << (k - 1)) + 1);
}

int main(int argc, char **argv) {
    if (argc < 4) {
        fprintf(stderr, "usage: shellwc {eval|brute|dp|sa|avg|cmp} N gaps ...\n");
        return 1;
    }
    const char *mode = argv[1];
    if (getenv("FSA_ROT")) FSA_ROT = atoi(getenv("FSA_ROT"));
    if (getenv("FSA_HDEC")) VCTX_HDEC = atoi(getenv("FSA_HDEC"));
    int luby_on = getenv("FSA_LUBY") ? atoi(getenv("FSA_LUBY")) : 0;   /* fwc: restart r runs iters*luby(r+1) moves */
    int n = atoi(argv[2]);
    seq_t s; read_seq(&s, n, argv[3]);
    char sb[512]; seq_str(&s, sb);

    if (!strcmp(mode, "eval")) {
        int x[MAXN];
        for (int i = 0; i < n; i++) x[i] = atoi(argv[4 + i]);
        printf("N=%d seq={%s} comparisons=%ld\n", n, sb, shell_cost(x, &s));
    } else if (!strcmp(mode, "brute")) {
        int w[MAXN]; double t = now();
        long W = brute_worst(&s, w);
        printf("N=%d seq={%s} brute_worst=%ld time=%.2fs perm=", n, sb, W, now() - t); print_perm(w, n);
    } else if (!strcmp(mode, "dp")) {
        long long lim = argc > 4 ? atoll(argv[4]) : 2000000000LL;
        int w[MAXN]; long long st; double t = now();
        long long W = exact_worst(&s, lim, w, &st);
        if (W < 0) { printf("N=%d seq={%s} dp=TOO_BIG states=%lld\n", n, sb, st); return 0; }
        long chk = shell_cost(w, &s);
        printf("N=%d seq={%s} dp_worst=%lld states=%lld check=%ld time=%.2fs perm=", n, sb, W, st, chk, now() - t);
        print_perm(w, n);
    } else if (!strcmp(mode, "dp2")) {
        long long lim = argc > 4 ? atoll(argv[4]) : 2000000000LL;
        int w[MAXN]; long long st; double t = now(); int kh;
        x2_best_kh(&s, lim, &kh);
        long long W = exact_worst2(&s, lim, w, &st);
        if (W < 0) { printf("N=%d seq={%s} dp2=TOO_BIG\n", n, sb); return 0; }
        long chk = shell_cost(w, &s);
        printf("N=%d seq={%s} dp2_worst=%lld kh=%d states=%lld check=%ld time=%.2fs perm=", n, sb, W, kh, st, chk, now() - t);
        print_perm(w, n);
    } else if (!strcmp(mode, "ils")) {
        /* ils N gaps [rounds] [w] [seed] [kickT] [kickIters] : VLNS iterated local search, all threads */
        long long rounds = argc > 4 ? atoll(argv[4]) : 200;
        int w = argc > 5 ? atoi(argv[5]) : 12;
        uint64_t seed = argc > 6 ? strtoull(argv[6], 0, 10) : 1;
        double kT = argc > 7 ? atof(argv[7]) : 1.0;
        long long kI = argc > 8 ? atoll(argv[8]) : 20LL * n;
        double t = now();
        long best = -1; int bp[MAXN];
        #pragma omp parallel
        {
            rng_t r; rng_seed(&r, seed * 7919ULL + omp_get_thread_num());
            int x[MAXN];
            long v = v_ils(&s, x, 0, rounds, w, kT, kI, 1LL << 60, &r);
            #pragma omp critical
            { if (v > best) { best = v; memcpy(bp, x, sizeof(int) * n); } }
        }
        int in0[MAXN];
        if (s.k > 1) unsort_first(bp, in0, n, s.g[0]); else memcpy(in0, bp, sizeof(int) * n);
        printf("N=%d seq={%s} ils_worst=%ld check=%ld time=%.2fs perm=", n, sb, best, shell_cost(in0, &s), now() - t);
        print_perm(in0, n);
    } else if (!strcmp(mode, "wc")) {
        /* wc N gaps [iters] [seed] [T0] [T1] [w] : per thread long anneal + window-DP polish */
        long long iters = argc > 4 ? atoll(argv[4]) : 2000000;
        uint64_t seed = argc > 5 ? strtoull(argv[5], 0, 10) : 1;
        double T0 = argc > 6 ? atof(argv[6]) : 1.5, T1 = argc > 7 ? atof(argv[7]) : 0.05;
        int w = argc > 8 ? atoi(argv[8]) : 10;
        wcprob_t P; wc_prob_seq(&P, &s);
        double t = now();
        long best = -1, bestsa = -1; int bp[MAXN];
        #pragma omp parallel
        {
            rng_t r; rng_seed(&r, seed * 7919ULL + omp_get_thread_num());
            int x[MAXN];
            long v1 = wc_anneal(&P, x, 0, iters, 1, T0, T1, 1LL << 60, &r);
            long v = s.k > 1 ? v_polish(&s, x, w, 50, &r) : v1;
            #pragma omp critical
            { if (v1 > bestsa) bestsa = v1; if (v > best) { best = v; memcpy(bp, x, sizeof(int) * n); } }
        }
        int in0[MAXN];
        if (s.k > 1) unsort_first(bp, in0, n, s.g[0]); else memcpy(in0, bp, sizeof(int) * n);
        printf("N=%d seq={%s} wc_worst=%ld (sa %ld) check=%ld time=%.2fs perm=", n, sb, best, bestsa, shell_cost(in0, &s), now() - t);
        print_perm(in0, n);
    } else if (!strcmp(mode, "semi")) {
        /* Numerical-semigroup upper bound.  After passes with gaps H the array is d-sorted for every d in
           the semigroup <H>, so pass h can only have an inversion between chain elements at distance d*h
           when d*h is NOT in <H_earlier>.  cost(pass) <= (n - h) + #such pairs; first pass: exact max. */
        long U = 0;
        static char inS[2 * MAXN];
        memset(inS, 0, sizeof(inS)); inS[0] = 1;
        printf("N=%d seq={%s}\n", n, sb);
        for (int j = 0; j < s.k; j++) {
            int h = s.g[j]; long pairs = 0;
            if (j == 0) { long A = pass_max(n, h); U += A; printf("  pass %3d: max %ld (decoupled)\n", h, A); }
            else {
                for (int i = 0; i < n; i++) for (int d = 1; i - d * h >= 0; d++) if (!inS[d * h]) pairs++;
                U += (n - h) + pairs;
                int gen = 0; for (int d = 1; d < n; d++) if (!inS[d]) gen++;
                printf("  pass %3d: <= %ld  (n-h=%d, invertible chain pairs=%ld; distances < n not yet sorted=%d)\n", h, (n - h) + pairs, n - h, pairs, gen);
            }
            for (int d = h; d < 2 * MAXN; d++) if (inS[d - h]) inS[d] = 1;
        }
        printf("  semigroup upper bound U = %ld\n", U);
    } else if (!strcmp(mode, "fwc")) {
        /* fwc N gaps [iters] [seed] [T0] [T1] [w] : incremental anneal + window-DP polish, all threads */
        long long iters = argc > 4 ? atoll(argv[4]) : 50000000;
        uint64_t seed = argc > 5 ? strtoull(argv[5], 0, 10) : 1;
        double T0 = argc > 6 ? atof(argv[6]) : 3.0, T1 = argc > 7 ? atof(argv[7]) : 0.05;
        int w = argc > 8 ? atoi(argv[8]) : 10;
        long long target = argc > 9 ? atoll(argv[9]) : (1LL << 60);   /* stop once this value is reached */
        int maxrep_arg = argc > 10 ? atoi(argv[10]) : 1000;            /* restarts per thread when targeting */
        static volatile int found_target = 0;
        long restarts = 0;
        /* FSA_SEEDS=file: every restart anneals from an input drawn from the file (one permutation of 0..N-1 per
           line, e.g. the worst inputs found for related sequences) instead of a random permutation */
        static int seeds[256][MAXN]; int nseeds = 0;
        if (getenv("FSA_SEEDS")) {
            FILE *fs = fopen(getenv("FSA_SEEDS"), "r");
            static char line[1 << 16];
            while (fs && nseeds < 256 && fgets(line, sizeof line, fs)) {
                int c = 0, ok = 1, seen[MAXN] = {0}; char *p = line;
                while (c < n) {
                    while (*p && (*p < '0' || *p > '9')) p++;
                    if (!*p) break;
                    int v = (int)strtol(p, &p, 10);
                    if (v >= n || seen[v]) { ok = 0; break; }
                    seen[v] = 1; seeds[nseeds][c++] = v;
                }
                if (ok && c == n) nseeds++;
            }
            if (fs) fclose(fs);
            fprintf(stderr, "fwc: %d seed inputs of size %d from %s\n", nseeds, n, getenv("FSA_SEEDS"));
        }
        double t = now();
        long best = -1, bestsa = -1; int bp[MAXN];
        if (s.k == 1) { best = bestsa = (long)n * (n - 1) / 2; for (int i = 0; i < n; i++) bp[i] = n - 1 - i; }
        else {
            vctx_t V; vctx_init(&V, &s);
            #pragma omp parallel
            {
                rng_t r; rng_seed(&r, seed * 7919ULL + omp_get_thread_num());
                fsa_t F; fsa_alloc(&F, &V);
                int x[MAXN], xb[MAXN];
                long v1 = -1, v = -1, v1b = -1, vb = -1;     /* best over this thread's restarts, not only the last */
                int maxrep = target < (1LL << 60) ? maxrep_arg : 1;
                for (int rep = 0; rep < maxrep && !found_target; rep++) {
                    if (nseeds) memcpy(x, seeds[rng_below(&r, nseeds)], sizeof(int) * n);
                    v1 = fsa_anneal(&s, &V, &F, x, nseeds > 0, luby_on ? iters * luby(rep + 1) : iters, T0, T1, target - 1, &r);
                    v = v1 >= target ? v1 : v_polish(&s, x, w, 50, &r);
                    #pragma omp atomic
                    restarts++;
                    if (v1 > v1b) v1b = v1;
                    if (v > vb) { vb = v; memcpy(xb, x, sizeof(int) * n); }
                    if (v >= target) { found_target = 1; break; }
                }
                fsa_free(&F);
                #pragma omp critical
                { if (v1b > bestsa) bestsa = v1b; if (vb > best) { best = vb; memcpy(bp, xb, sizeof(int) * n); } }
            }
            vctx_free(&V);
        }
        int in0[MAXN];
        if (s.k > 1) unsort_first(bp, in0, n, s.g[0]); else memcpy(in0, bp, sizeof(int) * n);
        printf("N=%d seq={%s} fwc_worst=%ld (sa %ld) check=%ld restarts=%ld time=%.2fs perm=", n, sb, best, bestsa,
               shell_cost(in0, &s), restarts, now() - t);
        print_perm(in0, n);
    } else if (!strcmp(mode, "pt")) {
        /* pt N gaps [sweeps] [seed] [R] [Tmin] [Tmax] [mpe] [target] [w] : one parallel-tempering ladder per
           thread (R replicas, mpe moves per replica between exchanges, window-DP polish of improved bests);
           all threads stop once `target` is reached */
        long long sweeps = argc > 4 ? atoll(argv[4]) : 10000;
        uint64_t seed = argc > 5 ? strtoull(argv[5], 0, 10) : 1;
        int R = argc > 6 ? atoi(argv[6]) : 16;
        double Tmin = argc > 7 ? atof(argv[7]) : 0.2, Tmax = argc > 8 ? atof(argv[8]) : 3.0;
        int mpe = argc > 9 ? atoi(argv[9]) : 1000;
        long long target = argc > 10 ? atoll(argv[10]) : (1LL << 60);
        int w = argc > 11 ? atoi(argv[11]) : 10;
        static volatile int found = 0;
        double t = now();
        long best = -1; int bp[MAXN];
        vctx_t V; vctx_init(&V, &s);
        #pragma omp parallel
        {
            rng_t r; rng_seed(&r, seed * 7919ULL + omp_get_thread_num());
            int x[MAXN];
            long v = fsa_pt(&s, &V, x, sweeps, R, Tmin, Tmax, mpe, w, target - 1, &found, &r);
            if (v < target) { long vp = v_polish(&s, x, w, 50, &r); if (vp > v) v = vp; }
            if (v >= target) found = 1;
            #pragma omp critical
            { if (v > best) { best = v; memcpy(bp, x, sizeof(int) * n); } }
        }
        vctx_free(&V);
        int in0[MAXN]; unsort_first(bp, in0, n, s.g[0]);
        printf("N=%d seq={%s} pt_worst=%ld check=%ld time=%.2fs perm=", n, sb, best, shell_cost(in0, &s), now() - t);
        print_perm(in0, n);
    } else if (!strcmp(mode, "beam")) {
        /* beam N gaps [B] [seed] [polish_w] [anneal_iters] [T0] : beam search over the threshold-set DP, then
           optionally a low-temperature anneal from its input and a window-DP polish */
        long B = argc > 4 ? atol(argv[4]) : 100000;
        uint64_t seed = argc > 5 ? strtoull(argv[5], 0, 10) : 1;
        int pw = argc > 6 ? atoi(argv[6]) : 0;
        long long ai = argc > 7 ? atoll(argv[7]) : 0;
        double aT0 = argc > 8 ? atof(argv[8]) : 0.5;
        int x[MAXN]; double t = now();
        long Wb = beam_worst(&s, B, seed, x, 1), W = Wb;
        if (Wb < 0) { printf("N=%d seq={%s} beam=NOT_APPLICABLE\n", n, sb); return 0; }
        rng_t r; rng_seed(&r, seed);
        if (ai > 0) {
            vctx_t V; vctx_init(&V, &s); fsa_t F; fsa_alloc(&F, &V);
            long v = fsa_anneal(&s, &V, &F, x, 1, ai, aT0, 0.05, 1LL << 60, &r);
            if (v > W) W = v;
            fsa_free(&F); vctx_free(&V);
        }
        if (pw > 0) { long v = v_polish(&s, x, pw, 50, &r); if (v > W) W = v; }
        int in0[MAXN]; unsort_first(x, in0, n, s.g[0]);
        printf("N=%d seq={%s} beam_worst=%ld final=%ld check=%ld B=%ld time=%.2fs perm=", n, sb, Wb, W,
               shell_cost(in0, &s), B, now() - t);
        print_perm(in0, n);
    } else if (!strcmp(mode, "batch")) {
        /* batch N file [iters] [seed] [T0] : evaluate many sequences (one per line, "1,4,9,...") in parallel,
           one sequence per thread (incremental anneal + polish).  Prints "W seq" lines. argv[3] = file. */
        FILE *fi = fopen(argv[3], "r");
        if (!fi) { fprintf(stderr, "cannot open %s\n", argv[3]); return 1; }
        long long iters = argc > 4 ? atoll(argv[4]) : 1000000;
        uint64_t seed = argc > 5 ? strtoull(argv[5], 0, 10) : 1;
        double T0 = argc > 6 ? atof(argv[6]) : 3.0;
        static char line[4096];
        int cap = 1024, cnt = 0;
        seq_t *L = (seq_t *)malloc(sizeof(seq_t) * cap);
        while (fgets(line, sizeof line, fi)) {
            int gaps[MAXK]; int c = parse_gaps(line, gaps);
            if (c == 0) continue;
            if (cnt == cap) { cap *= 2; L = (seq_t *)realloc(L, sizeof(seq_t) * cap); }
            seq_make(&L[cnt++], n, gaps, c);
        }
        fclose(fi);
        long *res = (long *)malloc(sizeof(long) * cnt);
        /* BATCH_PT="R,Tmin,Tmax,mpe": parallel tempering with the same number of moves instead of annealing */
        int ptR = 0, ptM = 0; double ptT0 = 0, ptT1 = 0;
        if (getenv("BATCH_PT")) sscanf(getenv("BATCH_PT"), "%d,%lf,%lf,%d", &ptR, &ptT0, &ptT1, &ptM);
        double t = now();
        #pragma omp parallel
        {
            rng_t r; rng_seed(&r, seed * 7919ULL + omp_get_thread_num());
            fsa_t F; fsa_alloc_n(&F, n);
            #pragma omp for schedule(dynamic, 1)
            for (int i = 0; i < cnt; i++) {
                int x[MAXN];
                if (L[i].k == 1) { res[i] = (long)n * (n - 1) / 2; continue; }
                vctx_t V; vctx_init(&V, &L[i]); fsa_bind(&F, &V);
                long v = ptR > 0 ? fsa_pt(&L[i], &V, x, iters / ((long long)ptR * ptM), ptR, ptT0, ptT1, ptM, 0, 1LL << 60, NULL, &r)
                                 : fsa_anneal(&L[i], &V, &F, x, 0, iters, T0, 0.05, 1LL << 60, &r);
                long v2 = v_polish(&L[i], x, 10, 20, &r);
                res[i] = v2 > v ? v2 : v;
                vctx_free(&V);
            }
            fsa_free(&F);
        }
        for (int i = 0; i < cnt; i++) { char b[512]; seq_str(&L[i], b); printf("%ld %s\n", res[i], b); }
        fprintf(stderr, "batch: %d sequences in %.1fs\n", cnt, now() - t);
        free(res); free(L);
        return 0;
    } else if (!strcmp(mode, "fsatest")) {
        /* stress test: random sequences of size n, random swaps; incremental total vs full simulation */
        rng_t r; rng_seed(&r, 99);
        long long checks = 0, bad = 0;
        for (int trial = 0; trial < 200; trial++) {
            int gaps[MAXK], c = 1 + rng_below(&r, 7);
            for (int i = 0; i < c; i++) gaps[i] = 2 + rng_below(&r, n - 2);
            seq_t q; seq_make(&q, n, gaps, c);
            if (q.k == 1) continue;
            vctx_t V; vctx_init(&V, &q);
            fsa_t F; fsa_alloc(&F, &V);
            int x[MAXN]; random_perm(x, n, &r); v_normalize(&V, x);
            fsa_init(&F, x);
            for (int it = 0; it < 3000; it++) {
                if (rng_below(&r, 3) == 0) {          /* rotation move */
                    int len = 3 + rng_below(&r, 14); if (len > n) len = n;
                    int a = rng_below(&r, n - len + 1), b = a + len - 1, np[64];
                    if (rng_below(&r, 2)) { np[0] = F.pos[b]; for (int i = 1; i < len; i++) np[i] = F.pos[a + i - 1]; }
                    else { for (int i = 0; i < len - 1; i++) np[i] = F.pos[a + i + 1]; np[len - 1] = F.pos[a]; }
                    if (!fsa_valid_gen(&F, a, b, np)) continue;
                    long d = fsa_delta_gen(&F, a, b, np);
                    if (rng_below(&r, 3)) fsa_commit_gen(&F, a, b, np, d);
                    long full = v_cost(&V, F.val);
                    checks++;
                    if (full != F.total) { bad++; if (bad < 5) printf("ROT MISMATCH trial %d it %d: inc %ld full %ld\n", trial, it, F.total, full); }
                    continue;
                }
                int a = rng_below(&r, n), b = rng_below(&r, n);
                if (rng_below(&r, 2)) b = a + 1 + rng_below(&r, 3);
                if (b >= n || a == b) continue;
                if (a > b) { int t2 = a; a = b; b = t2; }
                if (!fsa_valid_swap(&F, F.pos[a], F.pos[b], a, b)) continue;
                long d = fsa_delta(&F, a, b);
                if (rng_below(&r, 3)) fsa_commit(&F, a, b, d);
                long full = v_cost(&V, F.val);
                checks++;
                if (full != F.total) { bad++; if (bad < 5) printf("MISMATCH trial %d it %d: inc %ld full %ld\n", trial, it, F.total, full); }
            }
            fsa_free(&F); vctx_free(&V);
        }
        printf("fsatest n=%d: %lld checks, %lld mismatches\n", n, checks, bad);
    } else if (!strcmp(mode, "ilsdbg")) {
        int w = argc > 4 ? atoi(argv[4]) : 12;
        rng_t r; rng_seed(&r, 5);
        vctx_t V; vctx_init(&V, &s);
        vscratch_t Wk; vscratch_init(&Wk, &V, w);
        wcprob_t P; wc_prob_seq(&P, &s);
        int cur[MAXN], pos[MAXN];
        wc_random(&P, cur, &r);
        for (int i = 0; i < n; i++) pos[cur[i]] = i;
        printf("start cost=%ld\n", v_cost(&V, cur));
        for (int rr = 0; rr < 10; rr++) {
            int tot = 0;
            for (int v0 = 0; v0 < n - 1; v0 += w / 2) {
                v_arrays(&V, cur, Wk.arrs);
                long before = v_cost(&V, cur);
                int gsum = v_window(&V, &Wk, cur, pos, v0, w, Wk.arrs, 1, &r);
                long after = v_cost(&V, cur);
                if (gsum != after - before) printf("  MISMATCH v0=%d gain=%d actual=%ld\n", v0, gsum, after - before);
                tot += gsum;
            }
            printf("round %d gain=%d cost=%ld\n", rr, tot, v_cost(&V, cur));
        }
    } else if (!strcmp(mode, "lay")) {
        /* layered certification run: lay N gaps [max two-level states] [kh] */
        long long cap = argc > 4 ? atoll(argv[4]) : 6000000000LL;
        int fkh = argc > 5 ? atoi(argv[5]) : -1;
        const char *dump = argc > 6 ? argv[6] : NULL;   /* lay N gaps cap kh DIR : also reconstruct an input */
        int w[MAXN]; w[0] = -2;
        double t = now();
        long long W = exact_worst_layered_ex(&s, fkh, cap, 1, dump, dump ? w : NULL);
        if (W < 0) printf("N=%d seq={%s} lay=FAILED\n", n, sb);
        else if (dump && w[0] >= 0) {
            printf("N=%d seq={%s} lay_worst=%lld check=%ld time=%.2fs perm=", n, sb, W, shell_cost(w, &s), now() - t);
            print_perm(w, n);
        } else printf("N=%d seq={%s} lay_worst=%lld time=%.2fs\n", n, sb, W, now() - t);
    } else if (!strcmp(mode, "dp2r") || !strcmp(mode, "layr")) {
        /* exact worst case with the leading pair passes handled as front pair rules (x2_front_rules), on the
           state space of the remaining sequence:  dp2r N gaps [maxstates]  |  layr N gaps [cap] [DIR]
           (DIR: dump the levels there and reconstruct an attaining input) */
        seq_t r; int kp = x2_front_rules(&s, &r);
        if (kp <= 0) { printf("N=%d seq={%s} rules=NOT_APPLICABLE\n", n, sb); return 0; }
        long long pairs = 0, st = 0, W;
        for (int i = 0; i < kp; i++) pairs += n - s.g[i];
        int w[MAXN]; w[0] = -2;
        double t = now();
        if (!strcmp(mode, "dp2r")) W = exact_worst2(&r, argc > 4 ? atoll(argv[4]) : 2000000000LL, w, &st);
        else {
            const char *dump = argc > 5 ? argv[5] : NULL;
            W = exact_worst_layered_ex(&r, 0, argc > 4 ? atoll(argv[4]) : 6000000000LL, 1, dump, dump ? w : NULL);
        }
        if (W < 0) { printf("N=%d seq={%s} rules=FAILED\n", n, sb); return 0; }
        W += pairs;
        if (w[0] >= 0) {
            x2_rules_witness(n, r.g[0], w);
            printf("N=%d seq={%s} rules_worst=%lld states=%lld check=%ld time=%.2fs perm=", n, sb, W, st, shell_cost(w, &s), now() - t);
            print_perm(w, n);
        } else printf("N=%d seq={%s} rules_worst=%lld time=%.2fs\n", n, sb, W, now() - t);
    } else if (!strcmp(mode, "dp2p") || !strcmp(mode, "size")) {
        /* parallel certification run: dp2p N gaps [maxstates] [kh] ; size: print state counts per kh */
        long long lim = argc > 4 ? atoll(argv[4]) : 6000000000LL;
        int fkh = argc > 5 ? atoi(argv[5]) : -1;
        if (!strcmp(mode, "size")) {
            for (int kh = 0; kh < s.k; kh++) {
                if (kh > 0 && 2 * s.g[kh - 1] < n) break;
                printf("N=%d seq={%s} kh=%d states=%lld\n", n, sb, kh, x2_size(&s, kh, 1LL << 62));
            }
            return 0;
        }
        int w[MAXN]; long long st; double t = now();
        long long W = exact_worst2_par(&s, lim, fkh, w, &st, 1);
        if (W < 0) { printf("N=%d seq={%s} dp2p=TOO_BIG states=%lld\n", n, sb, st); return 0; }
        long chk = shell_cost(w, &s);
        printf("N=%d seq={%s} dp2p_worst=%lld states=%lld check=%ld time=%.2fs perm=", n, sb, W, st, chk, now() - t);
        print_perm(w, n);
    } else if (!strcmp(mode, "sa") || !strcmp(mode, "cmp")) {
        long long iters = argc > 4 ? atoll(argv[4]) : 200000;
        int restarts = argc > 5 ? atoi(argv[5]) : 8;
        uint64_t seed = argc > 6 ? strtoull(argv[6], 0, 10) : 1;
        wcprob_t P; wc_prob_seq(&P, &s);
        double t = now();
        long best = -1; int bp[MAXN];
        #pragma omp parallel
        {
            rng_t r; rng_seed(&r, seed * 7919ULL + omp_get_thread_num());
            int x[MAXN];
            long v = wc_anneal(&P, x, 0, iters, restarts, 1.5, 0.05, 1LL << 60, &r);
            #pragma omp critical
            { if (v > best) { best = v; memcpy(bp, x, sizeof(int) * n); } }
        }
        double tsa = now() - t;
        int in0[MAXN];
        if (s.k > 1) unsort_first(bp, in0, n, s.g[0]); else memcpy(in0, bp, sizeof(int) * n);
        printf("N=%d seq={%s} sa_worst=%ld check=%ld time=%.2fs perm=", n, sb, best, shell_cost(in0, &s), tsa);
        print_perm(in0, n);
        if (!strcmp(mode, "cmp")) {
            long long st; t = now();
            long long W = exact_worst(&s, 2000000000LL, NULL, &st);
            printf("   dp_worst=%lld states=%lld time=%.2fs  %s\n", W, st, now() - t,
                   W < 0 ? "(dp too big)" : (W == best ? "MATCH" : "MISS"));
        }
    } else if (!strcmp(mode, "xavg")) {
        long long lim = argc > 4 ? atoll(argv[4]) : 300000000LL;
        double t = now();
        double a = exact_avg(&s, lim);
        if (a < 0) printf("N=%d seq={%s} xavg=TOO_BIG\n", n, sb);
        else printf("N=%d seq={%s} xavg=%.6f time=%.2fs\n", n, sb, a, now() - t);
    } else if (!strcmp(mode, "avg")) {
        long samples = argc > 4 ? atol(argv[4]) : 10000000;
        uint64_t seed = argc > 5 ? strtoull(argv[5], 0, 10) : 1;
        double se; double t = now();
        double a = avg_cost(&s, samples, seed, &se);
        printf("N=%d seq={%s} avg=%.4f +/- %.4f time=%.2fs\n", n, sb, a, se, now() - t);
    } else {
        fprintf(stderr, "unknown mode %s\n", mode);
        return 1;
    }
    return 0;
}
