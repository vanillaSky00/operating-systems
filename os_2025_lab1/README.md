
## POSIX Shared Memory with `Semaphores`
```
+-----------+           shared mem (/shm_demo)             +-----------+
| producer  |  ->  [ struct { len, buf[256], done } ]  ->  | consumer  |
| (writer)  |  ->  sem_data_ready (post)               ->  | (reader)  |
|           |  <-  sem_space_free (wait/post)          <-  |           |
+-----------+                                              +-----------+
```


## System V Message Queue
```
+-----------+         msgget/ftok + msgsnd             +-----------+
|  sender   |  ->  [ mtype=1, text="hello world" ] ->  | receiver  |
| (writer)  |                                          | (reader)  |
+-----------+  <-            msgrcv                    +-----------+
```


## Use Two Semaphores (classic producer–consumer pattern)
```
| Semaphore   | Purpose                                                | Initial Value |
| ----------- | ------------------------------------------------------ | ------------- |
| `sem_empty` | indicates shared memory is **empty**, sender can write | 1             |
| `sem_full`  | indicates shared memory is **full**, receiver can read | 0             |
```

