## Overall
This README explains the architecture, the rationale for each IPC choice, and the exact C APIs used (what they do, their arguments, and why we use them). It matches a simple two-binary setup:

- sender: reads an input (file or CLI text) and a mode (`shm` or `mq`), prepares the mailbox, then `mailbox_send()`.

- receiver: takes the mode, prepares the same mailbox, then `mailbox_listen()`.
why using posix shared mem, and why use system v essage queue

## Why two implementations?
### POSIX Shared Memory (+ Semaphores)?
- Speed & zero-copy: both processes map the same memory; data isn’t copied by the kernel once mapped.
- Fine-grained control: you design your buffer format and concurrency (semaphores).
- Portable on modern Unix: `shm_open`, `mmap,` `sem_open` are widely available (Linux, BSD, macOS—with caveats*).
> Note: On some platforms, named POSIX semaphores (e.g., sem_open) may require special mount points (/dev/shm) or have naming/permission quirks. That’s normal.

### Why System V Message Queues?
- Kernel-buffered messages: the kernel queues whole messages; simpler mental model (no shared memory layout).
- Loose coupling: no shared data structure; only send/receive calls.
- Mature & ubiquitous: available on almost all Unix-likes.

## Overall Architecture?
### POSIX Shared Memory with `Semaphores`
```
+-----------+           shared mem (/shm_demo)             +-----------+
| producer  |  ->  [ struct { len, buf[256], done } ]  ->  | consumer  |
| (writer)  |  ->  sem_data_ready (post)               ->  | (reader)  |
|           |  <-  sem_space_free (wait/post)          <-  |           |
+-----------+                                              +-----------+
```

### Use Two Semaphores (classic producer–consumer pattern)
```
| Semaphore   | Purpose                                                | Initial Value |
| ----------- | ------------------------------------------------------ | ------------- |
| `sem_empty` | indicates shared memory is **empty**, sender can write | 1             |
| `sem_full`  | indicates shared memory is **full**, receiver can read | 0             |
```


### System V Message Queue
```
+-----------+         msgget/ftok + msgsnd             +-----------+
|  sender   |  ->  [ mtype=1, text="hello world" ] ->  | receiver  |
| (writer)  |                                          | (reader)  |
+-----------+  <-            msgrcv                    +-----------+
```

## Build & Run
```
make
./sender <mode> <input_file>
./receiver <mode>
```

## API Reference
| Family                     | Standard                    | Header         | Functions                                  |
| -------------------------- | --------------------------- | -------------- | ------------------------------------------ |
| **System V Shared Memory** | Older, but widely supported | `<sys/shm.h>`  | `shmget`, `shmat`, `shmdt`, `shmctl`       |
| **POSIX Shared Memory**    | Newer (IEEE 1003.1)         | `<sys/mman.h>` | `shm_open`, `mmap`, `munmap`, `shm_unlink` |


### POSIX Shared Memory
```
int shmget(key_t key, size_t size, int shmflg);
```
- What: Creates/opens a named shared memory object (appears under /dev/shm on Linux).
- Args: 
    - name: e.g. "/shm_demo" (must start with /).
    - oflag: O_CREAT | O_RDWR to create if missing, read/write.
    - mode: permissions, e.g. 0666.
- Why: Gives a file descriptor for a shared memory region both processes can map.


### POSIX Named Semaphores
```
sem_t *sem_open(const char *name, int oflag, mode_t mode, unsigned int value);
```
- What: Create/open a named semaphore.
- Args:
    - name: e.g. "/sem_space_free_demo" or "/sem_data_ready_demo".
- value: initial count (1 for sem_empty, 0 for sem_full).
- Why: Cross-process synchronization for the shared memory mailbox.

```
int sem_wait(sem_t *sem); / int sem_post(sem_t *sem);
```

- What: P (wait/decrement) and V (post/increment).
- Why: Enforce the single-slot protocol: writer waits for empty, reader waits for full.

```
int sem_close(sem_t *sem); / int sem_unlink(const char *name);
```

- What: Close local handle, then remove the named semaphore.
- Why: Avoid leaked kernel objects. Do the unlink once when the system is shutting down.

### System V Message Queues

```
key_t ftok(const char *path, int proj_id);
```
- What: Derives a stable IPC key from an existing file path + a project id character.
- Args: path="." (or another known file), proj_id='X'.
- Why: Both processes compute the same key without hard-coding a number.

```
int msgget(key_t key, int msgflg);
```

- What: Create/open a message queue.
- Args: key from ftok, msgflg = IPC_CREAT | 0666.
- Why: Obtain the queue id (qid) for subsequent send/receive.

```
int msgsnd(int msqid, const void *msgp, size_t msgsz, int msgflg);
```
- What: Send a message.

- Args:
    - msqid: from msgget,
    - msgp: pointer to a struct whose first field is long mtype,
    - msgsz: number of bytes after mtype (e.g., sizeof(mtext)),
    - msgflg: usually 0 (blocking) or IPC_NOWAIT.

- Why: Kernel enqueues messages atomically; receiver reads them in order (per type).

```
ssize_t msgrcv(int msqid, void *msgp, size_t msgsz, long msgtyp, int msgflg);
```
- What: Receive a message (optionally filtered by mtype).
- Args:
    - msgtyp: 0 = first message in queue; positive k = first message with mtype==k.
- Why: Simple blocking receive loop. We define:

- MSGTYPE_DATA = 1 for payloads,

- MSGTYPE_DONE = 2 to tell the receiver to exit and clean up.

```
int msgctl(int msqid, int cmd, struct msqid_ds *buf);
```
- What: Control ops; we use IPC_RMID to remove the queue when done.
- Why: Clean, no dangling queues.

## String
```
#include <string.h>
size_t strcspn(const char *string1, const char *string2);
```
Find the number of characters in the given string before the 1st occurrence of a character from the defined set of characters.

```c
#include <stdio.h>
#include <string.h>
 
#define SIZE 40
 
int main(void)
{
  char string[SIZE] = "This is the source string";
  char * substring = "axle";
 
  printf( "The first %i characters in the string \"%s\" "
          "are not in the string \"%s\" \n",
            strcspn(string, substring), string, substring);
 
}
 
/**********  Output should be similar to:  **************
 
The first 10 characters in the string "This is the source string"
are not in the string "axle"
*/
```


### when to use const
https://softwareengineering.stackexchange.com/questions/204500/when-and-for-what-purposes-should-the-const-keyword-be-used-in-c-for-variables