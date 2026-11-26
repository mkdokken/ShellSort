/*
 * anneal.h -- heuristic (lower-bound) search for bad inputs.
 *
 * We maximise   offset + comparisons(passes g[from..k-1] applied to x)
 * over permutations x in a constrained class:
 *   CL_NONE    : any permutation
 *   CL_CHAIN   : x is h-sorted (x[i] < x[i+h])            -- used with the first pass decoupled
 *   CL_LOCAL   : x[i] < x[j] whenever j - i > h ("h-local") -- used for prefix-family lower bounds
 * Every permutation found is a certificate: the reported value is always attained by a concrete input.
 */
#ifndef ANNEAL_H
#define ANNEAL_H
#include "core.h"

enum { CL_NONE = 0, CL_CHAIN = 1, CL_LOCAL = 2 };

typedef struct {
    int n, k, from;
    const int *g;
    long offset;
    int cls, ch;      /* class and its parameter h */
} wcprob_t;

static inline long wc_eval(const wcprob_t *P, const int *x) {
    int a[MAXN];
    memcpy(a, x, sizeof(int) * P->n);
    return P->offset + shell_passes(a, P->n, P->g, P->from, P->k);
}

/* Is position p consistent with the class constraint (after a modification)? */
static inline int wc_okpos(const wcprob_t *P, const int *x, int p) {
    int n = P->n, h = P->ch;
    if (P->cls == CL_CHAIN) {
        if (p - h >= 0 && x[p - h] > x[p]) return 0;
        if (p + h < n && x[p + h] < x[p]) return 0;
        return 1;
    }
    if (P->cls == CL_LOCAL) {
        for (int i = 0; i < p - h; i++) if (x[i] > x[p]) return 0;
        for (int i = p + h + 1; i < n; i++) if (x[i] < x[p]) return 0;
        return 1;
    }
    return 1;
}

/* random member of the class */
static inline void wc_random(const wcprob_t *P, int *x, rng_t *r) {
    int n = P->n;
    random_perm(x, n, r);
    if (P->cls == CL_CHAIN) { hsort(x, n, P->ch); return; }
    if (P->cls == CL_LOCAL) {
        /* random h-local permutation: assign values 0,1,2,... in order; value v may go to any
           unused position p with p <= lo + h (lo = leftmost unused position). */
        int h = P->ch;
        int pos_of_val[MAXN], used[MAXN] = {0}, lo = 0;
        for (int v = 0; v < n; v++) {
            while (used[lo]) lo++;
            /* any unused position p with p <= lo + h is allowed (all positions < p-h are used) */
            int cand[MAXN], c = 0;
            for (int p = lo; p < n && p <= lo + h; p++) if (!used[p]) cand[c++] = p;
            int p = cand[rng_below(r, c)];
            used[p] = 1; pos_of_val[v] = p;
        }
        for (int v = 0; v < n; v++) x[pos_of_val[v]] = v;
    }
}

/*
 * Simulated annealing with restarts.  Stops early once a value > stop_above is found.
 * x: starting point (if start_given) and output (best found).
 * Returns the best value found.
 */
static long wc_anneal(const wcprob_t *P, int *x, int start_given, long long iters, int restarts,
                      double T0, double T1, long long stop_above, rng_t *r) {
    int n = P->n;
    int cur[MAXN], best[MAXN], pos[MAXN];
    long bestv = -1;
    for (int rs = 0; rs < restarts; rs++) {
        if (rs == 0 && start_given) memcpy(cur, x, sizeof(int) * n);
        else if (rs > 0 && bestv >= 0 && (rs & 1)) memcpy(cur, best, sizeof(int) * n);  /* re-anneal from best */
        else wc_random(P, cur, r);
        for (int i = 0; i < n; i++) pos[cur[i]] = i;
        long cv = wc_eval(P, cur);
        if (cv > bestv) { bestv = cv; memcpy(best, cur, sizeof(int) * n); if (bestv > stop_above) break; }
        double lt0 = log(T0), lt1 = log(T1);
        for (long long it = 0; it < iters; it++) {
            double T = exp(lt0 + (lt1 - lt0) * (double)it / (double)iters);
            int p, q;
            uint32_t mv = rng_below(r, 4);
            if (mv == 0) {            /* swap values v and v+1 */
                int v = rng_below(r, n - 1); p = pos[v]; q = pos[v + 1];
            } else if (mv == 1) {     /* swap values v and v+d, small d */
                int d = 2 + rng_below(r, 6); if (d >= n) d = n - 1;
                int v = rng_below(r, n - d); p = pos[v]; q = pos[v + d];
            } else if (mv == 2) {     /* swap nearby positions */
                p = rng_below(r, n); int d = 1 + rng_below(r, 8); q = p + d; if (q >= n) { q = p - d; if (q < 0) q = 0; }
            } else {                  /* swap random positions */
                p = rng_below(r, n); q = rng_below(r, n);
            }
            if (p == q) continue;
            int t = cur[p]; cur[p] = cur[q]; cur[q] = t;
            if (!wc_okpos(P, cur, p) || !wc_okpos(P, cur, q)) { t = cur[p]; cur[p] = cur[q]; cur[q] = t; continue; }
            long nv = wc_eval(P, cur);
            long d = nv - cv;
            if (d >= 0 || rng_unit(r) < exp((double)d / T)) {
                cv = nv; pos[cur[p]] = p; pos[cur[q]] = q;
                if (cv > bestv) {
                    bestv = cv; memcpy(best, cur, sizeof(int) * n);
                    if (bestv > stop_above) goto done;
                }
            } else { t = cur[p]; cur[p] = cur[q]; cur[q] = t; }
        }
    }
done:
    memcpy(x, best, sizeof(int) * n);
    return bestv;
}

/* Metropolis walk at fixed temperature T; x is modified in place and the FINAL state is kept
   (used as a perturbation "kick" in iterated local search). Returns its value. */
static long wc_kick(const wcprob_t *P, int *x, long long iters, double T, rng_t *r) {
    int n = P->n;
    long cv = wc_eval(P, x);
    for (long long it = 0; it < iters; it++) {
        int p, q;
        if (rng_below(r, 2)) { p = rng_below(r, n); int d = 1 + rng_below(r, 8); q = p + d; if (q >= n) q = p - d; if (q < 0) q = 0; }
        else { p = rng_below(r, n); q = rng_below(r, n); }
        if (p == q) continue;
        int t = x[p]; x[p] = x[q]; x[q] = t;
        if (!wc_okpos(P, x, p) || !wc_okpos(P, x, q)) { t = x[p]; x[p] = x[q]; x[q] = t; continue; }
        long nv = wc_eval(P, x), d = nv - cv;
        if (d >= 0 || rng_unit(r) < exp((double)d / T)) cv = nv;
        else { t = x[p]; x[p] = x[q]; x[q] = t; }
    }
    return cv;
}

/* Problem for the worst case of a full sequence, with the first pass decoupled. */
static inline void wc_prob_seq(wcprob_t *P, const seq_t *s) {
    P->n = s->n; P->g = s->g; P->k = s->k;
    if (s->k == 1) { P->from = 1; P->offset = pass_max(s->n, 1); P->cls = CL_NONE; P->ch = 1; return; }
    P->from = 1; P->offset = pass_max(s->n, s->g[0]); P->cls = CL_CHAIN; P->ch = s->g[0];
}

#endif
