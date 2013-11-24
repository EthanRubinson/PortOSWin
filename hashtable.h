/*
 * Generic hashtable manipulation functions  
 */
#ifndef __HASHTABLE_H__
#define __HASHTABLE_H__

typedef struct table_entry *table_entry_t;
typedef struct hashtable *hashtable_t;

extern hashtable_t hashtable_new(unsigned long size);
extern int hashtable_put(hashtable_t ht, char *key, void *value);
extern int hashtable_get(hashtable_t ht, char *key, void* *item);
extern int hashtable_remove(hashtable_t ht, char *key);
extern void hashtable_destroy(hashtable_t ht);
extern void hashtable_stats(hashtable_t ht);

#endif __HASHTABLE_H__
