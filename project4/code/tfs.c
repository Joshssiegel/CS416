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
//bitmap_t inode_bitmap;
//bitmap_t data_bitmap;
struct superblock* SB;
int inodes_per_block;
int num_entries_per_data_block;

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
	bitmap_t inode_bitmap=malloc(BLOCK_SIZE);
	bio_read(INODE_BITMAP_BLOCK, inode_bitmap);
	// Step 2: Traverse inode bitmap to find an available slot
	int i=0;
	printf("max inodes is: %d\n",SB->max_inum);
	for(i=0;i<SB->max_inum;i++){
		int bit=get_bitmap(inode_bitmap,i);
		printf("bit %d is %d\n",i,bit);
		if(get_bitmap(inode_bitmap,i)==0){
			printf("Available inode slot at inode num %d\n",i);
			// Step 3: Update inode bitmap and write to disk
			set_bitmap(inode_bitmap,i);
			bio_write(INODE_BITMAP_BLOCK,inode_bitmap);
			free(inode_bitmap);
			return i;
		}
	}
	printf("!!!!!!!! No available inode slots found.\n");
	return -1;

}

/*
 * Get available data block number from bitmap
 */
int get_avail_blkno() {



	// Step 1: Read data block bitmap from disk
	bitmap_t data_bitmap=malloc(BLOCK_SIZE);
	bio_read(DATA_BITMAP_BLOCK, data_bitmap);
	// Step 2: Traverse data block bitmap to find an available slot
	int i=0;
	for(i=0;i<SB->max_dnum;i++){
		if(get_bitmap(data_bitmap,i)==0){
			printf("Available data slot at data block num %d\n",i);
			// Step 3: Update data block bitmap and write to disk
			set_bitmap(data_bitmap,i);
			bio_write(DATA_BITMAP_BLOCK,data_bitmap);
			free(data_bitmap);
			return i;
		}
	}

	printf("!!!!!! No available data slots found.\n");
	return -1;
}

/*
 * inode operations
 */
int readi(uint16_t ino, struct inode *inode) {
	int retstatus=0;

  // Step 1: Get the inode's on-disk block number
	//get the block num by adding starting block by floor of inode num / inodes per block
	int blockNum=SB->i_start_blk+ino/inodes_per_block;
	void* iBlock = malloc(BLOCK_SIZE);
	//read in the entire block of inodes (there are multiple inodes per block)
	retstatus=bio_read(blockNum,iBlock);
	if(retstatus<0){
		perror(" Error reading disk: \n");
		return retstatus;
	}
  // Step 2: Get offset of the inode in the inode on-disk block
	//get the offset in the block where our inode starts
	int offset=(ino%inodes_per_block)*sizeof(struct inode);
	//get the starting address of the inode
	struct inode* inodePtr=(struct inode*)malloc(sizeof(struct inode));//iBlock+offset;
	memcpy(inodePtr, iBlock+offset, sizeof(struct inode));
	free(iBlock);
	printf("Asked for inode number %d in block %d, found inode number %d\n", ino, blockNum, inodePtr->ino);
  // Step 3: Read the block from disk and then copy into inode structure
	*inode=*inodePtr;

	return retstatus;
}

int writei(uint16_t ino, struct inode *inode) {
	int retstatus=0;
	// Step 1: Get the block number where this inode resides on disk
	int blockNum=SB->i_start_blk+ino/inodes_per_block;
	char* inodeBlock=malloc(BLOCK_SIZE);
	retstatus=bio_read(blockNum,inodeBlock);
	if(retstatus<0){
		perror(" Error reading disk: \n");
		return retstatus;
	}
	// Step 2: Get the offset in the block where this inode resides on disk
	//get the offset in the block where our inode starts
	int offset=(ino%inodes_per_block)*sizeof(struct inode);
	// Step 3: Write inode to disk
	//overwrite the inode at the offset in memory
	struct inode* addrOfInode=(struct inode*) (inodeBlock+offset); // make sure the cast doesn't make things break
	*addrOfInode=*inode;
	//write it back to disk
	retstatus=bio_write(blockNum,inodeBlock);
	if(retstatus<0){
		perror(" Error writing to disk: \n");
		return retstatus;
	}
	return retstatus;
}


/*
 * directory operations
 */
int dir_find(uint16_t ino, const char *fname, size_t name_len, struct dirent *dirent) {

  // Step 1: Call readi() to get the inode using ino (inode number of current directory)
	struct inode* dir_inode=malloc(sizeof(struct inode));
	int retstatus=readi(ino,dir_inode);
	if(retstatus<0){
		perror("error reading the directory's inode\n");
		return retstatus;
	}
  // Step 2: Get data block of current directory from inode
	int* dir_inode_data=dir_inode->direct_ptr;//if problems, try memcpy

  // Step 3: Read directory's data block and check each directory entry.
  //If the name matches, then copy directory entry to dirent structure

	//index of the direct pointer
	int data_block_index=0;
	//number of dir-entries per data block, now a GLOBAL
	//int num_entries_per_data_block=BLOCK_SIZE/sizeof(struct dirent);
	//the current dir entry to read from disk
	struct dirent* data_block=calloc(1,BLOCK_SIZE);
	//get the index of the dir-entry within the data block
	int dirent_index=0;

	//Check every dir_entry in every data block to see if name exists already
	for (data_block_index=0;data_block_index<16;data_block_index++){
		//check the data bitmap to see if this data block is empty
		if(dir_inode_data[data_block_index]==-1){
			printf("in find: data block # %d is empty\n",data_block_index);
			//skip to next data block
			continue;
		}
		else{
			printf("checking non-empty data block # %d \n",dir_inode_data[data_block_index]);

		}
		//read the allocated data block containing dir entries
		bio_read(SB->d_start_blk+dir_inode_data[data_block_index],data_block);
		//iterate through all the dir entries in this data block
		for(dirent_index=0;dirent_index<num_entries_per_data_block;dirent_index++){
			//get next dir entry (pointer addition is increments of dir entry)
			struct dirent* dir_entry=data_block+dirent_index;
			//if the directory entry is either NULL or invalid, it can be overwritten
			if(dir_entry==NULL || dir_entry->valid==0){
				//continue because we don't care about null or invalid entries' names
				continue;
			}
			//compare the name of the entry with the new entry name
			if(strcmp(dir_entry->name,fname)==0){
				printf("Found %s\n",fname);
				*dirent=*dir_entry;
				//returning the block number that we found the dir entr in.
				return dir_inode_data[data_block_index];
			}
			else{
				printf("In find: compared %s to %s\n",dir_entry->name,fname);
			}

		}
	}
	printf("did not find %s in dir find\n.",fname);
	return -1;
}

int dir_add(struct inode dir_inode, uint16_t f_ino, const char *fname, size_t name_len) {

	// Step 1: Read dir_inode's data block and check each directory entry of dir_inode
	//get the direct pointers of the parent's data blocks
	int* dir_inode_data=calloc(16,sizeof(int));
	memcpy(dir_inode_data,dir_inode.direct_ptr,16*sizeof(int));

	// Step 2: Check if fname (directory name) is already used in other entries
	//index of the direct pointer
	int data_block_index=0;
	//number of dir-entries per data block, now a GLOBAL
	//int num_entries_per_data_block=BLOCK_SIZE/sizeof(struct dirent);
	//the current dir entry to read from disk
	struct dirent* data_block=calloc(1,BLOCK_SIZE);
	//get the index of the dir-entry within the data block
	int dirent_index=0;
	//boolean flag to see if there is an empty dir-entry within a data block
	char found_empty_slot=0;
	//unallocated data block index
	int empty_unallocated_block=-1;
	// index of block where there is an empty dir-entry
	int empty_block_index=0;
	// index of empty dir-entry
	int empty_dirent_index=0;

	bitmap_t data_bitmap=malloc(BLOCK_SIZE);
	bio_read(DATA_BITMAP_BLOCK, data_bitmap);
	//Check every dir_entry in every data block to see if name exists already
	for (data_block_index=0;data_block_index<16;data_block_index++){
		//check the data bitmap to see if this data block is empty
		if(dir_inode_data[data_block_index]==-1){
			//TODO: make sure to initialize all direct_ptrs to -1 when creating the directory
			//If empty, then we set the unallocated block to this index and skip to the next data block
			if(empty_unallocated_block==-1){
				empty_unallocated_block = data_block_index;
			}
			//skip to next data block
			continue;
		}
		//read the allocated data block containing dir entries
		bio_read(SB->d_start_blk+dir_inode_data[data_block_index],data_block);
		//iterate through all the dir entries in this data block
		for(dirent_index=0;dirent_index<num_entries_per_data_block;dirent_index++){
			//get next dir entry (pointer addition is increments of dir entry)
			struct dirent* dir_entry=data_block+dirent_index;
			//if the directory entry is either NULL or invalid, it can be overwritten
			if(dir_entry==NULL || dir_entry->valid==0){
				//found an empty slot in an allocated block. Takes priority over unallocated block.
				if(found_empty_slot==0){
					found_empty_slot=1;
					empty_block_index=data_block_index;
					empty_dirent_index=dirent_index;
				}
				//continue because we don't care about null or invalid entries' names
				continue;
			}
			//compare the name of the entry with the new entry name
			if(strcmp(dir_entry->name,fname)==0){
				printf("the name %s already exists, NOT overwriting.\n",fname);
				return -1;
			}
			else{
				printf("%s doesn't equal %s\n",dir_entry->name,fname);
			}
		}
	}
	// Step 3: Add directory entry in dir_inode's data block and write to disk
	//create new dir entry
	struct dirent* new_entry=malloc(sizeof(struct dirent));
	//set it to be valid
	new_entry->valid=1;
	//inode was given
	new_entry->ino=f_ino;
	//copy the string name that was given
	strncpy(new_entry->name,fname,name_len);
	//empty dir-entry in allocated data block takes priority over unallocated data block
	if(found_empty_slot==1){
		printf("found an empty slot in allocated data block: %d at entry # %d\n",dir_inode_data[empty_block_index],empty_dirent_index);
		//read the data block that contains an empty dir-entry
		bio_read(SB->d_start_blk+dir_inode_data[empty_block_index],data_block);
		//store the new entry in the empty entry
		data_block[empty_dirent_index]=*new_entry;
		//write back to disk
		bio_write(SB->d_start_blk+dir_inode_data[empty_block_index],data_block);

		//TODO: Add time for parent inode
		//free the locally stored entry
		free(new_entry);
	}
	// Allocate a new data block for this directory if it does not exist
	else if(empty_unallocated_block>-1){
		//just double checking with this if statement
		if(dir_inode_data[empty_unallocated_block]==-1){//this is an unallocated block
			//get next available block num and set the bit to 1
			int block_num = get_avail_blkno();// get_avail_blkno will set the data_bitmap to 1 for this block
			//make the inode's direct pointer point to this data block
			dir_inode_data[empty_unallocated_block]=block_num;
			printf("set pointer num %d to data block %d\n",empty_unallocated_block,block_num);
			//malloc space for the new block
			struct dirent* new_data_block=malloc(BLOCK_SIZE);
			//set the first entry of the new data block to store the new entry
			*new_data_block=*new_entry;
			//write to disk
			bio_write(SB->d_start_blk+block_num,new_data_block);

			free(new_data_block);
			free(new_entry);
		}
		else{
			printf("We done messed up.\n");
			free(new_entry);
		}
	}
	else{
		//No free data blocks in this directory
		printf("No free data blocks in directory.\n");
		//remove the bit for the inode that was passed in since we aren't creating it.
		bitmap_t inode_bitmap=malloc(BLOCK_SIZE);
		//update the bitmap
		bio_read(INODE_BITMAP_BLOCK, inode_bitmap);
		unset_bitmap(inode_bitmap,f_ino);
		bio_write(INODE_BITMAP_BLOCK, inode_bitmap);
		free(inode_bitmap);
		free(data_bitmap);
		free(data_block);
		free(dir_inode_data);
		return -1;
	}
	// Update directory inode

	//update parent inode
	int parent_ino=dir_inode.ino;
	struct inode* parent_inode=malloc(sizeof(struct inode));
	readi(parent_ino,parent_inode);
	//update size of parent directory
	parent_inode->size+=sizeof(struct dirent);

	//update the direct_ptrs
	//parent_inode->direct_ptr=dir_inode_data;
	memcpy(parent_inode->direct_ptr,dir_inode_data,16*sizeof(int));

	//update access time of parent
	time(& (parent_inode->vstat.st_mtime));
	// Write directory entry
	writei(parent_ino,parent_inode);
	free(parent_inode);
	free(data_bitmap);
	free(data_block);
	free(dir_inode_data);
	return 0;
}

int dir_remove(struct inode dir_inode, const char *fname, size_t name_len) {

	// Step 1: Read dir_inode's data block and checks each directory entry of dir_inode
	uint16_t parent_ino=dir_inode.ino;
	struct dirent *dirent_to_remove=calloc(1, sizeof(struct dirent));
	// Step 2: Check if fname exist
	int data_block_num=dir_find(parent_ino, fname, name_len, dirent_to_remove);
	if(data_block_num<0 || dirent_to_remove->valid==0){
		printf("%s is not in the directory, cannot remove.\n",fname);
		return data_block_num;
	}
	printf("data block of dir entry to remove is %d\n",data_block_num);
	// Step 3: If exist, then remove it from dir_inode's data block (set valid to 0) and write to disk
	//first set te dirent's valid bit to 0
	struct dirent* data_block=malloc(BLOCK_SIZE);
	// read block
	bio_read(SB->d_start_blk+data_block_num,data_block);
	int i=0;
	int num_valid_entries=0;
	for(i=0;i<num_entries_per_data_block;i++){
		struct dirent* dir_entry=data_block+i;
		//printf("checking inode # %d\n",dir_entry->ino);

		//remove directory entry by setting it's valid bit to 0
		if(dir_entry->ino==dirent_to_remove->ino){
			dir_entry->valid=0;

			printf("removed ino %d\n",dir_entry->ino);
		}
		else if(dir_entry->valid==1){
				num_valid_entries+=1;
		}

	}
	//commit changes to disk
	bio_write(SB->d_start_blk+data_block_num,data_block);

	free(data_block);
	printf("number of valid entries after removing %d\n",num_valid_entries);
	if(num_valid_entries==0){
//
//
// //test
// struct inode* d_inode=malloc(sizeof(struct inode));
// int retstatus=readi(parent_ino,d_inode);
// if(retstatus<0){
// 	perror("error reading the directory's inode\n");
// 	return retstatus;
// }
// int* dir_inode_data=d_inode->direct_ptr;//if problems, try memcpy

//end test
		//no valid entries left in the data block. Remove the data block.
		for(i=0;i<16;i++){
			printf("comparing direct pointer %d to %d with block to remove: %d\n",i,dir_inode.direct_ptr[i],data_block_num);
			//search for the pointer to the data block, so we can remove it
			if(dir_inode.direct_ptr[i]==data_block_num){
				printf("data block %d is now empty. Removing.\n",dir_inode.direct_ptr[i]);
				struct inode* parent_inode=malloc(sizeof(struct inode));
				readi(parent_ino,parent_inode);
				parent_inode->direct_ptr[i]=-1;
				writei(parent_ino,parent_inode);
				printf("set direct pointer index %d to -1\n",i );
				free(parent_inode);

				//clear the data bitmap since we removed this data block
				bitmap_t data_bitmap=malloc(BLOCK_SIZE);
				bio_read(DATA_BITMAP_BLOCK,data_bitmap);
				unset_bitmap(data_bitmap,parent_ino);
				bio_write(DATA_BITMAP_BLOCK,data_bitmap);
				free(data_bitmap);
				break;
			}
		}
	}


	//If you remove an entire data block, set it's bit to 0 and
	//the direct pointer to -1 !!!
	printf("removed succefully\n");
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
	sb->d_start_blk=sb->i_start_blk+ceil((float)(((float)(sb->max_inum))/((float)inodes_per_block)));
	printf("Initialized start of data_block to: %d\n", sb->d_start_blk);
	//write the superblock
  if(bio_write(0,sb)<0){
		printf("error writing the superblock to the disk. Exiting program.\n");
		exit(-1);
	}
	SB=sb;
	// initialize inode bitmap
	//TODO: Do we need to ceil?
	bitmap_t inode_bitmap=calloc(1,sb->max_inum/8);
	// initialize data block bitmap
	bitmap_t data_bitmap=calloc(1,sb->max_dnum/8);
	// update bitmap information for root directory
	set_bitmap(inode_bitmap,0);
	bio_write(INODE_BITMAP_BLOCK,inode_bitmap);
	bio_write(DATA_BITMAP_BLOCK,data_bitmap);
	// update inode for root directory
	struct inode* root_inode = calloc(1,sizeof(struct inode));
	root_inode->ino=0;				/* inode number */
	root_inode->valid=1;				/* validity of the inode */
	root_inode->size=0; //TODO: change to size of root dir				/* size of the file */
	root_inode->type=TFS_DIRECTORY;				/* type of the file */
	root_inode->link=1;				/* link count */
	int i=0;
	for(i=0;i<16;i++){
		root_inode->direct_ptr[i]=-1;
	}
	//Update access time within vstat?
	struct stat* vstat=malloc(sizeof(struct stat));
	vstat->st_mode   = S_IFDIR | 0755;
	time(& vstat->st_mtime);
	root_inode->vstat=*vstat;			/* inode stat */
	writei(0,root_inode);
	return 0;
}


/*
 * FUSE file operations
 */
static void *tfs_init(struct fuse_conn_info *conn) {
	printf("init************\n");
	inodes_per_block=BLOCK_SIZE/sizeof(struct inode);
	num_entries_per_data_block=BLOCK_SIZE/sizeof(struct dirent);

	// Step 1a: If disk file is not found, call mkfs
	disk=dev_open(diskfile_path);
	if(disk==-1){
		if(tfs_mkfs()!=0){
			printf("error making disk\n");
		}
	}
	// Step 1b: If disk file is found, just initialize in-memory data structures
  // and read superblock from disk
	else
		{
			printf("Disk is %d\n",disk);
			SB=(struct superblock*) malloc(sizeof(struct superblock));
			bio_read(0,(void*) SB);
	}


	printf("inode start location: %d \n",SB->i_start_blk );

	printf("inodes per block: %d with an inode size of %d\n",(int)inodes_per_block,(int)sizeof(struct inode));
	printf("\n------------TESTING-----------------\n");
	struct inode* test_inode = calloc(1,sizeof(struct inode));
	test_inode->ino=get_avail_ino();				/* inode number */
	test_inode->valid=1;				/* validity of the inode */
	test_inode->size=0; //TODO: change to size of root dir				/* size of the file */
	test_inode->type=TFS_FILE;				/* type of the file */
	test_inode->link=1;				/* link count */

  writei(test_inode->ino,test_inode);
	printf("Wrote inode %d\n",test_inode->ino);
	struct inode* test2_inode = calloc(1,sizeof(struct inode));
	readi(1,test2_inode);
	printf("test 2's inode number is: %d\n",test2_inode->ino);
	struct inode* root_inode = calloc(1,sizeof(struct inode));
	readi(0,root_inode);
	printf("root's inode number is: %d\n",root_inode->ino);
	dir_add(*root_inode, 1, "foo\0", 4);
	struct dirent *foo_dirent=calloc(1,sizeof(struct dirent));
	dir_find(0, "foo\0", 4, foo_dirent);
	printf("foo's dirent name is %s\n",foo_dirent->name);
	readi(0,root_inode);
	dir_remove(*root_inode, "foo\0", 4);
	printf("foo has been removed\n");
	dir_find(0, "foo\0", 4, foo_dirent);
	free(foo_dirent);
	foo_dirent=calloc(1,sizeof(struct dirent));
	printf("foo's dirent name is %s\n",foo_dirent->name);


	//Update access time within vstat?
	// for(i=0;i<SB->max_dnum;i++){
	// 	printf("available blkno %d\n ",get_avail_blkno());
	// }
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

	//TODO: initialize all direct_ptrs to -1 when creating the directory
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
