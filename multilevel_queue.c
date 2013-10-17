/*
 * Multilevel queue manipulation functions  
 */
#include "multilevel_queue.h"
#include <stdlib.h>
#include <stdio.h>

/*Struct representing a specific level of the queue*/
struct queue_level {
	queue_t base_queue;
	struct queue_level* next_level;
	struct queue_level* prev_level;
	int priority_level;
};

/*Main multilevel queue struct containing a pointer to the top and bottom levels (head and tail)*/
struct multilevel_queue {
	struct queue_level* top_level;
	struct queue_level* bottom_level;
	int num_levels;
};

/*
 * Returns an empty multilevel queue with number_of_levels levels. On error should return NULL.
 */
multilevel_queue_t multilevel_queue_new(int number_of_levels)
{	
	multilevel_queue_t empty_multilevel_queue;
	struct queue_level* current_level;
	struct queue_level* new_level;
	int current_priority = 0;
	int level_loop_counter = 0;
	queue_t temp_queue;

	if (number_of_levels <= 0) {
		printf("[ERROR] Multilevel queue must have one or more levels");
		return NULL;
	}

	empty_multilevel_queue = (multilevel_queue_t) malloc(sizeof(struct multilevel_queue));
	
	/*If memory allocation fails, return NULL*/
	if (empty_multilevel_queue == NULL) {
		return NULL;
	}

	empty_multilevel_queue->num_levels = number_of_levels;

	temp_queue = queue_new();
	if(temp_queue == NULL){
		printf("[ERROR] Multilevel queue initialization failed");
		return NULL;
	}
	
	new_level = (struct queue_level*) malloc(sizeof(struct queue_level));
	new_level->base_queue = temp_queue;
	new_level->next_level = NULL;
	new_level->prev_level = NULL;
	new_level->priority_level = current_priority;
	current_priority++;

	empty_multilevel_queue->top_level = new_level;

	
	current_level = new_level;
	

	for(level_loop_counter - 1; level_loop_counter < number_of_levels; level_loop_counter++){
		 temp_queue = queue_new();
		 if(temp_queue == NULL){
			printf("[ERROR] Multilevel queue initialization failed");
			return NULL;
		 }

		 new_level = (struct queue_level*) malloc(sizeof(struct queue_level));

		 new_level->prev_level = current_level;
		 new_level->next_level = NULL;
		 new_level->base_queue = temp_queue;
		 new_level->priority_level = current_priority;
		 current_priority++;

		 current_level->next_level = new_level;

		 current_level = new_level;
	}

	empty_multilevel_queue->bottom_level = current_level;

	return empty_multilevel_queue;
}

/*
 * Appends an void* to the multilevel queue at the specified level. Return 0 (success) or -1 (failure).
 */
int multilevel_queue_enqueue(multilevel_queue_t queue, int level, void* item)
{	
	int desired_level = level;
	struct queue_level* curr_level;
	if(queue == NULL || level < 0 || level >= queue->num_levels){
		return -1;
	}

	curr_level = queue->top_level;
	while(desired_level > 0){
		curr_level = curr_level->next_level;
		desired_level--;
	}

	return queue_append(curr_level->base_queue,item);

}

/*
 * Dequeue and return the first void* from the multilevel queue starting at the specified level. 
 * Levels wrap around so as long as there is something in the multilevel queue an item should be returned.
 * Return the level that the item was located on and that item if the multilevel queue is nonempty,
 * or -1 (failure) and NULL if queue is empty.
 */
int multilevel_queue_dequeue(multilevel_queue_t queue, int level, void** item)
{
	int flag = 0;
	int desired_level = level;
	struct queue_level* curr_level;
	if(queue == NULL || level < 0 || level >= queue->num_levels){
		*item = NULL;
		return -1;
	}

	curr_level = queue->top_level;
	while(desired_level > 0){
		curr_level = curr_level->next_level;
		desired_level--;
	}

	while(!(flag == 1 && level == curr_level->priority_level) && queue_dequeue(curr_level->base_queue,item) == -1){
		
		if(curr_level->next_level == NULL){
			//when we set currlevel = toplevel set the flag to true
			flag = 1;
			curr_level = queue->top_level;
		}
		else{
			curr_level = curr_level->next_level;
		}		
	}
		
	if(curr_level->priority_level == level && flag == 1){
		*item = NULL;
		return -1;
	}

	return curr_level->priority_level;
}

/* 
 * Free the queue and return 0 (success) or -1 (failure). Do not free the queue nodes; this is
 * the responsibility of the programmer.
 */
int multilevel_queue_free(multilevel_queue_t queue)
{
	struct queue_level* curr;

	if(queue == NULL){
		return -1;
	}

	if(queue->top_level == NULL){
		free(queue);
		return 0;
	}

	curr = queue->top_level->next_level;
	/*Remove all elements from the queue until size is 0 (dequeue return -1)*/
	while(curr != NULL){
		free(curr->prev_level);
		curr = curr->next_level;
	}
	free(queue->bottom_level);

	free(queue);
	return 0;
}
