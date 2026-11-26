/*
 * fastsa.h -- incremental annealing for worst-case inputs (first pass decoupled, any n <= 1024).
 *
 * Keeps, for every value v and every pass j >= 1, the threshold set T[v][j] = positions holding
 * values < v before pass j, the position q[v][j] of v before pass j, and the comparisons cost[v]
 * charged to v.  Swapping the values x < y of pi1 only changes S_u for x < u <= y, so a move is
 * re-evaluated in O((y-x+1) * passes * words) instead of a full simulation: a value-adjacent swap
 * costs a few dozen popcounts.  Same Metropolis scheme as anneal.h.
 */
#ifndef FASTSA_H
#define FASTSA_H
#include "vlns.h"

typedef struct {
    const vctx_t *V;
    int n, k, NW, G, KW;              /* KW = k*NW words per value */
    int val[MAXN], pos[MAXN];
    uint64_t *T;                       /* (n+1) * KW */
    int16_t *q;                        /* n * k */
    int *cost;                         /* n */
    long total;
    uint64_t *Ts; int16_t *qs; int *cs;  /* scratch */
} fsa_t;

/* buffers sized for any sequence of length <= MAXK on n elements; bind to a context with fsa_bind */
static void fsa_alloc_n(fsa_t *F, int n) {
    int NW = (n + 63) / 64;
    F->n = n; F->NW = NW;
    F->T = (uint64_t *)calloc((size_t)(n + 2) * MAXK * NW, sizeof(uint64_t));
    F->Ts = (uint64_t *)calloc((size_t)(n + 2) * MAXK * NW, sizeof(uint64_t));
    F->q = (int16_t *)malloc(sizeof(int16_t) * (n + 1) * MAXK);
    F->qs = (int16_t *)malloc(sizeof(int16_t) * (n + 1) * MAXK);
    F->cost = (int *)malloc(sizeof(int) * (n + 1));
    F->cs = (int *)malloc(sizeof(int) * (n + 1));
}
static void fsa_bind(fsa_t *F, const vctx_t *V) {
    F->V = V; F->n = V->n; F->k = V->k; F->NW = V->NW; F->G = V->G; F->KW = V->k * V->NW;
}
static void fsa_alloc(fsa_t *F, const vctx_t *V) { fsa_alloc_n(F, V->n); fsa_bind(F, V); }
static void fsa_free(fsa_t *F) { free(F->T); free(F->Ts); free(F->q); free(F->qs); free(F->cost); free(F->cs); }

/* cost of placing value with pi1-position p on top of sets Tin; writes next sets to Tout and q's */
static inline int fsa_step(const fsa_t *F, const uint64_t *Tin, uint64_t *Tout, int p, int16_t *qv) {
    const vctx_t *V = F->V;
    int n = F->n, k = F->k, NW = F->NW, cost = 0, qq = p;
    for (int j = 1; j < k; j++) {
        const uint64_t *Tj = Tin + (size_t)j * NW;
        int a = pc_and(Tj, V->belowchain + ((size_t)j * n + qq) * NW, NW);
        cost += V->idq[j * n + qq] - a + (a > 0);
        qv[j] = (int16_t)qq;
        if (j < k - 1) {
            int c = V->chq[j * n + qq];
            int nc = pc_and(Tj, V->chainmask + (size_t)(V->cmoff[j] + c) * NW, NW);
            qq = c + nc * V->g[j];
        }
    }
    if (Tout) {
        memcpy(Tout, Tin, sizeof(uint64_t) * F->KW);
        for (int j = 1; j < k; j++) Tout[(size_t)j * NW + (qv[j] >> 6)] |= 1ULL << (qv[j] & 63);
    }
    return cost;
}

static void fsa_init(fsa_t *F, const int *val) {
    int n = F->n;
    memcpy(F->val, val, sizeof(int) * n);
    for (int p = 0; p < n; p++) F->pos[val[p]] = p;
    memset(F->T, 0, sizeof(uint64_t) * F->KW);
    long tot = F->V->A;
    for (int v = 0; v < n; v++) {
        F->cost[v] = fsa_step(F, F->T + (size_t)v * F->KW, F->T + (size_t)(v + 1) * F->KW, F->pos[v], F->q + (size_t)v * F->k);
        tot += F->cost[v];
    }
    F->total = tot;
}

static inline int fsa_valid_swap(const fsa_t *F, int px, int py, int x, int y) {
    /* after the swap val[px]=y, val[py]=x ; check chain neighbours for every constraint gap */
    int n = F->n;
    #define VAL(p) ((p) == px ? y : ((p) == py ? x : F->val[p]))
    for (int ci = 0; ci < F->V->ncon; ci++) {
        int G = F->V->con[ci];
        if (px - G >= 0 && VAL(px - G) > y) return 0;
        if (px + G < n && VAL(px + G) < y) return 0;
        if (py - G >= 0 && VAL(py - G) > x) return 0;
        if (py + G < n && VAL(py + G) < x) return 0;
    }
    #undef VAL
    return 1;
}

/* delta of swapping values x<y (scratch filled; call fsa_commit to apply) */
static long fsa_delta(fsa_t *F, int x, int y) {
    int px = F->pos[x], py = F->pos[y], KW = F->KW, k = F->k;
    const uint64_t *cur = F->T + (size_t)x * KW;
    long d = 0;
    for (int u = x; u <= y; u++) {
        int p = (u == x) ? py : (u == y ? px : F->pos[u]);
        uint64_t *nxt = (u < y) ? F->Ts + (size_t)(u + 1) * KW : NULL;
        int c = fsa_step(F, cur, nxt, p, F->qs + (size_t)u * k);
        F->cs[u] = c; d += c - F->cost[u];
        cur = nxt;
    }
    return d;
}

static void fsa_commit(fsa_t *F, int x, int y, long d) {
    int KW = F->KW, k = F->k;
    int px = F->pos[x], py = F->pos[y];
    if (y > x + 1) memcpy(F->T + (size_t)(x + 1) * KW, F->Ts + (size_t)(x + 1) * KW, sizeof(uint64_t) * KW * (y - x));
    else memcpy(F->T + (size_t)(x + 1) * KW, F->Ts + (size_t)(x + 1) * KW, sizeof(uint64_t) * KW);
    memcpy(F->q + (size_t)x * k, F->qs + (size_t)x * k, sizeof(int16_t) * k * (y - x + 1));
    memcpy(F->cost + x, F->cs + x, sizeof(int) * (y - x + 1));
    F->val[px] = y; F->val[py] = x; F->pos[x] = py; F->pos[y] = px;
    F->total += d;
}

/*
 * Generic move: values x..y are re-assigned to the positions newpos[0..y-x] (a permutation of their
 * current positions).  S_{y+1} is unchanged, so only the costs of values x..y are recomputed.
 * Rotations of a run of consecutive values are "insertion" moves in the fill order.
 */
static int fsa_valid_gen(fsa_t *F, int x, int y, const int *newpos) {
    int n = F->n, ok = 1;
    int saved[64];
    int len = y - x + 1;
    for (int i = 0; i < len; i++) { saved[i] = F->val[newpos[i]]; }
    for (int i = 0; i < len; i++) F->val[newpos[i]] = x + i;
    for (int i = 0; i < len && ok; i++) {
        int p = newpos[i], v = x + i;
        for (int ci = 0; ci < F->V->ncon && ok; ci++) {
            int G = F->V->con[ci];
            if (p - G >= 0 && F->val[p - G] > v) ok = 0;
            else if (p + G < n && F->val[p + G] < v) ok = 0;
        }
    }
    for (int i = 0; i < len; i++) F->val[newpos[i]] = saved[i];
    return ok;
}

static long fsa_delta_gen(fsa_t *F, int x, int y, const int *newpos) {
    int KW = F->KW, k = F->k;
    const uint64_t *cur = F->T + (size_t)x * KW;
    long d = 0;
    for (int u = x; u <= y; u++) {
        uint64_t *nxt = (u < y) ? F->Ts + (size_t)(u + 1) * KW : NULL;
        int c = fsa_step(F, cur, nxt, newpos[u - x], F->qs + (size_t)u * k);
        F->cs[u] = c; d += c - F->cost[u];
        cur = nxt;
    }
    return d;
}

static void fsa_commit_gen(fsa_t *F, int x, int y, const int *newpos, long d) {
    int KW = F->KW, k = F->k;
    if (y > x) memcpy(F->T + (size_t)(x + 1) * KW, F->Ts + (size_t)(x + 1) * KW, sizeof(uint64_t) * KW * (y - x));
    memcpy(F->q + (size_t)x * k, F->qs + (size_t)x * k, sizeof(int16_t) * k * (y - x + 1));
    memcpy(F->cost + x, F->cs + x, sizeof(int) * (y - x + 1));
    for (int u = x; u <= y; u++) { F->pos[u] = newpos[u - x]; F->val[newpos[u - x]] = u; }
    F->total += d;
}

/* one Metropolis move at temperature T with the swap mix of fsa_anneal; returns 1 if a move was applied */
static inline int fsa_move(fsa_t *F, double T, rng_t *r) {
    int n = F->n, a, b;
    uint32_t mv = rng_below(r, 16);
    if (mv < 7) { a = rng_below(r, n - 1); b = a + 1; }
    else if (mv < 12) { int d = 2 + rng_below(r, 8); if (d >= n) d = n - 1; a = rng_below(r, n - d); b = a + d; }
    else if (mv < 15) { int p = rng_below(r, n), dd = 1 + rng_below(r, 8), qp = p + dd; if (qp >= n) qp = p - dd; if (qp < 0) return 0;
                        a = F->val[p]; b = F->val[qp]; }
    else { a = rng_below(r, n); b = rng_below(r, n); }
    if (a == b) return 0;
    if (a > b) { int t = a; a = b; b = t; }
    if (!fsa_valid_swap(F, F->pos[a], F->pos[b], a, b)) return 0;
    long d = fsa_delta(F, a, b);
    if (d >= 0 || rng_unit(r) < exp((double)d / T)) { fsa_commit(F, a, b, d); return 1; }
    return 0;
}

/*
 * Parallel tempering (replica exchange): R replicas at temperatures geometric in [Tmin, Tmax], `mpe` moves
 * per replica between exchange attempts of neighbouring temperatures (even/odd pairs alternately).
 * Runs `sweeps` exchange rounds or until a value > stop_above is seen; x = best arrangement found.  Every
 * 64 rounds a copy of an improved best is window-DP polished (width pw, if pw > 0).
 */
static long fsa_pt(const seq_t *s, const vctx_t *V, int *x, long long sweeps, int R, double Tmin, double Tmax,
                   int mpe, int pw, long long stop_above, volatile int *stop_flag, rng_t *r) {
    int n = s->n;
    if (s->k == 1) { for (int i = 0; i < n; i++) x[i] = n - 1 - i; return (long)n * (n - 1) / 2; }
    fsa_t *F = (fsa_t *)malloc(sizeof(fsa_t) * R);
    int at[64]; double Tl[64];
    long best = -1, polished = -1;
    for (int i = 0; i < R; i++) {
        int cur[MAXN];
        fsa_alloc(&F[i], V);
        random_perm(cur, n, r); v_normalize(V, cur); fsa_init(&F[i], cur);
        at[i] = i;
        Tl[i] = R > 1 ? Tmin * pow(Tmax / Tmin, (double)i / (R - 1)) : Tmin;
        if (F[i].total > best) { best = F[i].total; memcpy(x, F[i].val, sizeof(int) * n); }
    }
    for (long long sw = 0; sw < sweeps && best <= stop_above && !(stop_flag && *stop_flag); sw++) {
        for (int t = 0; t < R; t++) {
            fsa_t *G = &F[at[t]];
            for (int m = 0; m < mpe; m++)
                if (fsa_move(G, Tl[t], r) && G->total > best) {
                    best = G->total; memcpy(x, G->val, sizeof(int) * n);
                    if (best > stop_above) break;
                }
        }
        for (int t = (int)(sw & 1); t + 1 < R; t += 2) {
            long Ei = F[at[t]].total, Ej = F[at[t + 1]].total;
            double e = (double)(Ej - Ei) * (1.0 / Tl[t] - 1.0 / Tl[t + 1]);
            if (e >= 0 || rng_unit(r) < exp(e)) { int u = at[t]; at[t] = at[t + 1]; at[t + 1] = u; }
        }
        if (pw > 0 && (sw & 63) == 63 && best > polished && best <= stop_above) {
            int y[MAXN]; memcpy(y, x, sizeof(int) * n);
            long v = v_polish(s, y, pw, 50, r);
            polished = best;
            if (v > best) { best = polished = v; memcpy(x, y, sizeof(int) * n); }
        }
    }
    for (int i = 0; i < R; i++) fsa_free(&F[i]);
    free(F);
    return best;
}

/*
 * Annealing with incremental evaluation.  x: start (if start_given) / best out.  Stops early once a
 * value > stop_above is found.  Move mix: mostly small value distances (cheap), some position swaps,
 * and (if FSA_ROT > 0) rotations of runs of consecutive values (insertion moves in the fill order).
 */
static int FSA_ROT = 0;      /* percentage of rotation moves */
static long fsa_anneal(const seq_t *s, const vctx_t *V, fsa_t *F, int *x, int start_given, long long iters,
                       double T0, double T1, long long stop_above, rng_t *r) {
    int n = s->n;
    if (s->k == 1) { for (int i = 0; i < n; i++) x[i] = n - 1 - i; return (long)n * (n - 1) / 2; }
    int cur[MAXN];
    if (start_given) memcpy(cur, x, sizeof(int) * n);
    else random_perm(cur, n, r);
    v_normalize(V, cur);              /* into the search space (applies the constraint passes) */
    fsa_init(F, cur);
    long best = F->total;
    memcpy(x, F->val, sizeof(int) * n);
    if (best > stop_above) return best;
    double lt0 = log(T0), lt1 = log(T1);
    int np[64];
    for (long long it = 0; it < iters; it++) {
        double T = exp(lt0 + (lt1 - lt0) * (double)it / (double)iters);
        int a, b;
        if (FSA_ROT > 0 && (int)rng_below(r, 100) < FSA_ROT) {
            /* rotate the fill order of values a..b by one step (either direction) */
            int len = 3 + rng_below(r, 14); if (len > n) len = n;
            a = rng_below(r, n - len + 1); b = a + len - 1;
            if (rng_below(r, 2)) { np[0] = F->pos[b]; for (int i = 1; i < len; i++) np[i] = F->pos[a + i - 1]; }
            else { for (int i = 0; i < len - 1; i++) np[i] = F->pos[a + i + 1]; np[len - 1] = F->pos[a]; }
            if (!fsa_valid_gen(F, a, b, np)) continue;
            long d = fsa_delta_gen(F, a, b, np);
            if (d >= 0 || rng_unit(r) < exp((double)d / T)) {
                fsa_commit_gen(F, a, b, np, d);
                if (F->total > best) {
                    best = F->total; memcpy(x, F->val, sizeof(int) * n);
                    if (best > stop_above) return best;
                }
            }
            continue;
        }
        uint32_t mv = rng_below(r, 16);
        if (mv < 7) { a = rng_below(r, n - 1); b = a + 1; }
        else if (mv < 12) { int d = 2 + rng_below(r, 8); if (d >= n) d = n - 1; a = rng_below(r, n - d); b = a + d; }
        else if (mv < 15) { int p = rng_below(r, n), dd = 1 + rng_below(r, 8), qp = p + dd; if (qp >= n) qp = p - dd; if (qp < 0) continue;
                            a = F->val[p]; b = F->val[qp]; }
        else { a = rng_below(r, n); b = rng_below(r, n); }
        if (a == b) continue;
        if (a > b) { int t = a; a = b; b = t; }
        if (!fsa_valid_swap(F, F->pos[a], F->pos[b], a, b)) continue;
        long d = fsa_delta(F, a, b);
        if (d >= 0 || rng_unit(r) < exp((double)d / T)) {
            fsa_commit(F, a, b, d);
            if (F->total > best) {
                best = F->total; memcpy(x, F->val, sizeof(int) * n);
                if (best > stop_above) return best;
            }
        }
    }
    return best;
}

#endif
