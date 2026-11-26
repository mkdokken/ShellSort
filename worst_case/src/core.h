/*
 * core.h -- shared primitives for the Shellsort worst-case study.
 *
 * Comparison-count convention (standard gapped insertion sort):
 *     for each gap h (largest first), for i = h..n-1:
 *         t = a[i]; j = i;
 *         while (j >= h && a[j-h] > t) { a[j] = a[j-h]; j -= h; }   // each evaluation of a[j-h] > t is 1 comparison
 *         a[j] = t;
 * Gaps >= n are no-ops.  A sequence is written smallest-first, e.g. {1,4,9,11,21}.
 */
#ifndef CORE_H
#define CORE_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>

#define MAXN 1024
#define MAXK 40

typedef struct {
    int n;          /* array size */
    int k;          /* number of passes actually executed (gaps < n) */
    int g[MAXK];    /* gaps in pass order: g[0] > g[1] > ... > g[k-1] = 1 */
} seq_t;

/* ---------- RNG: xoshiro256** ---------- */
typedef struct { uint64_t s[4]; } rng_t;
static inline uint64_t rotl64(uint64_t x, int k) { return (x << k) | (x >> (64 - k)); }
static inline uint64_t rng_next(rng_t *r) {
    uint64_t *s = r->s;
    uint64_t res = rotl64(s[1] * 5, 7) * 9, t = s[1] << 17;
    s[2] ^= s[0]; s[3] ^= s[1]; s[1] ^= s[2]; s[0] ^= s[3]; s[2] ^= t; s[3] = rotl64(s[3], 45);
    return res;
}
static inline void rng_seed(rng_t *r, uint64_t seed) {
    for (int i = 0; i < 4; i++) {           /* splitmix64 */
        uint64_t z = (seed += 0x9E3779B97F4A7C15ULL);
        z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
        z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
        r->s[i] = z ^ (z >> 31);
    }
}
static inline uint32_t rng_below(rng_t *r, uint32_t n) { return (uint32_t)(((rng_next(r) >> 32) * (uint64_t)n) >> 32); }
static inline double rng_unit(rng_t *r) { return (rng_next(r) >> 11) * (1.0 / 9007199254740992.0); }

/* ---------- sequences ---------- */
static int cmp_int_desc(const void *a, const void *b) { return *(const int *)b - *(const int *)a; }

/* Build a seq_t from an arbitrary list of gaps (any order); drops gaps >= n and duplicates, adds 1. */
static inline void seq_make(seq_t *s, int n, const int *gaps, int cnt) {
    int tmp[MAXK + 1], m = 0;
    for (int i = 0; i < cnt; i++) if (gaps[i] >= 1 && gaps[i] < n) tmp[m++] = gaps[i];
    tmp[m++] = 1;
    qsort(tmp, m, sizeof(int), cmp_int_desc);
    s->n = n; s->k = 0;
    for (int i = 0; i < m; i++) if (s->k == 0 || s->g[s->k - 1] != tmp[i]) s->g[s->k++] = tmp[i];
}

/* Parse "1,4,9,11" (also accepts spaces / braces). */
static inline int parse_gaps(const char *str, int *out) {
    int cnt = 0; const char *p = str;
    while (*p) {
        while (*p && (*p < '0' || *p > '9')) p++;
        if (!*p) break;
        out[cnt++] = (int)strtol(p, (char **)&p, 10);
        if (cnt >= MAXK) break;
    }
    return cnt;
}

static inline void seq_str(const seq_t *s, char *buf) {   /* smallest-first */
    int o = 0; buf[0] = 0;
    for (int i = s->k - 1; i >= 0; i--) o += sprintf(buf + o, i == s->k - 1 ? "%d" : ",%d", s->g[i]);
}

/* ---------- comparison-counting Shellsort ---------- */
/* Runs passes g[from..k-1] on a (in place); returns number of comparisons. */
static inline long shell_passes(int *a, int n, const int *g, int from, int k) {
    long c = 0;
    for (int t = from; t < k; t++) {
        int h = g[t];
        for (int i = h; i < n; i++) {
            int v = a[i], j = i;
            while (j >= h) {
                c++;
                if (a[j - h] > v) { a[j] = a[j - h]; j -= h; }
                else break;
            }
            a[j] = v;
        }
    }
    return c;
}

static inline long shell_cost(const int *perm, const seq_t *s) {
    int a[MAXN];
    memcpy(a, perm, sizeof(int) * s->n);
    return shell_passes(a, s->n, s->g, 0, s->k);
}

/* Max cost of one h-pass over all inputs = sum over chains m(m-1)/2 (reverse each chain). */
static inline long pass_max(int n, int h) {
    long A = 0;
    for (int r = 0; r < h && r < n; r++) { long m = (n - r + h - 1) / h; A += m * (m - 1) / 2; }
    return A;
}

/* h-sort a in place (no counting). */
static inline void hsort(int *a, int n, int h) {
    for (int i = h; i < n; i++) {
        int v = a[i], j = i;
        while (j >= h && a[j - h] > v) { a[j] = a[j - h]; j -= h; }
        a[j] = v;
    }
}

/* Given a g[0]-sorted array pi1, produce the input pi0 that reverses every g[0]-chain
 * (so the first pass costs its maximum and outputs pi1). */
static inline void unsort_first(const int *pi1, int *pi0, int n, int h) {
    for (int r = 0; r < h && r < n; r++) {
        int m = (n - r + h - 1) / h;
        for (int t = 0; t < m; t++) pi0[r + t * h] = pi1[r + (m - 1 - t) * h];
    }
}

static inline void random_perm(int *a, int n, rng_t *r) {
    for (int i = 0; i < n; i++) a[i] = i;
    for (int i = n - 1; i > 0; i--) { int j = rng_below(r, i + 1); int t = a[i]; a[i] = a[j]; a[j] = t; }
}

#endif
