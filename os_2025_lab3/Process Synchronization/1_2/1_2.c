#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>

#define LOCK   0
#define UNLOCK 1

static int a = 0;

/* 4-byte aligned is good practice for exclusive ops */
static volatile int g_lock __attribute__((aligned(4))) = UNLOCK;

static inline void spin_lock(volatile int *lockp) {
    int tmp, st;

    __asm__ volatile(
        "1:\n"
        "   ldaxr   %w0, [%2]\n"        // tmp = *lockp   (acquire)
        "   cmp     %w0, %w3\n"         // tmp == UNLOCK ?
        "   b.eq    2f\n"               // yes -> try to take it
        "   yield\n"                    // hint to CPU: we're spinning
        "   b       1b\n"               // keep spinning

        "2:\n"
        "   mov     %w0, %w4\n"         // tmp = LOCK (0)
        "   stxr    %w1, %w0, [%2]\n"   // st = store-exclusive(lock=0)
        "   cbnz    %w1, 1b\n"          // if failed, retry
        : "=&r"(tmp), "=&r"(st)         // %0, %1 outputs
        : "r"(lockp), "r"(UNLOCK), "r"(LOCK)  // %2, %3, %4 inputs
        : "cc", "memory"
    );
}

static inline void spin_unlock(volatile int *lockp) {
    __asm__ volatile(
        "stlr %w1, [%0]\n"              // *lockp = UNLOCK (release)
        :
        : "r"(lockp), "r"(UNLOCK)
        : "memory"
    );
}

static void *thread_fn(void *arg) {
    (void)arg;
    for (int i = 0; i < 10000; i++) {
        spin_lock(&g_lock);
        a = a + 1;
        spin_unlock(&g_lock);
    }
    return NULL;
}

int main(void) {
    pthread_t t1, t2;

    if (pthread_create(&t1, NULL, thread_fn, NULL) != 0) {
        perror("pthread_create t1");
        return 1;
    }
    if (pthread_create(&t2, NULL, thread_fn, NULL) != 0) {
        perror("pthread_create t2");
        return 1;
    }

    pthread_join(t1, NULL);
    pthread_join(t2, NULL);

    FILE *fptr = fopen("1.txt", "a");
    if (!fptr) {
        perror("fopen");
        return 1;
    }
    fprintf(fptr, "%d ", a);
    fclose(fptr);

    return 0;
}
