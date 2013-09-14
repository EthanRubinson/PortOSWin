#include "queue.h"
#include <stdlib.h>
#include <stdio.h>

void main(void) {
	queue_t q = queue_new();

	int a = 1, b = 2, c = 3, d = 4, e = 5;
	int result = 0;
	void** data;

	result = queue_append(q, &a);
	printf("added element 1: %d \n", result);
	result = queue_append(q, &b);
	printf("added element 2: %d \n", result);
	result = queue_append(q, &c);
	printf("added element 3: %d \n", result);
	result = queue_append(q, &d);
	printf("added element 4: %d \n", result);
	result = queue_append(q, &e);
	printf("added element 5: %d \n", result);

	result = queue_dequeue(q, data);
	printf("removed element 5: %d \n", result);
}
