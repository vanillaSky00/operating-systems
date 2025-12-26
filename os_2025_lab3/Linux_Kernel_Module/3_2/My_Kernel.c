#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>   
#include <linux/spinlock.h>  
#include <linux/sched.h>     
#include <linux/ktime.h>     

#define PROCFS_NAME "Mythread_info"
#define MSG_BUF_SIZE 128

struct thread_info_data {
    char msg[MSG_BUF_SIZE];
    pid_t tgid;
    pid_t tid;
    u64 time_ms;
};

static struct thread_info_data last_info;
static DEFINE_SPINLOCK(info_lock); // Statically initialized, no need for spin_lock_init 
static struct proc_dir_entry *proc_entry;

static ssize_t threadinfo_write(struct file *fileptr, const char __user *ubuf, 
                                size_t count, loff_t *offset)
{
    char temp_buf[MSG_BUF_SIZE];
    size_t copy_size;
    unsigned long flags;

    copy_size = (count < MSG_BUF_SIZE) ? count : (MSG_BUF_SIZE - 1);

    if (copy_from_user(temp_buf, ubuf, copy_size))
        return -EFAULT;

    temp_buf[copy_size] = '\0';

    // Strip newline if present 
    // eg. echo hello > /proc/thread_info, it writes "hello\n"
    if (copy_size > 0 && temp_buf[copy_size - 1] == '\n')
        temp_buf[copy_size - 1] = '\0';

    spin_lock_irqsave(&info_lock, flags);
    
    strscpy(last_info.msg, temp_buf, sizeof(last_info.msg));
    last_info.tgid = current->tgid;
    last_info.tid  = current->pid;
    last_info.time_ms = current->utime / 1000000;

    spin_unlock_irqrestore(&info_lock, flags);

    return count;
}

static ssize_t threadinfo_read(struct file *fileptr, char __user *ubuf, 
                               size_t count, loff_t *offset)
{
    char kbuf[256];
    int len;
    struct thread_info_data local_copy;
    unsigned long flags;

    if (*offset > 0)
        return 0;

    // Snapshot data to local stack to keep critical section short 
    spin_lock_irqsave(&info_lock, flags);
    local_copy = last_info;
    spin_unlock_irqrestore(&info_lock, flags);

    len = scnprintf(kbuf, sizeof(kbuf),
                    "Message: %s\nPID: %d, TID: %d Time(ms): %llu\n",
                    local_copy.msg, 
                    local_copy.tgid, 
                    local_copy.tid, 
                    (unsigned long long)local_copy.time_ms); // Explicit cast for safety 

    if (len > count) 
        len = count;

    if (copy_to_user(ubuf, kbuf, len))
        return -EFAULT;

    *offset += len;
    return len;
}

static const struct proc_ops threadinfo_ops = {
    .proc_read  = threadinfo_read,
    .proc_write = threadinfo_write,
};

static int __init threadinfo_init(void)
{
    unsigned long flags;

    proc_entry = proc_create(PROCFS_NAME, 0644, NULL, &threadinfo_ops);
    if (!proc_entry) {
        pr_err("Error creating /proc/%s\n", PROCFS_NAME);
        return -ENOMEM;
    }
    
    // Fully initialize the struct to prevent garbage data on first read 
    spin_lock_irqsave(&info_lock, flags);
    strscpy(last_info.msg, "Initial", sizeof(last_info.msg));
    last_info.tgid = 0;
    last_info.tid = 0;
    last_info.time_ms = 0;
    spin_unlock_irqrestore(&info_lock, flags);
    
    pr_info("/proc/%s created\n", PROCFS_NAME);
    return 0;
}

static void __exit threadinfo_exit(void)
{
    proc_remove(proc_entry);
    pr_info("/proc/%s removed\n", PROCFS_NAME);
}

module_init(threadinfo_init);
module_exit(threadinfo_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Vanilla");
MODULE_DESCRIPTION("Assignment 3.2: Thread info tracker via procfs");