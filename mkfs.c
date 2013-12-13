/* test1.c

   Spawn a single thread.
*/

#include "minithread.h"
#include "minifile.h"
#include "disk.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>


int thread(int* arg) {
	int i;
	char buffer[DISK_BLOCK_SIZE];
	superblock_t new_superblock = (superblock_t) malloc(sizeof(struct superblock));
	inode_t root = (inode_t) malloc(sizeof(struct inode));

	new_superblock->data.magic_number = 4411;
	new_superblock->data.next_free_data_block = ceil(disk_size * 0.1);
	new_superblock->data.root = 0;
	new_superblock->data.next_free_inode = 1;
	new_superblock->data.size_of_disk = disk_size;
	disk_write_block(get_filesystem(), -1, new_superblock->padding);

	root->data.size = 0;
	root->data.type = INODE_DIR;
	disk_write_block(get_filesystem(), 0, root->padding);

	for(i = 1; i < disk_size - 2; i++) {
		memset(buffer, 0, DISK_BLOCK_SIZE);
		buffer[3] = (i>>24) & 0xff;
		buffer[2] = (i>>16) & 0xff;
		buffer[1] = (i>>8) & 0xff;
		buffer[0] = i & 0xff;
		protected_write(get_filesystem(), i, buffer);
	}
	memset(buffer, 0, DISK_BLOCK_SIZE);
	disk_write_block(get_filesystem(), disk_size - 2, buffer);
	disk_write_block(get_filesystem(), ceil(disk_size * 0.1) - 1, buffer);
	// intialize first free data block to ceil(disk_size * 0.1)
	// initialize first free inode to 1
	// write superblock at block -1
	// write root inode at block 0
	// write each data block with pointer to next data block
	// last data block is null

	printf("File system created\n");

	return 0;
}

void main(void) {
	minithread_system_initialize(thread, NULL);
}
