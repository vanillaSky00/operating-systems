#include <linux/fs.h>
#include <linux/uaccess.h>
#include "osfs.h"

/**
 * Function: osfs_read
 * Description: Reads data from a file.
 * Inputs:
 *   - filp: The file pointer representing the file to read from.
 *   - buf: The user-space buffer to copy the data into.
 *   - len: The number of bytes to read.
 *   - ppos: The file position pointer.
 * Returns:
 *   - The number of bytes read on success.
 *   - 0 if the end of the file is reached.
 *   - -EFAULT if copying data to user space fails.
 */
static ssize_t osfs_read(struct file *filp, char __user *buf, size_t len, loff_t *ppos)
{
    struct inode *inode = file_inode(filp);
    struct osfs_inode *osfs_inode = inode->i_private;
    struct osfs_sb_info *sb_info = inode->i_sb->s_fs_info;
    void *data_block;
    ssize_t bytes_read;

    // If the file has not been allocated a data block, it indicates the file is empty
    if (osfs_inode->i_blocks == 0)
        return 0;

    if (*ppos >= osfs_inode->i_size)
        return 0;

    if (*ppos + len > osfs_inode->i_size)
        len = osfs_inode->i_size - *ppos;

    // Upadte: Check if the block index is valid and allocated ----------------
    uint32_t block_idx = *ppos / sb_info->block_size;
    uint32_t block_offset = *ppos % sb_info->block_size;
    if (block_idx >= MAX_DIRECT_BLOCKS || osfs_inode->i_blocks_array[block_idx] == OSFS_INVALID_BLOCK)
        return 0; // Should not happen if ppos < i_size, but safe to check

    // Ensure we don't read past the end of the current block 
    if (block_offset + len > sb_info->block_size) 
        len = sb_info->block_size - block_offset;

    // Calculate physical address
    data_block = sb_info->data_blocks + 
                 (osfs_inode->i_blocks_array[block_idx] * sb_info->block_size) + 
                 block_offset;
    // ------------------------------------------------------------------------

    if (copy_to_user(buf, data_block, len))
        return -EFAULT;

    *ppos += len;
    bytes_read = len;

    return bytes_read;
}


/**
 * Function: osfs_write
 * Description: Writes data to a file.
 * Inputs:
 *   - filp: The file pointer representing the file to write to.
 *   - buf: The user-space buffer containing the data to write.
 *   - len: The number of bytes to write.
 *   - ppos: The file position pointer.
 * Returns:
 *   - The number of bytes written on success.
 *   - -EFAULT if copying data from user space fails.
 *   - Adjusted length if the write exceeds the block size.
 */
static ssize_t osfs_write(struct file *filp, const char __user *buf, size_t len, loff_t *ppos)
{   

    struct inode *inode = file_inode(filp);
    struct osfs_inode *osfs_inode = inode->i_private;
    struct osfs_sb_info *sb_info = inode->i_sb->s_fs_info;
    void *data_block;
    ssize_t bytes_written;
    int ret;

    if (!osfs_inode || !sb_info) 
        return -EIO;

    // Upadte: // 1. Calculate which block index we need ----------------
    uint32_t block_idx = *ppos / sb_info->block_size;
    uint32_t block_offset = *ppos % sb_info->block_size;

    // Cannot write beyond data blocks array
    if (block_idx >= MAX_DIRECT_BLOCKS) 
        return -EFBIG;

    // Check if this specific block is allocatied. data_block
    if (osfs_inode->i_blocks_array[block_idx] == OSFS_INVALID_BLOCK) {
        ret = osfs_alloc_data_block(sb_info, &osfs_inode->i_blocks_array[block_idx]);
        if (ret) {
            pr_err("osfs_write: Failed to allocate data block at index %u\n", block_idx);
            return ret;
        }
        osfs_inode->i_blocks++;
        mark_inode_dirty(inode);
    }
    
    if (block_offset + len > sb_info->block_size)
        len = sb_info->block_size - block_offset;
        
    // Step4: Write data from user space to the data block
    data_block = (char *)sb_info->data_blocks + 
                 (osfs_inode->i_blocks_array[block_idx] * sb_info->block_size) + 
                 block_offset;

    // --- UPDATE END -------------------------------------


    if (copy_from_user(data_block, buf, len)) 
        return -EFAULT;

    // Step5: Update inode & osfs_inode attribute
    *ppos += len;
    bytes_written = len;

    if (*ppos > osfs_inode->i_size) {
        osfs_inode->i_size = *ppos;
        inode->i_size = *ppos;
    }

    // Step6: Return the number of bytes written
    inode_set_mtime_to_ts(inode, current_time(inode));
    inode_set_ctime_to_ts(inode, current_time(inode));
    mark_inode_dirty(inode);
    
    return bytes_written;
}

/**
 * Struct: osfs_file_operations
 * Description: Defines the file operations for regular files in osfs.
 */
const struct file_operations osfs_file_operations = {
    .open = generic_file_open, // Use generic open or implement osfs_open if needed
    .read = osfs_read,
    .write = osfs_write,
    .llseek = default_llseek,
    // Add other operations as needed
};

/**
 * Struct: osfs_file_inode_operations
 * Description: Defines the inode operations for regular files in osfs.
 * Note: Add additional operations such as getattr as needed.
 */
const struct inode_operations osfs_file_inode_operations = {
    // Add inode operations here, e.g., .getattr = osfs_getattr,
};
