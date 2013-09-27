#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "defs.h"
#include "synch.h"
#include "queue.h"
#include "minithread.h"

/*
 * Semaphores:
 * limit -> Max threads that can consume a resource at the same time
 * mutex -> Lock
 * waiting -> Queue containing all currently blocked threads
 */
struct semaphore {
    int limit;
	//tas_lock_t mutex;
	queue_t waiting;
};

/*
 *	Create a new semaphore.
 */
semaphore_t semaphore_create() {
	semaphore_t sem = (semaphore_t) malloc(sizeof(struct semaphore));

	if(sem == NULL){
		printf("[ERROR] Could not create semaphore (Memory allocation failed)\n");
		return NULL;
	}

	return sem;
}

/*
 *	Deallocate/Delete a semaphore.
 */
void semaphore_destroy(semaphore_t sem) {
	queue_free(sem->waiting);
	free(sem);
}

/*
 *	Initialize the semaphore data structure pointed at by
 *	sem with an initial value cnt.
 */
void semaphore_initialize(semaphore_t sem, int cnt) {
	sem->limit = cnt;
	//sem->mutex = 0;
	sem->waiting = queue_new();
	
	if(sem->waiting == NULL){
		printf("[ERROR] Could not initialize semaphore (Internal structure initialization failed)\n");
	}
}

/*
 *	Procure the semaphore.
 */
void semaphore_P(semaphore_t sem) {
	
	//while(atomic_test_and_set(&(sem->mutex)));
	if (--sem->limit < 0) {
		queue_append(sem->waiting, minithread_self());
		//sem->mutex = 0;
		minithread_stop();
	}
	// else {
		//sem->mutex = 0;
	//}
}

/*
 *	Vacate the semaphore.
 */
void semaphore_V(semaphore_t sem) {
	minithread_t thread;
	//while(atomic_test_and_set(&(sem->mutex)));
	
	if(++sem->limit <= 0) {
		queue_dequeue(sem->waiting,(void**) &thread);
		minithread_start((minithread_t) thread);			 
	}
	//sem->mutex = 0;
}
