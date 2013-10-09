//Things to ask at office hours
// Do we need interrupt disabled on the minithread fork since it is not directly modifying internal data
//How to handle the interrupt delay cause by minithread yield
//Data structure for alarms?

//Statment pairs: these statements need to occur in pairs, otherwise invarients will be messed up
// setting current_priority_level & ticks_since_last_level_switch
// current_thread->runtime_remaining & minithread_switch



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
#include "interrupts.h"
#include "minithread.h"
#include "queue.h"
#include "alarm.h"
#include "multilevel_queue.h"
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
	int priority;
	int runtime_remaining;
	int destroyed;
	semaphore_t wait_on_alarm;
};

/*= How many scheduled priority levels there are in the minithread system*/
#define NUM_PRIORITY_LEVELS 4

/*= the currently sweeped priority level based on the current clock tick*/
int current_priority_level;

/*= the number of ticks since the last level sweep switch*/
int ticks_since_last_level_switch;

/*= Currently running thread*/
minithread_t current_thread;

/*= Idle thread {ID: 0} (Never terminated)*/
minithread_t idle_thread;

/*= Unique thread id counter. It is incremented each time a new thread is (attempted to be) created*/
int thread_id_counter;

/*= Queue containing all the currently runnable threads*/
//queue_t runnable_queue;
multilevel_queue_t runnable_queue;

/*= Queue containing all the threads that need to be cleaned up*/
queue_t cleanup_queue;

/*= Semaphore for blocking the cleanup thread until there are elements in the cleanup_queue.*/
semaphore_t cleanup_sem;


/*int pointer to pass as a final_arg*/
int final_proc_args = 0;

/*
 *-----------------------
 * minithread functions
 * ----------------------
 */

/*= approptiate runtime in ticks for a thread at the given priority level*/
int get_thread_runtime_for_priority(int pri){
	return 2 ^ pri;	
}

/*= total runtime in ticks a given priority level is sweeped for*/
int get_sweep_runtime_for_priority(int pri){
	if (pri == 0)
		return 80;
	else if(pri == 1)
		return 40;
	else if(pri == 2)
		return 24;
	else
		return 16;
}

/*
 * The final procedure a minithread executes before termination.
 * Marks the thread for termination and stops its execution
 */
int final_proc(arg_t final_args){
	interrupt_level_t intlevel = set_interrupt_level(DISABLED);

	minithread_self()->destroyed = 1;
	queue_append(cleanup_queue, minithread_self());
	semaphore_V(cleanup_sem);
	//printf("[INFO] Final procedure for thread {ID: %d} done\n", minithread_self()->id);

	//Remove this and add logic to switch to the next thread
	minithread_unlock_and_stop(final_args);
	while(1);
}

/* 
 * The idle thread {ID: 0} procedure
 * The idle thread never terminates so this procedure yields continously and forever
 */
int idle_thread_proc(arg_t idle_args){
	/*Never terminate, constantly yielding allowing any new threads to be run*/
	while(1){
		//minithread_yield();
	}
}

/* 
 * The cleanup thread {ID: 1} procedure
 * Frees the stack of threads marked for termination
 */
int cleanup_thread_proc(arg_t cleanup_args){
	interrupt_level_t intlevel = set_interrupt_level(DISABLED);
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
	interrupt_level_t intlevel = set_interrupt_level(DISABLED);
	if(thread_id_counter <= INT_MAX){
		temp = thread_id_counter;
		thread_id_counter++;
		set_interrupt_level(intlevel);
		return temp;
	}
	else{
		printf("[ERROR] Could not generate new thread id (Too many threads created)\n");
		set_interrupt_level(intlevel);
		return -1;
	}
}

/*
 * Creates a new minithread and makes it runnable
 * (new_thread) on Success, (NULL) on Failure
 */
minithread_t minithread_fork(proc_t proc, arg_t arg) {
	interrupt_level_t intlevel = set_interrupt_level(DISABLED);
	minithread_t new_thread = minithread_create(proc,arg);
	minithread_start(new_thread);
	set_interrupt_level(intlevel);
	return new_thread;
}

/* 
 * Creates a new minithread
 * (new_thread) on Success, (NULL) on Failure
 */
minithread_t minithread_create(proc_t proc, arg_t arg) {
	interrupt_level_t intlevel = set_interrupt_level(DISABLED);

	minithread_t new_thread;
	int new_id;
	new_id = new_thread_id();

	if(proc == NULL) {
		printf("[ERROR] Could not create thread (Procedure is NULL)\n");
		set_interrupt_level(intlevel);
		return NULL;
	}

	if(new_id == -1){
		printf("[ERROR] Could not create thread (ID assignment failed)\n");
		set_interrupt_level(intlevel);
		return NULL;
	}

	new_thread = (minithread_t) malloc(sizeof(struct minithread));

	if(new_thread == NULL){
		printf("[ERROR] Could not create thread (Memory allocation failed)\n");
		set_interrupt_level(intlevel);
		return NULL;
	}

	minithread_allocate_stack(&new_thread->stackbase,&new_thread->stacktop);
	
	new_thread->id = new_id;
	//All threads start at priorty level 0 with corresponding runtime
	new_thread->priority = 0;
	new_thread->runtime_remaining = get_thread_runtime_for_priority(new_thread->priority);
	new_thread->destroyed = 0;
	new_thread->wait_on_alarm = semaphore_create();
	semaphore_initialize(new_thread->wait_on_alarm,0);
	minithread_initialize_stack(&new_thread->stacktop, proc, arg, (proc_t)final_proc, &final_proc_args);
	//printf("[INFO] Created thread {ID: %d}\n",new_thread->id);

	set_interrupt_level(intlevel);
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

/* 
 * DEPRECATED. Beginning from project 2, you should use minithread_unlock_and_stop() instead
 * of this function.
 */
/*void minithread_stop() {
	interrupt_level_t intlevel = set_interrupt_level(DISABLED);
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
}*/

/* Makes the given minithread runnable at the appropriate priority level*/
void minithread_start(minithread_t t) {
	interrupt_level_t intlevel = set_interrupt_level(DISABLED);
	if (t != NULL){
		if(t->destroyed == 1){
			printf("[ERROR] Could not make thread {ID: %d} runnable (Thread is queued for cleanup)\n",t->id);
		}
		else{
			//printf("[INFO] Made thread {ID: %d} runnable\n",t->id);
			multilevel_queue_enqueue(runnable_queue,t->priority,t);
		}
	}
	else{
		printf("[ERROR] Could not make thread runnable (Thread is NULL)\n");
	}
	set_interrupt_level(intlevel);
}

/* 
 * Volentarily give up the CPU & let another thread run / Don't decrease thread priority
 * Attempts to context switch to another minithread
 */
void minithread_yield() {	
	interrupt_level_t intlevel = set_interrupt_level(DISABLED);

	minithread_t previous_thread = current_thread;
	
	//Gets the next possible thread to run starting the search at the current prioirty level
	int dequeue_result = multilevel_queue_dequeue(runnable_queue, current_priority_level,(void**) &current_thread);
	
	//If there are no other threads anywhere in the run-queue, don't switch to anything
	if(dequeue_result == -1){
		current_thread = previous_thread;
		set_interrupt_level(intlevel);
		return;
	}

	/*In this case, there are other threads to run. Append the previous thread to its same priority queue
	  as long as it is not the idle thread and then context switch to the next thread*/
	if(previous_thread != idle_thread){
		multilevel_queue_enqueue(runnable_queue,previous_thread->priority,previous_thread);
	}

	//printf("[INFO] Switching from thread {ID: %d} to thread {ID: %d}\n",previous_thread->id,current_thread->id);
	
	//If this new thread is on a different priority level than the current thread, we need to adjust our prioirty level and tick counter
	if(current_priority_level != current_thread->priority){
		ticks_since_last_level_switch = 0;
		current_priority_level = current_thread->priority;
	}

	//Context switch to this new thread (automatically re-enabled interrupts)

	current_thread->runtime_remaining = get_thread_runtime_for_priority(current_thread->priority);
	minithread_switch(&(previous_thread->stacktop),&(current_thread->stacktop));
}

/*
 * This is the clock interrupt handling routine.
 * You have to call minithread_clock_init with this
 * function as parameter in minithread_system_initialize
 */
void clock_handler(void* arg)
{
	interrupt_level_t intlevel = set_interrupt_level(DISABLED);
	minithread_t previous_thread = current_thread;
	int dequeue_result;

	//Increment/decremement the appriopriate tick counters
	ticks++;
	//if(ticks % 20 == 0)
		//printf("%d\n",ticks/20);


	process_alarms();


	//We are running the idle thread
	if(previous_thread == idle_thread){
		dequeue_result = multilevel_queue_dequeue(runnable_queue, 0, (void **)&current_thread);

		//There are threads waiting to run
		if(dequeue_result != -1){
			current_priority_level = current_thread->priority;
			ticks_since_last_level_switch = 0;
			current_thread->runtime_remaining = get_thread_runtime_for_priority(current_priority_level);
			minithread_switch(&(previous_thread->stacktop),&(current_thread->stacktop));
			return;
		}
		else{
			//There are no other threads to run, so just continue running the idle thread
			current_thread = idle_thread;
			set_interrupt_level(intlevel);
			return;
		}
	}
	//We're not running the idle thread
	else{
		ticks_since_last_level_switch++;
		previous_thread ->runtime_remaining--;

		//We have hit the max sweep time for the given priority and need to switch to the next one
		if(ticks_since_last_level_switch == get_sweep_runtime_for_priority(current_priority_level)){
			//Find the next thread in the run_queue starting at this new prioirty level
			dequeue_result = multilevel_queue_dequeue(runnable_queue,(current_priority_level + 1) % NUM_PRIORITY_LEVELS,(void **)&current_thread);

			//The thread has exhausted its alloted runtime quanta and needs to be switched & decreased in priority since it has not volentarrily given up execution
			if(previous_thread->runtime_remaining == 0){
				previous_thread->priority--;
			}

			if(dequeue_result == -1){
				//There are no other threads to run except for the one running, so reset its runtime (for the new prioirty) and continue running it
				previous_thread->runtime_remaining = get_thread_runtime_for_priority(previous_thread->priority);
				current_thread = previous_thread;
				current_priority_level = current_thread->priority;
				ticks_since_last_level_switch = 0;
				set_interrupt_level(intlevel);
				return;
			}

			multilevel_queue_enqueue(runnable_queue,previous_thread->priority,previous_thread);

			//Allocate the appropriate runtime for the new thread and switch to it
			current_priority_level = current_thread->priority;
			current_thread->runtime_remaining = get_thread_runtime_for_priority(current_priority_level);
			minithread_switch(&(previous_thread->stacktop),&(current_thread->stacktop));
			return;
		}
		//We still have time remaining on this priority and don't need to switch to the next one
		else{
			//The thread has exhausted its alloted runtime quanta and needs to be switched & decreased in priority since it has not volentarrily given up execution
			if(previous_thread->runtime_remaining == 0){
				previous_thread->priority = (min(previous_thread->priority + 1,NUM_PRIORITY_LEVELS-1));

				//Get the next runnable thread starting the search at the current priority level
				dequeue_result = multilevel_queue_dequeue(runnable_queue, current_priority_level, (void **)&current_thread);
				
				if(dequeue_result == -1){
					//There are no other threads to run except for the one running, so reset its runtime (for the new prioirty) and continue running it
					previous_thread->runtime_remaining = get_thread_runtime_for_priority(previous_thread->priority);
					current_thread = previous_thread;
					current_priority_level = current_thread->priority;
					ticks_since_last_level_switch = 0;
					set_interrupt_level(intlevel);
					return;
				}

				else{
					//There are other threads to run
					if (current_thread->priority != current_priority_level){
						//The next thread to run is on a different priority level than the one we are on currently
						current_priority_level = current_thread->priority;
						ticks_since_last_level_switch = 0;
					}
					
					//Re-enqueue the old thread in its new priority location
					multilevel_queue_enqueue(runnable_queue,previous_thread->priority,previous_thread);

					//Allocate the appropriate runtime for the new thread and switch to it
					current_thread->runtime_remaining = get_thread_runtime_for_priority(current_priority_level);
					minithread_switch(&(previous_thread->stacktop),&(current_thread->stacktop));
					return;
				}
			}
			else{
				//Do Nothing. We don't need to switch to the next priority yet and the thread runing still has alloted execution time left.
			}
		}
	}

	set_interrupt_level(intlevel);
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
	interrupt_level_t intlevel = set_interrupt_level(DISABLED);
	
	//Create and initialize the internal data structures of the minithread system
	minithread_t temp_ref;
	runnable_queue = multilevel_queue_new(NUM_PRIORITY_LEVELS);
	cleanup_queue = queue_new();
	cleanup_sem = semaphore_create();
	if(runnable_queue == NULL || cleanup_queue == NULL || cleanup_sem == NULL) {
		printf("[FATAL] Could not initialize minithread system (Internal structure initialization failed)\n");
		return;
	}
	semaphore_initialize(cleanup_sem,0);


	//Allocate and initialize space for the idle thread
	idle_thread = (minithread_t) malloc(sizeof(struct minithread));
	if(idle_thread == NULL){
		printf("[FATAL] Could not spawn idle thread (Memory allocation failed)\n");
		multilevel_queue_free(runnable_queue);
		queue_free(cleanup_queue);
		semaphore_destroy(cleanup_sem);
		return;
	}
	thread_id_counter = 0;
	idle_thread->id = thread_id_counter;
	idle_thread->destroyed = 0;
	//The idle thread has no priority/max runtime as it is only scheduled when there are other threads to run
	idle_thread->priority = -1;
	idle_thread->runtime_remaining = -1;
	idle_thread->wait_on_alarm = semaphore_create();
	semaphore_initialize(idle_thread->wait_on_alarm,0);
	thread_id_counter++;
	current_thread = idle_thread;
	
	//Create the cleanup thread
	temp_ref = minithread_fork(cleanup_thread_proc,NULL);
	if(temp_ref == NULL){
		printf("[FATAL] Could not spawn cleanup thread\n");
		multilevel_queue_free(runnable_queue);
		queue_free(cleanup_queue);
		semaphore_destroy(cleanup_sem);
		free(idle_thread);
		return;
	}

	//Create the mainproc thread
	minithread_fork(mainproc, mainarg);
	
	create_and_initialize_alarms();

	//Set the current priority level to 0 and initialize the clock handler
	current_priority_level = 0;
	ticks_since_last_level_switch = 0;
	minithread_clock_init(clock_handler);

	
	//Reset interrupt levels and begin program execution with the idle_proc
	set_interrupt_level(ENABLED);
	idle_thread_proc(NULL);
}

/*
 * minithread_unlock_and_stop(tas_lock_t* lock)
 *	Atomically release the specified test-and-set lock and
 *	block the calling thread.
 */
void minithread_unlock_and_stop(tas_lock_t* lock)
{
	interrupt_level_t intlevel = set_interrupt_level(DISABLED);
	minithread_t previous_thread = current_thread;
	
	//Gets the next possible thread to run starting the search at the current prioirty level
	int dequeue_result = multilevel_queue_dequeue(runnable_queue, current_priority_level,(void**) &current_thread);
	
	*lock = 0;
	//If there are no other threads anywhere in the run-queue, don't switch to anything
	if(dequeue_result == -1){
		current_thread = idle_thread;
		current_priority_level = 0;
		ticks_since_last_level_switch = 0;
		minithread_switch(&(previous_thread->stacktop),&(current_thread->stacktop));
	}

	//If this new thread is on a different priority level than the current thread, we need to adjust our prioirty level and tick counter
	if(current_priority_level != current_thread->priority){
		ticks_since_last_level_switch = 0;
		current_priority_level = current_thread->priority;
	}

	//Context switch to this new thread (automatically re-enabled interrupts)

	current_thread->runtime_remaining = get_thread_runtime_for_priority(current_thread->priority);
	minithread_switch(&(previous_thread->stacktop),&(current_thread->stacktop));
}
void minithread_wakeup(void *arg) {
	interrupt_level_t intlevel = set_interrupt_level(DISABLED);
	semaphore_V(((minithread_t) arg)->wait_on_alarm);
	set_interrupt_level(intlevel);
}
/*
 * sleep with timeout in milliseconds
 */
void minithread_sleep_with_timeout(int delay) {
	interrupt_level_t intlevel = set_interrupt_level(DISABLED);
	register_alarm(delay,minithread_wakeup,current_thread);
	semaphore_P(current_thread->wait_on_alarm);
	set_interrupt_level(intlevel);
}

