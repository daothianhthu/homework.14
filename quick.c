// Quick Sort with First, Random and Median-of-Three Pivot
#include <stddef.h>

extern long long g_cmp;
extern int g_depth;
extern int g_maxDepth;
extern unsigned int g_pivotSeed;
extern int g_mode;

#define FIRST 0
#define RANDOM 1
#define MEDIAN3 2

extern int lessThan(int x, int y);
extern unsigned int xorshift32(unsigned int *s);

static inline void swapInt(int *x, int *y) {
    int t = *x;
    *x = *y;
    *y = t;
}

static void quickSort(int *a, int lo, int hi) {
    if (lo >= hi) return;

    if (++g_depth > g_maxDepth) {
        g_maxDepth = g_depth;
    }

    int m = lo;
    if (g_mode == RANDOM) {
        m = lo + (int)(xorshift32(&g_pivotSeed) % (unsigned)(hi - lo + 1));
    } else if (g_mode == MEDIAN3) {
        int mid = lo + (hi - lo) / 2;

        if (lessThan(a[lo], a[mid])) {
            if (lessThan(a[mid], a[hi])) {
                m = mid;
            } else if (lessThan(a[lo], a[hi])) {
                m = hi;
            } else {
                m = lo;
            }
        } else {
            if (lessThan(a[lo], a[hi])) {
                m = lo;
            } else if (lessThan(a[mid], a[hi])) {
                m = hi;
            } else {
                m = mid;
            }
        }
    }

    swapInt(&a[lo], &a[m]);
    int pivot = a[lo];
    int i = lo;

    for (int j = lo + 1; j <= hi; j++) {
        if (lessThan(a[j], pivot)) {
            swapInt(&a[++i], &a[j]);
        }
    }
    swapInt(&a[lo], &a[i]);

    quickSort(a, lo, i - 1);
    quickSort(a, i + 1, hi);
    g_depth--;
}

void runQuick(int *a, int n) {
    quickSort(a, 0, n - 1);
}


#define QUICK_FRAME 40

size_t memQuick(int n) {
    (void)n;
    return (size_t)(g_maxDepth + 1) * QUICK_FRAME;
}
