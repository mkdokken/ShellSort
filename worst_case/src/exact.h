/*
 * exact.h -- exact worst case of Shellsort for a fixed gap sequence (n <= 64).
 *
 * Method: "threshold-set" dynamic programming.
 *   1. Decoupling of the first pass.  Let g1 be the largest gap.  Any g1-sorted array pi1 is
 *      produced by the input that reverses every g1-chain of pi1, and that input makes the first
 *      pass cost its absolute maximum A = sum_chains m(m-1)/2.  Hence
 *          W(G) = A + max over g1-sorted pi1 of cost(passes 2..k on pi1).
 *   2. A permutation pi1 is a maximal chain of threshold sets S_0 = {} < S_1 < ... < S_n,
 *      S_v = positions holding values < v.  Because sorting commutes with thresholding, the set
 *      of positions holding values < v before pass j is T_j(S_v), a deterministic function of S_v,
 *      and the comparisons spent on the element of value v in pass j depend only on
 *      (T_j(S_v), position of v).  So the total cost is a sum of edge weights along the chain,
 *      and the maximum is a longest path in the lattice of g1-downsets.
 *      A g1-downset is described by how many elements of each g1-chain it contains, so the
 *      state space has prod_r (m_r + 1) states (mixed-radix index).
 *   Cost of inserting the element at chain index t with a smaller elements before it in its chain:
 *          t - a + [a > 0]          (t shifts if it becomes the chain minimum, else t-a shifts + 1).
 */
#ifndef EXACT_H
#define EXACT_H
#include "core.h"

#ifndef popc
#define popc(x) __builtin_popcountll(x)
#endif

typedef struct {
    int n, k, G;               /* G = g[0] (decoupled pass) */
    int m[64];                 /* chain lengths of G */
    long long R[65];           /* mixed radix */
    long long M;               /* number of states */
    uint64_t fm0[64][65];      /* first t positions of G-chain r */
    /* passes j = 1..k-1 */
    int h[MAXK];
    uint64_t cm[MAXK][64];     /* chain masks */
    uint64_t fm[MAXK][64][65]; /* first-t masks per chain */
    uint8_t ch[MAXK][64], id[MAXK][64];
} dpctx_t;

/* returns number of states, or -1 if it would exceed limit */
static long long dp_setup(dpctx_t *c, const seq_t *s, long long limit) {
    int n = s->n;
    c->n = n; c->k = s->k; c->G = s->g[0];
    int G = c->G;
    c->R[0] = 1;
    for (int r = 0; r < G; r++) {
        c->m[r] = (n - r + G - 1) / G;
        if (c->R[r] > limit / (c->m[r] + 1) + 1) return -1;
        c->R[r + 1] = c->R[r] * (c->m[r] + 1);
    }
    c->M = c->R[G];
    if (c->M > limit) return -1;
    for (int r = 0; r < G; r++) {
        uint64_t x = 0; c->fm0[r][0] = 0;
        for (int t = 0; t < c->m[r]; t++) { x |= 1ULL << (r + t * G); c->fm0[r][t + 1] = x; }
    }
    for (int j = 1; j < s->k; j++) {
        int h = s->g[j]; c->h[j] = h;
        for (int q = 0; q < n; q++) { c->ch[j][q] = (uint8_t)(q % h); c->id[j][q] = (uint8_t)(q / h); }
        for (int cc = 0; cc < h; cc++) {
            uint64_t x = 0; c->fm[j][cc][0] = 0; int t = 0;
            for (int q = cc; q < n; q += h) { x |= 1ULL << q; c->fm[j][cc][++t] = x; }
            c->cm[j][cc] = x;
        }
    }
    return c->M;
}

static inline uint64_t dp_sortset(const dpctx_t *c, uint64_t U, int j) {
    uint64_t out = 0; int h = c->h[j];
    for (int cc = 0; cc < h; cc++) out |= c->fm[j][cc][popc(U & c->cm[j][cc])];
    return out;
}

/* U[j] for j=1..k-1 : threshold set at the input of pass j, given S = U[1] */
static inline void dp_sets(const dpctx_t *c, uint64_t S, uint64_t *U) {
    U[1] = S;
    for (int j = 1; j < c->k - 1; j++) U[j + 1] = dp_sortset(c, U[j], j);
}

/* comparisons spent in passes 1..k-1 on a new (largest-so-far) element at position p of pi1 */
static inline int dp_addcost(const dpctx_t *c, const uint64_t *U, int p) {
    int cost = 0, q = p, k = c->k;
    for (int j = 1; j < k; j++) {
        int cc = c->ch[j][q];
        uint64_t chainU = U[j] & c->cm[j][cc];
        int a = popc(chainU & ((1ULL << q) - 1));
        cost += c->id[j][q] - a + (a > 0);
        if (j < k - 1) q = cc + popc(chainU) * c->h[j];
    }
    return cost;
}

/*
 * Exact worst case.  Returns W, or -1 if the state space exceeds `limit`.
 * perm_out (optional): a worst-case input permutation (values 0..n-1).
 */
static long long exact_worst(const seq_t *s, long long limit, int *perm_out, long long *states_out) {
    int n = s->n;
    if (states_out) *states_out = 0;
    if (n > 64) return -1;
    if (s->k == 1) {
        if (perm_out) for (int i = 0; i < n; i++) perm_out[i] = n - 1 - i;
        if (states_out) *states_out = 1;
        return (long long)n * (n - 1) / 2;
    }
    dpctx_t *c = (dpctx_t *)malloc(sizeof(dpctx_t));
    long long M = dp_setup(c, s, limit);
    if (states_out) *states_out = M;
    if (M < 0) { free(c); return -1; }
    uint16_t *f = (uint16_t *)malloc(sizeof(uint16_t) * (size_t)M);
    if (!f) { free(c); return -1; }
    int G = c->G, k = c->k;
    int cnt[64];
    for (int r = 0; r < G; r++) cnt[r] = c->m[r];
    long long idx = M - 1;
    f[idx] = 0;
    uint64_t U[MAXK];
    while (idx > 0) {
        int r = 0;
        while (cnt[r] == 0) { cnt[r] = c->m[r]; r++; }
        cnt[r]--; idx--;
        uint64_t S = 0;
        for (int q = 0; q < G; q++) S |= c->fm0[q][cnt[q]];
        dp_sets(c, S, U);
        int best = 0;
        for (int q = 0; q < G; q++) if (cnt[q] < c->m[q]) {
            int val = dp_addcost(c, U, q + cnt[q] * G) + f[idx + c->R[q]];
            if (val > best) best = val;
        }
        f[idx] = (uint16_t)best;
    }
    long long W = pass_max(n, G) + f[0];
    if (perm_out) {
        int pi1[64], cc[64] = {0}, v = 0;
        long long id = 0;
        while (id != M - 1) {
            uint64_t S = 0;
            for (int q = 0; q < G; q++) S |= c->fm0[q][cc[q]];
            dp_sets(c, S, U);
            int pick = -1;
            for (int q = 0; q < G; q++) if (cc[q] < c->m[q]) {
                int val = dp_addcost(c, U, q + cc[q] * G) + f[id + c->R[q]];
                if (val == f[id]) { pick = q; break; }
            }
            int p = pick + cc[pick] * G;
            pi1[p] = v++; cc[pick]++; id += c->R[pick];
        }
        unsort_first(pi1, perm_out, n, G);
    }
    free(f); free(c);
    (void)k;
    return W;
}

/*
 * Exact AVERAGE number of comparisons over all n! inputs (n <= 64, small state spaces).
 * A uniform input makes pi1 = g1-sort(input) uniform over g1-sorted arrays (each has prod m_r!
 * preimages), i.e. uniform over maximal chains of the g1-downset lattice.  With Nd(S) = number of
 * completions from S, the next element is p with probability Nd(S+p)/Nd(S), so
 *     E(S) = sum_p Nd(S+p)/Nd(S) * (cost(S,p) + E(S+p)).
 * The first pass contributes sum over chains of sum_{i=1}^{m-1} (i/2 + i/(i+1)).
 * Returns -1 if the state space exceeds limit.
 */
static double exact_avg(const seq_t *s, long long limit) {
    int n = s->n;
    if (n > 64) return -1;
    double E1 = 0;
    int G = s->g[0];
    for (int r = 0; r < G && r < n; r++) {
        int m = (n - r + G - 1) / G;
        for (int i = 1; i < m; i++) E1 += i / 2.0 + (double)i / (i + 1);
    }
    if (s->k == 1) return E1;
    dpctx_t *c = (dpctx_t *)malloc(sizeof(dpctx_t));
    long long M = dp_setup(c, s, limit);
    if (M < 0) { free(c); return -1; }
    double *Nd = (double *)malloc(sizeof(double) * (size_t)M);
    double *Ed = (double *)malloc(sizeof(double) * (size_t)M);
    if (!Nd || !Ed) { free(Nd); free(Ed); free(c); return -1; }
    int cnt[64];
    for (int r = 0; r < G; r++) cnt[r] = c->m[r];
    long long idx = M - 1;
    Nd[idx] = 1; Ed[idx] = 0;
    uint64_t U[MAXK];
    while (idx > 0) {
        int r = 0;
        while (cnt[r] == 0) { cnt[r] = c->m[r]; r++; }
        cnt[r]--; idx--;
        uint64_t S = 0;
        for (int q = 0; q < G; q++) S |= c->fm0[q][cnt[q]];
        dp_sets(c, S, U);
        double nd = 0, acc = 0;
        for (int q = 0; q < G; q++) if (cnt[q] < c->m[q]) {
            long long j = idx + c->R[q];
            nd += Nd[j];
            acc += Nd[j] * (dp_addcost(c, U, q + cnt[q] * G) + Ed[j]);
        }
        Nd[idx] = nd; Ed[idx] = acc / nd;
    }
    double E = E1 + Ed[0];
    free(Nd); free(Ed); free(c);
    return E;
}

#endif
