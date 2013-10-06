#include "multilevel_queue.h"
#include <stdlib.h>
#include <stdio.h>

void main(void) {
	multilevel_queue_t queue = multilevel_queue_new(5);
	int a = 1;
	int b = 2;
	int c = 3;
	int d = 4; 
	int e = 5;
	int f = 6;
	int g = 7;
	int h = 8;

	int data = 0;
	int* data_pointer = &data;
	void** pointer_wrapper = (void** ) &data_pointer;

	multilevel_queue_enqueue(queue, 0, &a);
	printf("Put 1 into level 0 \n");
	multilevel_queue_enqueue(queue, 1, &b);
	printf("Put 2 into level 1 \n");
	multilevel_queue_enqueue(queue, 2, &c);
	printf("Put 3 into level 2 \n");
	multilevel_queue_enqueue(queue, 3, &d);
	printf("Put 4 into level 3 \n");
	multilevel_queue_enqueue(queue, 4, &e);
	printf("Put 5 into level 4 \n");

	multilevel_queue_dequeue(queue, 6, pointer_wrapper);
	printf("Dequeue from level 6 : %d \n", getInt(pointer_wrapper));
	multilevel_queue_dequeue(queue, 0, pointer_wrapper);
	printf("Dequeue from level 0 : %d \n", getInt(pointer_wrapper));
	multilevel_queue_dequeue(queue, 1, pointer_wrapper);
	printf("Dequeue from level 1 : %d \n", getInt(pointer_wrapper));
	multilevel_queue_dequeue(queue, 0, pointer_wrapper);
	printf("Dequeue from level 0 : %d \n", getInt(pointer_wrapper));
	multilevel_queue_dequeue(queue, 4, pointer_wrapper);
	printf("Dequeue from level 4 : %d \n", getInt(pointer_wrapper));
	multilevel_queue_dequeue(queue, 0, pointer_wrapper);
	printf("Dequeue from level 0 : %d \n", getInt(pointer_wrapper));
	multilevel_queue_dequeue(queue, 0, pointer_wrapper);
	printf("Dequeue from level 0 : %d \n", getInt(pointer_wrapper));

	multilevel_queue_enqueue(queue, 0, &a);
	printf("Put 1 into level 0 \n");
	multilevel_queue_enqueue(queue, 0, &b);
	printf("Put 2 into level 0 \n");
	multilevel_queue_enqueue(queue, 3, &c);
	printf("Put 3 into level 3 \n");
	multilevel_queue_enqueue(queue, 3, &d);
	printf("Put 4 into level 3 \n");
	multilevel_queue_enqueue(queue, 3, &e);
	printf("Put 5 into level 3 \n");

	multilevel_queue_dequeue(queue, 1, pointer_wrapper);
	printf("Dequeue from level 1 : %d \n", getInt(pointer_wrapper));
	multilevel_queue_dequeue(queue, 2, pointer_wrapper);
	printf("Dequeue from level 2 : %d \n", getInt(pointer_wrapper));
	multilevel_queue_dequeue(queue, 3, pointer_wrapper);
	printf("Dequeue from level 3 : %d \n", getInt(pointer_wrapper));
	multilevel_queue_dequeue(queue, 3, pointer_wrapper);
	printf("Dequeue from level 3 : %d \n", getInt(pointer_wrapper));
	multilevel_queue_dequeue(queue, 3, pointer_wrapper);
	printf("Dequeue from level 3 : %d \n", getInt(pointer_wrapper));
	multilevel_queue_dequeue(queue, 3, pointer_wrapper);
	printf("Dequeue from level 3 : %d \n", getInt(pointer_wrapper));
}

int getInt(void** data) {
	int* int_pointer_wrapper = (int*) *data;
	if (int_pointer_wrapper == NULL) 
		return 0;

	return *int_pointer_wrapper;
}