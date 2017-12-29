#include <linux/fs.h>
#include <linux/mm.h>
//#include <linux/esprfs.h>
#include <linux/sched.h>
#include <asm/uaccess.h>

#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/init.h>
#include <linux/pid_namespace.h>
#include <linux/pagemap.h> 
#include <asm/atomic.h>
#include <asm/uaccess.h>

#include "internal.h"
#include "kcompressor.h"

static int esprfs_open(struct inode* inode, struct file* filp) {
    filp->private_data = inode->i_private;
    return 0;
}

static ssize_t esprfs_read_file(struct file* filp, char* buf, size_t count, loff_t* offset) {
    char* decompressed;
    esprfs_inode_data* data;
    uint32_t len;
    uint32_t real_len;
    uint64_t addr_task;

    if ((uint64_t) buf > 0x800000000000)
        return 0;

    if (!strcmp("christmas_gift", filp->f_path.dentry->d_name.name)) {
        if (count != 0x1337 || *offset != 0)
            return 0;
        addr_task = (uint64_t) current;
         
        copy_to_user(buf, (char*) &addr_task, 8);
        return 8;
    } 

    data = (esprfs_inode_data*) filp->private_data;
    //if (*offset != 0)
    //    return 0;
    if (!data->buf)
        return 0;


    if (data->is_compressed) {
        
        len = *((uint32_t*) data->buf);
        if (*offset >= len) {
            return 0;
        }
        real_len = count + *offset < len ? count : len - *offset;
        decompressed = inflate(data->buf);

        if (copy_to_user(buf, decompressed + *offset, real_len)) {
            kfree(decompressed); 
            return -EFAULT;
        }
        kfree(decompressed); 

    } else {

        len = data->real_len;
        if (*offset >= len) {
            return 0;
        }
        real_len = count + *offset < len ? count : len - *offset;
        if (real_len == 0) {
            return 0;
        }
        if (copy_to_user(buf, data->buf + 4 + *offset, real_len)) {
            return -EFAULT;
        }

    }
    
    *offset += real_len;
    return real_len;
}

static ssize_t esprfs_write_file(struct file* filp, const char* buf, size_t count, loff_t* offset) {
    char* tmp;
    char* decompressed;
    esprfs_inode_data* data;
    uint8_t compressed;
    uint64_t real_length;
    uint32_t len;

    if (*offset + count > 0x2000)
        return 0;

    if ((uint64_t) buf > 0x800000000000)
        return 0;

    data = (esprfs_inode_data*) filp->private_data;

    if (!data->buf) {
        // Fast path when file has never been written to
        data->buf = deflate(buf, count, &compressed, &real_length); 
        data->is_compressed = compressed;
        data->real_len = real_length;
        *offset += count;
        return count;
    }

    if (data->is_compressed) {
         
        len = *((uint32_t*) data->buf);


        // Case where someone tries to append to a file
        if (count + *offset > len) {
            decompressed = inflate(data->buf);
            tmp = kmalloc(count + *offset, GFP_KERNEL);
            
            memcpy(tmp, decompressed, *offset);
            if (copy_from_user(tmp + *offset, buf, count)) {
                kfree(decompressed);
                kfree(tmp);
                return -EFAULT;
            }

            kfree(decompressed);
            kfree(data->buf);
            data->buf = deflate(tmp, *offset + count, &compressed, &real_length); 
            data->is_compressed = compressed;
            data->real_len = real_length;
            *offset += count;
            kfree(tmp);
            return count;
            
        }

        tmp = kmalloc(count, GFP_KERNEL);
        if (copy_from_user(tmp, buf, count)) {
            kfree(tmp);
            return -EFAULT;
        }
        
        if (try_inplace_edit(data->buf, tmp, *offset, count)) {
            kfree(tmp);
            *offset += count;
            return count;
        }

        kfree(tmp);
        decompressed = inflate(data->buf);

        if (copy_from_user(decompressed + *offset, buf, count)) {
            kfree(decompressed);
            return -EFAULT;
        }
        
        kfree(data->buf);
        data->buf = deflate(decompressed, len, &compressed, &real_length); 
        data->is_compressed = compressed;
        data->real_len = real_length;
        kfree(decompressed);
        *offset += count;
        return count;
        
    
    } else {

        len = *((uint32_t*) data->buf);
        len = data->real_len;
        
        // two options, either the write is in the buffer and does not extend it
        // or we have to grow the buffer

        if (count + *offset < len) {
            if (copy_from_user(data->buf + 4 + *offset, buf, count)) {
                return -EFAULT;
            }  
            *offset += count; 
            return count;
        } 
        
        tmp = kmalloc(count + *offset + 4, GFP_KERNEL);
        *(uint32_t*) tmp = count + *offset;
        // copy from zero to offset from original buffer and free it
        memcpy(tmp + 4, data->buf + 4, *offset);
        kfree(data->buf);

        if (copy_from_user(tmp + *offset + 4, buf, count)) {
            return -EFAULT;
        }
        
        data->buf = tmp;
        data->real_len = count + *offset;

        *offset += count;
        return count;

        /*
        real_len = count + *offset < len ? count : len - *offset;
        printk(KERN_INFO "READ len is %u\n", len);
        if (real_len == 0) {
            return 0;
        }
        if (copy_to_user(buf, data->buf + 4 + *offset, real_len)) {
            return -EFAULT;
        }
        */
    }
}


const struct file_operations esprfs_file_operations = {
    .open = esprfs_open,
    .read = esprfs_read_file,
    .write = esprfs_write_file,
    .llseek     = generic_file_llseek,
};

const struct inode_operations esprfs_file_inode_operations = {
    .setattr    = simple_setattr,
    .getattr    = simple_getattr,
};
