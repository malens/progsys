#include <linux/module.h>
#include <linux/miscdevice.h>
#include <linux/gfp.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <asm/uaccess.h>
#include <linux/proc_fs.h>
#include <linux/pid_namespace.h>
#include <linux/namei.h>
#include <linux/mount.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Jakub Wilkosz");
MODULE_DESCRIPTION("Simple advanced buffer device");

int ret;
int pid = 0;
struct task_struct * task;
struct pid *pid_struct;
struct file *file;
struct path path;

unsigned long getjiffies(void){
  unsigned long now_tick = jiffies;
  return now_tick;
}

void * memory_alloc(int size, void * memory){
    if (memory == NULL){
        return kzalloc(size, GFP_KERNEL);
    } else {
        return krealloc(memory, size, GFP_KERNEL);
    }
}

ssize_t mountderef_read(struct file *filep, char *buff, size_t count, loff_t *offp){
  void *tmpbuf = memory_alloc(sizeof(task->comm) + 7, NULL);
  sprintf(tmpbuf, "mntpnt: %s\n", path.mnt->mnt_root->d_name.name);
  return simple_read_from_buffer(buff, count, offp, tmpbuf, sizeof(task->comm)+7);
}

ssize_t mountderef_write(struct file *filep, const char *buff, size_t count, loff_t *offp){
  void * tmpbuf = memory_alloc(count, NULL);
  if (copy_from_user(tmpbuf, buff, count-1)!=0){
    printk("Userspace -> kernelspace copy failed!\n");
    return -EFAULT;
  }
  printk("%s\n",(char*)tmpbuf);

  ret = kern_path((char*)tmpbuf, LOOKUP_FOLLOW, &path);

  printk("%d\n",ret);
  follow_up(&path);
  printk("%s\n", path.mnt->mnt_root->d_name.name);

  return count;
}

ssize_t prname_read(struct file *filep, char *buff, size_t count, loff_t *offp){
  void *tmpbuf = memory_alloc(sizeof(task->comm) + 7, NULL);
  sprintf(tmpbuf, "name: %s\n", task->comm);
  return simple_read_from_buffer(buff, count, offp, tmpbuf, sizeof(task->comm)+7);
}

ssize_t prname_write(struct file *filep, const char *buff, size_t count, loff_t *offp){
  void * tmpbuf = memory_alloc(count, NULL);
  if (copy_from_user(tmpbuf, buff, count)!=0){
    printk("Userspace -> kernelspace copy failed!\n");
    return -EFAULT;
  }
  printk("%d\n",kstrtoint((char*)tmpbuf, 10, &pid));
  pid_struct = find_get_pid(pid);
  task = pid_task(pid_struct, PIDTYPE_PID);

  return count;
}

ssize_t jiffies_read(struct file *filep, char *buff, size_t count, loff_t *offp){
  void *tmpbuf = memory_alloc(24, NULL);
  sprintf(tmpbuf, "jiffies: %ld\n", getjiffies());
  return simple_read_from_buffer(buff, count, offp, tmpbuf, 20);
}

struct file_operations prname_ops = {
        read: prname_read,
        write: prname_write
};

struct file_operations mountderef_ops = {
        read: mountderef_read,
        write: mountderef_write
};

struct file_operations jiffies_ops = {
        read: jiffies_read
};

static struct miscdevice prname_miscdevice = {
        .minor = MISC_DYNAMIC_MINOR,
        .name = "prname",
        .fops = &prname_ops
};

static struct miscdevice jiffies_miscdevice = {
        .minor = MISC_DYNAMIC_MINOR,
        .name = "jiffies",
        .fops = &jiffies_ops
};

static struct miscdevice mountderef_misdevice = {
        .minor = MISC_DYNAMIC_MINOR,
        .name = "mountderef",
        .fops = &mountderef_ops
};

static int __init advanced_init(void){
    misc_register(&jiffies_miscdevice);
    misc_register(&prname_miscdevice);
    misc_register(&mountderef_misdevice);

    return 0;
}

static void __exit advanced_exit(void){
  misc_deregister(&jiffies_miscdevice);
  misc_deregister(&prname_miscdevice);
  misc_deregister(&mountderef_misdevice);
}


module_init(advanced_init);
module_exit(advanced_exit);
