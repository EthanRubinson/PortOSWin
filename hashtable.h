/*
 * Generic hashtable manipulation functions  
 */
#ifndef __HASHTABLE_H__
#define __HASHTABLE_H__

typedef struct table_entry *table_entry_t;
typedef struct hashtable *hashtable_t;

/*
 * Return an empty hashtable. On error should return NULL.
 */
extern hashtable_t hashtable_new(unsigned long size);

/*
 * Put an item in the hashtable.
 * Return 0 (success) -1 (failure)
 */
extern int hashtable_put(hashtable_t ht, char *key, void *value);

/*
 * Find an item with the specified key in the hashtable.\
 * Multiple items with the same 
 * item=the item or NULL if the item does not exists
 * Return 0 (success) -1 (failure)
 */
extern int hashtable_get(hashtable_t ht, char *key, void* *item);

/*
 * Remove the item with the specified key from the hashtable
 * Return 0 (success) -1 (failure)
 */
extern int hashtable_remove(hashtable_t ht, char *key);

/*
 * Destry the hashtable
 */
extern void hashtable_destroy(hashtable_t ht);

#endif __HASHTABLE_H__
