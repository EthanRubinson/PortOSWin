#include <stdio.h>

#include "interrupts.h"
#include "alarm.h"
#include "minithread.h"
#include "queue.h"


struct alarm_node{
	int alarm_id;
	long wakeup_time;
	void *passed_args;
	void (*passed_funct)(void*);
	struct alarm_node* next;
};

struct alarm_list {
	struct alarm_node* head;
};

typedef struct alarm_list* alarm_list_t;
typedef struct alarm_node* alarm_node_t;

alarm_list_t registered_alarms;
int alarm_id_counter = 0;


// create/initialze
int create_and_initialize_alarms() {
	registered_alarms = (alarm_list_t) malloc(sizeof(struct alarm_list));
	
	/*If memory allocation fails, return NULL*/
	if (registered_alarms == NULL) {
		return -1;
	}

	/*Memory allocation was successful, initialize fields*/
	registered_alarms->head = NULL;
	return 0;
}

//delete
void alarm_list_delete(int id) {
	alarm_node_t current_alarm;
	alarm_node_t previous_alarm;

	if (registered_alarms == NULL) {
		printf("[ERROR] Cannot delete from empty list \n");
		return;
	}
	current_alarm = registered_alarms->head;
	previous_alarm = current_alarm;

	while (current_alarm != NULL) {
		if (current_alarm->alarm_id == id) {
			if(current_alarm == registered_alarms->head){
				registered_alarms->head = registered_alarms->head->next;
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

//insert (returns alarm id)
int alarm_list_insert(int wakeup, void (*func)(void*), void *arg) {
	alarm_node_t current_alarm;
	alarm_node_t previous_alarm;
	alarm_node_t new_alarm_node;

	if (registered_alarms == NULL) {
		printf("[ERROR] Cannot insert into null list \n");
		return -1;
	}

	new_alarm_node = (alarm_node_t) malloc(sizeof(struct alarm_node));
	new_alarm_node->passed_args = arg;
	new_alarm_node->passed_funct = func;
	new_alarm_node->wakeup_time = wakeup;
	new_alarm_node->alarm_id = alarm_id_counter++;
	new_alarm_node->next = NULL;

	if(registered_alarms->head == NULL){
		registered_alarms->head = new_alarm_node;
	}
	else{
		previous_alarm = registered_alarms->head;
		current_alarm = registered_alarms->head;
	
		while(current_alarm != NULL && current_alarm->wakeup_time < wakeup) {
			previous_alarm = current_alarm;
			current_alarm = current_alarm->next;
		}

		if(current_alarm == previous_alarm){
			new_alarm_node->next = registered_alarms->head;
			registered_alarms->head = new_alarm_node;
		}
		else{
			previous_alarm->next = new_alarm_node;
			new_alarm_node->next = current_alarm;
		}
	}
	return new_alarm_node->alarm_id;
}


void process_alarms(){
	while(registered_alarms != NULL && registered_alarms->head != NULL && registered_alarms->head->wakeup_time <= ticks){
		registered_alarms->head->passed_funct(registered_alarms->head->passed_args);
		deregister_alarm(registered_alarms->head->alarm_id);
	}
	
	return;
}


/*
 * insert alarm event into the alarm queue
 * returns an "alarm id", which is an integer that identifies the
 * alarm.
 */
int register_alarm(int delay, void (*func)(void*), void *arg)
{
	interrupt_level_t intlevel = set_interrupt_level(DISABLED);
	int wait_period = ticks + (delay * MILLISECOND)/PERIOD;
	int new_id = alarm_list_insert(wait_period, func, arg);
	set_interrupt_level(intlevel);
	return new_id;
}

/*
 * delete a given alarm  
 * it is ok to try to delete an alarm that has already executed.
 */
void deregister_alarm(int alarmid)
{
	interrupt_level_t intlevel = set_interrupt_level(DISABLED);
	alarm_list_delete(alarmid);
	set_interrupt_level(intlevel);
}

