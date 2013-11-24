#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

struct table_entry {
	char *key;
	void *value;
};

typedef struct table_entry *table_entry_t;

struct hashtable {
	unsigned long num_entries;
	unsigned long max_entries;
	table_entry_t *table;
};

typedef struct hashtable *hashtable_t;


unsigned long hash(unsigned char *str)
    {
        unsigned long hash = 5381;
        int c;
	
        while (c = *str++)
            hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

		printf("[DEGUB] Got hash %lu\n",hash);
        return hash;
    }


hashtable_t hashtable_new(unsigned long starting_size) {
	hashtable_t new_table = (hashtable_t) malloc(sizeof(struct hashtable));
	
	printf("[DEGUB] Creating table of size %lu.\n", starting_size);

	//Memory allocation of the hashtable structure failed
	if (new_table == NULL || starting_size < 0){
		printf("[DEGUB] Failed to create table of size %lu. Memory allocation of hashtable failed.\n", starting_size);
		return NULL;
	}
	new_table->max_entries = starting_size;
	new_table->num_entries = 0;
	new_table->table = (table_entry_t *)calloc(new_table->max_entries, sizeof(table_entry_t));

	//Memory allocation of the actual table failed
	if (new_table->table == NULL){
		free(new_table);
		printf("[DEGUB] Failed to create table of size %lu. Memory allocation of the virtual table failed.\n", starting_size);
		return NULL;
	}
	
	printf("[DEGUB] Created table of size %lu.\n", starting_size);
	return new_table;
}

//Increases table size exponentially
// Returns 0 on success, -1 on failure
int hashtable_expand(hashtable_t ht) {
	table_entry_t *new_table;
	table_entry_t temp_entry;
	unsigned long new_index;
	unsigned long elem_iter;
	unsigned long new_size;
	
	if(ht->max_entries <= 1){
		new_size = 2;
	}
	else{
		new_size = ht->max_entries * ht->max_entries; //(int)pow(2.0,(int)floor(log10((double)ht->max_entries)/log10(2.0) + 1));
	}

	printf("[DEGUB] Trying to expand hashtable with current limit %lu to %lu.\n", ht->max_entries, new_size);

	new_table = (table_entry_t *)calloc(new_size,sizeof(table_entry_t));

	if(new_table == NULL){
		printf("[DEGUB] Could not expand hashtable. Memory allocation of new virtual table failed.\n", ht->max_entries, new_size);
		return -1;
	}
	
	//Rehash and copy the elements to the new table
	for(elem_iter=0; elem_iter < ht->max_entries; elem_iter++) {
		temp_entry = (table_entry_t)malloc(sizeof(struct table_entry));;
		
		if(ht->table[elem_iter] != NULL) {
			
			temp_entry->key = ht->table[elem_iter]->key;
			
			temp_entry->value = ht->table[elem_iter]->value;
			printf("[DEGUB] Moving value %d with key %s at index %lu.\n",*(int *)(temp_entry->value),temp_entry->key,elem_iter);

			new_index = hash((unsigned char*)temp_entry->key) % new_size;
			while(new_table[new_index] != NULL){
				new_index += 1;
				if(new_index == new_size){
					new_index = 0;
				}
			}
			new_table[new_index] = temp_entry;
			free(ht->table[elem_iter]); /**This is a problem.... why?*/
			
			printf("[DEGUB] Moved value %d with key %s to index %lu.\n",*(int *)(temp_entry->value),temp_entry->key,new_index);

		}
	}

	free(ht->table);
	ht->max_entries=new_size;
	ht->table=new_table;

	printf("[DEGUB] Expansion successful. Hashtable can now contain %lu elements.\n", ht->max_entries);

	return 0;
}

//Puts the element with the given key in the hashtable
//Returns 0 on success, -1 on failure
int hashtable_put(hashtable_t ht, char *key, void *value) {
	table_entry_t new_entry = (table_entry_t)malloc(sizeof(struct table_entry));
	unsigned long new_index;

	printf("[DEGUB] Adding element %d with key \"%s\" to hashtable\n", *(int *)value,key);
	if (ht->max_entries / 2.0 <= ht->num_entries + 1){
		if (hashtable_expand(ht) == -1) {
			printf("[DEGUB] Failed to add element.\n");
			return -1;
		}
	}

	new_entry->key = key;
	new_entry->value = value;	
	new_index = hash((unsigned char*)key) % ht->max_entries;

	while(ht->table[new_index] != NULL){
		new_index += 1;
		if(new_index == ht->max_entries){
			new_index = 0;
		}
	}
	ht->table[new_index] = new_entry;
	ht->num_entries++;
	printf("[DEGUB] Element %d with key \"%s\" added at index %lu successfully\n",*(int *)value, key, new_index);
	return 0;
}

//Retrieves the element with the given key from the hashtable
//Return 0 on success, -1 on failure (item did not exist)
int hashtable_get(hashtable_t ht, char *key, void* *item) {
	unsigned long search_index;
	unsigned long end_search_index;

	printf("[DEGUB] Looking for element in table\n");

	if(ht->num_entries == 0){
		printf("[DEGUB] Table has no elements, it's not here.\n");
		*item = NULL;
		return -1;
	}
	
	search_index = hash((unsigned char*)key) % ht->max_entries;
	end_search_index = (search_index - 1 > 0) ? search_index - 1 : ht->max_entries -1;
	
	printf("Key:\"%lu\"",search_index);
	while (search_index != end_search_index  && ht->table[search_index] != NULL && strcmp(ht->table[search_index]->key,key) != 0){
		
		if (search_index + 1 == ht->max_entries) {
			search_index = 0;
		}
		else{
			search_index += 1;
		}
	}
	if (ht->table[search_index] == NULL){
		printf("[DEGUB] Element not found\n");
		*item = NULL;
		return -1;
	}
	else if(search_index == end_search_index && strcmp(ht->table[search_index]->key,key) != 0){
		printf("[DEGUB] Element not found\n");
		*item = NULL;
		return -1;
	}

	else{
		*item = ht->table[search_index]->value;
		printf("[DEGUB] Found the element!\n");
		return 0;
	}

}

//Removes the element with the given key from the hashtable
//Return 0 on success, -1 on failure (element did not exist)
int hashtable_remove(hashtable_t ht, char *key) {
	unsigned long search_index = hash((unsigned char*)key) % ht->max_entries;
	unsigned long end_search_index = (search_index - 1 > 0) ? search_index - 1 : ht->max_entries -1;
	
	if(ht->num_entries == 0){
		return -1;
	}

	while (search_index != end_search_index  && ht->table[search_index] != NULL && strcmp(ht->table[search_index]->key,key) != 0){
		if (search_index + 1 == ht->max_entries) {
			search_index = 0;
		}
		else{
			search_index += 1;
		}
	}

	if (ht->table[search_index] == NULL){
		return -1;
	}
	else if(search_index == end_search_index && strcmp(ht->table[search_index]->key,key) != 0){
		return -1;
	}

	else{
		ht->table[search_index] = NULL;
		free(ht->table[search_index]);
		ht->num_entries--;
		return 0;
	}

}

void hashtable_destroy(hashtable_t ht){
	unsigned long elem_iter;
	
	//Iterate through the table and free all of the table element structures
	for(elem_iter=0; elem_iter < ht->max_entries; elem_iter++) {
		free(ht->table[elem_iter]);
	}
	//Free the table structure
	free(ht->table);
	//Free the overlying hashtable structure
	free(ht);
}

void hashtable_stats(hashtable_t ht) {
}


