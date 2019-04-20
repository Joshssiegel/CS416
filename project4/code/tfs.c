/*
 *  Copyright (C) 2019 CS416 Spring 2019
 *
 *	Tiny File System
 *
 *	File:	tfs.c
 *  Author: Yujie REN
 *	Date:	April 2019
 *
 */

#define FUSE_USE_VERSION 26

#include <fuse.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>
#include <sys/time.h>
#include <libgen.h>
#include <limits.h>

#include "block.h"
#include "tfs.h"

char diskfile_path[PATH_MAX];

// Declare your in-memory data structures here
int disk;
bitmap_t inode_bitmap;
bitmap_t data_bitmap;
struct superblock* SB;
int inodes_per_block;

struct inode* getInode(int inum){
	//get the block num by adding starting block by floor of inode num / inodes per block
	int blockNum=SB->i_start_blk+inum/inodes_per_block;
	void* iBlock = malloc(BLOCK_SIZE);
	//read in the entire block of inodes (there are multiple inodes per block)
	bio_read(blockNum,iBlock);
	//get the offset in the block where our inode starts
	int offset=(inum%inodes_per_block)*sizeof(struct inode);
	//get the starting address of the inode
	struct inode* inodePtr=(struct inode*)malloc(sizeof(struct inode));//iBlock+offset;
	memcpy(inodePtr, iBlock+offset, sizeof(struct inode));
	free(iBlock);
	printf("Asked for inode number %d in block %d, found inode number %d\n", inum, blockNum, inodePtr->ino);

	return inodePtr;
}


/*
 * Get available inode number from bitmap
 */
int get_avail_ino() {

	// Step 1: Read inode bitmap from disk

	// Step 2: Traverse inode bitmap to find an available slot

	// Step 3: Update inode bitmap and write to disk

	return 0;
}

/*
 * Get available data block number from bitmap
 */
int get_avail_blkno() {

	// Step 1: Read data block bitmap from disk

	// Step 2: Traverse data block bitmap to find an available slot

	// Step 3: Update data block bitmap and write to disk

	return 0;
}

/*
 * inode operations
 */
int readi(uint16_t ino, struct inode *inode) {

  // Step 1: Get the inode's on-disk block number

  // Step 2: Get offset of the inode in the inode on-disk block

  // Step 3: Read the block from disk and then copy into inode structure

	return 0;
}

int writei(uint16_t ino, struct inode *inode) {

	// Step 1: Get the block number where this inode resides on disk
	int blockNum=ino/inodes_per_block;
	char* inodeBlock=malloc(BLOCK_SIZE);
	bio_read(blockNum,inodeBlock);
	// Step 2: Get the offset in the block where this inode resides on disk
	//get the offset in the block where our inode starts
	int offset=(ino%inodes_per_block)*sizeof(struct inode);
	// Step 3: Write inode to disk
	//overwrite the inode at the offset in memory
	struct inode* addrOfInode=inodeBlock+offset;
	*addrOfInode=*inode;
	//write it back to disk
	bio_write(blockNum,inodeBlock);
	return 0;
}


/*
 * directory operations
 */
int dir_find(uint16_t ino, const char *fname, size_t name_len, struct dirent *dirent) {

  // Step 1: Call readi() to get the inode using ino (inode number of current directory)

  // Step 2: Get data block of current directory from inode

  // Step 3: Read directory's data block and check each directory entry.
  //If the name matches, then copy directory entry to dirent structure

	return 0;
}

int dir_add(struct inode dir_inode, uint16_t f_ino, const char *fname, size_t name_len) {

	// Step 1: Read dir_inode's data block and check each directory entry of dir_inode

	// Step 2: Check if fname (directory name) is already used in other entries

	// Step 3: Add directory entry in dir_inode's data block and write to disk

	// Allocate a new data block for this directory if it does not exist

	// Update directory inode

	// Write directory entry

	return 0;
}

int dir_remove(struct inode dir_inode, const char *fname, size_t name_len) {

	// Step 1: Read dir_inode's data block and checks each directory entry of dir_inode

	// Step 2: Check if fname exist

	// Step 3: If exist, then remove it from dir_inode's data block and write to disk

	return 0;
}

/*
 * namei operation
 */
int get_node_by_path(const char *path, uint16_t ino, struct inode *inode) {

	// Step 1: Resolve the path name, walk through path, and finally, find its inode.
	// Note: You could either implement it in a iterative way or recursive way

	return 0;
}

/*
 * Make file system
 */
int tfs_mkfs() {
	printf("Making file system\n");
	// Call dev_init() to initialize (Create) Diskfile
	dev_init(diskfile_path);
	//Open the diskfile
	disk=dev_open(diskfile_path);
	if(disk==-1){
		printf("error opening the disk. Exiting program.\n");
		exit(-1);
	}

	// write superblock information
	struct superblock* sb=malloc(sizeof(struct superblock));
	sb->magic_num=MAGIC_NUM;
	sb->max_inum=MAX_INUM;
	sb->max_dnum=MAX_DNUM;

	//store inode bitmap in block 1 (0 for superblock)
	sb->i_bitmap_blk=1;
	//store data bitmap in block 2
	sb->d_bitmap_blk=2;
	//store inode blocks starting in block 3
	sb->i_start_blk=3;
	printf("Made inode start block 3\n");
	//TODO: change if we store more than one inode per block
	//TODO: Ceil or naw?
	sb->d_start_blk=sb->i_start_blk+sb->max_inum+1;
	//write the superblock
  if(bio_write(0,sb)<0){
		printf("error writing the superblock to the disk. Exiting program.\n");
		exit(-1);
	}

	// initialize inode bitmap
	//TODO: Do we need to ceil?
	inode_bitmap=calloc(1,sb->max_inum/8);
	// initialize data block bitmap
	data_bitmap=calloc(1,sb->max_dnum/8);
	// update bitmap information for root directory
	set_bitmap(inode_bitmap,0);
	// update inode for root directory
	struct inode* root_inode = calloc(1,sizeof(struct inode));
	root_inode->ino=1;				/* inode number */
	root_inode->valid=1;				/* validity of the inode */
	root_inode->size=0; //TODO: change to size of root dir				/* size of the file */
	root_inode->type=TFS_DIRECTORY;				/* type of the file */
	root_inode->link=1;				/* link count */
	//Update access time within vstat?
	struct stat* vstat=malloc(sizeof(struct stat));
	vstat->st_mode   = S_IFDIR | 0755;
	time(&vstat->st_mtime);
	root_inode->vstat=*vstat;			/* inode stat */
	return 0;
}


/*
 * FUSE file operations
 */
static void *tfs_init(struct fuse_conn_info *conn) {
	printf("init************\n");

	// Step 1a: If disk file is not found, call mkfs
	disk=dev_open(diskfile_path);
	if(disk==-1){
		if(tfs_mkfs()!=0){
			printf("error making disk\n");
		}
	}
	else
		{printf("Disk is %d\n",disk);}
  // Step 1b: If disk file is found, just initialize in-memory data structures
  // and read superblock from disk
	SB=(struct superblock*) malloc(sizeof(struct superblock));
	bio_read(0,(void*) SB);
	printf("Superblock inode start location: %d \n",SB->i_start_blk );
	inodes_per_block=BLOCK_SIZE/sizeof(struct inode);
	printf("inodes per block: %d with an inode size of %d\n",(int)inodes_per_block,(int)sizeof(struct inode));
	return NULL;
}

static void tfs_destroy(void *userdata) {

	// Step 1: De-allocate in-memory data structures

	// Step 2: Close diskfile
	dev_close(disk);

}

static int tfs_getattr(const char *path, struct stat *stbuf) {

	// Step 1: call get_node_by_path() to get inode from path

	// Step 2: fill attribute of file into stbuf from inode

		stbuf->st_mode   = S_IFDIR | 0755;
		stbuf->st_nlink  = 2;
		time(&stbuf->st_mtime);

	return 0;
}

static int tfs_opendir(const char *path, struct fuse_file_info *fi) {

	// Step 1: Call get_node_by_path() to get inode from path

	// Step 2: If not find, return -1

    return 0;
}

static int tfs_readdir(const char *path, void *buffer, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi) {

	// Step 1: Call get_node_by_path() to get inode from path

	// Step 2: Read directory entries from its data blocks, and copy them to filler

	return 0;
}


static int tfs_mkdir(const char *path, mode_t mode) {

	// Step 1: Use dirname() and basename() to separate parent directory path and target directory name

	// Step 2: Call get_node_by_path() to get inode of parent directory

	// Step 3: Call get_avail_ino() to get an available inode number

	// Step 4: Call dir_add() to add directory entry of target directory to parent directory

	// Step 5: Update inode for target directory

	// Step 6: Call writei() to write inode to disk


	return 0;
}

static int tfs_rmdir(const char *path) {

	// Step 1: Use dirname() and basename() to separate parent directory path and target directory name

	// Step 2: Call get_node_by_path() to get inode of target directory

	// Step 3: Clear data block bitmap of target directory

	// Step 4: Clear inode bitmap and its data block

	// Step 5: Call get_node_by_path() to get inode of parent directory

	// Step 6: Call dir_remove() to remove directory entry of target directory in its parent directory

	return 0;
}

static int tfs_releasedir(const char *path, struct fuse_file_info *fi) {
	// For this project, you don't need to fill this function
	// But DO NOT DELETE IT!
    return 0;
}

static int tfs_create(const char *path, mode_t mode, struct fuse_file_info *fi) {
	printf("***********************in tfs_create\n");
	// Step 1: Use dirname() and basename() to separate parent directory path and target file name

	// Step 2: Call get_node_by_path() to get inode of parent directory

	// Step 3: Call get_avail_ino() to get an available inode number

	// Step 4: Call dir_add() to add directory entry of target file to parent directory

	// Step 5: Update inode for target file

	// Step 6: Call writei() to write inode to disk

	return 0;
}

static int tfs_open(const char *path, struct fuse_file_info *fi) {

	// Step 1: Call get_node_by_path() to get inode from path

	// Step 2: If not find, return -1

	return 0;
}

static int tfs_read(const char *path, char *buffer, size_t size, off_t offset, struct fuse_file_info *fi) {

	// Step 1: You could call get_node_by_path() to get inode from path

	// Step 2: Based on size and offset, read its data blocks from disk

	// Step 3: copy the correct amount of data from offset to buffer

	// Note: this function should return the amount of bytes you copied to buffer
	return 0;
}

static int tfs_write(const char *path, const char *buffer, size_t size, off_t offset, struct fuse_file_info *fi) {
	// Step 1: You could call get_node_by_path() to get inode from path

	// Step 2: Based on size and offset, read its data blocks from disk

	// Step 3: Write the correct amount of data from offset to disk

	// Step 4: Update the inode info and write it to disk

	// Note: this function should return the amount of bytes you write to disk
	return size;
}

static int tfs_unlink(const char *path) {

	// Step 1: Use dirname() and basename() to separate parent directory path and target file name

	// Step 2: Call get_node_by_path() to get inode of target file

	// Step 3: Clear data block bitmap of target file

	// Step 4: Clear inode bitmap and its data block

	// Step 5: Call get_node_by_path() to get inode of parent directory

	// Step 6: Call dir_remove() to remove directory entry of target file in its parent directory

	return 0;
}

static int tfs_truncate(const char *path, off_t size) {
	// For this project, you don't need to fill this function
	// But DO NOT DELETE IT!
    return 0;
}

static int tfs_release(const char *path, struct fuse_file_info *fi) {
	// For this project, you don't need to fill this function
	// But DO NOT DELETE IT!
	return 0;
}

static int tfs_flush(const char * path, struct fuse_file_info * fi) {
	// For this project, you don't need to fill this function
	// But DO NOT DELETE IT!
    return 0;
}

static int tfs_utimens(const char *path, const struct timespec tv[2]) {
	// For this project, you don't need to fill this function
	// But DO NOT DELETE IT!
    return 0;
}


static struct fuse_operations tfs_ope = {
	.init		= tfs_init,
	.destroy	= tfs_destroy,

	.getattr	= tfs_getattr,
	.readdir	= tfs_readdir,
	.opendir	= tfs_opendir,
	.releasedir	= tfs_releasedir,
	.mkdir		= tfs_mkdir,
	.rmdir		= tfs_rmdir,

	.create		= tfs_create,
	.open		= tfs_open,
	.read 		= tfs_read,
	.write		= tfs_write,
	.unlink		= tfs_unlink,

	.truncate   = tfs_truncate,
	.flush      = tfs_flush,
	.utimens    = tfs_utimens,
	.release	= tfs_release
};


int main(int argc, char *argv[]) {
	int fuse_stat;

	getcwd(diskfile_path, PATH_MAX);
	strcat(diskfile_path, "/DISKFILE");

	fuse_stat = fuse_main(argc, argv, &tfs_ope, NULL);

	return fuse_stat;
}
