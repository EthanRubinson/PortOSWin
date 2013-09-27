#include <stdio.h>
#include <stdlib.h>
#include "minithread.h"
#include "synch.h"
#include "queue.h"

/*= number of employees continuously unpacking phones*/
#define N 20

/*= number of customers that want phones*/
#define M 1000

/*Semaphore for blocking employees when 'enough' phones have been unpacked*/
semaphore_t empty;

/*Semaphore for the phone queue*/
semaphore_t full;

/*A mutex lock*/
semaphore_t mutex;

/*= Queue containing the currently unpacked and available phones*/
queue_t phone_queue;

/*= the phone structure where each phone contains a unique serial*/
struct phone {
	int serial;
};
typedef struct phone* phone_t;

/*= Unique serial counter. Used to generate unique serials whenever a new phone is unpacked*/
int serialCounter = 0;


/*= The next unique serial number*/
int getNextSerial(){
	int id = serialCounter;
	serialCounter++;
	return id;
}

/*The consumer thread. Retrieve a phone when available and immediately print out its serial*/
int consumer(int* arg) {
	phone_t got_phone;
	
	semaphore_P(full);
	semaphore_P(mutex);

	queue_dequeue(phone_queue, (void **) &got_phone);
	
	printf("Got phone with serial %d \n",got_phone->serial);

	free(got_phone);
	semaphore_V(mutex);
	semaphore_V(empty);
	return 0;
}

/*The employee thread. Continously unpack phones in sequential order*/
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
		minithread_yield();
	}
	return 0;
}

/*Initialze the retail scenario*/
int beginScenario(int* arg) {
  int employeeCounter;
  int customerCounter;

  empty = semaphore_create();
  full = semaphore_create();
  mutex = semaphore_create();
  
  //Initialize the semaphores.
  //'M' designates that the maximum phones that can be unpacked at a time is the same as the number of customers.
  semaphore_initialize(empty, M);
  semaphore_initialize(full, 0);
  semaphore_initialize(mutex, 1);
  
  phone_queue = queue_new();

  for(employeeCounter = 0; employeeCounter < N; employeeCounter++){
	minithread_fork(producer, NULL);
  }
  
  for(customerCounter = 0; customerCounter < M; customerCounter++){
	minithread_fork(consumer, NULL);
  }
  return 0;
}

void main(void) {
  minithread_system_initialize(beginScenario, NULL);
}
