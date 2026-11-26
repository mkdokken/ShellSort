/*
 * vlns.h -- very-large-neighbourhood search for bad Shellsort inputs (lower bounds, any n <= 1024).
 *
 * Works on pi1 (the g1-sorted array; the first pass is decoupled and always costs its maximum).
 * Move = "value-window re-optimisation": choose w consecutive values v0..v0+w-1 and re-assign them to
 * the same set of positions in the best possible order.  By the threshold-set decomposition, only the
 * comparisons charged to these w values change, and they depend only on the order in which the window
 * positions are added on top of the fixed set of smaller values.  A DP over the 2^w subsets of window
 * positions therefore finds the optimal re-ordering exactly (subject to keeping pi1 g1-sorted).
 * Threshold sets of every pass are carried incrementally (T_j(S+q) = T_j(S) + {q_j}).
 * This move changes many inversions jointly instead of one isolated swap.
 */
#ifndef VLNS_H
#define VLNS_H
#include "core.h"

#define NWMAX 16
static inline long wc_eval_full_dummy(int n) { return (long)n * (n - 1) / 2; }

typedef struct {
    int n, k, NW, G;
    const int *g;                 /* = gv: gv[0] decoupled (not simulated), gv[1..k-1] simulated passes */
    int gv[MAXK];
    int ncon, con[MAXK];          /* the search space: arrays that are h-sorted for every h in con */
    int base_j[MAXK];             /* offsets into masks */
    uint64_t *chainmask;          /* [j][c][NW] */
    uint64_t *belowchain;         /* [j][q][NW] : chain(q) & [0,q) */
    int16_t *idq, *chq;           /* [j][q] */
    int cmoff[MAXK], bcoff[MAXK];
    long A;                       /* constant cost of the decoupled pass(es) */
} vctx_t;

/*
 * Search space and constant part:
 *  - first gap not a pair pass: arrays sorted for g[0], first pass decoupled at its maximum A = sum m(m-1)/2;
 *  - leading pair passes H (2h >= n): arrays sorted for every h in H (exactly their possible outputs),
 *    each costs exactly n-h, so A = sum (n-h) and simulation starts after H.  This removes the plateaus
 *    created by simulating a pair pass on inputs that differ only inside its pairs.
 */
static int VCTX_HDEC = 0;   /* 1: search over arrays sorted for all leading pair passes (env FSA_HDEC=1).
                               Exact either way; default off because on tested instances the stricter
                               space rejects many proposed moves and was not faster per unit time. */
static void vctx_init(vctx_t *V, const seq_t *s) {
    int n = s->n, k0 = s->k, NW = (n + 63) / 64;
    int kh = 0;
    if (VCTX_HDEC) while (kh < k0 - 1 && 2 * s->g[kh] >= n) kh++;
    V->n = n; V->NW = NW; V->G = s->g[0];
    if (kh >= 1) {
        V->A = 0; V->ncon = kh;
        for (int i = 0; i < kh; i++) { V->A += n - s->g[i]; V->con[i] = s->g[i]; }
        V->gv[0] = s->g[0];
        for (int i = kh; i < k0; i++) V->gv[1 + i - kh] = s->g[i];
        V->k = 1 + k0 - kh;
    } else {
        V->A = pass_max(n, s->g[0]); V->ncon = 1; V->con[0] = s->g[0];
        for (int i = 0; i < k0; i++) V->gv[i] = s->g[i];
        V->k = k0;
    }
    V->g = V->gv;
    int k = V->k;
    int totc = 0;
    for (int j = 1; j < k; j++) { V->cmoff[j] = totc; totc += V->g[j]; }
    V->chainmask = (uint64_t *)calloc((size_t)(totc + 1) * NW, sizeof(uint64_t));
    V->belowchain = (uint64_t *)calloc((size_t)k * n * NW, sizeof(uint64_t));
    V->idq = (int16_t *)malloc(sizeof(int16_t) * k * n);
    V->chq = (int16_t *)malloc(sizeof(int16_t) * k * n);
    for (int j = 1; j < k; j++) {
        int h = V->g[j];
        for (int q = 0; q < n; q++) {
            int c = q % h;
            V->idq[j * n + q] = (int16_t)(q / h); V->chq[j * n + q] = (int16_t)c;
            uint64_t *cm = V->chainmask + (size_t)(V->cmoff[j] + c) * NW;
            uint64_t *bc = V->belowchain + ((size_t)j * n + q) * NW;
            memcpy(bc, cm, sizeof(uint64_t) * NW);          /* positions of chain c before q */
            cm[q >> 6] |= 1ULL << (q & 63);
        }
    }
}
static void vctx_free(vctx_t *V) { free(V->chainmask); free(V->belowchain); free(V->idq); free(V->chq); }

/* map any array into the search space: apply the constraint passes (largest first) */
static inline void v_normalize(const vctx_t *V, int *a) {
    for (int i = 0; i < V->ncon; i++) hsort(a, V->n, V->con[i]);
}

static inline int pc_and(const uint64_t *a, const uint64_t *b, int NW) {
    int s = 0; for (int i = 0; i < NW; i++) s += __builtin_popcountll(a[i] & b[i]); return s;
}

/* cost of passes 1..k-1 on pi1 (simulation) */
static inline long v_cost(const vctx_t *V, const int *val) {
    int a[MAXN]; memcpy(a, val, sizeof(int) * V->n);
    return V->A + shell_passes(a, V->n, V->g, 1, V->k);
}

typedef struct {
    int w;
    int *f; int8_t *par; uint64_t *sets;      /* sets: [1<<w][k-1][NW] */
    int (*arrs)[MAXN];                          /* arrays before each pass */
} vscratch_t;

static void vscratch_init(vscratch_t *W, const vctx_t *V, int w) {
    W->w = w;
    W->f = (int *)malloc(sizeof(int) << w);
    W->par = (int8_t *)malloc((size_t)1 << w);
    W->sets = (uint64_t *)malloc(sizeof(uint64_t) * ((size_t)1 << w) * (V->k) * V->NW);
    W->arrs = (int (*)[MAXN])malloc(sizeof(int) * MAXN * MAXK);
}
static void vscratch_free(vscratch_t *W) { free(W->f); free(W->par); free(W->sets); free(W->arrs); }

/*
 * Re-optimise values v0..v0+w-1.  arrs: arrs[j][p] = array before pass j (j=1..k-1) for current val.
 * Returns the gain (>= 0) and applies the new order if gain > 0 (or gain == 0 && accept_equal).
 */
static int v_window(const vctx_t *V, vscratch_t *Wk, int *val, int *pos, int v0, int w,
                    int (*arrs)[MAXN], int accept_equal, rng_t *rng) {
    int n = V->n, k = V->k, NW = V->NW;
    if (v0 + w > n) w = n - v0;
    if (w < 2) return 0;
    int P[32];
    for (int i = 0; i < w; i++) P[i] = pos[v0 + i];
    size_t stride = (size_t)k * NW;
    int full = (1 << w) - 1;
    for (int X = 0; X <= full; X++) Wk->f[X] = -1;
    /* base sets: positions holding values < v0 before each pass */
    uint64_t *s0 = Wk->sets;
    memset(s0, 0, sizeof(uint64_t) * stride);
    for (int j = 1; j < k; j++) {
        uint64_t *T = s0 + (size_t)j * NW;
        const int *arr = arrs[j];
        for (int p = 0; p < n; p++) if (arr[p] < v0) T[p >> 6] |= 1ULL << (p & 63);
    }
    Wk->f[0] = 0;
    /* original cost of the window along the identity order (computed during the sweep) */
    int orig = -1, origX = 0, origacc = 0;
    for (int X = 0; X <= full; X++) {
        if (Wk->f[X] < 0) continue;
        uint64_t *SX = Wk->sets + (size_t)X * stride;
        const uint64_t *S1 = SX + NW;    /* pass-1 input = pi1 coordinates */
        for (int i = 0; i < w; i++) {
            if (X >> i & 1) continue;
            int q = P[i], okc = 1;
            for (int ci = 0; ci < V->ncon && okc; ci++) {
                int G2 = V->con[ci];
                if (q >= G2 && !(S1[(q - G2) >> 6] >> ((q - G2) & 63) & 1)) okc = 0;
                else if (q + G2 < n && (S1[(q + G2) >> 6] >> ((q + G2) & 63) & 1)) okc = 0;
            }
            if (!okc) continue;
            int cost = 0, qq = q, qp[MAXK];
            for (int j = 1; j < k; j++) {
                const uint64_t *T = SX + (size_t)j * NW;
                int a = pc_and(T, V->belowchain + ((size_t)j * n + qq) * NW, NW);
                cost += V->idq[j * n + qq] - a + (a > 0);
                qp[j] = qq;
                if (j < k - 1) {
                    int c = V->chq[j * n + qq];
                    int nc = pc_and(T, V->chainmask + (size_t)(V->cmoff[j] + c) * NW, NW);
                    qq = c + nc * V->g[j];
                }
            }
            int Y = X | (1 << i);
            if (X == origX && i == __builtin_popcount(X)) { origacc += cost; origX = Y; if (Y == full) orig = origacc; }
            int v = Wk->f[X] + cost;
            if (Wk->f[Y] < 0) {
                uint64_t *SY = Wk->sets + (size_t)Y * stride;
                memcpy(SY, SX, sizeof(uint64_t) * stride);
                for (int j = 1; j < k; j++) SY[(size_t)j * NW + (qp[j] >> 6)] |= 1ULL << (qp[j] & 63);
                Wk->f[Y] = v; Wk->par[Y] = (int8_t)i;
            } else if (v > Wk->f[Y] || (v == Wk->f[Y] && rng && (rng_next(rng) & 1))) {
                Wk->f[Y] = v; Wk->par[Y] = (int8_t)i;
            }
        }
    }
    if (orig < 0) return 0;   /* should not happen */
    int gain = Wk->f[full] - orig;
    if (gain > 0 || (gain == 0 && accept_equal)) {
        int X = full, v = v0 + w - 1;
        while (X) { int i = Wk->par[X]; val[P[i]] = v; pos[v] = P[i]; v--; X &= ~(1 << i); }
    }
    return gain;
}

/* arrays before each pass for the current pi1 */
static void v_arrays(const vctx_t *V, const int *val, int (*arrs)[MAXN]) {
    int n = V->n, a[MAXN];
    memcpy(a, val, sizeof(int) * n);
    for (int j = 1; j < V->k; j++) {
        memcpy(arrs[j], a, sizeof(int) * n);
        hsort(a, n, V->g[j]);
    }
}

/* one sweep of windows; returns total gain */
static int v_sweep(const vctx_t *V, vscratch_t *Wk, int *val, int *pos, int w, int step, int offset,
                   int accept_equal, rng_t *rng) {
    int n = V->n, total = 0;
    for (int v0 = offset; v0 < n - 1; v0 += step) {
        v_arrays(V, val, Wk->arrs);
        total += v_window(V, Wk, val, pos, v0, w, Wk->arrs, accept_equal, rng);
    }
    return total;
}

/* Window-sweep descent from val (in/out) until no strict gain in a full sweep; returns final value. */
static long v_polish(const seq_t *s, int *val, int w, int max_sweeps, rng_t *rng) {
    int n = s->n;
    if (s->k == 1) return wc_eval_full_dummy(n);
    vctx_t V; vctx_init(&V, s);
    vscratch_t Wk; vscratch_init(&Wk, &V, w);
    int pos[MAXN];
    v_normalize(&V, val);                       /* into the search space (value-preserving) */
    for (int i = 0; i < n; i++) pos[val[i]] = i;
    for (int sw = 0; sw < max_sweeps; sw++) {
        int gain = v_sweep(&V, &Wk, val, pos, w, w / 2 > 0 ? w / 2 : 1, (int)rng_below(rng, w / 2 + 1), 1, rng);
        if (gain <= 0 && sw > 2) break;
    }
    long c = v_cost(&V, val);
    vscratch_free(&Wk); vctx_free(&V);
    return c;
}

/*
 * Iterated local search: window sweeps (exact large-neighbourhood moves) until no strict gain, then a
 * short warm annealing kick (anneal.h) to leave the basin; keeps the best.  val: in (start if given) / out.
 */
#include "anneal.h"
static long v_ils(const seq_t *s, int *val, int start_given, long long rounds, int w, double kick_T,
                  long long kick_iters, long long stop_above, rng_t *rng) {
    int n = s->n;
    if (s->k == 1) { for (int i = 0; i < n; i++) val[i] = n - 1 - i; return (long)n * (n - 1) / 2; }
    vctx_t V; vctx_init(&V, s);
    vscratch_t Wk; vscratch_init(&Wk, &V, w);
    wcprob_t P; wc_prob_seq(&P, s);
    int cur[MAXN], pos[MAXN], best[MAXN];
    if (start_given) memcpy(cur, val, sizeof(int) * n); else wc_random(&P, cur, rng);
    v_normalize(&V, cur);
    for (int i = 0; i < n; i++) pos[cur[i]] = i;
    long cv = v_cost(&V, cur), bv = cv;
    memcpy(best, cur, sizeof(int) * n);
    for (long long r = 0; r < rounds && bv <= stop_above; r++) {
        int gain = v_sweep(&V, &Wk, cur, pos, w, w / 2 > 0 ? w / 2 : 1, (int)rng_below(rng, w / 2 + 1), 1, rng);
        cv = v_cost(&V, cur);
        if (cv > bv) { bv = cv; memcpy(best, cur, sizeof(int) * n); }
        if (gain <= 0) {
            /* kick: short annealing at a warm temperature from the best solution */
            memcpy(cur, best, sizeof(int) * n);
            wc_kick(&P, cur, kick_iters, kick_T, rng);
            v_normalize(&V, cur);
            for (int i = 0; i < n; i++) pos[cur[i]] = i;
            cv = v_cost(&V, cur);
            if (cv > bv) { bv = cv; memcpy(best, cur, sizeof(int) * n); }
        }
    }
    memcpy(val, best, sizeof(int) * n);
    vscratch_free(&Wk); vctx_free(&V);
    return bv;
}

#endif
