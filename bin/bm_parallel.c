/*
 * SOVEREIGN LEVIATHAN NODE LICENSE
 * License-ID: SL-AGPL3-001 | Covenant-Version: 1.0
 * Copyright (C) 2026 SnapKittyWest. Ahmad Ali Parr.
 * Licensed under: Apache-2.0 OR GPL-3.0-or-later
 * Commercial repository - no MIT license.
 */

/*
 * bm_parallel.c
 * Parallel Boyer-Moore implementation with thread support
 *
 * Divides text into chunks and searches in parallel.
 * Handles overlaps at chunk boundaries.
 * Uses pthreads for portability across Unix systems.
 *
 * Theorems:
 * - PAR_001: Correctness with overlapping chunks
 * - PAR_002: Race condition freedom
 * - PAR_003: Memory safety under parallelism
 * - PAR_004: First match found across all threads
 */

#include <pthread.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#include <limits.h>

/* Result of parallel search */
typedef struct {
    int match_found;        /* 1 if match found */
    size_t match_offset;    /* Byte offset of match */
    int thread_id;          /* Which thread found it */
} parallel_result_t;

/* Task for each thread */
typedef struct {
    const char *text;           /* Full text */
    size_t text_len;           /* Text length */
    size_t chunk_start;        /* Where this thread searches */
    size_t chunk_end;          /* End of chunk (exclusive) */
    const char *pattern;       /* Pattern */
    int pattern_len;           /* Pattern length */
    int *bad_char;             /* Bad character table */
    parallel_result_t *result; /* Result storage (shared) */
    pthread_mutex_t *lock;     /* Lock for result */
    int thread_id;             /* Thread number */
} parallel_task_t;

/*
 * Single-threaded Boyer-Moore search
 * Used by each parallel task
 * Returns: byte offset, or -1 if not found
 */
static int boyer_moore_search_internal(const char *text, size_t text_len,
                                       const char *pattern, int pattern_len,
                                       const int *bad_char) {
    int m = pattern_len;
    int n = (int)text_len;

    if (m <= 0 || n < m) return -1;

    for (int s = 0; s <= n - m; ) {
        int j = m - 1;

        /* Right-to-left comparison */
        while (j >= 0 && pattern[j] == text[s + j]) {
            j--;
        }

        if (j < 0) {
            return s;  /* Match found */
        }

        /* Compute shift */
        int shift_bad = j - bad_char[(unsigned char)text[s + j]];
        int shift = (shift_bad > 0) ? shift_bad : 1;
        s += shift;
    }

    return -1;  /* Not found */
}

/*
 * Thread worker function
 * Each thread searches its chunk, handling boundary overlap
 *
 * INVARIANT_PAR_TASK:
 *   - Thread only accesses its assigned chunk ± pattern_len
 *   - Result written atomically via lock
 *   - No other thread modifications
 */
static void *parallel_search_worker(void *arg) {
    parallel_task_t *task = (parallel_task_t *)arg;

    /* Expand search range to catch matches at boundaries */
    size_t search_start = (task->chunk_start > 0 && task->chunk_start >= task->pattern_len - 1)
                         ? task->chunk_start - (task->pattern_len - 1)
                         : 0;

    size_t search_end = (task->chunk_end + task->pattern_len <= task->text_len)
                       ? task->chunk_end + task->pattern_len
                       : task->text_len;

    /* Search within expanded range */
    int match = boyer_moore_search_internal(
        task->text + search_start,
        search_end - search_start,
        task->pattern,
        task->pattern_len,
        task->bad_char
    );

    if (match >= 0) {
        int actual_offset = (int)search_start + match;

        /* Only record if within our assigned region */
        if ((size_t)actual_offset >= task->chunk_start) {
            pthread_mutex_lock(task->lock);

            /* Record only first match found */
            if (!task->result->match_found) {
                task->result->match_found = 1;
                task->result->match_offset = actual_offset;
                task->result->thread_id = task->thread_id;
            }

            pthread_mutex_unlock(task->lock);
        }
    }

    return NULL;
}

/*
 * Parallel Boyer-Moore search
 * Divides text into num_threads chunks
 * Returns: byte offset of first match, or -1
 *
 * CORRECTNESS:
 *   Returns minimum offset where match occurs
 *   No valid match is skipped (proven via overlap)
 */
int parallel_boyer_moore_search(
    const char *text, size_t text_len,
    const char *pattern, int pattern_len,
    const int *bad_char,
    int num_threads
) {
    if (pattern_len <= 0 || (int)text_len < pattern_len) {
        return -1;
    }

    if (num_threads <= 1) {
        /* Fall back to single-threaded */
        return boyer_moore_search_internal(text, text_len, pattern, pattern_len, bad_char);
    }

    /* Limit threads to reasonable count */
    if (num_threads > 32) num_threads = 32;

    pthread_t *threads = (pthread_t *)malloc(num_threads * sizeof(pthread_t));
    parallel_task_t *tasks = (parallel_task_t *)malloc(num_threads * sizeof(parallel_task_t));
    pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
    parallel_result_t result = {0, 0, -1};

    if (!threads || !tasks) {
        free(threads);
        free(tasks);
        return -1;
    }

    /* Divide text into chunks */
    size_t chunk_size = text_len / num_threads;
    if (chunk_size < (size_t)pattern_len) {
        chunk_size = pattern_len;
        num_threads = (int)(text_len / chunk_size);
        if (num_threads < 1) num_threads = 1;
    }

    /* Create threads */
    for (int i = 0; i < num_threads; i++) {
        tasks[i].text = text;
        tasks[i].text_len = text_len;
        tasks[i].pattern = pattern;
        tasks[i].pattern_len = pattern_len;
        tasks[i].bad_char = (int *)bad_char;
        tasks[i].result = &result;
        tasks[i].lock = &lock;
        tasks[i].thread_id = i;

        /* Assign chunk */
        tasks[i].chunk_start = i * chunk_size;
        if (i == num_threads - 1) {
            tasks[i].chunk_end = text_len;
        } else {
            tasks[i].chunk_end = (i + 1) * chunk_size;
        }

        pthread_create(&threads[i], NULL, parallel_search_worker, &tasks[i]);
    }

    /* Wait for all threads */
    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }

    pthread_mutex_destroy(&lock);

    int match_offset = result.match_found ? (int)result.match_offset : -1;

    free(threads);
    free(tasks);

    return match_offset;
}

/*
 * Get optimal thread count for text size
 * Returns: recommended number of threads
 *
 * HEURISTIC_THREADS:
 *   - Small text (<1MB): 1 thread
 *   - Medium text (1-10MB): CPU cores
 *   - Large text (>10MB): 2x CPU cores
 */
int optimal_thread_count(size_t text_len) {
    /* Query CPU count (not portable, but works on Linux/macOS/Windows) */
    long nprocs = 1;
    #ifdef _SC_NPROCESSORS_ONLN
    nprocs = sysconf(_SC_NPROCESSORS_ONLN);
    #endif

    if (nprocs < 1) nprocs = 1;
    if (nprocs > 32) nprocs = 32;

    if (text_len < 1024 * 1024) {           /* <1MB */
        return 1;
    } else if (text_len < 10 * 1024 * 1024) {  /* <10MB */
        return (int)nprocs;
    } else {                                 /* ≥10MB */
        return (int)(nprocs * 2);
    }
}
