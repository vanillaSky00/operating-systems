#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <stdbool.h>
#include <errno.h>

#include "3_2_Config.h"

#define matrix_row_x 1234
#define matrix_col_x 250

#define matrix_row_y 250
#define matrix_col_y 1234

static FILE *fptr1;
static FILE *fptr2;
static FILE *fptr3;
static int **x;
static int **y;
static int **z;

static void data_processing(void){
    int tmp;

    // read header of m1
    fscanf(fptr1, "%d", &tmp);
    fscanf(fptr1, "%d", &tmp);

    for(int i=0; i<matrix_row_x; i++){
        for(int j=0; j<matrix_col_x; j++){
            if (fscanf(fptr1, "%d", &x[i][j]) != 1){
                fprintf(stderr, "Error reading m1.txt at (%d,%d)\n", i, j);
                exit(1);
            }
        }
    }

    // read header of m2
    fscanf(fptr2, "%d", &tmp);
    fscanf(fptr2, "%d", &tmp);

    for(int i=0; i<matrix_row_y; i++){
        for(int j=0; j<matrix_col_y; j++){
            if (fscanf(fptr2, "%d", &y[i][j]) != 1){
                fprintf(stderr, "Error reading m2.txt at (%d,%d)\n", i, j);
                exit(1);
            }
        }
    }
}

/*
 * 3.2 core: write to /proc triggers kernel module proc_write,
 * and re-opening then reading triggers proc_read.
 *
 * We open/close each time because proc files commonly implement:
 *   if (*offset > 0) return 0;   (EOF after first read)
 */
static void proc_write_then_read(const char *msg)
{
    int fd;
    char buf[256];
    ssize_t n;

    // (1) Write message -> triggers module's proc_write
    fd = open("/proc/Mythread_info", O_WRONLY);
    if (fd < 0) {
        fprintf(stderr, "open(/proc/Mythread_info, O_WRONLY) failed: %s\n", strerror(errno));
        return;
    }

    if (write(fd, msg, strlen(msg)) < 0) {
        fprintf(stderr, "write(/proc/Mythread_info) failed: %s\n", strerror(errno));
        close(fd);
        return;
    }
    close(fd);

    // (2) Read back -> triggers module's proc_read
    fd = open("/proc/Mythread_info", O_RDONLY);
    if (fd < 0) {
        fprintf(stderr, "open(/proc/Mythread_info, O_RDONLY) failed: %s\n", strerror(errno));
        return;
    }

    while ((n = read(fd, buf, sizeof(buf) - 1)) > 0) {
        buf[n] = '\0';
        printf("%s", buf);
    }
    if (n < 0) {
        fprintf(stderr, "read(/proc/Mythread_info) failed: %s\n", strerror(errno));
    }

    close(fd);
}

static void *thread1(void *arg){
    (void)arg;

    char msg[64];
    snprintf(msg, sizeof(msg), "Thread 1 says hello!");

#if (THREAD_NUMBER == 1)
    for(int i=0; i<matrix_row_x; i++){
        for(int j=0; j<matrix_col_y; j++){
            for(int k=0; k<matrix_row_y; k++){
                z[i][j] += x[i][k] * y[k][j];
            }
        }
    }
#elif (THREAD_NUMBER == 2)
    for(int i=0; i<matrix_row_x/2; i++){
        for(int j=0; j<matrix_col_y; j++){
            for(int k=0; k<matrix_row_y; k++){
                z[i][j] += x[i][k] * y[k][j];
            }
        }
    }
#endif

    proc_write_then_read(msg);
    return NULL;
}

#if (THREAD_NUMBER == 2)
static void *thread2(void *arg){
    (void)arg;

    char msg[64];
    snprintf(msg, sizeof(msg), "Thread 2 says hello!");

    for(int i=matrix_row_x/2; i<matrix_row_x; i++){
        for(int j=0; j<matrix_col_y; j++){
            for(int k=0; k<matrix_row_y; k++){
                z[i][j] += x[i][k] * y[k][j];
            }
        }
    }

    proc_write_then_read(msg);
    return NULL;
}
#endif

int main(void){
    // allocate x
    x = malloc(sizeof(int*) * matrix_row_x);
    for(int i=0; i<matrix_row_x; i++){
        x[i] = malloc(sizeof(int) * matrix_col_x);
    }

    // allocate y
    y = malloc(sizeof(int*) * matrix_row_y);
    for(int i=0; i<matrix_row_y; i++){
        y[i] = malloc(sizeof(int) * matrix_col_y);
    }

    // allocate z (IMPORTANT: zero-init because you use +=)
    z = malloc(sizeof(int*) * matrix_row_x);
    for(int i=0; i<matrix_row_x; i++){
        z[i] = calloc(matrix_col_y, sizeof(int));
    }

    fptr1 = fopen("m1.txt", "r");
    fptr2 = fopen("m2.txt", "r");
    fptr3 = fopen("3_2.txt", "w");   // overwrite for grading

    if (!fptr1 || !fptr2 || !fptr3) {
        perror("fopen");
        return 1;
    }

    data_processing();
    fprintf(fptr3, "%d %d\n", matrix_row_x, matrix_col_y);

    pthread_t t1;
#if (THREAD_NUMBER == 2)
    pthread_t t2;
#endif

    pthread_create(&t1, NULL, thread1, NULL);
#if (THREAD_NUMBER == 2)
    pthread_create(&t2, NULL, thread2, NULL);
#endif

    pthread_join(t1, NULL);
#if (THREAD_NUMBER == 2)
    pthread_join(t2, NULL);
#endif

    // write output matrix
    for(int i=0; i<matrix_row_x; i++){
        for(int j=0; j<matrix_col_y; j++){
            fprintf(fptr3, "%d ", z[i][j]);
            if (j == matrix_col_y - 1) fprintf(fptr3, "\n");
        }
    }

    fclose(fptr1);
    fclose(fptr2);
    fclose(fptr3);
    return 0;
}