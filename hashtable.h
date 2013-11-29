/*
 * Generic hashtable manipulation functions  
 */
#ifndef __HASHTABLE_H__
#define __HASHTABLE_H__

typedef struct table_entry *table_entry_t;
typedef struct hashtable *hashtable_t;

/* 
 * Creates a new hastable
 * Returns a new hashtable_t on Success, NULL on Failure
 */
extern hashtable_t hashtable_new(unsigned long size);

/*
 * Puts the element with the given key in the hashtable
 * Returns 0 on success, -1 on failure
 */
extern int hashtable_put(hashtable_t ht, char *key, void *value);

/*
 * Find an item with the specified key in the hashtable.
 * Multiple items with the same 
 * item=the item or NULL if the item does not exists
 * Return 0 (success) -1 (failure)
 */
extern int hashtable_get(hashtable_t ht, char *key, void* *item);

/*
 * Removes the element with the given key from the hashtable
 * Return 0 on success, -1 on failure (element did not exist)
 */
extern int hashtable_remove(hashtable_t ht, char *key);

/* 
 * Destroys the hashtable
 * DOES NOT free the actual data stored. (The calling program must do that)
 */
extern void hashtable_destroy(hashtable_t ht);

#endif __HASHTABLE_H__
