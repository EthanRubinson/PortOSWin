#include <stdio.h>
#include <stdlib.h>
#include "minithread.h"
#include "synch.h"
#include "queue.h"

#define N 5
#define M 5

semaphore_t empty;
semaphore_t full;
semaphore_t mutex;

queue_t phone_queue;

int serialCounter = 0;

int getNextSerial(){
	int id = serialCounter;
	serialCounter++;
	return id;
}

int consumer(int* arg) {
	int *phone_id;
	semaphore_P(full);
	semaphore_P(mutex);
	queue_dequeue(phone_queue, (void **) &phone_id);
	printf("Got phone with serial %d \n",*phone_id);
	semaphore_V(mutex);
	semaphore_V(empty);
}

int producer(int* arg) {
	int phone_id;
	while(1) {
		printf("a");
		semaphore_P(empty);
		printf("b");
		semaphore_P(mutex);

		printf("Starting to unpack\n");
		phone_id = getNextSerial();
		queue_append(phone_queue, &phone_id);
		printf("Unpacked a phone with serial %d \n",phone_id);

		semaphore_V(mutex);
		printf("c");
		semaphore_V(full);
		printf("d");
	}
}

int beginScenario(int* arg) {
  int employeeCounter;
  int customerCounter;

  empty = semaphore_create();
  full = semaphore_create();
  mutex = semaphore_create();
  
  semaphore_initialize(empty, 15);
  semaphore_initialize(full, 0);
  semaphore_initialize(mutex, 1);
  


  for(employeeCounter = 0; employeeCounter < N; employeeCounter++){
	minithread_fork(producer, NULL);
  }
  printf("Done producing p threads");
  for(customerCounter = 0; customerCounter < M; customerCounter++){
	minithread_fork(consumer, NULL);
  }
  printf("Done producing c threads");
}

void main(void) {
  minithread_system_initialize(beginScenario, NULL);
}
