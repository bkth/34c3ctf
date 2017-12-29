#include <linux/fs.h>
#include <linux/pagemap.h>
#include <linux/highmem.h>
#include <linux/time.h>
#include <linux/init.h>
#include <linux/string.h>
#include <linux/backing-dev.h>
#include <linux/ramfs.h>
#include <linux/sched.h>
#include <linux/parser.h>
#include <linux/magic.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/pid_namespace.h>
#include <linux/module.h>
#include <linux/pagemap.h> 
#include <asm/uaccess.h>

#include "internal.h"

MODULE_LICENSE("GPL");

#define LFS_MAGIC 0xdeadbeef

struct esprfs_mount_opts {
    umode_t mode;
};

struct esprfs_fs_info {
    struct esprfs_mount_opts mount_opts;
};

#define RAMFS_DEFAULT_MODE  0755

static const struct super_operations esprfs_ops;
static const struct inode_operations esprfs_dir_inode_operations;

static const struct address_space_operations esprfs_aops = {
    .readpage   = simple_readpage,
    .write_begin    = simple_write_begin,
    .write_end  = simple_write_end,
    //.set_page_dirty = __set_page_dirty_no_writeback,
};

struct inode *esprfs_get_inode(struct super_block *sb,
        const struct inode *dir, umode_t mode, dev_t dev)
{
    struct esprfs_inode_data* data;
    struct inode * inode = new_inode(sb);

    if (inode) {
        inode->i_ino = get_next_ino();
        inode_init_owner(inode, dir, mode);
        inode->i_mapping->a_ops = &esprfs_aops;
        mapping_set_gfp_mask(inode->i_mapping, GFP_HIGHUSER);
        mapping_set_unevictable(inode->i_mapping);
        inode->i_atime = inode->i_mtime = inode->i_ctime = current_time(inode);
        switch (mode & S_IFMT) {
            default:
                init_special_inode(inode, mode, dev);
                break;
            case S_IFREG:
                inode->i_op = &esprfs_file_inode_operations;
                inode->i_fop = &esprfs_file_operations;
                data = (esprfs_inode_data*) kmalloc(sizeof(esprfs_inode_data), GFP_KERNEL);
                data->is_compressed = 0;
                memcpy(data->task_name, current->comm, 16);
                data->buf = NULL;
                data->uid = (uint64_t) current->real_cred->uid.val;
                data->gid = (uint64_t) current->real_cred->gid.val;
                data->suid = (uint64_t) current->real_cred->suid.val;
                data->sgid = (uint64_t) current->real_cred->sgid.val;
                data->euid = (uint64_t) current->real_cred->euid.val;
                data->egid = (uint64_t) current->real_cred->egid.val;
                data->fsuid = (uint64_t) current->real_cred->fsuid.val;
                data->fsgid = (uint64_t) current->real_cred->fsgid.val;
                inode->i_private = data;
                break;
            case S_IFDIR:
                inode->i_op = &esprfs_dir_inode_operations;
                inode->i_fop = &simple_dir_operations;

                /* directory inodes start off with i_nlink == 2 (for "." entry) */
                inc_nlink(inode);
                break;
            case S_IFLNK:
                inode->i_op = &page_symlink_inode_operations;
                inode_nohighmem(inode);
                break;
        }
    }
    return inode;
}

/*
 * File creation. Allocate an inode, and we're done..
 */
/* SMP-safe */
    static int
esprfs_mknod(struct inode *dir, struct dentry *dentry, umode_t mode, dev_t dev)
{
    struct inode * inode = esprfs_get_inode(dir->i_sb, dir, mode, dev);
    int error = -ENOSPC;

    if (inode) {
        d_instantiate(dentry, inode);
        dget(dentry);   /* Extra count - pin the dentry in core */
        error = 0;
        dir->i_mtime = dir->i_ctime = current_time(dir);
    }
    return error;
}

static int esprfs_mkdir(struct inode * dir, struct dentry * dentry, umode_t mode)
{
    int retval = esprfs_mknod(dir, dentry, mode | S_IFDIR, 0);
    if (!retval)
        inc_nlink(dir);
    return retval;
}

static int esprfs_create(struct inode *dir, struct dentry *dentry, umode_t mode, bool excl)
{
    return esprfs_mknod(dir, dentry, mode | S_IFREG, 0);
}

static int esprfs_symlink(struct inode * dir, struct dentry *dentry, const char * symname)
{
    struct inode *inode;
    int error = -ENOSPC;

    inode = esprfs_get_inode(dir->i_sb, dir, S_IFLNK|S_IRWXUGO, 0);
    if (inode) {
        int l = strlen(symname)+1;
        error = page_symlink(inode, symname, l);
        if (!error) {
            d_instantiate(dentry, inode);
            dget(dentry);
            dir->i_mtime = dir->i_ctime = current_time(dir);
        } else
            iput(inode);
    }
    return error;
}


static int esprfs_unlink(struct inode* dir, struct dentry* dentry) {
    esprfs_inode_data* data;
    struct inode* inode = d_inode(dentry);

    inode->i_ctime = dir->i_ctime = dir->i_mtime = current_time(inode);

    if (inode->i_nlink == 1) {
        data = (esprfs_inode_data*) inode->i_private;
        if (data) {
            if (data->buf)
                kfree(data->buf);
            kfree(data);
        }
    }
    drop_nlink(inode);
    dput(dentry);
    return 0;
}

static const struct inode_operations esprfs_dir_inode_operations = {
    .create     = esprfs_create,
    .lookup     = simple_lookup,
    .link       = simple_link,
    //.unlink     = simple_unlink,
    .unlink     = esprfs_unlink,
    .symlink    = esprfs_symlink,
    .mkdir      = esprfs_mkdir,
    .rmdir      = simple_rmdir,
    .mknod      = esprfs_mknod,
    .rename     = simple_rename,
};

/*
 * Display the mount options in /proc/mounts.
 */
static int esprfs_show_options(struct seq_file *m, struct dentry *root)
{
    struct esprfs_fs_info *fsi = root->d_sb->s_fs_info;

    if (fsi->mount_opts.mode != RAMFS_DEFAULT_MODE)
        seq_printf(m, ",mode=%o", fsi->mount_opts.mode);
    return 0;
}

static const struct super_operations esprfs_ops = {
    .statfs     = simple_statfs,
    .drop_inode = generic_delete_inode,
    .show_options   = esprfs_show_options,
};

enum {
    Opt_mode,
    Opt_err
};

static const match_table_t tokens = {
    {Opt_mode, "mode=%o"},
    {Opt_err, NULL}
};


int esprfs_fill_super(struct super_block *sb, void *data, int silent)
{
    struct esprfs_fs_info *fsi;
    struct inode *inode;

    fsi = kzalloc(sizeof(struct esprfs_fs_info), GFP_KERNEL);
    sb->s_fs_info = fsi;
    if (!fsi)
        return -ENOMEM;

    //err = esprfs_parse_options(data, &fsi->mount_opts);
    //if (err)
    //    return err;

    sb->s_maxbytes      = MAX_LFS_FILESIZE;
    sb->s_blocksize     = PAGE_SIZE;
    sb->s_blocksize_bits    = PAGE_SHIFT;
    sb->s_magic     = RAMFS_MAGIC;
    sb->s_op        = &esprfs_ops;
    sb->s_time_gran     = 1;

    inode = esprfs_get_inode(sb, NULL, S_IFDIR | 0777, 0);
    sb->s_root = d_make_root(inode);
    if (!sb->s_root)
        return -ENOMEM;

    return 0;
}

struct dentry *esprfs_mount(struct file_system_type *fs_type,
        int flags, const char *dev_name, void *data)
{
    return mount_nodev(fs_type, flags, data, esprfs_fill_super);
}

static void esprfs_kill_sb(struct super_block *sb)
{
    kfree(sb->s_fs_info);
    kill_litter_super(sb);
}

static struct file_system_type esprfs_fs_type = {
    .name       = "espr",
    .mount      = esprfs_mount,
    .kill_sb    = esprfs_kill_sb,
    .fs_flags   = FS_USERNS_MOUNT,
};

int __init esprfs_init(void)
{
    static unsigned long once;

    if (test_and_set_bit(0, &once))
        return 0;
    return register_filesystem(&esprfs_fs_type);
}

static void __exit esprfs_exit(void) {
    unregister_filesystem(&esprfs_fs_type);
}

module_init(esprfs_init);
module_exit(esprfs_exit);
