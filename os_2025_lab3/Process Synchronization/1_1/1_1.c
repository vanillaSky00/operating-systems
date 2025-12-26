#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h> // strerror

volatile int a = 0;
static pthread_spinlock_t lock;

static void die_pthread(const char *msg, int err) {
    fprintf(stderr, "%s: %s\n", msg, strerror(err));
    exit(1);
}

void *thread_fn(void *arg) {
    (void)arg;

    int ret = pthread_spin_lock(&lock);
    if (ret != 0) die_pthread("pthread_spin_lock", ret);

    for (int i = 0; i < 10000; i++) a = a + 1;
    
    ret = pthread_spin_unlock(&lock);
    if (ret != 0) die_pthread("pthread_spin_unlock", ret);

    return NULL;
}

int main(void) {
    FILE *fptr = fopen("1.txt", "a");
    if (!fptr) {
        perror("fopen");
        return 1;
    }

    pthread_t t1, t2;

    int ret = pthread_spin_init(&lock, PTHREAD_PROCESS_PRIVATE);
    if (ret != 0) die_pthread("pthread_spin_init", ret);

    ret = pthread_create(&t1, NULL, thread_fn, NULL);
    if (ret != 0) die_pthread("pthread_create t1", ret);

    ret = pthread_create(&t2, NULL, thread_fn, NULL);
    if (ret != 0) die_pthread("pthread_create t2", ret);

    ret = pthread_join(t1, NULL);
    if (ret != 0) die_pthread("pthread_join t1", ret);

    ret = pthread_join(t2, NULL);
    if (ret != 0) die_pthread("pthread_join t2", ret);

    ret = pthread_spin_destroy(&lock);
    if (ret != 0) die_pthread("pthread_spin_destroy", ret);

    fprintf(fptr, "%d ", a); // should be 20000
    fclose(fptr);
    return 0;
}
