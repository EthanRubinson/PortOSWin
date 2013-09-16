#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "defs.h"
#include "synch.h"
#include "queue.h"
#include "minithread.h"

/*
 *	You must implement the procedures and types defined in this interface.
 */


/*
 * Semaphores.
 */
struct semaphore {
	int current;
    int limit;
};


/*
 * semaphore_t semaphore_create()
 *	Allocate a new semaphore.
 */
semaphore_t semaphore_create() {
	semaphore_t sem = (semaphore_t) malloc(sizeof(semaphore));
	return sem;
}

/*
 * semaphore_destroy(semaphore_t sem);
 *	Deallocate a semaphore.
 */
void semaphore_destroy(semaphore_t sem) {
	free(sem);
}

 
/*
 * semaphore_initialize(semaphore_t sem, int cnt)
 *	initialize the semaphore data structure pointed at by
 *	sem with an initial value cnt.
 */
void semaphore_initialize(semaphore_t sem, int cnt) {
	sem->current = 0;
	sem->limit = cnt;
}


/*
 * semaphore_P(semaphore_t sem)
 *	P on the sempahore.
 */
void semaphore_P(semaphore_t sem) {
	// acquire mutex
	sem->current--;
	// release mutex
}

/*
 * semaphore_V(semaphore_t sem)
 *	V on the sempahore.
 */
void semaphore_V(semaphore_t sem) {
	// acquire mutex
	if (sem->current < sem->limit) {
		sem->current++;
		// release mutex
	} else {
		// release mutex
		// add to wait queue or
		// busy wait

	}
}
