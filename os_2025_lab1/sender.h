#ifndef IPC_COMMON_H
#define IPC_COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <semaphore.h>
#include <time.h>

#define MSG_PASSING 1
#define SHARED_MEM 2

#define MQ_PATH "."
#define MQ_PROJ 65

#define SEM_EMPTY "/sem_empty"
#define SEM_FULL  "/sem_full"

typedef struct {
    int flag;      // 1 for message passing, 2 for shared memory
    union{
        int msqid; //for system V api. You can replace it with structure for POSIX api
        char* shm_addr;
    } storage;
} mailbox_t;

typedef struct {
    long mType; // 1 for char*
    char msgText[1024];
} message_t;

// Pass message_t * instead of copying entire struct
void mailbox_send(const message_t* message, mailbox_t* mailbox_ptr);

#endif