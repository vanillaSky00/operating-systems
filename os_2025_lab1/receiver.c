#include "receiver.h"

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

        int shmid = shmget(key, sizeof(message_t), 0666 | IPC_CREAT);
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

    }
    else if (mb->flag == SHARED_MEM) {
        shmdt(mb->storage.shm_addr);
    }
}

void mailbox_receive(message_t* msg, mailbox_t* mb){
    /*  TODO: 
        1. Use flag to determine the communication method
        2. According to the communication method, receive the message
    */
    
    if (mb->flag == MSG_PASSING) {
        int qid = mb->storage.msqid;

        ssize_t n = msgrcv(qid, msg, sizeof(msg->msgText), 0, 0);
        if (n == -1) {
            perror("msgrcv");
        }
    }
    
    else if (mb->flag == SHARED_MEM) {
        message_t *share_msg = (message_t *) mb->storage.shm_addr;
        memcpy(msg, share_msg, sizeof(message_t));
    }
}

static void maibox_listen(mailbox_t* mb) {
    message_t msg;
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    while (1) {
        mailbox_receive(&msg, mb);
        printf("Receiving message: %s\n", msg.msgText);
        if (strcmp(msg.msgText, "EOF") == 0) break;
    }

    clock_gettime(CLOCK_MONOTONIC, &end);

    double elapsed = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) * 1e-9; 
    printf("Total time taken in receiving msg: %.6f\n", elapsed);
}

int main(int argc, char* argv[]){
    /*  TODO: 
        1) Call receive(&message, &mailbox) according to the flow in slide 4
        2) Measure the total receiving time
        3) Get the mechanism from command line arguments
            • e.g. ./receiver 1
        4) Print information on the console according to the output format
        5) If the exit message is received, print the total receiving time and terminate the receiver.c
    */
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <mode> \n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int mode = atoi(argv[1]);
    mailbox_t mb;
    mailbox_init(&mb, mode);

    maibox_listen(&mb);

    mailbox_close(&mb);
    return 0;
    }
