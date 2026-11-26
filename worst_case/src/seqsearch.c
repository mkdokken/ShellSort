/*
 * seqsearch.c -- parallel branch-and-bound over Shellsort gap sequences minimising the WORST-CASE
 * number of comparisons for a fixed array size N.
 *
 * Search tree.  A node is a prefix P = {1 = p1 < p2 < ... < pm} of small gaps (the LAST passes).
 * Its family F(P) = { P u Q : every gap of Q is > pm } contains P itself and all its extensions.
 *
 * Family bound (sound, witness-certified).  If x is "pm-local" (x[i] < x[j] whenever j-i > pm), then x
 * is q-sorted for every q > pm, so the passes of any Q leave x untouched and cost >= 0, and then P's
 * passes run on x.  Hence for every member of F(P):   W(P u Q) >= cost_P(x).
 * One pm-local permutation with cost_P(x) > T therefore prunes the whole family at once.  The bound
 * only grows down the tree (cost_{P+g}(x) = cost_P(x) + N - g for the parent's witness), so pruning is
 * "continuous": every witness found is reused for all descendants and kept in a per-thread pool of
 * killer inputs that is tried first on every new node.
 *
 * Each node is also evaluated as a complete sequence: annealing for a killer input (> T) first, then
 * the exact DP (exact2.h) for the survivors.  Only exact values lower the shared threshold T (unless
 * --trust), so every pruning decision is backed by a concrete input permutation exceeding a proven
 * optimum candidate: the final set of sequences with W == T is complete and exact.
 *
 * Parallelisation: depth-3 prefixes {1,a,b} are independent tasks (OpenMP dynamic schedule); the
 * threshold T is shared and only decreases.
 *
 * usage: seqsearch N [--T0 v] [--maxgap g] [--exact M] [--fb iters] [--seq iters] [--big iters]
 *                    [--trust] [--seed s] [--out file] [--quiet]
 */
#include <omp.h>
#include <time.h>
#include "core.h"
#include "exact.h"
#include "exact2.h"
#include "anneal.h"
#include "fastsa.h"

static int N, MAXGAP;
static volatile int T;
static long long EXACT_LIMIT = 60000000LL;
static long long FB_ITERS = 4000, SEQ_ITERS = 20000, BIG_ITERS = 2000000, CONFIRM_ITERS = 4000000;
static double TEMP = 1.5;      /* starting annealing temperature (hotter helps for large N) */
static int TRUST = 0, QUIET = 0;
static uint64_t SEED = 1;

typedef struct { int m; int g[MAXK]; } pref_t;           /* ascending gaps, g[0] = 1 */

typedef struct { int W; int exact; long long states; pref_t p; int perm[MAXN]; } result_t;
static result_t *RES; static int NRES = 0, CAPRES = 0;
static int UNRES_BEST = 1 << 30;                          /* best heuristic-only candidate value */

#define POOLSZ 48
typedef struct {
    rng_t rng;
    fsa_t F;                      /* incremental annealer buffers (sized for N) */
    int pool[POOLSZ][MAXN]; int poolloc[POOLSZ]; int npool, poolpos;
    long long nodes, fb_pruned, fb_pool, seq_eval, seq_killed, seq_exact, seq_heur;
    double t_exact;
} tctx_t;

static void pref_to_seq(const pref_t *P, seq_t *s) { seq_make(s, N, P->g, P->m); }

/* optional restriction: only families compatible with one of these prefixes are explored */
static int NPFX = 0;
static pref_t PFX[256];
static int allowed(const pref_t *P) {
    if (NPFX == 0) return 1;
    for (int i = 0; i < NPFX; i++) {
        int L = P->m < PFX[i].m ? P->m : PFX[i].m, ok = 1;
        for (int t = 0; t < L && ok; t++) if (P->g[t] != PFX[i].g[t]) ok = 0;
        if (ok) return 1;
    }
    return 0;
}

static void pref_str(const pref_t *P, char *buf) {
    int o = 0; buf[0] = 0;
    for (int i = 0; i < P->m; i++) o += sprintf(buf + o, i ? ",%d" : "%d", P->g[i]);
}

static int locality(const int *x, int n) {
    int L = 0;
    for (int i = 0; i < n; i++) for (int j = n - 1; j > i + L; j--) if (x[i] > x[j]) { L = j - i; break; }
    return L;
}

static void pool_add(tctx_t *tc, const int *x) {
    memcpy(tc->pool[tc->poolpos], x, sizeof(int) * N);
    tc->poolloc[tc->poolpos] = locality(x, N);
    tc->poolpos = (tc->poolpos + 1) % POOLSZ;
    if (tc->npool < POOLSZ) tc->npool++;
}

/* record a sequence with value W <= T */
static void record(const seq_t *s, const pref_t *P, int W, int exact, long long states, const int *perm) {
    #pragma omp critical(results)
    {
        if (exact) {     /* only exact values ever lower the shared threshold */
            if (W < T) {
                T = W;
                int o = 0;   /* drop results now above T */
                for (int i = 0; i < NRES; i++) if (RES[i].W <= T) RES[o++] = RES[i];
                NRES = o;
                if (!QUIET) { char b[256]; pref_str(P, b); fprintf(stderr, "  new best T=%d {%s}%s\n", W, b, exact ? "" : " (heuristic)"); }
            }
        }
        if (!exact && W < UNRES_BEST) UNRES_BEST = W;
        if (W <= T) {
            if (NRES == CAPRES) { CAPRES = CAPRES ? 2 * CAPRES : 64; RES = (result_t *)realloc(RES, sizeof(result_t) * CAPRES); }
            RES[NRES].W = W; RES[NRES].exact = exact; RES[NRES].states = states; RES[NRES].p = *P;
            memcpy(RES[NRES].perm, perm, sizeof(int) * N);
            NRES++;
        }
    }
    (void)s;
}

/* Evaluate P as a complete sequence. wit: the family witness (pm-local). */
static void eval_sequence(const pref_t *P, const int *wit, tctx_t *tc) {
    seq_t s; pref_to_seq(P, &s);
    tc->seq_eval++;
    wcprob_t SP; wc_prob_seq(&SP, &s);
    int y[MAXN], z[MAXN];
    int Tc = T;
    memcpy(y, wit, sizeof(int) * N);
    if (s.k > 1) hsort(y, N, s.g[0]);
    long v = wc_eval(&SP, y);
    if (v > Tc) { tc->seq_killed++; return; }
    for (int i = 0; i < tc->npool; i++) {
        memcpy(z, tc->pool[i], sizeof(int) * N);
        if (s.k > 1) hsort(z, N, s.g[0]);
        if (wc_eval(&SP, z) > Tc) { tc->seq_killed++; return; }
    }
    if (s.k == 1) {                                  /* insertion sort only: v = n(n-1)/2 is exact */
        int rev[MAXN]; for (int i = 0; i < N; i++) rev[i] = N - 1 - i;
        record(&s, P, (int)v, 1, 1, rev);
        return;
    }
    vctx_t V; vctx_init(&V, &s); fsa_bind(&tc->F, &V);
    /* a kill is accepted only after re-simulating the killer input (defence against evaluator bugs) */
    #define KILL(arr) do { if (wc_eval(&SP, arr) > T) { tc->seq_killed++; pool_add(tc, arr); vctx_free(&V); return; } \
                           fprintf(stderr, "WARNING: incremental value not confirmed by simulation\n"); } while (0)
    /* cold greedy climb first (incremental evaluation): kills clearly bad sequences quickly */
    v = fsa_anneal(&s, &V, &tc->F, y, 1, SEQ_ITERS, 0.25, 0.02, Tc, &tc->rng);
    if (v > Tc) KILL(y);
    v = fsa_anneal(&s, &V, &tc->F, y, 1, 2 * SEQ_ITERS, TEMP, 0.05, Tc, &tc->rng);
    if (v > Tc) KILL(y);
    /* survivor.  If the exact DP is expensive, first try much harder to find a killer input. */
    int khx; long long size = N <= 64 ? x2_best_kh(&s, EXACT_LIMIT, &khx) : -1;
    if (size < 0 || size > 2000000) {
        long long it = (size < 0 || size > 20000000) ? BIG_ITERS : BIG_ITERS / 4;
        for (int rep = 0; rep < 3; rep++) {
            memcpy(z, y, sizeof(int) * N);
            long w = fsa_anneal(&s, &V, &tc->F, z, rep == 0, it, TEMP + 0.5, 0.05, T, &tc->rng);
            if (w > T) KILL(z);
            if (w > v) { v = w; memcpy(y, z, sizeof(int) * N); }
        }
    }
    int perm[MAXN]; long long st; double t0 = omp_get_wtime();
    long long W = exact_worst2(&s, EXACT_LIMIT, perm, &st);
    tc->t_exact += omp_get_wtime() - t0;
    if (W >= 0) {
        tc->seq_exact++;
        if (W <= T) record(&s, P, (int)W, 1, st, perm);
        else pool_add(tc, perm);
        vctx_free(&V);
        return;
    }
    /* too big for the exact DP: long annealing (lower bound only) */
    tc->seq_heur++;
    long best = v;
    for (int rep = 0; rep < 6 && best <= T; rep++) {
        memcpy(z, y, sizeof(int) * N);
        long w = fsa_anneal(&s, &V, &tc->F, z, rep == 0, 2 * BIG_ITERS, TEMP + 1.0, 0.05, T, &tc->rng);
        if (w > best) { best = w; memcpy(y, z, sizeof(int) * N); }
    }
    vctx_free(&V);
    #undef KILL
    if (best > T) { tc->seq_killed++; pool_add(tc, y); return; }
    /* Heuristic-only values never lower T (a weakly searched sequence could otherwise set a falsely
       low threshold and prune better ones).  The driver re-evaluates these candidates strongly and
       re-runs the search with the confirmed value as T0. */
    int in0[MAXN];
    if (s.k > 1) unsort_first(y, in0, N, s.g[0]); else memcpy(in0, y, sizeof(int) * N);
    record(&s, P, (int)best, 0, -1, in0);
}

/* family check; returns 1 if pruned. x: in = parent witness, out = this node's witness */
static int family_pruned(const pref_t *P, int *x, tctx_t *tc) {
    seq_t s; pref_to_seq(P, &s);
    int pm = P->g[P->m - 1];
    wcprob_t FP; FP.n = N; FP.k = s.k; FP.from = 0; FP.g = s.g; FP.offset = 0; FP.cls = CL_LOCAL; FP.ch = pm;
    int Tc = T;
    long v = wc_eval(&FP, x);
    if (v > Tc) { tc->fb_pruned++; return 1; }
    for (int i = 0; i < tc->npool; i++) if (tc->poolloc[i] <= pm) {
        if (wc_eval(&FP, tc->pool[i]) > Tc) { tc->fb_pruned++; tc->fb_pool++; return 1; }
    }
    v = wc_anneal(&FP, x, 1, FB_ITERS, 1, 0.25, 0.02, Tc, &tc->rng);
    if (v > Tc) { tc->fb_pruned++; pool_add(tc, x); return 1; }
    v = wc_anneal(&FP, x, 1, FB_ITERS, 1, 1.0, 0.05, Tc, &tc->rng);
    if (v > Tc) { tc->fb_pruned++; pool_add(tc, x); return 1; }
    return 0;
}

static void visit(pref_t *P, const int *pwit, tctx_t *tc, int maxdepth_children) {
    int x[MAXN];
    if (!allowed(P)) return;
    memcpy(x, pwit, sizeof(int) * N);
    tc->nodes++;
    if (family_pruned(P, x, tc)) return;
    eval_sequence(P, x, tc);
    if (!maxdepth_children) return;
    int pm = P->g[P->m - 1];
    if (P->m >= MAXK - 1) return;
    for (int g = pm + 1; g <= MAXGAP && g < N; g++) {
        P->g[P->m++] = g;
        visit(P, x, tc, 1);
        P->m--;
    }
}

int main(int argc, char **argv) {
    if (argc < 2) { fprintf(stderr, "usage: seqsearch N [options]\n"); return 1; }
    setvbuf(stderr, NULL, _IONBF, 0);
    N = atoi(argv[1]);
    MAXGAP = N - 1;
    int T0 = N * (N - 1) / 2;
    const char *outfile = NULL;
    for (int i = 2; i < argc; i++) {
        if (!strcmp(argv[i], "--T0")) T0 = atoi(argv[++i]);
        else if (!strcmp(argv[i], "--maxgap")) MAXGAP = atoi(argv[++i]);
        else if (!strcmp(argv[i], "--exact")) EXACT_LIMIT = atoll(argv[++i]);
        else if (!strcmp(argv[i], "--fb")) FB_ITERS = atoll(argv[++i]);
        else if (!strcmp(argv[i], "--seq")) SEQ_ITERS = atoll(argv[++i]);
        else if (!strcmp(argv[i], "--big")) BIG_ITERS = atoll(argv[++i]);
        else if (!strcmp(argv[i], "--confirm")) CONFIRM_ITERS = atoll(argv[++i]);
        else if (!strcmp(argv[i], "--temp")) TEMP = atof(argv[++i]);
        else if (!strcmp(argv[i], "--trust")) TRUST = 1;
        else if (!strcmp(argv[i], "--quiet")) QUIET = 1;
        else if (!strcmp(argv[i], "--seed")) SEED = strtoull(argv[++i], 0, 10);
        else if (!strcmp(argv[i], "--out")) outfile = argv[++i];
        else if (!strcmp(argv[i], "--prefixes")) {
            /* "1,4,9,11;1,4,9;..." each must start with 1 */
            char *str = argv[++i], *save = NULL;
            for (char *tok = strtok_r(str, ";", &save); tok && NPFX < 256; tok = strtok_r(NULL, ";", &save)) {
                int gg[MAXK]; int c = parse_gaps(tok, gg);
                if (c > 0) { PFX[NPFX].m = c; memcpy(PFX[NPFX].g, gg, sizeof(int) * c); NPFX++; }
            }
        }
    }
    if (N > 64 && !TRUST) { fprintf(stderr, "N > 64 has no exact DP: use --trust (heuristic thresholds)\n"); return 1; }
    if (N > MAXN) { fprintf(stderr, "N <= %d\n", MAXN); return 1; }
    T = T0;
    double t0 = omp_get_wtime();

    /* ---- sequential top levels: {1} and {1,a}; collect depth-3 tasks ---- */
    int nth = omp_get_max_threads();
    tctx_t *TC = (tctx_t *)calloc(nth, sizeof(tctx_t));
    for (int i = 0; i < nth; i++) { rng_seed(&TC[i].rng, SEED * 1000003ULL + i); fsa_alloc_n(&TC[i].F, N); }
    pref_t root; root.m = 1; root.g[0] = 1;
    int id[MAXN]; for (int i = 0; i < N; i++) id[i] = i;
    size_t maxtask = (size_t)MAXGAP * MAXGAP / 2 + 8;
    pref_t *tasks = (pref_t *)malloc(sizeof(pref_t) * maxtask);
    int *twit = (int *)malloc(sizeof(int) * (size_t)N * maxtask);
    int ntask = 0;
    {
        int x1[MAXN]; memcpy(x1, id, sizeof(int) * N);
        TC[0].nodes++;
        if (!family_pruned(&root, x1, &TC[0])) {
            eval_sequence(&root, x1, &TC[0]);
            /* depth 1 ({1,a}) in parallel */
            int top = MAXGAP < N - 1 ? MAXGAP : N - 1;
            char *alive = (char *)calloc(top + 2, 1);
            int *w1 = (int *)malloc(sizeof(int) * (size_t)N * (top + 2));
            #pragma omp parallel for schedule(dynamic, 1)
            for (int a = 2; a <= top; a++) {
                tctx_t *tc = &TC[omp_get_thread_num()];
                pref_t P = root; P.g[P.m++] = a;
                if (!allowed(&P)) continue;
                int *x2 = w1 + (size_t)a * N;
                memcpy(x2, x1, sizeof(int) * N);
                tc->nodes++;
                double tq = omp_get_wtime();
                int pr = family_pruned(&P, x2, tc);
                double tf = omp_get_wtime() - tq;
                if (!pr) { eval_sequence(&P, x2, tc); alive[a] = 1; }
                if (getenv("SEQDEBUG")) fprintf(stderr, "  {1,%d}: family %s in %.2fs, total %.2fs\n", a, pr ? "PRUNED" : "kept", tf, omp_get_wtime() - tq);
            }
            for (int a = 2; a <= top; a++) if (alive[a]) {
                pref_t P = root; P.g[P.m++] = a;
                for (int b = a + 1; b <= top; b++) {
                    pref_t Q = P; Q.g[Q.m++] = b;
                    if (!allowed(&Q)) continue;
                    tasks[ntask] = Q; memcpy(twit + (size_t)ntask * N, w1 + (size_t)a * N, sizeof(int) * N); ntask++;
                }
            }
            free(alive); free(w1);
        }
    }
    if (!QUIET) fprintf(stderr, "N=%d T0=%d tasks=%d threads=%d\n", N, T0, ntask, nth);

    int done_tasks = 0; double last_report = omp_get_wtime();
    #pragma omp parallel for schedule(dynamic, 1)
    for (int i = 0; i < ntask; i++) {
        tctx_t *tc = &TC[omp_get_thread_num()];
        pref_t P = tasks[i];
        visit(&P, twit + (size_t)i * N, tc, 1);
        #pragma omp atomic
        done_tasks++;
        if (!QUIET && omp_get_wtime() - last_report > 30) {
            #pragma omp critical(report)
            {
                if (omp_get_wtime() - last_report > 30) {
                    long long nodes = 0, sx = 0, sh = 0;
                    for (int q = 0; q < nth; q++) { nodes += TC[q].nodes; sx += TC[q].seq_exact; sh += TC[q].seq_heur; }
                    fprintf(stderr, "  [%.0fs] tasks %d/%d nodes=%lld exact=%lld heur=%lld T=%d results=%d\n",
                            omp_get_wtime() - t0, done_tasks, ntask, nodes, sx, sh, T, NRES);
                    last_report = omp_get_wtime();
                }
            }
        }
    }
    double el = omp_get_wtime() - t0;

    long long nodes = 0, fbp = 0, fbpool = 0, se = 0, sk = 0, sx = 0, sh = 0; double tex = 0;
    for (int i = 0; i < nth; i++) {
        nodes += TC[i].nodes; fbp += TC[i].fb_pruned; fbpool += TC[i].fb_pool; se += TC[i].seq_eval;
        sk += TC[i].seq_killed; sx += TC[i].seq_exact; sh += TC[i].seq_heur; tex += TC[i].t_exact;
    }
    /* sort results: W, then exact first, then length, then lexicographic */
    for (int i = 0; i < NRES; i++) for (int j = i + 1; j < NRES; j++) {
        result_t *a = &RES[i], *b = &RES[j];
        int sw = 0;
        if (b->W != a->W) sw = b->W < a->W;
        else if (b->p.m != a->p.m) sw = b->p.m < a->p.m;
        else { for (int t = 0; t < a->p.m; t++) if (a->p.g[t] != b->p.g[t]) { sw = b->p.g[t] < a->p.g[t]; break; } }
        if (sw) { result_t tmp = *a; *a = *b; *b = tmp; }
    }
    FILE *fo = outfile ? fopen(outfile, "w") : stdout;
    fprintf(fo, "# N=%d T_final=%d T0=%d maxgap=%d time=%.1fs nodes=%lld fb_pruned=%lld (pool %lld) seq_eval=%lld seq_killed=%lld seq_exact=%lld seq_heur=%lld t_exact=%.1fs\n",
            N, T, T0, MAXGAP, el, nodes, fbp, fbpool, se, sk, sx, sh, tex);
    int nopt = 0;
    for (int i = 0; i < NRES; i++) if (RES[i].W == T) nopt++;
    fprintf(fo, "# optimal_count=%d (sequences with W == T_final)\n", nopt);
    for (int i = 0; i < NRES; i++) {
        if (RES[i].W > T) continue;
        char b[256]; pref_str(&RES[i].p, b);
        fprintf(fo, "W=%d %s seq={%s} states=%lld worst_input=[", RES[i].W, RES[i].exact ? "exact" : "LOWER_BOUND_ONLY",
                b, RES[i].states);
        for (int t = 0; t < N; t++) fprintf(fo, t ? ",%d" : "%d", RES[i].perm[t]);
        fprintf(fo, "]\n");
    }
    if (outfile) fclose(fo);
    if (!QUIET) fprintf(stderr, "done N=%d T=%d optimal=%d time=%.1fs nodes=%lld\n", N, T, nopt, el, nodes);
    return 0;
}
