#include <linux/init.h>
#include <linux/jiffies.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/sched.h>
#include <linux/version.h>
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 11, 0)
#include <linux/sched/signal.h>
#endif
#include <linux/seq_file.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/uaccess.h>

#define PROC_NAME "oslab_monitor"
#define NOTE_MAX 96

static char note[NOTE_MAX] = "os course design procfs extension";
static unsigned long read_count;
static unsigned long write_count;

static void count_tasks(unsigned long *running, unsigned long *sleeping,
                        unsigned long *stopped, unsigned long *zombie,
                        unsigned long *other) {
    struct task_struct *task;
    *running = *sleeping = *stopped = *zombie = *other = 0;

    rcu_read_lock();
    for_each_process(task) {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 14, 0)
        unsigned int state = READ_ONCE(task->__state);
#else
        unsigned long state = READ_ONCE(task->state);
#endif
        if (state == TASK_RUNNING) {
            (*running)++;
        } else if (state & TASK_INTERRUPTIBLE || state & TASK_UNINTERRUPTIBLE) {
            (*sleeping)++;
        } else if (state & __TASK_STOPPED) {
            (*stopped)++;
        } else if (READ_ONCE(task->exit_state) & EXIT_ZOMBIE) {
            (*zombie)++;
        } else {
            (*other)++;
        }
    }
    rcu_read_unlock();
}

static int oslab_show(struct seq_file *m, void *v) {
    unsigned long running, sleeping, stopped, zombie, other;
    struct sysinfo si;

    read_count++;
    si_meminfo(&si);
    count_tasks(&running, &sleeping, &stopped, &zombie, &other);

    seq_puts(m, "OSLab Linux procfs monitor\n");
    seq_printf(m, "note: %s\n", note);
    seq_printf(m, "jiffies: %lu\n", jiffies);
    seq_printf(m, "reads: %lu\n", read_count);
    seq_printf(m, "writes: %lu\n", write_count);
    seq_puts(m, "process_state_count:\n");
    seq_printf(m, "  running:  %lu\n", running);
    seq_printf(m, "  sleeping: %lu\n", sleeping);
    seq_printf(m, "  stopped:  %lu\n", stopped);
    seq_printf(m, "  zombie:   %lu\n", zombie);
    seq_printf(m, "  other:    %lu\n", other);
    seq_puts(m, "memory_pages:\n");
    seq_printf(m, "  totalram: %lu\n", si.totalram);
    seq_printf(m, "  freeram:  %lu\n", si.freeram);
    seq_printf(m, "  shared:   %lu\n", si.sharedram);
    seq_printf(m, "  buffer:   %lu\n", si.bufferram);
    seq_puts(m, "commands:\n");
    seq_puts(m, "  echo reset > /proc/oslab_monitor\n");
    seq_puts(m, "  echo note=your_text > /proc/oslab_monitor\n");
    return 0;
}

static int oslab_open(struct inode *inode, struct file *file) {
    return single_open(file, oslab_show, NULL);
}

static ssize_t oslab_write(struct file *file, const char __user *ubuf,
                           size_t count, loff_t *ppos) {
    char *kbuf;
    size_t len = min(count, (size_t)NOTE_MAX - 1);

    kbuf = kzalloc(NOTE_MAX, GFP_KERNEL);
    if (!kbuf) return -ENOMEM;
    if (copy_from_user(kbuf, ubuf, len)) {
        kfree(kbuf);
        return -EFAULT;
    }
    for (len = 0; len < NOTE_MAX && kbuf[len] != '\0'; ++len) {
        if (kbuf[len] == '\r' || kbuf[len] == '\n') {
            kbuf[len] = '\0';
            break;
        }
    }

    if (strcmp(kbuf, "reset") == 0) {
        read_count = 0;
        write_count = 0;
        strlcpy(note, "reset by user", NOTE_MAX);
    } else if (strncmp(kbuf, "note=", 5) == 0) {
        strlcpy(note, kbuf + 5, NOTE_MAX);
        write_count++;
    } else {
        strlcpy(note, kbuf, NOTE_MAX);
        write_count++;
    }

    kfree(kbuf);
    return count;
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 6, 0)
static const struct proc_ops oslab_proc_ops = {
    .proc_open = oslab_open,
    .proc_read = seq_read,
    .proc_write = oslab_write,
    .proc_lseek = seq_lseek,
    .proc_release = single_release,
};
#else
static const struct file_operations oslab_proc_ops = {
    .owner = THIS_MODULE,
    .open = oslab_open,
    .read = seq_read,
    .write = oslab_write,
    .llseek = seq_lseek,
    .release = single_release,
};
#endif

static int __init oslab_proc_init(void) {
    if (!proc_create(PROC_NAME, 0666, NULL, &oslab_proc_ops)) {
        pr_err("oslab_proc: failed to create /proc/%s\n", PROC_NAME);
        return -ENOMEM;
    }
    pr_info("oslab_proc: loaded, /proc/%s created\n", PROC_NAME);
    return 0;
}

static void __exit oslab_proc_exit(void) {
    remove_proc_entry(PROC_NAME, NULL);
    pr_info("oslab_proc: unloaded\n");
}

module_init(oslab_proc_init);
module_exit(oslab_proc_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("OS Course Design");
MODULE_DESCRIPTION("Procfs monitor extension for operating system course design");
