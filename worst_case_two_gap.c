/*
 * worst_case_two_gap.c
 *
 * Computes the exact worst-case number of comparisons for a two-gap
 * Shell sort using the gap sequence {1, g} (first a pass with gap g,
 * then a final insertion-sort pass with gap 1) on an array of size N.
 *
 * The comparison-counting model matches shellSortCustom in main.c:
 *   - each key comparison counts as 1,
 *   - hitting the chain boundary ends an insertion WITHOUT a final
 *     comparison, while stopping on a "not greater" neighbor DOES cost
 *     that comparison.
 *
 * The formula below is exact (verified by exhaustive search over all
 * permutations for g = 2..6 and N up to ~12). The rest of this comment
 * explains WHY it works.
 *
 * ======================================================================
 * OVERVIEW: the worst case splits into two passes, maximized separately
 * ======================================================================
 *
 * A {1, g} sort does two passes:
 *   Pass 1 (gap g): sort each of the g "chains" independently. Chain r
 *                   is the set of positions r, r+g, r+2g, ... Sorting a
 *                   chain is just an insertion sort on that subsequence.
 *   Pass 2 (gap 1): an ordinary insertion sort on the now g-sorted array
 *                   (an array is "g-sorted" when a[i] <= a[i+g] for all i).
 *
 * Total comparisons = (cost of pass 1) + (cost of pass 2).
 *
 * Key observation: the adversary can maximize the two passes INDEPENDENTLY.
 *   - Pass 1's maximum cost depends only on the chain SIZES (you make each
 *     chain reverse-sorted). It does NOT depend on which values land in
 *     which chain.
 *   - Pass 2's cost depends only on the PARTITION of values into chains
 *     (because pass 1 leaves each chain sorted, so the array entering
 *     pass 2 is fully determined by which values are in each chain, not by
 *     their original within-chain order).
 *
 * These are two separate knobs, so the global worst case is simply
 *     (max over within-chain orderings of pass 1)
 *   + (max over value partitions of pass 2),
 * and both maxima are achievable at the same time. That is why we can
 * derive P1 and P2 separately and add them.
 *
 * Let q = floor(N / g) and s = N mod g. Then s chains have q+1 elements
 * (residues 0..s-1) and g-s chains have q elements (residues s..g-1).
 *
 * ======================================================================
 * P1 : the gap-g pass
 * ======================================================================
 *
 * Each chain is an independent insertion sort, whose worst case on m
 * elements is C(m,2) = m(m-1)/2 (reverse-sorted chain: every element
 * slides to the front of its chain, no early-stop comparison). Summing
 * over the chains:
 *
 *     P1 = s * C(q+1, 2) + (g - s) * C(q, 2)
 *        = q * (g*q - g + 2*s) / 2.
 *
 * ======================================================================
 * P2 : the gap-1 (insertion sort) pass on a g-sorted array
 * ======================================================================
 *
 * For this insertion sort the comparison count obeys the identity
 *
 *     cost = (number of inversions in the array)
 *          + (N - L),
 *
 * where L = number of "left-to-right minima" (positions i whose value is
 * smaller than everything before them). Intuition: each inversion is one
 * element sliding past one larger element (one comparison), and every
 * element that does NOT slide all the way to position 0 pays exactly one
 * extra "stopping" comparison. An element slides all the way to 0 exactly
 * when it is a new running minimum, i.e. a left-to-right minimum; there
 * are L of those, so N - L elements pay the extra comparison.
 *
 * So maximizing pass 2 means maximizing  inv + N - L  over all g-sorted
 * arrays. This has two parts: maximize inversions, and (independently as
 * far as the leading behavior goes) make L small.
 *
 * ----------------------------------------------------------------------
 * invmax : the most inversions a g-sorted array can have
 * ----------------------------------------------------------------------
 *
 * Within a chain there are no inversions (each chain is sorted). All
 * inversions are between two different chains. For any pair of chains the
 * inversions are maximized by making one chain's values uniformly larger
 * than the other's. You can satisfy "smaller residue gets the larger
 * values" for every pair at once (a single consistent ranking of the
 * chains by value), and this attains the per-pair maximum for all pairs
 * simultaneously. Carefully counting (chains differ in size by at most 1)
 * collapses to:
 *
 *     invmax = (q+1) * ( g*(g-1)*q + 2*s*(s-1) ) / 4.
 *
 * ----------------------------------------------------------------------
 * L : the fewest left-to-right minima we can get away with
 * ----------------------------------------------------------------------
 *
 * All left-to-right minima sit in the first g positions: position r holds
 * the minimum of chain r, and once the global minimum has appeared no
 * later position can be a new running minimum. So L equals the number of
 * left-to-right minima of the sequence of chain-minima.
 *
 * The "max inversions" ranking forces the s equal-size big chains
 * (residues 0..s-1) into a strictly decreasing run of minima, which is s
 * left-to-right minima no matter what. But the relative order of the big
 * group vs the small group is free, so we can push the global minimum to
 * appear right after them and contribute nothing more. That gives:
 *
 *     L = s    when s >= 1,
 *     L = g    when s == 0   (all chains equal size: the chain minima are
 *                             forced into a full strictly-decreasing run,
 *                             so all g of them are left-to-right minima).
 *
 * ======================================================================
 * PUTTING IT TOGETHER
 * ======================================================================
 *
 *     W = P1 + invmax + (N - L).
 *
 * Simplifying gives the compact closed form used in worstCaseTwoGap():
 *
 *   if s != 0:
 *     W = ( g*(g+1)*q*q + (g*(g+1) + 2*s*(s+1))*q + 2*s*(s-1) ) / 4
 *
 *   if s == 0:
 *     W = ( g*(g+1)*q*q + g*(g+1)*q ) / 4  -  g
 *
 * The leading term is about ((g+1)/(4*g)) * N*N, so each extra gap value
 * shaves the worst-case constant from insertion sort's 1/2 down toward
 * 1/4 as g grows, but it is always Theta(N*N).
 */

#include <stdio.h>

typedef long long I64;

/* ------------------------------------------------------------------ */
/* The three component pieces, exposed separately so the breakdown in  */
/* the comments above is directly checkable in code.                   */
/* ------------------------------------------------------------------ */

/* P1: worst-case cost of the gap-g pass. */
I64 passOneCost(I64 N, I64 g) {
    I64 q = N / g;
    I64 s = N % g;
    return q * (g * q - g + 2 * s) / 2;
}

/* invmax: most inversions possible in a g-sorted array of size N. */
I64 maxInversions(I64 N, I64 g) {
    I64 q = N / g;
    I64 s = N % g;
    return (q + 1) * (g * (g - 1) * q + 2 * s * (s - 1)) / 4;
}

/* L: fewest left-to-right minima achievable while keeping max inversions. */
I64 minLeftToRightMinima(I64 N, I64 g) {
    I64 s = N % g;
    return (s == 0) ? g : s;
}

/* ------------------------------------------------------------------ */
/* The headline function: worst-case comparisons for {1, g} on size N. */
/* ------------------------------------------------------------------ */
I64 worstCaseTwoGap(I64 N, I64 g) {
    if (N <= 1) {
        return 0;  /* nothing to sort */
    }

    /* If g >= N the gap-g pass does nothing and it is plain insertion
     * sort, whose worst case is N*(N-1)/2. Our formula already produces
     * this when g >= N (then q = 0, s = N), but we keep the model honest
     * by noting it explicitly. */

    I64 q = N / g;
    I64 s = N % g;

    I64 numerator = g * (g + 1) * q * q
                  + (g * (g + 1) + 2 * s * (s + 1)) * q
                  + 2 * s * (s - 1);
    I64 W = numerator / 4;
    if (s == 0) {
        W -= g;
    }
    return W;

    /* Equivalent, written as the explicit sum of the three pieces:
     *
     *   return passOneCost(N, g)
     *        + maxInversions(N, g)
     *        + (N - minLeftToRightMinima(N, g));
     */
}

int main(void) {
    /* Demonstrate the function and show the breakdown for a few gaps. */
    I64 gaps[] = {2, 3, 4, 5};
    int numGaps = (int)(sizeof(gaps) / sizeof(gaps[0]));

    for (int gi = 0; gi < numGaps; gi++) {
        I64 g = gaps[gi];
        printf("Worst case for gap sequence {1, %lld}:\n", g);
        printf("  N    W      (P1  + invmax + (N - L))\n");
        for (I64 N = 2; N <= 16; N++) {
            I64 W      = worstCaseTwoGap(N, g);
            I64 P1     = passOneCost(N, g);
            I64 inv    = maxInversions(N, g);
            I64 L      = minLeftToRightMinima(N, g);
            I64 P2     = inv + (N - L);
            printf("  %-4lld %-6lld (%-3lld + %-5lld + %lld)  %s\n",
                   N, W, P1, inv, N - L,
                   (W == P1 + P2) ? "" : "<-- mismatch!");
        }
        printf("\n");
    }

    return 0;
}
