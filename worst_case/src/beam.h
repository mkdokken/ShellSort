/*
 * beam.h -- beam search over the threshold-set DP (lower bounds for large N, n <= 128).
 *
 * Values are placed in increasing order into pi1 (the g1-sorted array; the first pass is decoupled at its
 * maximum).  A state is the set S of pi1-positions holding the values placed so far -- a down-set of the
 * g1-chains -- carried with the threshold sets of every later pass, which depend on S only; the cost
 * charged to the next value depends only on S and its position (fsa_step).  The exact DP keeps every state
 * (infeasible beyond N ~ 46); the beam keeps, per level, the B states with the largest accumulated cost
 * (equal costs broken at random), so its result is the cost of a real input: a lower bound on the worst case.
 */
#ifndef BEAM_H
#define BEAM_H
#include "fastsa.h"
#include <omp.h>

typedef struct { uint64_t k0, k1; int cost, par; int p; } bchild_t;

static inline int beam_avail(const uint64_t *S, int p, int g0) {
    if ((S[p >> 6] >> (p & 63)) & 1) return 0;
    return p < g0 || ((S[(p - g0) >> 6] >> ((p - g0) & 63)) & 1);
}

/* returns the best total found (incl. the decoupled pass) and the pi1 arrangement in val; -1 if n > 128 */
static long beam_worst(const seq_t *s, long B, uint64_t seed, int *val, int verbose) {
    int n = s->n;
    if (s->k < 2 || n > 128) return -1;
    vctx_t V; vctx_init(&V, s);
    fsa_t F; fsa_bind(&F, &V);
    int k = V.k, NW = V.NW, KW = k * NW, g0 = V.con[0];
    if (V.ncon != 1) { vctx_free(&V); return -1; }
    uint64_t *T = (uint64_t *)calloc((size_t)B * KW, sizeof(uint64_t));
    uint64_t *T2 = (uint64_t *)calloc((size_t)B * KW, sizeof(uint64_t));
    int *cost = (int *)calloc(B, sizeof(int)), *cost2 = (int *)calloc(B, sizeof(int));
    int32_t **par = (int32_t **)calloc(n + 1, sizeof(int32_t *));
    uint8_t **pp = (uint8_t **)calloc(n + 1, sizeof(uint8_t *));
    int nth = omp_get_max_threads();
    enum { HMAX = 1 << 16 };
    long *hist = (long *)calloc((size_t)nth * HMAX, sizeof(long));
    long cnt = 1;
    double dupr = 2.0;
    rng_t R; rng_seed(&R, seed);
    double t0 = omp_get_wtime();
    for (int L = 0; L < n; L++) {
        /* pass A: histogram of child costs */
        memset(hist, 0, sizeof(long) * nth * HMAX);
        #pragma omp parallel for schedule(dynamic, 256)
        for (long i = 0; i < cnt; i++) {
            long *h = hist + (size_t)omp_get_thread_num() * HMAX;
            const uint64_t *Ti = T + (size_t)i * KW, *S = Ti + NW;
            int16_t qv[MAXK];
            for (int p = 0; p < n; p++) if (beam_avail(S, p, g0)) {
                int c = cost[i] + fsa_step(&F, Ti, NULL, p, qv);
                h[c < HMAX ? c : HMAX - 1]++;
            }
        }
        long tot = 0;
        for (int t = 1; t < nth; t++) for (int c = 0; c < HMAX; c++) hist[c] += hist[(size_t)t * HMAX + c];
        for (int c = 0; c < HMAX; c++) tot += hist[c];
        /* keep children with cost > theta, and those at theta with probability q (about `want` kept, enough
           for B distinct states at the duplicate rate of the previous level) */
        long want = (long)(B * (dupr * 1.25 > 2.0 ? dupr * 1.25 : 2.0)), above = 0; int theta = 0; double q = 1.0;
        if (tot > want) {
            for (theta = HMAX - 1; theta > 0 && above + hist[theta] < want; theta--) above += hist[theta];
            q = (double)(want - above) / (double)hist[theta];
        }
        /* pass B: collect */
        bchild_t **buf = (bchild_t **)calloc(nth, sizeof(bchild_t *));
        long *bn = (long *)calloc(nth, sizeof(long)), *bcap = (long *)calloc(nth, sizeof(long));
        #pragma omp parallel
        {
            int t = omp_get_thread_num();
            rng_t r; rng_seed(&r, seed * 1000003ULL + (uint64_t)L * 7919ULL + t);
            int16_t qv[MAXK];
            #pragma omp for schedule(dynamic, 256)
            for (long i = 0; i < cnt; i++) {
                const uint64_t *Ti = T + (size_t)i * KW, *S = Ti + NW;
                for (int p = 0; p < n; p++) if (beam_avail(S, p, g0)) {
                    int c = cost[i] + fsa_step(&F, Ti, NULL, p, qv);
                    if (c < theta || (c == theta && q < 1.0 && rng_unit(&r) >= q)) continue;
                    if (bn[t] == bcap[t]) { bcap[t] = bcap[t] ? 2 * bcap[t] : 4096; buf[t] = (bchild_t *)realloc(buf[t], sizeof(bchild_t) * bcap[t]); }
                    bchild_t *b = &buf[t][bn[t]++];
                    b->k0 = S[0] | (p < 64 ? 1ULL << p : 0);
                    b->k1 = (NW > 1 ? S[1] : 0) | (p >= 64 ? 1ULL << (p - 64) : 0);
                    b->cost = c; b->par = (int)i; b->p = p;
                }
            }
        }
        long m = 0;
        for (int t = 0; t < nth; t++) m += bn[t];
        bchild_t *ch = (bchild_t *)malloc(sizeof(bchild_t) * (m ? m : 1));
        m = 0;
        for (int t = 0; t < nth; t++) { memcpy(ch + m, buf[t], sizeof(bchild_t) * bn[t]); m += bn[t]; free(buf[t]); }
        free(buf); free(bn); free(bcap);
        /* shuffle (random tie-breaking), then dedup by key keeping the largest cost */
        for (long i = m - 1; i > 0; i--) { long j = (long)(rng_next(&R) % (uint64_t)(i + 1)); bchild_t x = ch[i]; ch[i] = ch[j]; ch[j] = x; }
        long hs = 1; while (hs < 2 * m + 2) hs <<= 1;
        int32_t *ht = (int32_t *)malloc(sizeof(int32_t) * hs);
        memset(ht, 0xff, sizeof(int32_t) * hs);
        long u = 0;
        for (long i = 0; i < m; i++) {
            uint64_t hk = (ch[i].k0 * 0x9E3779B97F4A7C15ULL) ^ (ch[i].k1 * 0xC2B2AE3D27D4EB4FULL);
            long slot = (long)((hk ^ (hk >> 29)) & (uint64_t)(hs - 1));
            for (;;) {
                if (ht[slot] < 0) { ch[u] = ch[i]; ht[slot] = (int32_t)u++; break; }
                bchild_t *o = &ch[ht[slot]];
                if (o->k0 == ch[i].k0 && o->k1 == ch[i].k1) { if (ch[i].cost > o->cost) *o = ch[i]; break; }
                slot = (slot + 1) & (hs - 1);
            }
        }
        free(ht);
        if (u > 0) dupr = (double)m / (double)u;
        if (dupr > 16.0) dupr = 16.0;
        /* top B by cost (ties: shuffled order) */
        long keep = u < B ? u : B;
        if (u > B) {
            static long h2[HMAX];
            memset(h2, 0, sizeof(h2));
            for (long i = 0; i < u; i++) h2[ch[i].cost]++;
            long ab = 0; int c = HMAX - 1;
            while (ab + h2[c] < B) ab += h2[c--];
            long need = B - ab, w = 0;
            for (long i = 0; i < u; i++)
                if (ch[i].cost > c || (ch[i].cost == c && need-- > 0)) ch[w++] = ch[i];
        }
        par[L + 1] = (int32_t *)malloc(sizeof(int32_t) * keep);
        pp[L + 1] = (uint8_t *)malloc(keep);
        int best = -1;
        #pragma omp parallel for schedule(static) reduction(max:best)
        for (long i = 0; i < keep; i++) {
            int16_t qv[MAXK];
            const uint64_t *Tp = T + (size_t)ch[i].par * KW;
            fsa_step(&F, Tp, T2 + (size_t)i * KW, ch[i].p, qv);
            cost2[i] = ch[i].cost;
            par[L + 1][i] = ch[i].par; pp[L + 1][i] = (uint8_t)ch[i].p;
            if (ch[i].cost > best) best = ch[i].cost;
        }
        free(ch);
        { uint64_t *x = T; T = T2; T2 = x; int *y = cost; cost = cost2; cost2 = y; }
        cnt = keep;
        if (verbose && (L % 8 == 7 || L == n - 1))
            fprintf(stderr, "  level %d: %ld children (theta %d), %ld unique, kept %ld, best %ld  (%.1fs)\n",
                    L + 1, m, theta, u, keep, V.A + best, omp_get_wtime() - t0);
    }
    /* the last level holds the single full state */
    long W = V.A + cost[0];
    long idx = 0;
    for (int L = n; L >= 1; L--) { val[pp[L][idx]] = L - 1; idx = par[L][idx]; }
    for (int L = 1; L <= n; L++) { free(par[L]); free(pp[L]); }
    free(par); free(pp); free(T); free(T2); free(cost); free(cost2); free(hist);
    vctx_free(&V);
    return W;
}

#endif
