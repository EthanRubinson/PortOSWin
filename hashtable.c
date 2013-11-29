#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <string.h>

/*
 * A structure representing an entry in the hastable
 * key -> the unhashed key
 * value -> the data stored in the table
 */
struct table_entry {
	char *key;
	void *value;
};
typedef struct table_entry *table_entry_t;

/*
 * A hashtable
 * num_entries -> Number of elements stored in the table
 * max_entries -> Maximum number of elements that can be stored in the table
 * table -> The virtual hashtable
 */
struct hashtable {
	unsigned long num_entries;
	unsigned long max_entries;
	table_entry_t *table;
};
typedef struct hashtable *hashtable_t;

/*
 * = a hash representation of the provied char*
 * Hashing function provided by: http://www.cse.yorku.ca/~oz/hash.html
 */
unsigned long hash(unsigned char *str)
    {
        unsigned long hash = 5381;
        int c;
	
        while (c = *str++)
            hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

        return hash;
    }

/* 
 * Creates a new hastable
 * Returns a new hashtable_t on Success, NULL on Failure
 */
hashtable_t hashtable_new(unsigned long starting_size) {
	hashtable_t new_table = (hashtable_t) malloc(sizeof(struct hashtable));
	
	//Memory allocation of the hashtable structure failed or an invalid size was provided
	if (new_table == NULL || starting_size < 0){
		return NULL;
	}
	new_table->max_entries = starting_size;
	new_table->num_entries = 0;
	new_table->table = (table_entry_t *)calloc(new_table->max_entries, sizeof(table_entry_t));

	//Memory allocation of the actual table failed
	if (new_table->table == NULL){
		free(new_table);
		return NULL;
	}
	
	return new_table;
}

/*
 * Destroys the hashtable's table structure.
 * destroy_keys specifies whether the element keys should be deleted also (1 == destroy)
 */
void table_destroy(table_entry_t *table, unsigned long max_entries, int destroy_keys){
	unsigned long elem_iter;
	
	//Iterate through the table and free all of the table element structures
	for(elem_iter=0; elem_iter < max_entries; elem_iter++) {
		if(table[elem_iter] != NULL){
			//Only free the keys if they are marked for destruction
			if(destroy_keys == 1){
				free(table[elem_iter]->key);
				table[elem_iter]->key = NULL;
			}
			free(table[elem_iter]);
			table[elem_iter] = NULL;
		}
	}
	//Free the table structure
	free(table);
	table = NULL;
}

/*
 * Doubles the provided hashtable size
 * Returns 0 on success, -1 on failure.
 */
int hashtable_expand(hashtable_t ht) {
	table_entry_t *new_table;
	table_entry_t temp_entry;
	unsigned long new_index;
	unsigned long elem_iter;
	unsigned long new_size;
	
	//Provided hashtable was NULL
	if(ht == NULL){
		return -1;
	}

	//Double the size of the hashtable (A table of size 0 gets size 2)
	if(ht->max_entries == 0){
		new_size = 2;
	}
	else{
		new_size = ht->max_entries * 2; //(int)pow(2.0,(int)floor(log10((double)ht->max_entries)/log10(2.0) + 1));
	}

	new_table = (table_entry_t *)calloc(new_size,sizeof(table_entry_t));

	//Memory allocation of the new table failed
	if(new_table == NULL){
		return -1;
	}
	
	//Rehash and copy the elements to the new table
	for(elem_iter=0; elem_iter < ht->max_entries; elem_iter++) {
		temp_entry = (table_entry_t)malloc(sizeof(struct table_entry));;
		
		// The memory allocation for the new element failed.
		if(temp_entry == NULL){
			table_destroy(new_table, new_size,0);
			return -1;
		}

		if(ht->table[elem_iter] != NULL) {
			//Reassigning the pointers
			temp_entry->key = ht->table[elem_iter]->key;
			temp_entry->value = ht->table[elem_iter]->value;
			
			new_index = hash((unsigned char*)temp_entry->key) % new_size;
			while(new_table[new_index] != NULL){
				new_index += 1;
				if(new_index == new_size){
					new_index = 0;
				}
			}
			new_table[new_index] = temp_entry;
			
		}
	}

	table_destroy(ht->table,ht->max_entries,0);

	ht->max_entries=new_size;
	ht->table=new_table;

	return 0;
}

/*
 * Puts the element with the given key in the hashtable
 * Returns 0 on success, -1 on failure
 */
int hashtable_put(hashtable_t ht, char *key, void *value) {
	table_entry_t new_entry = (table_entry_t)malloc(sizeof(struct table_entry));
	unsigned long new_index;

	//Memory allocation of the new element failed
	if(new_entry == NULL){
		return -1;
	}

	new_entry->key = (char *)malloc(strlen(key) + 1);

	if(new_entry->key == NULL){
		free(new_entry);
		return -1;
	}

	if (ht->max_entries / 2.0 <= ht->num_entries + 1){
		if (hashtable_expand(ht) == -1) {
			return -1;
		}
	}

	strcpy(new_entry->key,key);

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

	return 0;
}

/*
 * Find an item with the specified key in the hashtable.
 * Multiple items with the same 
 * item=the item or NULL if the item does not exists
 * Return 0 (success) -1 (failure)
 */
int hashtable_get(hashtable_t ht, char *key, void* *item) {
	unsigned long search_index;
	unsigned long end_search_index;

	if(ht->num_entries == 0){
		*item = NULL;
		return -1;
	}
	
	search_index = hash((unsigned char*)key) % ht->max_entries;
	end_search_index = (search_index - 1 > 0) ? search_index - 1 : ht->max_entries -1;
	
	while (search_index != end_search_index  && ((ht->table[search_index] == NULL) ? 1 : strcmp(ht->table[search_index]->key,key) != 0)){
		if (search_index + 1 == ht->max_entries) {
			search_index = 0;
		}
		else{
			search_index += 1;
		}
	}
	if (ht->table[search_index] == NULL){
		*item = NULL;
		return -1;
	}
	else if(search_index == end_search_index && strcmp(ht->table[search_index]->key,key) != 0){
		*item = NULL;
		return -1;
	}

	else{
		*item = ht->table[search_index]->value;
		return 0;
	}

}

/*
 * Removes the element with the given key from the hashtable
 * Return 0 on success, -1 on failure (element did not exist)
 */
int hashtable_remove(hashtable_t ht, char *key) {
	unsigned long search_index;
	unsigned long end_search_index; 
	
	if(ht->num_entries == 0){
		return -1;
	}

	search_index = hash((unsigned char*)key) % ht->max_entries;
	end_search_index = (search_index - 1 > 0) ? search_index - 1 : ht->max_entries -1;

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
		free(ht->table[search_index]->key);
		free(ht->table[search_index]);
		ht->table[search_index] = NULL;
		
		ht->num_entries--;
		return 0;
	}

}

/* 
 * Destroys the hashtable
 * DOES NOT free the actual data stored. (The calling program must do that)
 */
void hashtable_destroy(hashtable_t ht){
	
	table_destroy(ht->table, ht->max_entries,1);

	//Free the overlying hashtable structure
	free(ht);
	ht = NULL;
}
