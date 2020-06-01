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

void * buffmemory;
int ptr = 0;
int bufsize = 20;

struct proc_dir_entry *proc_entry;

void * memory_alloc(int size, void * memory){
    if (memory == NULL){
        return kzalloc(size, GFP_KERNEL);
    } else {
        return krealloc(memory, size, GFP_KERNEL);
    }
}


void write_to_buf(void * buf, int size){
    if (ptr + size < bufsize - 1){
        memset(buffmemory + ptr, 0, size);
        strcpy(buffmemory + ptr, (char*)buf);
        ptr = ptr + size;
    } else {
        memset(buffmemory + ptr, 0, bufsize - ptr);
        memcpy(buffmemory + ptr, buf, bufsize - ptr);
        memset(buffmemory, 0, size - (bufsize - ptr));
        memcpy(buffmemory, buf + bufsize - ptr, size - (bufsize - ptr));
        ptr = size - bufsize + ptr;
    }
}

ssize_t circular_read(struct file *filep, char *buff, size_t count, loff_t *offp){
  return simple_read_from_buffer(buff, count, offp, buffmemory, bufsize);
}

ssize_t circular_proc_write(struct file *filep, const char *buff, size_t count, loff_t *offp){
    void * tmpbuf = memory_alloc(count, NULL);
    if (copy_from_user(tmpbuf, buff, count)!=0){
      printk("Userspace -> kernelspace copy failed!\n");
      return -1;
    }
    printk("%d\n",kstrtoint((char*)tmpbuf, 10, &bufsize));
    printk("%d\n", bufsize);
    buffmemory = memory_alloc(bufsize, buffmemory);
    return count;
}

ssize_t circular_write(struct file *filep, const char *buff, size_t count, loff_t *offp){
    void *tmpbuf = memory_alloc(count, NULL);
    if (copy_from_user(tmpbuf, buff, count) != 0){
        printk("Userspace -> kernelspace copy failed!\n");
        return -1;
    }
    printk("%d\n", bufsize);
    write_to_buf(tmpbuf, count);
    return bufsize;
}

struct file_operations circular_ops = {
        read: circular_read,
        write: circular_write
};
const struct file_operations proc_fops = {
  .write = circular_proc_write,
};

static struct miscdevice circular_miscdevice = {
        .minor = MISC_DYNAMIC_MINOR,
        .name = "circular",
        .fops = &circular_ops
};

static int __init circular_init(void){
    misc_register(&circular_miscdevice);
    buffmemory = memory_alloc(bufsize, NULL);
    if (!buffmemory){
      printk("memalloc failed\n");
    }
    memset(buffmemory, 0, bufsize);
    printk("ksize: %d", (int) ksize(buffmemory));
    printk("bmem as ptr %p\n", (int *)buffmemory);
    printk("bmemas str %s\n", (char * )buffmemory);
    proc_entry = proc_create("circular", 000, NULL, &proc_fops);
    return 0;
}

static void __exit circular_exit(void){
    misc_deregister(&circular_miscdevice);
    if(proc_entry){
      proc_remove(proc_entry);
    }
    kfree(buffmemory);
}



module_init(circular_init);
module_exit(circular_exit);
