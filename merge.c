// Merge Sort
#include <stddef.h>

extern long long g_cmp;
extern int g_depth;
extern int g_maxDepth;
extern int *g_tmpBuf;

// Merge Sort_top-down.
static void mergeSort(int *a, int *tmp, int lo, int hi) {
    if (lo >= hi) return;

    if (++g_depth > g_maxDepth) {
        g_maxDepth = g_depth;
    }

    int mid = lo + (hi - lo) / 2;
    mergeSort(a, tmp, lo, mid);
    mergeSort(a, tmp, mid + 1, hi);

    int i = lo;
    int j = mid + 1;
    int k = lo;

    while (i <= mid && j <= hi) {
        g_cmp++;
        if (a[i] <= a[j]) {
            tmp[k++] = a[i++];
        } else {
            tmp[k++] = a[j++];
        }
    }

    while (i <= mid) tmp[k++] = a[i++];
    while (j <= hi) tmp[k++] = a[j++];

    for (k = lo; k <= hi; k++) {
        a[k] = tmp[k];
    }

    g_depth--;
}

void runMerge(int *a, int n) {
    mergeSort(a, g_tmpBuf, 0, n - 1);
}


#define MERGE_FRAME 48

size_t memMerge(int n) {
    return (size_t)n * sizeof(int)
         + (size_t)(g_maxDepth + 1) * MERGE_FRAME;
}
