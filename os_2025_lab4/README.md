
| Feature             | osfs_basic (Standard)                   | osfs_indexed (Bonus)                                        |
| ------------------- | --------------------------------------- | ----------------------------------------------------------- |
| Inode Structure     | `uint32_t i_block;`                     | `uint32_t i_blocks_array[16];`                              |
| Max File Size       | 4 KB                                    | 64 KB (16 × 4 KB)                                           |
| Allocation Strategy | Single Block Allocation                 | Direct Indexed Allocation                                   |
| Complexity          | Simple, no loops needed for read/write. | Needs math (`ppos / BLOCK_SIZE`) to find the correct index. |


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


update note:
Old (Buggy): i_blocks_array[0] = 0; (This pointed to Root's data).

New (Fixed): i_blocks_array[0] = OSFS_INVALID_BLOCK; (This points to "nothing", waiting for write to allocate a fresh block).


## Test 
Compile and load module
```
make
sudo rmmod osfs.ko
sudo insmod osfs.ko
sudo dmesg | tail
```

Mount our fs to `mnt/`
```
sudo mount -t osfs none mnt/
sudo dmesg | tail
```

```
sudo touch mnt/test1.txt
sudo touch test1.txt
ls -l mnt/
```

```
sudo bash –c "echo 'I LOVE OSLAB' > test1.txt"
cat test1.txt
```

clean up
```
sudo umount mnt
```
```
make clean
```


reference
https://hackmd.io/@sysprog/linux-file-system