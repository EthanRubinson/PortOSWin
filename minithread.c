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

/*Queue ds representing the runnable_threads*/
queue_t runnable_queue;


/*
 *-----------------------
 * minithread functions
 * ----------------------
 */


/*The final procedure a minithreads executes on termination
 * TODO: What is the propper arguements/return type of this function?
 */
int final_proc(arg_t final_args){
	//TODO: Figure out what to do in this final procedure
	//Use Idle thread to terminiate the *current thread*
	return 0;
}

int idle_proc(arg_t idle_args){
	/*Never terminate, constantly yielding allowing any new threads to be run*/
	while(1){
		minithread_yield();
	}

	return 0;
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
		printf("ERROR: Cannot assign new thread id");
		return -1;
	}
}

minithread_t minithread_fork(proc_t proc, arg_t arg) {
	minithread_t new_thread = minithread_create(proc,arg);

	if(new_thread == NULL) {
		printf("ERROR: Could not start thread. [thread is null]");
		return NULL;
	}

	/*Append the new thread to the run queue*/
	queue_append(runnable_queue, new_thread);

	return new_thread;
}

minithread_t minithread_create(proc_t proc, arg_t arg) {
	minithread_t new_thread = (minithread_t) malloc(sizeof(struct minithread));

	if(new_thread == NULL){
		printf("ERROR: Memmory allocation for new thread failed");
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
	//TODO: IMPLEMENT
}

void minithread_start(minithread_t t) {
	if (t != NULL){
		queue_append(runnable_queue, t);
	}
	else{
		printf("ERROR: Could not start thread. [thread is null]");
	}	
}

void minithread_yield() {
	//Volentarily give up the CPU & let another thread from the runnable queue run
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
	//TODO: IMPLEMENT FULLY
	thread_id_counter = 0;
	runnable_queue = queue_new();
	idle_thread = minithread_create((proc_t)idle_proc, NULL);

	//?......
}


