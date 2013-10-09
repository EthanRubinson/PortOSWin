#include <stdio.h>

#include "interrupts.h"
#include "alarm.h"
#include "minithread.h"
#include "queue.h"

struct alarm_node {
	struct alarm_node* next;
	minithread_t waiting_thread;
	int alarm_id;
	int wakeup_time;
};

struct alarm_list {
	struct alarm_node* head;
	int size;
};

typedef struct alarm_list* alarm_list_t;
typedef struct alarm_node* alarm_node_t;

alarm_list_t registered_alarms;




// create
int initialize_alarm_handler() {
	alarm_list_t empty_list = (alarm_list_t) malloc(sizeof(struct alarm_list));
	
	/*If memory allocation fails, return NULL*/
	if (empty_list == NULL) {
		registered_alarms = NULL;
		return -1;
	}

	/*Memory allocation was successful, initialize fields*/
	empty_list->head = NULL;
	empty_list->size = 0;
	registered_alarms = empty_list;
	return 0;
}


//delete
void alarm_list_delete(alarm_list_t alarms, int id) {
	alarm_node_t current_alarm;
	alarm_node_t previous_alarm;
	if (alarms == NULL) {
		printf("[ERROR] Cannot delete from empty list \n");
		return;
	}
	current_alarm = alarms->head;
	previous_alarm = current_alarm;

	while (current_alarm != NULL) {
		if (current_alarm->alarm_id == id) {
			if(current_alarm == alarms->head){
				alarms->head = alarms->head->next;
				free(current_alarm);
				return;
			}
			else{
				previous_alarm->next = current_alarm->next;
				free(current_alarm);
				return;
			}
		}
		previous_alarm = current_alarm;
		current_alarm = current_alarm->next;
	}
	printf("[INFO] ID not found in alarm list, could not delete \n");
}

//insert
void alarm_list_insert(alarm_list_t alarms, int wakeup, minithread_t new_alarm_thread) {
	alarm_node_t current_alarm;
	if (alarms == NULL) {
		printf("[ERROR] Cannot insert into null list \n");
		return;
	}
	current_alarm = alarms->head;
	while (current_alarm != NULL && current_alarm->wakeup_time < wakeup) {

	}

}
//initialize
//free

/*
 * insert alarm event into the alarm queue
 * returns an "alarm id", which is an integer that identifies the
 * alarm.
 */
int register_alarm(int delay, void (*func)(void*), void *arg)
{
	// interrupts ?
	return -1;
}

/*
 * delete a given alarm  
 * it is ok to try to delete an alarm that has already executed.
 */
void deregister_alarm(int alarmid)
{

}

