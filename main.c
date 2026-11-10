
#define _POSIX_C_SOURCE 199309L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define PIVOT_SEED 2024u
#define REPS 21

// ---------------- Global Variables ----------------
long long g_cmp = 0;             // Number of comparisons
int g_depth = 0;                 // Current recursion depth
int g_maxDepth = 0;              // Maximum recursion depth
unsigned int g_pivotSeed = PIVOT_SEED;
int *g_tmpBuf = NULL;            // Temporary buffer for Merge Sort
int g_mode = 0;                  // 0: FIRST, 1: RANDOM, 2: MEDIAN3

// ---------------- Common Functions ----------------
int lessThan(int x, int y) {
    g_cmp++;
    return x < y;
}

unsigned int xorshift32(unsigned int *s) {
    unsigned int x = *s;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    *s = x;
    return x;
}

static void *xmalloc(size_t bytes) {
    void *p = malloc(bytes);

    if (p == NULL) {
        fprintf(stderr,
                "Error: Unable to allocate %zu bytes of memory.\n",
                bytes);
        exit(EXIT_FAILURE);
    }

    return p;
}

// ---------------- Algorithm Function Prototypes ----------------
void runHeap(int *a, int n);
void runMerge(int *a, int n);
void runQuick(int *a, int n);

size_t memHeap(int n);
size_t memMerge(int n);
size_t memQuick(int n);

// ---------------- Result Structures ----------------
typedef struct {
    long long cmp;
    double ms;
    size_t mem;
    int ok;
} Result;

typedef struct {
    const char *name;
    Result r;
} Row;

// ---------------- Comparison Functions ----------------
static int cmpInt(const void *p, const void *q) {
    int x = *(const int *)p;
    int y = *(const int *)q;

    return (x > y) - (x < y);
}

static int cmpDouble(const void *p, const void *q) {
    double x = *(const double *)p;
    double y = *(const double *)q;

    return (x > y) - (x < y);
}

// ---------------- Time Measurement ----------------
static double nowMs(void) {
    struct timespec ts;

    clock_gettime(CLOCK_MONOTONIC, &ts);

    return ts.tv_sec * 1000.0 + ts.tv_nsec / 1e6;
}

// ---------------- Performance Measurement ----------------
// Run the algorithm 21 times and use the median execution time.
static Result measure(
    const int *src,
    const int *ref,
    int n,
    void (*run)(int *, int),
    size_t (*memFn)(int)
) {
    int *a = (int *)xmalloc((size_t)n * sizeof(int));

    double times[REPS];

    Result r = {0, 0.0, 0, 1};

    for (int rep = 0; rep < REPS; rep++) {

        // Copy the original data. Copying time is excluded.
        memcpy(a, src, (size_t)n * sizeof(int));

        // Reset measurement variables.
        g_cmp = 0;
        g_depth = 0;
        g_maxDepth = 0;
        g_pivotSeed = PIVOT_SEED;

        // Start measuring execution time.
        double t0 = nowMs();

        run(a, n);

        double t1 = nowMs();

        times[rep] = t1 - t0;

        // Store the comparison count and memory usage.
        r.cmp = g_cmp;
        r.mem = memFn(n);

        // Verify the sorting result.
        if (memcmp(a, ref, (size_t)n * sizeof(int)) != 0) {
            r.ok = 0;
        }
    }

    // Sort execution times to calculate the median.
    qsort(times, REPS, sizeof(double), cmpDouble);

    r.ms = times[REPS / 2];

    free(a);

    return r;
}

// ---------------- Main Function ----------------
int main(void) {

    const int sizes[] = {
        1000, 2000, 4000, 8000, 16000, 20000
    };

    const int numSizes =
        (int)(sizeof(sizes) / sizeof(sizes[0]));

    unsigned int dataSeed = 12345u;

    for (int s = 0; s < numSizes; s++) {

        int n = sizes[s];

        int *src = (int *)xmalloc((size_t)n * sizeof(int));
        int *ref = (int *)xmalloc((size_t)n * sizeof(int));

        g_tmpBuf = (int *)xmalloc((size_t)n * sizeof(int));

        // Generate random integers from 0 to 1,000,000.
        for (int i = 0; i < n; i++) {
            src[i] = (int)(xorshift32(&dataSeed) % 1000001u);
        }

        // Create the reference array using qsort.
        memcpy(ref, src, (size_t)n * sizeof(int));

        qsort(ref, (size_t)n, sizeof(int), cmpInt);

        // Store the results of the five sorting algorithms.
        Row rows[5];

        rows[0].name = "Heap Sort";
        rows[0].r = measure(src, ref, n, runHeap, memHeap);

        rows[1].name = "Merge Sort";
        rows[1].r = measure(src, ref, n, runMerge, memMerge);

        g_mode = 0;
        rows[2].name = "Quick (First)";
        rows[2].r = measure(src, ref, n, runQuick, memQuick);

        g_mode = 1;
        rows[3].name = "Quick (Random)";
        rows[3].r = measure(src, ref, n, runQuick, memQuick);

        g_mode = 2;
        rows[4].name = "Quick (Median3)";
        rows[4].r = measure(src, ref, n, runQuick, memQuick);

        // Print the performance results.
        printf("Data Size : %d\n\n", n);

        printf("%-17s%-15s%-12s%-14s\n",
               "Algorithm",
               "Comparisons",
               "Time(ms)",
               "Memory(Bytes)");

        printf("--------------------------------------------------------\n");

        for (int i = 0; i < 5; i++) {

            printf("%-17s%-15lld%-12.4f%-14zu%s\n",
                   rows[i].name,
                   rows[i].r.cmp,
                   rows[i].r.ms,
                   rows[i].r.mem,
                   rows[i].r.ok ? "" : "  [SORT ERROR]");
        }

        printf("\n");

        // Free allocated memory.
        free(src);
        free(ref);
        free(g_tmpBuf);

        g_tmpBuf = NULL;
    }

    return 0;
}
