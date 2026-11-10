// Heap Sort 
#include <stddef.h>

extern int lessThan(int x, int y);

static inline void swapInt(int *x, int *y) {
    int t = *x;
    *x = *y;
    *y = t;
}


static void siftDown(int *a, int root, int size) {
    int tmp = a[root];

    while (1) {
        int child = 2 * root + 1;
        if (child >= size) break;

        if (child + 1 < size && lessThan(a[child], a[child + 1])) {
            child++;
        }
        if (!lessThan(tmp, a[child])) break;

        a[root] = a[child];
        root = child;
    }
    a[root] = tmp;
}

void runHeap(int *a, int n) {
    // Max Heap.
    for (int i = n / 2 - 1; i >= 0; i--) {
        siftDown(a, i, n);
    }

    for (int i = n - 1; i > 0; i--) {
        swapInt(&a[0], &a[i]);
        siftDown(a, 0, i);
    }
}

// stack
#define RET 8
#define HEAPSORT_FRAME ((8 + 4) + 4 + RET) // 24 bytes theo mô hình
#define SIFT_FRAME     ((8 + 4 + 4) + 4 + 4 + RET) // 32 bytes theo mô hình

size_t memHeap(int n) {
    (void)n;
    return HEAPSORT_FRAME + SIFT_FRAME;
}
