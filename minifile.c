#include "minifile.h"
#include "disk.h"
#include "minithread.h"

enum inode_type {
	INODE_FILE = 1, INODE_DIR = 2
};

typedef struct item{
	char name[252];
	inode_t pointer;
} item_t;

struct data_block {
	union {
		struct {
			item_t items[DISK_BLOCK_SIZE / (sizeof(struct item))];
		} dir_contents;
		
		char padding[DISK_BLOCK_SIZE];
	};
};

struct inode {
	union {
		struct {
			// inode metadata
			type_t type;
			size_t size;
			data_block_t direct[DISK_BLOCK_SIZE - sizeof(type_t) - sizeof(size_t) - 4];
			data_block_t indirect;
		} data;
		
		char padding[DISK_BLOCK_SIZE];
	};
};

struct superblock {
	union {
		struct {
			// Members of superblock here
			int magic_number;
			size_t size_of_disk;
			inode_t root;
			inode_t first_free_inode;
			data_block_t first_free_data_block;
		} data;
		
		char padding[DISK_BLOCK_SIZE];
	};
};


/*
 * struct minifile:
 *     This is the structure that keeps the information about 
 *     the opened file like the position of the cursor, etc.
 */

struct minifile {
  int size;
};

char* get_first_token(char* string) {
	int i = 0;
	char* buffer = (char*) malloc(252);
	memset(buffer, 0, 252);
	while(string[i] != '\0' && string[i] != '\\') {
		i++;
	}
	memcpy(buffer,string,i);
	buffer[i+1] = '\0';
	return buffer;
}

/* Make sure that path does not begin with slash */
inode_t resolve_absolute_path(char* path) {
	int i,j;
	inode_t cwd = get_root_directory();

	if(path[0] == '\0') {
		printf("[INFO] Could not resolve empty path \n");
		return NULL;
	}

	// direct blocks
	for(i = 0; i < DISK_BLOCK_SIZE - sizeof(type_t) - sizeof(size_t) - sizeof(data_block_t); i++) {

		// directory data block contents (items)
		for(j = 0; j < DISK_BLOCK_SIZE / (sizeof(struct item)); j++) {

			// item (match name)
			if(0 == strcmp(cwd->data.direct[i]->dir_contents.items[j].name, get_first_token(path))) {
				cwd = cwd->data.direct[i]->dir_contents.items[j].pointer;
				path += strlen(get_first_token(path)) + 1;
				i = 0;
				break;
			} else if (cwd->data.direct[i]->dir_contents.items[j].pointer == (inode_t) 1) {
				continue;
			} else {
				printf("[DEBUG] Patch could not be resolved \n");
				return NULL;
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

inode_t resolve_relative_path(char* path) {
	int i,j;
	inode_t cwd = get_current_working_directory();

	if(path[0] == '\0') {
		printf("[INFO] Could not resolve empty path \n");
		return NULL;
	}

	// direct blocks
	for(i = 0; i < DISK_BLOCK_SIZE - sizeof(type_t) - sizeof(size_t) - sizeof(data_block_t); i++) {

		// directory data block contents (items)
		for(j = 0; j < DISK_BLOCK_SIZE / (sizeof(struct item)); j++) {

			// item (match name)
			if(0 == strcmp(cwd->data.direct[i]->dir_contents.items[j].name, get_first_token(path))) {
				cwd = cwd->data.direct[i]->dir_contents.items[j].pointer;
				path += strlen(get_first_token(path)) + 1;
				i = 0;
				break;
			} else if (cwd->data.direct[i]->dir_contents.items[j].pointer == (inode_t) 1) {
				continue;
			} else {
				printf("[DEBUG] Patch could not be resolved \n");
				return NULL;
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
	memset(temp_data, 0, 4096);
	sprintf(temp_data, "hahahaha");
	printf("[DEBUG] Entered command: write \n");
	disk_write_block(get_filesystem(), -1, temp_data);
	printf("[DEBUG] got here \n");
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
	return NULL;
}

char* minifile_pwd(void){
	return "MINIFILE_PWD not implemented \n";
}
