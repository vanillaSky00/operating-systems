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
                   // or for shm_id
        char* shm_addr;
    }storage;
} mailbox_t;


typedef struct {
    /*  TODO: 
        Message structure for wrapper
    */
    long mType;
    char msgText[1024];
} message_t;

void mailbox_receive(message_t* msg, mailbox_t* mb);

#endif