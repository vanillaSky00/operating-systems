#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>       /* copy_to_user */
#include <linux/sched.h>         /* current, task_struct */
#include <linux/sched/signal.h>  /* for_each_thread */
#include <linux/compiler.h>      /* READ_ONCE */

#define PROCFS_NAME "Mythread_info"
#define KBUF_SIZE   1024

static struct proc_dir_entry *proc_entry;

static ssize_t threadinfo_read(struct file *fileptr, char __user *ubuf,
                               size_t count, loff_t *offset)
{
    char kbuf[KBUF_SIZE];
    int len = 0;
    struct task_struct *t;

    // Return EOF after first read to prevent `cat` infinite loop
    /**
     *  while ((n = read(fd, buf, sizeof(buf))) > 0) {
            write(1, buf, n);
        }
        First read -> return content
        Second read -> return 0 -> tell userspace EOF -> done
     */
    if (*offset > 0)
        return 0;

    for_each_thread(current, t) {
        if (t->pid == t->tgid)
        continue;   // skip main thread 

        len += scnprintf(kbuf + len, KBUF_SIZE - len,
                         "PID: %d, TID: %d, Priority: %d, State: %ld\n",
                         current->pid,
                         t->pid,
                         (int)READ_ONCE(t->prio),
                         (long)READ_ONCE(t->__state));

        if (len >= KBUF_SIZE)
            break;
    }

    if (len > count)
        len = count;

    if (copy_to_user(ubuf, kbuf, len))
        return -EFAULT;

    *offset += len;
    return len;
}

static const struct proc_ops threadinfo_ops = {
    .proc_read = threadinfo_read,
};

static int __init threadinfo_init(void)
{
    // read-only → 0444 
    proc_entry = proc_create(PROCFS_NAME, 0444, NULL, &threadinfo_ops);
    if (!proc_entry) {
        pr_err("Error creating /proc/%s\n", PROCFS_NAME);
        return -ENOMEM;
    }

    pr_info("/proc/%s created (read-only)\n", PROCFS_NAME);
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
MODULE_DESCRIPTION("Assignment 3.1: show thread info via procfs read");