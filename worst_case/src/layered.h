/*
 * layered.h -- memory-lean exact worst case for very large state spaces (certification runs).
 *
 * Same state space as exact2.h: (F = filled constrained positions, c' = free positions filled per
 * l-chain).  Every transition places exactly one more value, so the level L = |F| + sum(c') increases
 * by one and only two consecutive levels of f are kept in memory.
 *
 * Ranking inside a level: chains are split into groups A and B (mixed radix idx_A, idx_B).  A level is
 * a concatenation of blocks (F, s = L - |F|); inside a block, states with sum(c') = s are ordered by idx_B
 * and then by the position of idx_A among A-vectors of the same digit sum:
 *        rank = offL[F] + offB[s][idx_B] + posA[idx_A]          (all O(1) table lookups)
 * Levels are processed from n-1 down to 0 in parallel (OpenMP over (F, idx_B) work items).
 * No witness is reconstructed (the matching lower bound comes from a concrete input found separately).
 */
#ifndef LAYERED_H
#define LAYERED_H
#include "exact2.h"
#include <omp.h>

typedef struct { int r, x; long long nfr; int cost; uint8_t type; } lmove_t;

/* Storage per state: the residual f - floor(L).  One byte by default; sequences with few passes and a heavy
   final pass can exceed 255 (detected, the run aborts), so bin/shellwc16.exe is built with
   -DLAY_CELL=uint16_t -DLAY_CELL_MAX=65535 (twice the memory and dump size). */
#ifndef LAY_CELL
#define LAY_CELL uint8_t
#define LAY_CELL_MAX 255
#endif

static inline int x2_moves_raw(const x2ctx_t *c, long long frank, const int *lr, uint64_t F, const int *cp, lmove_t *mv) {
    int L = c->L, nm = 0;
    uint64_t U[MAXK];
    int cs[64];
    uint64_t S = 0;
    for (int r = 0; r < c->nch; r++) { cs[r] = cp[r] + popc(F & c->chainK[r]); S |= c->fm0[r][cs[r]]; }
    int k2 = c->pc.k;
    if (k2 > 1) dp_sets(&c->pc, S, U);
    for (int r = 0; r < c->nch; r++) {
        if (cs[r] >= c->m[r]) continue;
        int pen = 0;                     /* front pair rules (x2_front_rules) */
        if (c->needQ[r] && cs[r] >= 1 && (S & c->needQ[r]) != c->needQ[r]) {
            if (cs[r] == c->m[r] - 1) continue;
            pen = 1;
        }
        uint64_t filled = (F & c->chainK[r]) | c->freetail[r][cp[r]];
        int later = (k2 > 1) ? dp_addcost(&c->pc, U, r + cs[r] * L) : 0;
        if (cp[r] < c->nfree[r]) {
            int x = c->freelist[r][c->nfree[r] - 1 - cp[r]];
            int a = popc(filled & ((1ULL << x) - 1)), t = x / L;
            mv[nm].type = 0; mv[nm].r = r; mv[nm].x = x; mv[nm].nfr = frank; mv[nm].cost = t - a + (a > 0) + later - pen; nm++;
        }
        uint64_t cand = c->chainK[r] & ~F;
        while (cand) {
            int x = __builtin_ctzll(cand); cand &= cand - 1;
            if ((c->pred[x] & F) != c->pred[x]) continue;
            int ci = c->compof[x];
            int nl = c->cmask[ci][lr[ci]] | (1 << c->locbit[x]);
            int nr = c->crank[ci][nl];
            int a = popc(filled & ((1ULL << x) - 1)), t = x / L;
            mv[nm].type = 1; mv[nm].r = r; mv[nm].x = x;
            mv[nm].nfr = frank + (long long)(nr - lr[ci]) * c->CR[ci];
            mv[nm].cost = t - a + (a > 0) + later - pen; nm++;
        }
    }
    return nm;
}

/* Memory plan for a given kh: bytes needed by the layered DP (two f levels, per-F offset tables,
   F levels, offB table).  Level sizes come from a convolution (no enumeration).  -1 if unsupported. */
static double lay_plan_bytes(const seq_t *s, int kh, double *maxpair_out) {
    x2ctx_t *c = (x2ctx_t *)malloc(sizeof(x2ctx_t));
    if (x2_setup(c, s, kh, 1LL << 62) < 0) { x2_free(c); free(c); return -1; }
    int n = s->n;
    double cf[70] = {0}, cc[130] = {0}, tmp[130];
    cf[0] = 1;
    for (int ci = 0; ci < c->nc; ci++) {            /* F-level distribution */
        memset(tmp, 0, sizeof(tmp));
        for (int l = 0; l < 64; l++) if (cf[l] > 0)
            for (int r = 0; r < c->cnd[ci]; r++) tmp[l + __builtin_popcount(c->cmask[ci][r])] += cf[l];
        memcpy(cf, tmp, sizeof(double) * 70);
    }
    cc[0] = 1;
    for (int r = 0; r < c->nch; r++) {              /* c'-sum distribution */
        memset(tmp, 0, sizeof(tmp));
        for (int t = 0; t < 128; t++) if (cc[t] > 0) for (int u = 0; u <= c->nfree[r]; u++) tmp[t + u] += cc[t];
        memcpy(cc, tmp, sizeof(double) * 130);
    }
    double lsize[130] = {0}, maxpair = 0;
    for (int L = 0; L <= n; L++) for (int l = 0; l <= L && l < 70; l++) lsize[L] += cf[l] * cc[L - l];
    for (int L = 0; L < n; L++) if (lsize[L] + lsize[L + 1] > maxpair) maxpair = lsize[L] + lsize[L + 1];
    double D = (double)c->D;
    double bytes = maxpair * sizeof(LAY_CELL) + D * 17 + 2e8;   /* f levels + offL/offL1/levF + tables & slack */
    if (maxpair_out) *maxpair_out = maxpair;
    x2_free(c); free(c);
    return bytes;
}

/* returns W, or -1 on failure.  maxmem_states: cap on (two levels) states held in memory.
   The absorbed-pass count kh is chosen to minimise TOTAL memory, and a hard cap of
   LAYERED_MAX_BYTES (default 12 GB) is enforced. */
#ifndef LAYERED_MAX_BYTES
#define LAYERED_MAX_BYTES 12e9
#endif
static long long exact_worst_layered_ex(const seq_t *s, int force_kh, long long maxmem_states, int verbose,
                                        const char *dumpdir, int *perm_out);
static long long exact_worst_layered(const seq_t *s, int force_kh, long long maxmem_states, int verbose) {
    return exact_worst_layered_ex(s, force_kh, maxmem_states, verbose, NULL, NULL);
}

/*
 * dumpdir != NULL: every level's one-byte residuals are written to dumpdir/lev_L.bin, and afterwards an
 * attaining input is reconstructed by a forward walk (at each level take a move with
 * cost + f(successor) == f(current), reading f(successor) from the dumped level).
 */
static long long exact_worst_layered_ex(const seq_t *s, int force_kh, long long maxmem_states, int verbose,
                                        const char *dumpdir, int *perm_out) {
    int n = s->n;
    if (n > 64 || s->k == 1) return -1;
    int kh = -1;
    if (force_kh >= 0) kh = force_kh;
    else {
        double best = -1;
        for (int k2 = 0; k2 < s->k; k2++) {
            if (k2 > 0 && 2 * s->g[k2 - 1] < n) break;
            double mp, b = lay_plan_bytes(s, k2, &mp);
            if (verbose) fprintf(stderr, "  plan kh=%d: %.2f GB (two-level states %.3g)\n", k2, b / 1e9, mp);
            if (b > 0 && (best < 0 || b < best)) { best = b; kh = k2; }
        }
        if (kh < 0) return -1;
    }
    {
        double mp, b = lay_plan_bytes(s, kh, &mp);
        if (b < 0 || b > LAYERED_MAX_BYTES || mp > (double)maxmem_states) {
            if (verbose) fprintf(stderr, "layered: kh=%d needs %.2f GB (cap %.1f GB) -> refused\n", kh, b / 1e9, LAYERED_MAX_BYTES / 1e9);
            return -1;
        }
    }
    x2ctx_t *c = (x2ctx_t *)malloc(sizeof(x2ctx_t));
    if (x2_setup(c, s, kh, 1LL << 62) < 0) { x2_free(c); free(c); return -1; }
    long long D = c->D;
    int nch = c->nch;
    /* ---- choose split: A = chains [0,a), B = chains [a,nch) ---- */
    long long target_B = D >= 200000 ? 1 : 200000 / D;
    int a = nch; long long sizeB = 1;
    while (a > 0 && sizeB * (c->nfree[a - 1] + 1) <= (target_B > 1 ? target_B : 1)) { a--; sizeB *= (c->nfree[a] + 1); }
    long long sizeA = 1;
    for (int r = 0; r < a; r++) sizeA *= (c->nfree[r] + 1);
    if (sizeA > 400000000LL) { if (verbose) fprintf(stderr, "split too unbalanced\n"); x2_free(c); free(c); return -1; }
    int maxA = 0, maxB = 0;
    for (int r = 0; r < a; r++) maxA += c->nfree[r];
    for (int r = a; r < nch; r++) maxB += c->nfree[r];
    int smax = maxA + maxB;
    long long RA[64], RB[64];
    RA[0] = 1; for (int r = 1; r <= a; r++) RA[r] = RA[r - 1] * (c->nfree[r - 1] + 1);
    RB[0] = 1; for (int r = a + 1; r <= nch; r++) RB[r - a] = RB[r - a - 1] * (c->nfree[r - 1] + 1);
    /* A tables */
    uint8_t *sumA = (uint8_t *)malloc(sizeA);
    uint32_t *posA = (uint32_t *)malloc(sizeof(uint32_t) * sizeA);
    long long cntA[130] = {0};
    for (long long i = 0; i < sizeA; i++) {
        long long t = i; int sm = 0;
        for (int r = 0; r < a; r++) { sm += (int)(t % (c->nfree[r] + 1)); t /= (c->nfree[r] + 1); }
        sumA[i] = (uint8_t)sm; posA[i] = (uint32_t)cntA[sm]++;
    }
    long long startA[131]; startA[0] = 0;
    for (int t = 0; t <= 129; t++) startA[t + 1] = startA[t] + cntA[t];
    uint32_t *listA = (uint32_t *)malloc(sizeof(uint32_t) * sizeA);
    { long long p[130]; for (int t = 0; t < 130; t++) p[t] = startA[t];
      for (long long i = 0; i < sizeA; i++) listA[p[sumA[i]]++] = (uint32_t)i; }
    /* B tables */
    uint8_t *sumB = (uint8_t *)malloc(sizeB);
    for (long long i = 0; i < sizeB; i++) {
        long long t = i; int sm = 0;
        for (int r = a; r < nch; r++) { sm += (int)(t % (c->nfree[r] + 1)); t /= (c->nfree[r] + 1); }
        sumB[i] = (uint8_t)sm;
    }
    /* offB[s][idx_B], bsize[s] */
    uint64_t *offB = (uint64_t *)malloc(sizeof(uint64_t) * (size_t)(smax + 2) * sizeB);
    uint64_t bsize[130] = {0};
    for (int sm = 0; sm <= smax + 1; sm++) {
        uint64_t acc = 0;
        for (long long i = 0; i < sizeB; i++) {
            offB[(size_t)sm * sizeB + i] = acc;
            int ta = sm - sumB[i];
            if (ta >= 0 && ta <= maxA) acc += cntA[ta];
        }
        bsize[sm] = acc;
    }
    /* F levels */
    uint8_t *levF = (uint8_t *)malloc((size_t)D);
    int maxF = 0;
    for (long long fr = 0; fr < D; fr++) {
        long long x = fr; int pc = 0;
        for (int ci = 0; ci < c->nc; ci++) { int r = (int)(x % c->cnd[ci]); x /= c->cnd[ci]; pc += __builtin_popcount(c->cmask[ci][r]); }
        levF[fr] = (uint8_t)pc; if (pc > maxF) maxF = pc;
    }
    uint64_t *offL = (uint64_t *)malloc(sizeof(uint64_t) * D), *offL1 = (uint64_t *)malloc(sizeof(uint64_t) * D);
    /* level sizes */
    uint64_t lsize[130] = {0}, maxpair = 0;
    for (int L = 0; L <= n; L++) {
        for (long long fr = 0; fr < D; fr++) { int sm = L - levF[fr]; if (sm >= 0 && sm <= smax) lsize[L] += bsize[sm]; }
    }
    for (int L = 0; L < n; L++) if (lsize[L] + lsize[L + 1] > maxpair) maxpair = lsize[L] + lsize[L + 1];
    if (verbose) fprintf(stderr, "layered: kh=%d D=%lld sizeA=%lld sizeB=%lld smax=%d max two-level states=%llu (%.2f GB)\n",
                         kh, D, sizeA, sizeB, smax, (unsigned long long)maxpair, maxpair * 2.0 / 1e9);
    if ((long long)maxpair > maxmem_states) { if (verbose) fprintf(stderr, "exceeds memory cap\n"); return -1; }
    /* One LAY_CELL per state: f is stored relative to a provable per-level floor.  At level L, n-L values
       remain; in every pass whose cost is in f (the decoupled pass and all later ones) at most h of them
       can sit on a chain head, the others cost >= 1, so f >= floor(L) = sum_j max(0, n-L-h_j).
       A residual outside 0..LAY_CELL_MAX is detected and aborts the run (never silently wrong). */
    int flo[130];
    for (int L = 0; L <= n; L++) { flo[L] = 0; for (int j = kh; j < s->k; j++) { int t = n - L - s->g[j]; if (t > 0) flo[L] += t; } }
    volatile int overflow = 0;
    LAY_CELL *fL1 = (LAY_CELL *)malloc(sizeof(LAY_CELL) * (lsize[n] + 1));
    fL1[0] = 0;   /* level n: the full state, f = 0 = floor(n) */
    char fname[1024];
    if (dumpdir) {
        snprintf(fname, sizeof fname, "%s/lev_%d.bin", dumpdir, n);
        FILE *fo = fopen(fname, "wb"); if (!fo) { fprintf(stderr, "cannot write %s\n", fname); return -1; }
        fwrite(fL1, sizeof(LAY_CELL), 1, fo); fclose(fo);
    }
    /* offsets for level n */
    { uint64_t acc = 0; for (long long fr = 0; fr < D; fr++) { offL1[fr] = acc; int sm = n - levF[fr]; if (sm >= 0 && sm <= smax) acc += bsize[sm]; } }
    double t0 = omp_get_wtime();
    for (int L = n - 1; L >= 0; L--) {
        uint64_t acc = 0;
        for (long long fr = 0; fr < D; fr++) { offL[fr] = acc; int sm = L - levF[fr]; if (sm >= 0 && sm <= smax) acc += bsize[sm]; }
        LAY_CELL *fL = (LAY_CELL *)malloc(sizeof(LAY_CELL) * (lsize[L] + 1));
        if (!fL) { fprintf(stderr, "alloc failed level %d\n", L); return -1; }
        const int fl1 = flo[L + 1], fl0 = flo[L];
        long long work = D * sizeB;
        #pragma omp parallel for schedule(dynamic, 64)
        for (long long u = 0; u < work; u++) {
            long long fr = u / sizeB, ib = u % sizeB;
            int sm = L - levF[fr];
            if (sm < 0 || sm > smax) continue;
            int ta = sm - sumB[ib];
            if (ta < 0 || ta > maxA) continue;
            int lr[X2_MAXCOMP], cp[64];
            lmove_t mv[160];
            uint64_t F = x2_decodeF(c, fr, lr);
            { long long t = ib; for (int r = a; r < nch; r++) { cp[r] = (int)(t % (c->nfree[r] + 1)); t /= (c->nfree[r] + 1); } }
            uint64_t base = offL[fr] + offB[(size_t)sm * sizeB + ib];
            for (long long q = startA[ta]; q < startA[ta + 1]; q++) {
                long long ia = listA[q];
                { long long t = ia; for (int r = 0; r < a; r++) { cp[r] = (int)(t % (c->nfree[r] + 1)); t /= (c->nfree[r] + 1); } }
                int nm = x2_moves_raw(c, fr, lr, F, cp, mv);
                int best = 0;
                for (int i = 0; i < nm; i++) {
                    uint64_t rk;
                    if (mv[i].type == 0) {
                        int r = mv[i].r;
                        if (r < a) rk = offL1[fr] + offB[(size_t)(sm + 1) * sizeB + ib] + posA[ia + RA[r]];
                        else rk = offL1[fr] + offB[(size_t)(sm + 1) * sizeB + ib + RB[r - a]] + posA[ia];
                    } else {
                        rk = offL1[mv[i].nfr] + offB[(size_t)sm * sizeB + ib] + posA[ia];
                    }
                    int v = mv[i].cost + fL1[rk] + fl1;
                    if (v > best) best = v;
                }
                int d = best - fl0;
                if (d < 0 || d > LAY_CELL_MAX) { overflow = 1; d = d < 0 ? 0 : LAY_CELL_MAX; }
                fL[base + posA[ia]] = (LAY_CELL)d;
            }
        }
        if (dumpdir) {
            snprintf(fname, sizeof fname, "%s/lev_%d.bin", dumpdir, L);
            FILE *fo = fopen(fname, "wb");
            int okw = fo != NULL;
            /* chunked: a single msvcrt fwrite of more than 4 GiB crawls at ~1 MB/s */
            for (uint64_t off = 0, ch = 1ULL << 27; okw && off < lsize[L]; off += ch) {
                uint64_t m = lsize[L] - off < ch ? lsize[L] - off : ch;
                okw = fwrite(fL + off, sizeof(LAY_CELL), m, fo) == m;
            }
            if (!okw) { fprintf(stderr, "dump failed at level %d\n", L); return -1; }
            fclose(fo);
        }
        free(fL1); fL1 = fL;
        uint64_t *tmp = offL1; offL1 = offL; offL = tmp;
        if (verbose) fprintf(stderr, "  level %2d: %12llu states  t=%.1fs\n", L, (unsigned long long)lsize[L], omp_get_wtime() - t0);
        if (overflow) {
            fprintf(stderr, "layered: residual outside 0..%d at level %d -> aborted\n", LAY_CELL_MAX, L);
            free(fL1); free(offL); free(offL1); free(levF); free(offB); free(sumB); free(listA); free(posA); free(sumA);
            x2_free(c); free(c);
            return -2;
        }
    }
    long long W = fL1[0] + flo[0];
    for (int i = 0; i < kh; i++) W += n - s->g[i];
    if (dumpdir && perm_out) {
        /* forward reconstruction of an attaining input */
        long long fr = 0, ia = 0, ib = 0; int sm = 0;
        int fcur = fL1[0] + flo[0];
        int ok = 1;
        for (int L = 0; L < n && ok; L++) {
            uint64_t acc = 0;              /* offsets of level L+1 */
            for (long long q = 0; q < D; q++) { offL1[q] = acc; int s2 = L + 1 - levF[q]; if (s2 >= 0 && s2 <= smax) acc += bsize[s2]; }
            snprintf(fname, sizeof fname, "%s/lev_%d.bin", dumpdir, L + 1);
            FILE *fi = fopen(fname, "rb");
            if (!fi) { ok = 0; break; }
            int lr[X2_MAXCOMP], cp[64];
            lmove_t mv[160];
            uint64_t F = x2_decodeF(c, fr, lr);
            { long long t = ib; for (int r = a; r < nch; r++) { cp[r] = (int)(t % (c->nfree[r] + 1)); t /= (c->nfree[r] + 1); } }
            { long long t = ia; for (int r = 0; r < a; r++) { cp[r] = (int)(t % (c->nfree[r] + 1)); t /= (c->nfree[r] + 1); } }
            int nm = x2_moves_raw(c, fr, lr, F, cp, mv), pick = -1;
            for (int i = 0; i < nm && pick < 0; i++) {
                uint64_t rk;
                if (mv[i].type == 0) {
                    int r = mv[i].r;
                    if (r < a) rk = offL1[fr] + offB[(size_t)(sm + 1) * sizeB + ib] + posA[ia + RA[r]];
                    else rk = offL1[fr] + offB[(size_t)(sm + 1) * sizeB + ib + RB[r - a]] + posA[ia];
                } else rk = offL1[mv[i].nfr] + offB[(size_t)sm * sizeB + ib] + posA[ia];
                LAY_CELL byte;
                _fseeki64(fi, (long long)rk * (long long)sizeof(LAY_CELL), SEEK_SET);
                if (fread(&byte, sizeof(LAY_CELL), 1, fi) != 1) { ok = 0; break; }
                if (mv[i].cost + byte + flo[L + 1] == fcur) {
                    pick = i; fcur -= mv[i].cost;
                    if (mv[i].type == 0) { int r = mv[i].r; if (r < a) ia += RA[r]; else ib += RB[r - a]; sm++; }
                    else fr = mv[i].nfr;
                }
            }
            fclose(fi);
            if (pick < 0) { ok = 0; break; }
            perm_out[mv[pick].x] = L;
        }
        if (!ok) { fprintf(stderr, "reconstruction failed\n"); perm_out[0] = -1; }
    }
    free(fL1); free(offL); free(offL1); free(levF); free(offB); free(sumB); free(listA); free(posA); free(sumA);
    x2_free(c); free(c);
    return W;
}

#endif
