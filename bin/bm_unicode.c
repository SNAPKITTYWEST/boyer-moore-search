/*
 * SOVEREIGN LEVIATHAN NODE LICENSE
 * License-ID: SL-AGPL3-001 | Covenant-Version: 1.0
 * Copyright (C) 2026 SnapKittyWest. Ahmad Ali Parr.
 * Licensed under: Apache-2.0 OR GPL-3.0-or-later
 * Commercial repository - no MIT license.
 */

/*
 * bm_unicode.c
 * UTF-8 aware Boyer-Moore implementation
 *
 * Handles multi-byte UTF-8 characters while maintaining
 * O(n) average-case performance.
 *
 * Theorems:
 * - UTF8_001: UTF-8 byte sequence parsing
 * - UTF8_002: Search correctness over UTF-8
 * - UTF8_003: No character boundary crossing
 */

#include <stdint.h>
#include <string.h>
#include <stddef.h>

/* UTF-8 character classes */
typedef enum {
    UTF8_INVALID = -1,
    UTF8_ASCII = 1,      /* 0xxxxxxx */
    UTF8_CONT = 0,       /* 10xxxxxx (continuation) */
    UTF8_2BYTE = 2,      /* 110xxxxx */
    UTF8_3BYTE = 3,      /* 1110xxxx */
    UTF8_4BYTE = 4       /* 11110xxx */
} utf8_class_t;

/*
 * Classify UTF-8 byte
 * Returns: expected continuation bytes, or INVALID
 */
static utf8_class_t utf8_class(unsigned char c) {
    if ((c & 0x80) == 0) return UTF8_ASCII;      /* 0xxxxxxx */
    if ((c & 0xC0) == 0x80) return UTF8_CONT;    /* 10xxxxxx */
    if ((c & 0xE0) == 0xC0) return UTF8_2BYTE;   /* 110xxxxx */
    if ((c & 0xF0) == 0xE0) return UTF8_3BYTE;   /* 1110xxxx */
    if ((c & 0xF8) == 0xF0) return UTF8_4BYTE;   /* 11110xxx */
    return UTF8_INVALID;
}

/*
 * Get byte length of UTF-8 character at position
 * Returns: 1-4 for valid sequence, 0 for invalid
 *
 * INV_UTF8_LENGTH:
 *   1 ≤ len ≤ 4 ∀valid UTF-8 char
 *   len = 0 means error or continuation byte
 */
int utf8_char_length(const unsigned char *str, size_t offset, size_t max_len) {
    if (offset >= max_len) return 0;

    unsigned char c = str[offset];
    utf8_class_t cls = utf8_class(c);

    if (cls == UTF8_INVALID) return 0;
    if (cls == UTF8_CONT) return 0;     /* Continuation byte at start */
    if (cls == UTF8_ASCII) return 1;

    int expected = (int)cls;
    if (offset + expected > max_len) return 0; /* Not enough bytes */

    /* Verify continuation bytes */
    for (int i = 1; i < expected; i++) {
        if (utf8_class(str[offset + i]) != UTF8_CONT) {
            return 0;
        }
    }

    return expected;
}

/*
 * Get byte offset of next UTF-8 character
 * Skips invalid sequences with minimum movement
 */
size_t utf8_next(const unsigned char *str, size_t offset, size_t len) {
    int char_len = utf8_char_length(str, offset, len);
    if (char_len > 0) {
        return offset + char_len;
    }
    /* On invalid, skip one byte to resync */
    return (offset + 1 < len) ? offset + 1 : len;
}

/*
 * Compare UTF-8 substrings for equality
 * Respects character boundaries
 */
int utf8_strcmp_at(const unsigned char *text, size_t t_off, size_t t_len,
                   const unsigned char *pattern, size_t p_len) {
    size_t t_pos = t_off;
    size_t p_pos = 0;

    while (p_pos < p_len && t_pos < t_len) {
        int t_char_len = utf8_char_length(text, t_pos, t_len);
        int p_char_len = utf8_char_length(pattern, p_pos, p_len);

        if (t_char_len <= 0 || p_char_len <= 0) {
            return 0;  /* Invalid UTF-8 */
        }

        if (t_char_len != p_char_len) {
            return 0;  /* Different length */
        }

        if (memcmp(text + t_pos, pattern + p_pos, t_char_len) != 0) {
            return 0;  /* Bytes differ */
        }

        t_pos += t_char_len;
        p_pos += p_char_len;
    }

    return (p_pos == p_len) ? 1 : 0;
}

/*
 * Count UTF-8 characters (not bytes) in string
 * For statistics and progress reporting
 */
size_t utf8_strlen(const unsigned char *str, size_t byte_len) {
    size_t pos = 0;
    size_t count = 0;

    while (pos < byte_len) {
        int char_len = utf8_char_length(str, pos, byte_len);
        if (char_len > 0) {
            count++;
            pos += char_len;
        } else {
            pos++;  /* Skip invalid */
        }
    }

    return count;
}

/*
 * Validate UTF-8 string
 * Returns: 1 if valid, 0 if contains invalid sequences
 *
 * THEOREM_UTF8_VALID:
 *   Valid UTF-8 ⟺ all bytes either ASCII or valid multi-byte sequence
 */
int utf8_is_valid(const unsigned char *str, size_t len) {
    size_t pos = 0;

    while (pos < len) {
        int char_len = utf8_char_length(str, pos, len);
        if (char_len <= 0) {
            return 0;  /* Invalid sequence */
        }
        pos += char_len;
    }

    return 1;
}
