/* test2.c

   Spawn a three threads.
*/

#include "minithread.h"

#include <stdio.h>
#include <stdlib.h>


int thread3(int* arg) {
  printf("Thread 3.\n");
  while(1){}
  return 0;
}

int thread2(int* arg) {
  minithread_t thread = minithread_fork(thread3, NULL);
  printf("Thread 2.\n");
  while(1){}

  return 0;
}

int thread1(int* arg) {
  minithread_t thread = minithread_fork(thread2, NULL);
  printf("Thread 1.\n");
  while(1){}

  return 0;
}

main() {
  minithread_system_initialize(thread1, NULL);
}
