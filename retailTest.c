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


struct phone {
	int serial;
};
typedef struct phone* phone_t;

int serialCounter = 0;

int getNextSerial(){
	int id = serialCounter;
	serialCounter++;
	return id;
}

int consumer(int* arg) {
	
	phone_t got_phone;
	
	semaphore_P(full);
	semaphore_P(mutex);

	queue_dequeue(phone_queue, (void **) &got_phone);
	
	printf("Got phone with serial %d \n",got_phone->serial);

	free(got_phone);
	semaphore_V(mutex);
	semaphore_V(empty);
}

int producer(int* arg) {
	phone_t new_Phone;
	while(1) {
		semaphore_P(empty);
		semaphore_P(mutex);
		new_Phone = (phone_t) malloc(sizeof(struct phone));
		new_Phone->serial = getNextSerial();
		queue_append(phone_queue, new_Phone);
		printf("Unpacked a phone with serial %d \n", new_Phone->serial);
		semaphore_V(mutex);
		semaphore_V(full);
		
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
  
  phone_queue = queue_new();

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
