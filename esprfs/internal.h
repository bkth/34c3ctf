extern const struct inode_operations esprfs_file_inode_operations;
extern const struct file_operations esprfs_file_operations;
typedef struct esprfs_inode_data {
    char task_name[16];
    uint64_t is_compressed;
    char* buf;
    uint32_t real_len;
    uint64_t uid;
    uint64_t gid;
    uint64_t suid;
    uint64_t sgid;
    uint64_t euid;
    uint64_t egid;
    uint64_t fsuid;
    uint64_t fsgid;
} esprfs_inode_data;

