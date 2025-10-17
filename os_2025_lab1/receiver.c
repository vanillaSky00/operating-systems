#include "receiver.h"

int shmid;
struct timespec start, end;
double elapsed = 0.0;

static void mailbox_init(mailbox_t* mb, int mode) {
    mb->flag = mode;
    
    if (mode == MSG_PASSING) {
        key_t key = ftok(MQ_PATH, MQ_PROJ);
        if (key == -1) {
            perror("ftok");
            exit(EXIT_FAILURE);
        }

        int qid = msgget(key, 0666 | IPC_CREAT);
        if (qid == -1) {
            perror("msgget");
            exit(EXIT_FAILURE);
        }

        mb->storage.msqid = qid;
    }

    else if (mode == SHARED_MEM) {
        key_t key = ftok(MQ_PATH, MQ_PROJ + 1);
        if (key == -1) {
            perror("ftok");
            exit(EXIT_FAILURE);
        }

        shmid = shmget(key, sizeof(message_t), 0666 | IPC_CREAT);
        if (shmid == -1) {
            perror("shmget");
            exit(EXIT_FAILURE);
        }

        mb->storage.shm_addr = (char*) shmat(shmid, NULL, 0);
        if (mb->storage.shm_addr == (char*) -1) {
            perror("shmat");
            exit(EXIT_FAILURE);
        }
    }
}

static void mailbox_close(mailbox_t* mb) {
    if (mb->flag == MSG_PASSING) {
        msgctl(mb->storage.msqid, IPC_RMID, NULL);
    }
    else if (mb->flag == SHARED_MEM) {
        shmdt(mb->storage.shm_addr);
        mb->storage.shm_addr = NULL;
        shmctl(shmid, IPC_RMID, NULL);

        // sem_close(sem_open(SEM_EMPTY, 0));
        // sem_close(sem_open(SEM_FULL, 0));

        // Optional cleanup (remove semaphores completely)
    }
    sem_unlink(SEM_EMPTY);
    sem_unlink(SEM_FULL);
}

void mailbox_receive(message_t* msg, mailbox_t* mb){
    sem_t* sem_empty = sem_open(SEM_EMPTY, 0);
    sem_t* sem_full = sem_open(SEM_FULL, 0);

    if (mb->flag == MSG_PASSING) {
        int qid = mb->storage.msqid;
        sem_wait(sem_full);

        clock_gettime(CLOCK_MONOTONIC, &start);
        ssize_t n = msgrcv(qid, msg, sizeof(msg->msgText), 0, 0);
        if (n == -1) {
            perror("msgrcv");
        }
        clock_gettime(CLOCK_MONOTONIC, &end);
        elapsed += (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) * 1e-9;
        
        sem_post(sem_empty); 
    }
    
    else if (mb->flag == SHARED_MEM) {
        sem_wait(sem_full);

        clock_gettime(CLOCK_MONOTONIC, &start);
        message_t *share_msg = (message_t *) mb->storage.shm_addr;
        memcpy(msg, share_msg, sizeof(message_t));
        clock_gettime(CLOCK_MONOTONIC, &end);
        elapsed += (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) * 1e-9; 

        sem_post(sem_empty);
    }
}

static void maibox_listen(mailbox_t* mb) {
    message_t msg;

    while (1) {
        mailbox_receive(&msg, mb);
        printf("Receiving message: %s\n", msg.msgText);
        if (strcmp(msg.msgText, "EOF") == 0) break;
    }

    clock_gettime(CLOCK_MONOTONIC, &end);


    printf("Total time taken in receiving msg: %.6f\n", elapsed);
}

int main(int argc, char* argv[]){
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <mode> \n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int mode = atoi(argv[1]);
    mailbox_t mb;

    if (mode == MSG_PASSING) printf("Message Passing\n");
    else if (mode == SHARED_MEM) printf("Shared Memory\n");

    mailbox_init(&mb, mode);

    maibox_listen(&mb);

    mailbox_close(&mb);
    return 0;
}
