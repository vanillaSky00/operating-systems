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

    // create semaphores
    sem_t* sem_empty = sem_open(SEM_EMPTY, O_CREAT, 0666, 0); 
    sem_t* sem_full = sem_open(SEM_FULL, O_CREAT, 0666, 0);
    if (sem_empty == SEM_FAILED || sem_full == SEM_FAILED) {
        perror("sem_open");
        exit(EXIT_FAILURE);
    }   
}

static void mailbox_close(mailbox_t *mb) {
    if (mb->flag == MSG_PASSING) {
        
    }
    else if (mb->flag == SHARED_MEM) {
        shmdt(mb->storage.shm_addr);
    }
    sem_close(sem_open(SEM_EMPTY, 0));
    sem_close(sem_open(SEM_FULL, 0));
}

static message_t make_message(const char *text) {
    message_t msg;
    msg.mType = 1; 
    strncpy(msg.msgText, text, sizeof(msg.msgText) - 1);
    msg.msgText[sizeof(msg.msgText) - 1] = '\0';
    return msg;
}



struct timespec start, end;
double elapsed = 0.0;

void mailbox_send(const message_t* message, mailbox_t* mb){
    sem_t* sem_empty = sem_open(SEM_EMPTY, 0); 
    sem_t* sem_full = sem_open(SEM_FULL, 0);

    if (message == NULL || mb == NULL) {
        perror("mailbox_send");
        exit(EXIT_FAILURE);
    }

    if (mb->flag == MSG_PASSING) {
        int qid = mb->storage.msqid;

        clock_gettime(CLOCK_MONOTONIC, &start);
        if (msgsnd(qid, message, sizeof(message->msgText), 0) == -1) {
            perror("msgsnd");
        }
        clock_gettime(CLOCK_MONOTONIC, &end);
        elapsed += (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) * 1e-9;
        
        // Signal receiver that message is ready
        sem_post(sem_full);

        // Wait until shared memory is empty
        sem_wait(sem_empty);

    }
    else if (mb->flag == SHARED_MEM) {

        clock_gettime(CLOCK_MONOTONIC, &start);

        memcpy(mb->storage.shm_addr, message, sizeof(message_t));
        
        clock_gettime(CLOCK_MONOTONIC, &end);
        elapsed += (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) * 1e-9;

        // Signal receiver that message is ready
        sem_post(sem_full);

        // Wait until shared memory is empty
        sem_wait(sem_empty);
    } 
}

int main(int argc, char* argv[]){
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

    char line[MAX_LEN];
    mailbox_t mb;
    mb.flag = mode;

    mailbox_init(&mb, mode);

    if (mode == MSG_PASSING) printf("Message Passing\n");
    else if (mode == SHARED_MEM) printf("Shared Memory\n");

    while (fgets(line, sizeof(line), fp)) {
        line[strcspn(line, "\n")] = '\0';
        printf("Sending message: %s\n", line);

        message_t msg = make_message(line);
        mailbox_send(&msg, &mb);
    }

    // send exit signal
    message_t exit_msg = make_message("EOF");
    mailbox_send(&exit_msg, &mb);
    
    printf("Total time taken in sending msg: %.6f\n", elapsed);

    mailbox_close(&mb);
    fclose(fp);
    return 0;
}