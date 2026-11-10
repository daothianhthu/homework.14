
#define _POSIX_C_SOURCE 199309L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>


static long long g_cmp = 0;      
static int g_depth = 0;          
static int g_maxDepth = 0;      
static unsigned int g_pivotSeed = 2024;
static int *g_tmpBuf = NULL;    


static unsigned int xorshift32(unsigned int *s) {
    unsigned int x = *s;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    return *s = x;
}

static inline void swapInt(int *x, int *y) { int t = *x; *x = *y; *y = t; }


static inline int lessThan(int x, int y) { g_cmp++; return x < y; }

// ---------------- stack frame (64bit, byte) ----------------
#define RET 8
#define HEAP_FRAME  ((8 + 4) + 4 + RET + (8 + 4 + 4) + 4 + 4 + RET)  // heapSort + siftDown
#define QUICK_FRAME ((8 + 4 + 4) + 4 * 4 + RET)                      // = 40
#define MERGE_FRAME ((8 + 8 + 4 + 4) + 4 * 4 + RET)                  // = 48

// ---------------- 1. Heap sort (max heap) ----------------
static void siftDown(int *a, int root, int size) {
    int tmp = a[root];
    while (1) {
        int child = 2 * root + 1;
        if (child >= size) break;
        if (child + 1 < size && lessThan(a[child], a[child + 1])) child++; // chọn con lớn hơn
        if (!lessThan(tmp, a[child])) break;                               // thỏa tính chất heap
        a[root] = a[child];
        root = child;
    }
    a[root] = tmp;
}

static void heapSort(int *a, int n) {
    for (int i = n / 2 - 1; i >= 0; i--) siftDown(a, i, n);  
    for (int i = n - 1; i > 0; i--) {
        swapInt(&a[0], &a[i]);                               
        siftDown(a, 0, i);                                   
    }
}

// ---------------- 2. Merge sort (top-down) ----------------
static void mergeSort(int *a, int *tmp, int lo, int hi) {
    if (lo >= hi) return;
    if (++g_depth > g_maxDepth) g_maxDepth = g_depth;
    int mid = lo + (hi - lo) / 2;
    mergeSort(a, tmp, lo, mid);
    mergeSort(a, tmp, mid + 1, hi);
    int i = lo, j = mid + 1, k = lo;
    while (i <= mid && j <= hi) {
        g_cmp++;
        if (a[i] <= a[j]) tmp[k++] = a[i++];
        else              tmp[k++] = a[j++];
    }
    while (i <= mid) tmp[k++] = a[i++];
    while (j <= hi)  tmp[k++] = a[j++];
    for (k = lo; k <= hi; k++) a[k] = tmp[k];
    g_depth--;
}

// ---------------- 3. Quick sort ( pivot) ----------------
typedef enum { FIRST, RANDOM, MEDIAN3 } PivotMode;
static PivotMode g_mode = FIRST;

static void quickSort(int *a, int lo, int hi) {
    if (lo >= hi) return;
    if (++g_depth > g_maxDepth) g_maxDepth = g_depth;

    int m = lo;
    if (g_mode == RANDOM) {
        m = lo + (int)(xorshift32(&g_pivotSeed) % (unsigned)(hi - lo + 1)); // không so sánh
    } else if (g_mode == MEDIAN3) {
        int mid = lo + (hi - lo) / 2;
        if (lessThan(a[lo], a[mid])) {
            if (lessThan(a[mid], a[hi]))      m = mid;
            else if (lessThan(a[lo], a[hi]))  m = hi;
            else                              m = lo;
        } else {
            if (lessThan(a[lo], a[hi]))       m = lo;
            else if (lessThan(a[mid], a[hi])) m = hi;
            else                              m = mid;
        }
    }
    swapInt(&a[lo], &a[m]);
    int pivot = a[lo];

    int i = lo;
    for (int j = lo + 1; j <= hi; j++)
        if (lessThan(a[j], pivot)) swapInt(&a[++i], &a[j]);
    swapInt(&a[lo], &a[i]);

    quickSort(a, lo, i - 1);
    quickSort(a, i + 1, hi);
    g_depth--;
}


static void runHeap(int *a, int n)  { heapSort(a, n); }
static void runMerge(int *a, int n) { mergeSort(a, g_tmpBuf, 0, n - 1); }
static void runQuick(int *a, int n) { quickSort(a, 0, n - 1); }

static size_t memHeap(int n)  { (void)n; return HEAP_FRAME; }  
static size_t memMerge(int n) { return (size_t)n * sizeof(int) + (size_t)g_maxDepth * MERGE_FRAME; }
static size_t memQuick(int n) { (void)n; return (size_t)g_maxDepth * QUICK_FRAME; }


typedef struct { long long cmp; double ms; size_t mem; int ok; } Result;
typedef struct { const char *name; Result r; } Row;

static int cmpInt(const void *p, const void *q) {
    int x = *(const int *)p, y = *(const int *)q;
    return (x > y) - (x < y);
}

static double nowMs(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000.0 + ts.tv_nsec / 1e6;
}

static Result measure(const int *src, const int *ref, int n,
                      void (*run)(int *, int), size_t (*memFn)(int)) {
    int *a = (int *)malloc((size_t)n * sizeof(int));   
    memcpy(a, src, (size_t)n * sizeof(int));
    g_cmp = 0; g_depth = 0; g_maxDepth = 0;

    double t0 = nowMs();
    run(a, n);
    double t1 = nowMs();

    Result r;
    r.cmp = g_cmp;
    r.ms  = t1 - t0;
    r.mem = memFn(n);
    r.ok  = (memcmp(a, ref, (size_t)n * sizeof(int)) == 0);  
    free(a);
    return r;
}

int main(void) {
    const int sizes[] = {1000, 2000, 4000, 8000, 16000, 20000};
    const int numSizes = (int)(sizeof(sizes) / sizeof(sizes[0]));
    unsigned int seed = 12345;

    for (int s = 0; s < numSizes; s++) {
        int n = sizes[s];
        int *src = (int *)malloc((size_t)n * sizeof(int));
        int *ref = (int *)malloc((size_t)n * sizeof(int));
        g_tmpBuf = (int *)malloc((size_t)n * sizeof(int)); 

        for (int i = 0; i < n; i++) src[i] = (int)(xorshift32(&seed) % 1000001u);  
        memcpy(ref, src, (size_t)n * sizeof(int));
        qsort(ref, (size_t)n, sizeof(int), cmpInt);

        Row rows[5];
        rows[0].name = "Heap Sort";
        rows[0].r = measure(src, ref, n, runHeap, memHeap);
        rows[1].name = "Merge Sort";
        rows[1].r = measure(src, ref, n, runMerge, memMerge);
        g_mode = FIRST;
        rows[2].name = "Quick (First)";
        rows[2].r = measure(src, ref, n, runQuick, memQuick);
        g_mode = RANDOM;
        rows[3].name = "Quick (Random)";
        rows[3].r = measure(src, ref, n, runQuick, memQuick);
        g_mode = MEDIAN3;
        rows[4].name = "Quick (Median3)";
        rows[4].r = measure(src, ref, n, runQuick, memQuick);

        printf("Data Size : %d\n\n", n);
        printf("%-17s%-15s%-12s%-14s\n", "Algorithm", "Comparisons", "Time(ms)", "Memory(Bytes)");
        printf("--------------------------------------------------------\n");
        for (int i = 0; i < 5; i++) {
            printf("%-17s%-15lld%-12.4f%-14zu%s\n", rows[i].name, rows[i].r.cmp, rows[i].r.ms,
                   rows[i].r.mem, rows[i].r.ok ? "" : "  [SORT ERROR]");
        }
        printf("\n");

        free(src); free(ref); free(g_tmpBuf);
    }
    return 0;
}