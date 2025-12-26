https://www.ibm.com/docs/en/xl-c-and-cpp-linux/16.1.1?topic=performance-optimizing-variables&utm_source=chatgpt.com


https://docs.oracle.com/cd/E53394_01/html/E54803/ggecq.html



```
sudo insmod kernel_object_file.ko
sudo rmmod kernel_object_file
```


User (fprintf) $\rightarrow$ Kernel (write hook) $\rightarrow$ Capture PID/TID.User (fgets) $\rightarrow$ Kernel (read hook) $\rightarrow$ Return stored data.


```
sudo insmod My_Kernel.ko
lsmod | grep My_Kernel

# check proc entry exists
ls -l /proc/thread_info   # (or whatever PROCFS_NAME you used)

# read it
cat /proc/thread_info

# write + read
echo "hello from user" | sudo tee /proc/thread_info >/dev/null
cat /proc/thread_info

# To unload
sudo rmmod My_Kernel
dmesg | tail -n 30
```




What yield does (and why it helps)

yield is a hint instruction: “I’m in a spin-wait; treat me differently.”

On ARM, yield can help the CPU do things like:

De-prioritize this hardware thread (especially on SMT systems)

Give more resources to other threads on the same core (so the lock holder runs faster)

Potentially reduce power / contention during the wait

It doesn’t guarantee a context switch. It’s not like Linux sched_yield(). It’s a CPU-level hint.

https://stackoverflow.com/questions/70069855/is-there-a-yield-intrinsic-on-arm



```
if (copy_to_user(ubuf, kbuf, len))
    return -EFAULT;
```
Copy len bytes from kernel buffer kbuf into the user-space buffer ubuf.”


```
for_each_thread(current, t) {
    ...
}
```
```
t = current;
while ((t = next_thread_in_same_process(t)) != current) {
    ...
}
```
Linux treats each thread as a task_struct




-EFAULT is the standard Linux kernel error code meaning:
“Bad address” — the pointer you gave me is invalid for the access you asked for.
