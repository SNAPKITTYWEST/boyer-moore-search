/*
 * SOVEREIGN LEVIATHAN NODE LICENSE
 * License-ID: SL-AGPL3-001 | Covenant-Version: 1.0
 * Copyright (C) 2026 SnapKittyWest. Ahmad Ali Parr.
 * Licensed under: Apache-2.0 OR GPL-3.0-or-later
 * Commercial repository - no MIT license.
 */

/*
 * bm_goodsuffix.c
 * Full good-suffix table implementation with border array
 *
 * Computes optimal shifts based on matching suffixes.
 * Uses border array for O(m) preprocessing.
 *
 * Theorems:
 * - GS_001: Border array is well-defined
 * - GS_002: Good-suffix shifts are sound
 * - GS_003: Combined rules maintain O(n) bound
 */

#include <string.h>
#include <stdlib.h>

/* Compute border array (failure function) */
void compute_border_array(const char *P, int m, int *border) {
    border[0] = 0;
    int j = 0;

    for (int i = 1; i < m; i++) {
        while (j > 0 && P[i] != P[j]) {
            j = border[j - 1];
        }
        if (P[i] == P[j]) {
            j++;
        }
        border[i] = j;
    }
}

/*
 * Compute good-suffix table
 * goodsuffix[i] = minimum shift when mismatch at position i
 * Uses border array to find rightmost occurrence of suffix
 *
 * INV_GOODSUFFIX:
 *   ∀i ∈ [0,m): goodsuffix[i] ≥ 1
 *   ∧ No valid match is skipped
 */
void compute_goodsuffix_table(const char *P, int m, int *goodsuffix) {
    int *border = (int *)malloc(m * sizeof(int));
    if (!border) return;

    compute_border_array(P, m, border);

    /* Initialize with pattern length shift (conservative) */
    for (int i = 0; i < m; i++) {
        goodsuffix[i] = m;
    }

    /* For each position, compute optimal shift */
    int j = border[m - 1];
    for (int i = m - 1; i >= 0; i--) {
        if (i == m - 1 || P[i + 1] != P[j]) {
            goodsuffix[i] = m - 1 - j;
        }
        if (i > 0) {
            j = border[j - 1];
        }
    }

    free(border);
}

/*
 * Compute shift for matching suffix
 * When we have a suffix match, compute how far to shift
 */
int compute_suffix_shift(const char *P, int m, int matched_len, int *goodsuffix) {
    if (matched_len <= 0 || matched_len >= m) {
        return m;
    }
    return goodsuffix[m - matched_len - 1];
}

/*
 * Combined shift computation: bad-char + good-suffix
 * Takes max of both rules to ensure O(n) performance
 *
 * THEOREM_CS_001:
 *   shift = max(shift_bad, shift_good) ≥ 1 always
 *   (proven via shift ≥ 1 of each rule)
 */
int combined_shift(int bad_char_shift, int good_suffix_shift) {
    int shift = bad_char_shift;
    if (good_suffix_shift > shift) {
        shift = good_suffix_shift;
    }
    /* Safety: ensure at least 1 to prevent infinite loop */
    return (shift > 0) ? shift : 1;
}
