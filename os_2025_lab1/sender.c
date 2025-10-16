#include "sender.h"
#define MAX_LEN 256

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

        mb->storage.shm_addr = (char *) shmat(shmid, NULL, 0);
        if (mb->storage.shm_addr == (char*) -1) {
            perror("shmat");
            exit(EXIT_FAILURE);
        }
    }
}

static void mailbox_close(mailbox_t *mb) {
    if (mb->flag == MSG_PASSING) {

    }
    else if (mb->flag == SHARED_MEM) {
        shmdt(mb->storage.shm_addr);
    }
}

static message_t make_message(const char *text) {
    message_t msg;
    msg.mType = 1; 
    strncpy(msg.msgText, text, sizeof(msg.msgText) - 1);
    msg.msgText[sizeof(msg.msgText) - 1] = '\0';
    return msg;
}

void mailbox_send(const message_t* message, mailbox_t* mb){
    /*  TODO: 
        1. Use flag to determine the communication method
        2. According to the communication method, mailbox_send the message
    */
    if (mb == NULL) {
        perror("mailbox_send");
        exit(EXIT_FAILURE);
    }

    if (mb->flag == MSG_PASSING) {
        int qid = mb->storage.msqid;
        if (msgsnd(qid, message, sizeof(message->msgText), 0) == -1) {
            perror("msgsnd");
        }
    }
    else if (mb->flag == SHARED_MEM) {
        memcpy(mb->storage.shm_addr, message, sizeof(message_t));
    } 
}

int main(int argc, char* argv[]){
    /*  TODO: 
        1) Call mailbox_send(message, &mailbox) according to the flow in slide 4
        2) Measure the total sending time
        3) Get the mechanism and the input file from command line arguments
            • e.g. ./sender 1 input.txt
                    (1 for Message Passing, 2 for Shared Memory)
        4) Get the messages to be sent from the input file
        5) Print information on the console according to the output format
        6) If the message form the input file is EOF, mailbox_send an exit message to the receiver.c
        7) Print the total sending time and terminate the sender.c
    */
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <mode> <input_file>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // handle input
    int mode = atoi(argv[1]);
    FILE* fp = fopen(argv[2], "r");
    if (!fp) {
        perror("open file");
        exit(EXIT_FAILURE);
    }

    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    char line[MAX_LEN];
    mailbox_t mb;
    mb.flag = mode;
    mailbox_init(&mb, mode);

    while (fgets(line, sizeof(line), fp)) {
        line[strcspn(line, "\n")] = '\0';
        printf("Sending message: %s\n", line);

        message_t msg = make_message(line);
        mailbox_send(&msg, &mb);
    }

    // send exit signal
    message_t exit_msg = make_message("EOF");
    mailbox_send(&exit_msg, &mb);
    
    clock_gettime(CLOCK_MONOTONIC, &end);
    double elapsed = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) * 1e-9;
    printf("Total time taken in sending msg: %.6f\n", elapsed);

    mailbox_close(&mb);
    fclose(fp);
    return 0;
}