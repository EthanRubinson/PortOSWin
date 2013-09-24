/*
 * minithread.c:
 *	This file provides a few function headers for the procedures that
 *	you are required to implement for the minithread assignment.
 *
 *	EXCEPT WHERE NOTED YOUR IMPLEMENTATION MUST CONFORM TO THE
 *	NAMING AND TYPING OF THESE PROCEDURES.
 *
 */
#include <stdlib.h>
#include <stdio.h>
#include "minithread.h"
#include "queue.h"
#include "synch.h"

#include <assert.h>

/*
 * A minithread should be defined either in this file or in a private
 * header file.  Minithreads have a stack pointer with to make procedure
 * calls, a stackbase which points to the bottom of the procedure
 * call stack, the ability to be enqueueed and dequeued, and any other state
 * that you feel they must have.
 */

/*
 * Minithread struct. Contains the stack base and the stack top along with the
 * unique id of the thread.
 */
struct minithread {
	stack_pointer_t stackbase;
	stack_pointer_t stacktop;
	int id;
};

/*The currently executing thread*/
minithread_t current_thread;

/*The idle thread (Used for cleanup / Never terminated)*/
minithread_t idle_thread;

/*Unique thread id generator. Assigned and incremented each time a new thread is spawned*/
int thread_id_counter;

/*Queue ds representing the currently runnable threads*/
queue_t runnable_queue;

/*Queue ds representing the threads that need to be cleaned up*/
queue_t cleanup_queue;

/*Semaphore blocking the cleanup thread until there are elements in the cleanup_queue*/
semaphore_t cleanup_sem;

/*
 *-----------------------
 * minithread functions
 * ----------------------
 */


/*The final procedure a minithreads executes on termination
 * TODO: What is the propper arguements/return type of this function?
 */
int final_proc(arg_t final_args){
	stack_pointer_t previous_sp = current_thread->stacktop;
	queue_append(cleanup_queue, minithread_self());
	semaphore_V(cleanup_sem);
	printf("Final procedure for thread id %d done, switching to idle thread\n",minithread_self()->id);
	current_thread = idle_thread;
	minithread_switch(&(previous_sp),&(idle_thread->stacktop));
	while(1);
}

int idle_thread_proc(arg_t idle_args){
	/*Never terminate, constantly yielding allowing any new threads to be run*/
	while(1){
		minithread_yield();
	}
}

int cleanup_thread_proc(arg_t cleanup_args){
	int thread_id;
	minithread_t temp;
	while(1){
		semaphore_P(cleanup_sem);
		
		queue_dequeue(cleanup_queue,(void**) &temp);
		thread_id = temp->id;
		minithread_free_stack(temp->stackbase);
		printf("Freed thread ID: %d\n",thread_id);
	}
}


/*Returns a new 'unique' (thread_id >= 0) on Sucess, (-1) on Failure*/
int new_thread_id(){
	int temp;
	if(thread_id_counter <= INT_MAX){
		temp = thread_id_counter;
		thread_id_counter++;
		return temp;
	}
	else{
		printf("ERROR: Cannot assign new thread id\n");
		return -1;
	}
}

minithread_t minithread_fork(proc_t proc, arg_t arg) {
	minithread_t new_thread = minithread_create(proc,arg);

	if(new_thread == NULL) {
		printf("ERROR: Could not start thread. [thread is null]\n");
		return NULL;
	}

	/*Append the new thread to the run queue*/
	queue_append(runnable_queue, new_thread);

	return new_thread;
}

minithread_t minithread_create(proc_t proc, arg_t arg) {
	minithread_t new_thread = (minithread_t) malloc(sizeof(struct minithread));

	if(new_thread == NULL){
		printf("ERROR: Memmory allocation for new thread failed\n");
		return NULL;
	}

	minithread_allocate_stack(&new_thread->stackbase,&new_thread->stacktop);
	new_thread->id = new_thread_id();
	minithread_initialize_stack(&new_thread->stacktop, proc, arg, (proc_t)final_proc, NULL);
	return new_thread;
}

minithread_t minithread_self() {
	return current_thread;
}

int minithread_id() {
	if(current_thread != NULL){
		return current_thread->id;
	}
	return -1;
}

void minithread_stop() {
	minithread_t previous_thread = current_thread;
	
	//Get the number of threads waiting to run
	int length = queue_length(runnable_queue);

	//There are no threads to context switch to so switch to the idle thread
	if(length == 0) {
		current_thread = idle_thread;
		
	}
	else{
		queue_dequeue(runnable_queue,(void**) &current_thread);
	}
	printf("[MINITHREAD_STOP] Switching from thread %d to thread %d\n",previous_thread->id,current_thread->id);
	minithread_switch(&(previous_thread->stacktop),&(current_thread->stacktop));
}


//minithread stop only for blocking
//make a new cleanup thread that is blocked until the runnable quee is not empty
//minithread stop moves to waitqueue
//

void minithread_start(minithread_t t) {
	if (t != NULL){
		queue_append(runnable_queue, t);
	}
	else{
		printf("ERROR: Could not start thread. [thread is null]\n");
	}	
}

void minithread_yield() {	
	
	//Volentarily give up the CPU & let another thread from the runnable queue run

	minithread_t previous_thread = current_thread;
	
	int length = queue_length(runnable_queue);
	
	//There are no threads to context switch to just return
	if(length == 0) {
		printf("[MINITHREAD_YIELD] Not yielding thread %d, no other runnable threads\n",previous_thread->id);
		return;
	}

	//There are runnable threads
	if(current_thread != idle_thread){
		queue_append(runnable_queue,previous_thread);
	}

	queue_dequeue(runnable_queue,(void**) &current_thread);


	printf("[MINITHREAD_YIELD] Switching from thread %d to thread %d\n",previous_thread->id,current_thread->id);
	minithread_switch(&(previous_thread->stacktop),&(current_thread->stacktop));
	
}

/*
 * Initialization.
 *
 * 	minithread_system_initialize:
 *	 This procedure should be called from your C main procedure
 *	 to turn a single threaded UNIX process into a multithreaded
 *	 program.
 *
 *	 Initialize any private data structures.
 * 	 Create the idle thread.
 *       Fork the thread which should call mainproc(mainarg)
 * 	 Start scheduling.
 *
 */
void minithread_system_initialize(proc_t mainproc, arg_t mainarg) {
	runnable_queue = queue_new();
	cleanup_queue = queue_new();
	cleanup_sem = semaphore_create();
	semaphore_initialize(cleanup_sem,0);

	//Allocate space for the idle thread store the sp of the main thread
	idle_thread = (minithread_t) malloc(sizeof(struct minithread));
	thread_id_counter = 0;
	idle_thread->id = thread_id_counter;
	
	thread_id_counter++;

	current_thread = idle_thread;
	
	minithread_fork(cleanup_thread_proc,NULL);
	minithread_fork(mainproc, mainarg);
	
	idle_thread_proc(NULL);
}


/*
minithread_t temp;
			
			queue_dequeue(cleanup_queue,(void**) &temp);
			thread_id = temp->id;
			printf("Freeing thread ID: %d\n",thread_id);
			minithread_free_stack(temp->stackbase);
			printf("Freed up thread ID: %d\n",thread_id);
*/

