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
 * Minithread TCB:
 * stackbase -> Base of the stack
 * stacktop -> Top of the stack
 * id -> Unique thread ID
 * destroyed -> (1 = Thread is queued for cleanup) OR (0 = Thread is not queued for cleanup)
 */
struct minithread {
	stack_pointer_t stackbase;
	stack_pointer_t stacktop;
	int id;
	int destroyed;
};

/*= Currently running thread*/
minithread_t current_thread;

/*= Idle thread {ID: 0} (Never terminated)*/
minithread_t idle_thread;

/*= Unique thread id counter. It is incremented each time a new thread is (attempted to be) created*/
int thread_id_counter;

/*= Queue containing all the currently runnable threads*/
queue_t runnable_queue;

/*= Queue containing all the threads that need to be cleaned up*/
queue_t cleanup_queue;

/*= Semaphore for blocking the cleanup thread until there are elements in the cleanup_queue.*/
semaphore_t cleanup_sem;


/*
 *-----------------------
 * minithread functions
 * ----------------------
 */

/*
 * The final procedure a minithread executes before termination.
 * Marks the thread for termination and stops its execution
 */
int final_proc(arg_t final_args){
	minithread_self()->destroyed = 1;
	queue_append(cleanup_queue, minithread_self());
	semaphore_V(cleanup_sem);
	//printf("[INFO] Final procedure for thread {ID: %d} done\n", minithread_self()->id);
	minithread_stop();
	while(1);
}

/* 
 * The idle thread {ID: 0} procedure
 * The idle thread never terminates so this procedure yields continously and forever
 */
int idle_thread_proc(arg_t idle_args){
	/*Never terminate, constantly yielding allowing any new threads to be run*/
	while(1){
		minithread_yield();
	}
}

/* 
 * The cleanup thread {ID: 1} procedure
 * Frees the stack of threads marked for termination
 */
int cleanup_thread_proc(arg_t cleanup_args){
	int thread_id;
	minithread_t temp;
	while(1){
		semaphore_P(cleanup_sem);
		
		queue_dequeue(cleanup_queue,(void**) &temp);
		thread_id = temp->id;
		minithread_free_stack(temp->stackbase);
		//printf("[INFO] Freed stack of thread {ID: %d}\n",thread_id);
	}
}

/*
 * = A unique thread id
 * (>= 0) on Success, (-1) on Failure
 */
int new_thread_id(){
	int temp;
	if(thread_id_counter <= INT_MAX){
		temp = thread_id_counter;
		thread_id_counter++;
		return temp;
	}
	else{
		printf("[ERROR] Could not generate new thread id (Too many threads created)\n");
		return -1;
	}
}

/*
 * Creates a new minithread and makes it runnable
 * (new_thread) on Success, (NULL) on Failure
 */
minithread_t minithread_fork(proc_t proc, arg_t arg) {
	minithread_t new_thread = minithread_create(proc,arg);
	minithread_start(new_thread);
	return new_thread;
}

/* 
 * Creates a new minithread
 * (new_thread) on Success, (NULL) on Failure
 */
minithread_t minithread_create(proc_t proc, arg_t arg) {
	minithread_t new_thread;
	int new_id;
	new_id = new_thread_id();

	if(proc == NULL) {
		printf("[ERROR] Could not create thread (Procedure is NULL)\n");
		return NULL;
	}

	if(new_id == -1){
		printf("[ERROR] Could not create thread (ID assignment failed)\n");
		return NULL;
	}

	new_thread = (minithread_t) malloc(sizeof(struct minithread));

	if(new_thread == NULL){
		printf("[ERROR] Could not create thread (Memory allocation failed)\n");
		return NULL;
	}

	minithread_allocate_stack(&new_thread->stackbase,&new_thread->stacktop);
	
	new_thread->id = new_id;
	new_thread->destroyed = 0;

	minithread_initialize_stack(&new_thread->stacktop, proc, arg, (proc_t)final_proc, NULL);
	//printf("[INFO] Created thread {ID: %d}\n",new_thread->id);
	return new_thread;
}

/* = The currently running minithread*/
minithread_t minithread_self() {
	return current_thread;
}

/* = ID of the currently running minithread*/
int minithread_id() {
	if(current_thread != NULL){
		return current_thread->id;
	}
	return -1;
}

/* Stops the calling minithread and context switches to another minithread*/
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
	//printf("[INFO] Stopping thread {ID: %d} and switching to thread {ID: %d}\n",previous_thread->id,current_thread->id);
	minithread_switch(&(previous_thread->stacktop),&(current_thread->stacktop));
}

/* Makes the given minithread runnable*/
void minithread_start(minithread_t t) {
	if (t != NULL){
		if(t->destroyed == 1){
			printf("[ERROR] Could not make thread {ID: %d} runnable (Thread is queued for cleanup)\n",t->id);
		}
		else{
			//printf("[INFO] Made thread {ID: %d} runnable\n",t->id);
			queue_append(runnable_queue, t);
		}
	}
	else{
		printf("[ERROR] Could not make thread runnable (Thread is NULL)\n");
	}	
}

/* 
 * Volentarily give up the CPU & let another thread run
 * Attempts to context switch to another minithread
 */
void minithread_yield() {	
	
	minithread_t previous_thread = current_thread;
	
	int length = queue_length(runnable_queue);
	
	//There are no threads to context switch to just return
	if(length == 0) {
		//printf("[INFO] Not yielding thread {ID: %d} (No threads waiting to run)\n",previous_thread->id);
		return;
	}

	//There are runnable threads
	if(current_thread != idle_thread){
		queue_append(runnable_queue,previous_thread);
	}

	queue_dequeue(runnable_queue,(void**) &current_thread);


	//printf("[INFO] Switching from thread {ID: %d} to thread {ID: %d}\n",previous_thread->id,current_thread->id);
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
	minithread_t temp_ref;
	runnable_queue = queue_new();
	cleanup_queue = queue_new();
	cleanup_sem = semaphore_create();

	if(runnable_queue == NULL || cleanup_queue == NULL || cleanup_sem == NULL) {
		printf("[FATAL] Could not initialize minithread system (Internal structure initialization failed)\n");
		return;
	}
	
	semaphore_initialize(cleanup_sem,0);

	//Allocate space for the idle thread
	idle_thread = (minithread_t) malloc(sizeof(struct minithread));

	if(idle_thread == NULL){
		printf("[FATAL] Could not spawn idle thread (Memory allocation failed)\n");
		queue_free(runnable_queue);
		queue_free(cleanup_queue);
		semaphore_destroy(cleanup_sem);
		return;
	}

	thread_id_counter = 0;
	idle_thread->id = thread_id_counter;
	idle_thread->destroyed = 0;

	thread_id_counter++;

	current_thread = idle_thread;
	
	temp_ref = minithread_fork(cleanup_thread_proc,NULL);

	if(temp_ref == NULL){
		printf("[FATAL] Could not spawn cleanup thread\n");
		queue_free(runnable_queue);
		queue_free(cleanup_queue);
		semaphore_destroy(cleanup_sem);
		free(idle_thread);
		return;
	}


	minithread_fork(mainproc, mainarg);
	
	idle_thread_proc(NULL);
}
