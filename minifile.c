#include "minifile.h"
#include "disk.h"
#include "minithread.h"
#include "hashtable.h"
#include "synch.h"

hashtable_t pending_reads;
hashtable_t pending_writes;

void minifile_initialize(void) {
	pending_reads = hashtable_new(disk_size);
	pending_writes = hashtable_new(disk_size);
}

hashtable_t get_pending_reads(void) {
	return pending_reads;
}

hashtable_t get_pending_writes(void) {
	return pending_writes;
}

int protected_read(disk_t* disk, int blocknum, char* buffer) {
	char key[5];
	semaphore_t wait = semaphore_create();
	semaphore_initialize(wait,0);
	sprintf(key,"%d",blocknum);
	hashtable_put(pending_reads,key, wait);
	disk_read_block(disk, blocknum, buffer);
	semaphore_P(wait);
	hashtable_remove(pending_reads,key);
}

int protected_write(disk_t* disk, int blocknum, char* buffer) {
	char key[5];
	semaphore_t wait = semaphore_create();
	semaphore_initialize(wait,0);
	sprintf(key,"%d",blocknum);
	hashtable_put(pending_writes,key, wait);
	disk_write_block(disk, blocknum, buffer);
	semaphore_P(wait);
	hashtable_remove(pending_writes,key);
}

// returns the first token before the slash
char* get_first_token(char* string) {
	int i = 0;
	char* buffer = (char*) malloc(252);
	memset(buffer, 0, 252);
	while(string[i] != '\0' && string[i] != '/') {
		i++;
	}
	memcpy(buffer,string,i);
	buffer[i+1] = '\0';
	return buffer;
}


/* Make sure that path does not begin with slash */
inode_t resolve_absolute_path(char* path, inode_t cwd) {
	int i,j;
	inode_t resolved_path_inode = (inode_t) malloc(sizeof(struct inode));
	data_block_t temp_data_block = (data_block_t) malloc(sizeof(struct data_block));
	inode_t cwd_copy = (inode_t) malloc(sizeof(struct inode));

	memcpy(cwd_copy,cwd,sizeof(struct inode));
	free(cwd);

	if(path[0] == '\0') {
		printf("[INFO] Could not resolve empty path \n");
		free(temp_data_block);
		free(cwd_copy);
		free(resolved_path_inode);
		return NULL;
	}

	// direct blocks
	for(i = 0; i < DISK_BLOCK_SIZE - sizeof(type_t) - sizeof(size_t) - sizeof(data_block_t); i++) {

		// directory data block contents (items)
		for(j = 0; j < DISK_BLOCK_SIZE / (sizeof(struct item)); j++) {
			// item (match name)
			if(strlen(path) == 0) {
				free(temp_data_block);
				free(resolved_path_inode);
				return cwd_copy;
			}
			else{
				protected_read(get_filesystem(),cwd_copy->data.direct[i],temp_data_block->padding);
				
				// item (match name)
				if(0 == strcmp(temp_data_block->dir_contents.items[j].name, get_first_token(path))) {

					protected_read(get_filesystem(),temp_data_block->dir_contents.items[j].blocknum,resolved_path_inode->padding);
					
					resolved_path_inode = resolve_absolute_path(path += strlen(get_first_token(path)) + 1, resolved_path_inode);
					//path += strlen(get_first_token(path)) + 1;
					//i = 0;
					free(temp_data_block);
					free(cwd_copy);
					return resolved_path_inode;
				} 
				else if (temp_data_block->dir_contents.items[j].blocknum ==  1) {
					continue;
				}
				else if (temp_data_block->dir_contents.items[j].blocknum == 0) {
					printf("[DEBUG] Path could not be resolved \n");
					free(temp_data_block);
					free(cwd_copy);
					free(resolved_path_inode);
					return NULL;
				}
				else {
					continue;
				}
			}

			
			/*for(k = 0;;k++) {
				if((cwd->data.direct[i]->dir_contents.items[j].name[k] != NULL 
					&& path[match_char] != NULL 
					&& cwd->data.direct[i]->dir_contents.items[j].name[k] != path[match_char])
					||
					(cwd->data.direct[i]->dir_contents.items[j].name[k] == NULL
					^ path[match_char] == NULL)) {
					break;
				} else {

				}
			}*/
		}
	}
}

inode_t resolve_relative_path(char* path, inode_t cwd) {
	int i,j;
	inode_t resolved_path_inode = (inode_t) malloc(sizeof(struct inode));
	data_block_t temp_data_block = (data_block_t) malloc(sizeof(struct data_block));
	inode_t cwd_copy = (inode_t) malloc(sizeof(struct inode));

	memcpy(cwd_copy,cwd,sizeof(struct inode));
	free(cwd);

	if(path[0] == '\0') {
		printf("[INFO] Could not resolve empty path \n");
		free(temp_data_block);
		free(cwd_copy);
		free(resolved_path_inode);
		return NULL;
	}

	// direct blocks
	for(i = 0; i < DISK_BLOCK_SIZE - sizeof(type_t) - sizeof(size_t) - sizeof(data_block_t); i++) {

		// directory data block contents (items)
		for(j = 0; j < DISK_BLOCK_SIZE / (sizeof(struct item)); j++) {
			// item (match name)
			if(strlen(path) == 0) {
				free(temp_data_block);
				free(resolved_path_inode);
				return cwd;
			}
			else{
				protected_read(get_filesystem(),cwd->data.direct[i],temp_data_block->padding);
				
				// item (match name)
				if(0 == strcmp(temp_data_block->dir_contents.items[j].name, get_first_token(path))) {

					protected_read(get_filesystem(),temp_data_block->dir_contents.items[j].blocknum,resolved_path_inode->padding);
					
					resolved_path_inode = resolve_absolute_path(path += strlen(get_first_token(path)) + 1, resolved_path_inode);
					//path += strlen(get_first_token(path)) + 1;
					//i = 0;
					free(temp_data_block);
					free(cwd_copy);
					return resolved_path_inode;
				} 
				else if (temp_data_block->dir_contents.items[j].blocknum ==  1) {
					continue;
				}
				else if (temp_data_block->dir_contents.items[j].blocknum == 0) {
					printf("[DEBUG] Path could not be resolved \n");
					free(temp_data_block);
					free(cwd_copy);
					free(resolved_path_inode);
					return NULL;
				}
				else {
					continue;
				}
			}
		}
	}

	// indirect
}

minifile_t minifile_creat(char *filename){
	printf("[DEBUG] Entered command: creat \n");
	



	return NULL;
}

minifile_t minifile_open(char *filename, char *mode){
	printf("[DEBUG] Entered command: open \n");
	return NULL;
}

int minifile_read(minifile_t file, char *data, int maxlen){
	printf("[DEBUG] Entered command: read \n");
	return -1;
}

int minifile_write(minifile_t file, char *data, int len){
	char* temp_data = (char*) malloc(4096);
	char key[5];
	semaphore_t wait;
	memset(temp_data, 0, 4096);
	sprintf(key, "%d", -1);
	sprintf(temp_data, "hahahaha");
	printf("[DEBUG] Entered command: write \n");
	disk_write_block(get_filesystem(), 8, temp_data);

	memset(temp_data, 0, 4096);
	sprintf(temp_data, "lololol");
	disk_write_block(get_filesystem(), -1, temp_data);

	protected_read(get_filesystem(), -1, temp_data);

	if(hashtable_get(pending_reads,key,(void**) &wait) != 0) {
		printf("[ERROR] wait semaphore for read was not in hashtable \n");
	}

	semaphore_P(wait);

	// temp_data is ready

	temp_data[30] = '\0';
	printf(temp_data);

	return 0;
}

int minifile_close(minifile_t file){
	printf("[DEBUG] Entered command: close \n");
	return -1;
}

int minifile_unlink(char *filename){
	printf("[DEBUG] Entered command: unlink \n");
	return -1;
}

int minifile_mkdir(char *dirname){
	printf("[DEBUG] Entered command: mkdir \n");
	return -1;
}

int minifile_rmdir(char *dirname){
	printf("[DEBUG] Entered command: rmdir \n");
	return -1;
}

int minifile_stat(char *path){
	printf("[DEBUG] Entered command: stat \n");
	return -1;
} 

int minifile_cd(char *path){
	printf("[DEBUG] Entered command: cd \n");
	return -1;
}

char **minifile_ls(char *path){
	inode_t returned_node;
	char* buffer;
	char* *list;
	int block_iter = 0;
	int item_iter = 0;

	if(path[0] == '\\'){
		returned_node = resolve_absolute_path(path, get_current_working_directory());
	}
	else{
		returned_node = resolve_relative_path(path, get_current_working_directory());
	}

	if(returned_node == NULL){
		list = (char* *)malloc(sizeof(char*));
		list[0] = NULL;
		return list;
	}

	list = (char* *)malloc(sizeof(returned_node->data.size + 1) * sizeof(char*));
	list[returned_node->data.size] = NULL;

	if(returned_node->data.size == 0){
		free(returned_node);
		return list;
	}
	
	protected_read(get_filesystem(),returned_node->data.direct[block_iter],buffer);
	block_iter++;
	while(((data_block_t)buffer)->dir_contents.items[item_iter].blocknum != 0){
		
		if(((data_block_t)buffer)->dir_contents.items[item_iter].blocknum != 1){
			list[item_iter] = ((data_block_t)buffer)->dir_contents.items->name;
		}
		item_iter++;
		if(item_iter == DISK_BLOCK_SIZE / sizeof(struct item)){
			item_iter = 0;
			protected_read(get_filesystem(),returned_node->data.direct[block_iter],buffer);
			block_iter++;
		}
	}

	free(returned_node);
	return list;
}

char* minifile_pwd(void){
	return NULL;
}
