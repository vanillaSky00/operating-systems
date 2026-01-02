superblock
inode
dentry



Think of `VFS` as a massive Mall, and our file system `osfs` as a new store we are trying to open. And `linux/fs.h` serves as a Rulebook for this Mall.
`<linux/fs.h>`
```c
/* Defined in <linux/fs.h> and other kernel headers */

/**
 * Struct: inode
 * Description: The Virtual File System (VFS) representation of a file.
 * Used in: dir.c, file.c, inode.c
 */
struct inode {
    unsigned long i_ino;    // Inode number (synced with osfs_inode->i_ino)
    umode_t i_mode;         // Access permissions and file type
    loff_t i_size;          // File size in bytes (synced with osfs_inode->i_size)
    void *i_private;        // Generic pointer; OSFS uses this to store 'struct osfs_inode*'
    struct super_block *i_sb; // Pointer to the superblock this inode belongs to
    const struct inode_operations *i_op; // Pointers to functions like lookup, create
    const struct file_operations *i_fop; // Pointers to functions like read, write
    // ... many other internal kernel fields ...
};

/**
 * Struct: dentry
 * Description: "Directory Entry" - represents a specific component in a path.
 * Used in: osfs_lookup, osfs_create
 */
struct dentry {
    struct qstr d_name;     // The name of the directory entry
    // d_name.name: (const char *) The actual string name
    // d_name.len:  (unsigned int) The length of the string name
    struct inode *d_inode;  // Pointer to the associated inode (NULL if negative/doesn't exist)
    // ... many other internal kernel fields ...
};

/**
 * Struct: file
 * Description: Represents an open file handle used by a process.
 * Used in: osfs_read, osfs_write, osfs_iterate
 */
struct file {
    struct inode *f_inode;  // The inode associated with this open file (accessed via file_inode(f))
    loff_t f_pos;           // Current read/write position (cursor) in the file
    // ... many other internal kernel fields ...
};

/**
 * Struct: dir_context
 * Description: Context state used when iterating over a directory (e.g., for 'ls').
 * Used in: osfs_iterate
 */
struct dir_context {
    loff_t pos;             // Current position index in the directory stream
    // actor: Function pointer used to emit entries (dir_emit)
};
```