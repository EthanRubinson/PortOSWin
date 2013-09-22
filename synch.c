//Make sure vacate does not go above original count
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
    int limit;
	tas_lock_t mutex;
	queue_t waiting;
};


/*
 * semaphore_t semaphore_create()
 *	Allocate a new semaphore.
 */
semaphore_t semaphore_create() {
	semaphore_t sem = (semaphore_t) malloc(sizeof(struct semaphore));
	return sem;
}

/*
 * semaphore_destroy(semaphore_t sem);
 *	Deallocate a semaphore.
 */
void semaphore_destroy(semaphore_t sem) {
	queue_free(sem->waiting);
	free(sem);
}

 
/*
 * semaphore_initialize(semaphore_t sem, int cnt)
 *	initialize the semaphore data structure pointed at by
 *	sem with an initial value cnt.
 */
void semaphore_initialize(semaphore_t sem, int cnt) {
	sem->limit = cnt;
	sem->mutex = 0;
	sem->waiting = queue_new();
}


/*
 * semaphore_P(semaphore_t sem)
 *	Wait on the semaphore.
 */
void semaphore_P(semaphore_t sem) {
	while(atomic_test_and_set(&(sem->mutex)));
	if (--sem->limit < 0) {
		queue_append(sem->waiting, minithread_self());
		sem->mutex = 0;
		minithread_yield();
		// deschedule this thread;
	} else {
		sem->mutex = 0;
	}
}
/*
 * semaphore_V(semaphore_t sem)
 *	Signal on the semaphore.
 */
void semaphore_V(semaphore_t sem) {
	while(atomic_test_and_set(&(sem->mutex)));
	if(++sem->limit <= 0) {
		// dequeue from wait queue
	}
	sem->mutex = 0;
}
