/*
 * Generic queue implementation.
 *
 */
#include "queue.h"
#include <stdlib.h>
#include <stdio.h>

struct list_node {
	struct list_node* next;
	struct list_node* prev; 
	void* data;
};

struct queue {
	struct list_node* head;
	struct list_node* tail;
	int size;
};


/*
 * Return an empty queue.
 */
queue_t queue_new() {
	queue_t empty_queue = (queue_t) malloc(sizeof(struct queue));
	
	/*If memory allocation fails, return NULL*/
	if (empty_queue == NULL) {
		return NULL;
	}

	/*Memory allocation was successful, initialize fields*/
	empty_queue->head = NULL;
	empty_queue->tail = NULL;
	empty_queue->size = 0;
	return empty_queue;
}

/*
 * Prepend a void* to a queue (both specifed as parameters).  Return
 * 0 (success) or -1 (failure).
 */
int queue_prepend(queue_t queue, void* item) {
	struct list_node* node = (struct list_node*) malloc(sizeof(struct list_node));

	if(queue == NULL || node == NULL){
		return -1;
	}

	node->data = item;
	node->next = queue->head;
	node->prev = NULL;

	/*queue is empty*/
	if (queue->head == NULL) {
		queue->tail = node;
	}
	/*queue is not empty*/
	else {
		queue->head->prev = node;
	}

	queue->head = node;
	queue->size++;

	return 0;
}

/*
 * Append a void* to a queue (both specifed as parameters). Return
 * 0 (success) or -1 (failure). 
 */
int queue_append(queue_t queue, void* item) {
	struct list_node* node = (struct list_node*) malloc(sizeof(struct list_node));
	
	if(queue == NULL || node == NULL){
		return -1;
	}

	node->data = item;
	node->next = NULL;
	node->prev = queue->tail;

	/*queue is empty*/
	if (queue->tail = NULL) {
		queue->head = node;
	}
	/*queue is not empty*/
	else {
		queue->tail->next = node;
	}

	queue->tail = node;
	queue->size++;

	return 0;
}

/*
 * Dequeue and return the first void* from the queue or NULL if queue
 * is empty.  Return 0 (success) or -1 (failure).
 */
int queue_dequeue(queue_t queue, void** item) {
	struct list_node* temp;
	
	/*If the queue is empty or does not exist, dequeue fails*/
	if(queue == NULL || queue->size == 0){
		*item = NULL;
		return -1;
	}
	
	temp = queue->head;	

	*item = temp->data;

	if (temp->next != NULL) {					// two or more elements
		temp->next->prev = NULL;
	} else {									// last element in queue
		queue->tail = NULL;
	}

	queue->head = temp->next;
	queue->size--;
	
	free(temp);

	return 0;
}

/*
 * Iterate the function parameter over each element in the queue.  The
 * additional void* argument is passed to the function as its first
 * argument and the queue element is the second.  Return 0 (success)
 * or -1 (failure).
 */
int queue_iterate(queue_t queue, PFany f, void* item) {
	struct list_node* curr;

	if(queue == NULL){
		return -1;
	}
	
	curr = queue->head;

	while(curr != NULL) {
		/*
		 *If function f returns -1, indicate a failure
		 */
		if (f(item, curr->data) == -1) {
			return -1;
		}
		curr = curr->next;
	}
	return 0;
}

/*
 * Free the queue and return 0 (success) or -1 (failure).
 */
int queue_free (queue_t queue) {
	struct list_node* curr;

	if(queue == NULL){
		return -1;
	}

	curr = queue->head->next;
	/*Remove all elements from the queue until size is 0 (dequeue return -1)*/
	while(curr != null){
		free(curr->prev);
		curr = curr->next;
	}
	free(queue->tail);

	free(queue);
	return 0;

}

/*
 * Return the number of items in the queue.
 */
int queue_length(queue_t queue) {
	if(queue == NULL){
		return -1;
	}

	return queue->size;
}


/* 
 * Delete the specified item from the given queue. 
 * Return -1 on error.
 */
int queue_delete(queue_t queue, void** item) {
	struct list_node* node;
	if(queue == NULL){
		return -1;
	}
	
	node = queue->head;

	while(node != NULL) {
		if (node->data == *item) {

			if(node == queue->head) {
				node->next->prev = NULL;
				queue->head = node->next;
			} else if(node == queue->tail) {
				node->prev->next = NULL;
				queue->tail = node->prev;
			} else {
				node->prev->next = node->next;
				node->next->prev = node->prev;
			}
			
			queue->size--;

			free(node);
			return 0;
		}
	}
	return -1;
}
