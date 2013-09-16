#include "queue.h"
#include <stdlib.h>
#include <stdio.h>

int add(void* data1, void* data2) {
	int* val1 = (int*) data1;
	int* val2 = (int*) data2;

	*val2 = *val2 + *val1;
	return 0;
}

main() {
	printf("lol \n");
	queue_t q = queue_new();

	int a = 1, b = 2, c = 3, d = 4, e = 5;
	int result = 0;
	void** data;
	printf("before append \n");
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
	result = queue_length(q);
	printf("length of queue: %d \n", result);
	
	result = queue_delete(q, data);
	printf("remove item that does not exist: %d \n", result);

	result = queue_iterate(q, &add, &b);
}
