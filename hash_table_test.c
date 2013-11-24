#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include "hashtable.h"

//Original tests
void test1(){
	int *received_item;
	int test_value = 99;
	hashtable_t a = hashtable_new(0);
	
	hashtable_put(a, "I'm a key", (void *) &test_value);
	hashtable_get(a, "I'm a key", (void **) &received_item);
	
	if (*received_item != 99){
		printf("==autograde== {'name': 'test1', 'pass': False, 'reason': 'key 0 not 99. This does not bode well.'\n");
		return;
	}

	hashtable_destroy(a);
	printf("==autograde== {'name': 'test1', 'pass': True, 'reason': 'success'\n");
}

//Test insertions
void test2(){
	hashtable_t a = hashtable_new(0);
	int *received_item;
	int *val;
	char* key;
	int i = 0;

	for (i=0; i<1000; i+=1){
		key = (char *)malloc(15);
		val = (int *)malloc(sizeof(int));
		*val = i+1;
		sprintf(key, "Key: %d", i);
		hashtable_put(a, key, (void *) val);
		hashtable_get(a, key, (void **) &received_item);
		free(key);
		if (*received_item != i+1){
			printf("==autograde== {'name': 'test2', 'pass': False, 'reason': 'failed to retrieve %d with key %d; got %d. Insertions do not appear to work.'\n", i+1, i,*received_item);
			return;
		}
	}

	for (i=0; i<1000; i++){
		key = (char *)malloc(15);
		sprintf(key, "Key: %d", i);
		hashtable_get(a, key, (void **) &received_item);
		free(key);
		if (*received_item != i+1){ 
			printf("==autograde== {'name': 'test2', 'pass': False, 'reason': 'failed to retrieve %d with key %d; got %d. fyguy do not appear to work.'\n", i+1, i,*received_item);
			return;
		}
	}
	hashtable_destroy(a);
	printf("==autograde== {'name': 'test2', 'pass': True, 'reason': 'success'\n");
}

//Test insertions w/ deletions
void test3(){
	hashtable_t a = hashtable_new(0);
	int *item;
	int *val;
	char* key;
	int i = 0;

	for (i=0; i<100; i++){
		key = (char *)malloc(15);
		val = (int *)malloc(sizeof(int));
		*val = i+1;
		sprintf(key, "Key: %d", i);
		hashtable_put(a, key, (void *) val);
		hashtable_get(a, key, (void ** ) &item);
		if (*item != i+1){
			printf("==autograde== {'name': 'test3', 'pass': False, 'reason': 'failed to retrieve %d with key %d;  got  %d.  Deletions  appear  to  interfere  with  operation  if  test2  succeeded.'\n",  i+1, i,*item);
			free(key);
			return;
		}
		if (i%2){
			hashtable_remove(a, key);
			free(item);
		}
		free(key);
	}

	for (i=0; i<100; i++){
		key = (char *)malloc(15);
		sprintf(key, "Key: %d", i);
		hashtable_get(a, key,(void **) &item);
		free(key);
		if ((i%2)==0){
			if (*item != i+1){
				printf("==autograde== {'name': 'test3', 'pass': False, 'reason': 'failed to retrieve %d with key %d; got %d. Deletions appear to interfere with operation if test2 succeeded.'\n", i+1, i,*item);
				return;
			}
		}
	}

	hashtable_destroy(a);
	printf("==autograde== {'name': 'test3', 'pass': True, 'reason': 'success'\n");
}

//Test collisions, doubles as a memory leak test
void test4(){
	hashtable_t a = hashtable_new(0);
	int *item;
	int *val;
	char* key;
	int i = 0;

	key = (char*)malloc(15);
	val = (int*)malloc(sizeof(int));
	*val = 88;
	sprintf(key,"Key: %d",1001);
	hashtable_put(a, key, (void *)val);
	free(key);
	//This pretty much guarantees a collision, although slow
	for (i=0; i < 1000; i++){
		key = (char*)malloc(15);
		val = (int*)malloc(sizeof(int));
		sprintf(key,"Key: %d", i);
		*val = i+1;
		hashtable_put(a, key, (void *) val);
		hashtable_get(a, key, (void **) &item);
		if (*item != i+1){
			printf("==autograde== {'name': 'test4', 'pass': False, 'reason': 'failed to retrieve %d with key %d; got %d. This may indicate a problem with your collision handling. Check your hash function if previous tests passes.'\n", i+1, i,*item);
			return;
		}
		hashtable_remove(a, key);
		free(key);
		free(item);
	}
	key = (char*)malloc(15);
	sprintf(key,"Key: %d",1001);
	hashtable_get(a, key,(void **) &item);
	free(key);
	if (*item != 88){
		printf("==autograde== {'name': 'test4', 'pass': False, 'reason': 'failed to retrieve %d with key %d; got %d. This may indicate a problem with your collision handling. Check your hash function if previous tests passes.'\n", 88, 1001,*item);
		return; 
	}

	hashtable_destroy(a);
	printf("==autograde== {'name': 'test4', 'pass': True, 'reason': 'success'\n");
}

//General use case with smallish positive numbers
void test5(){
	hashtable_t a = hashtable_new(0);
	int *item;
	int *val;
	char* key;
	int i = 0;
	int temp = 0;
	srand(42);


	for (i=0; i<5000; i++){
		temp = rand() % 32768;
		key = (char *)malloc(15);
		val = (int *)malloc(sizeof(int));
		sprintf(key,"Key: %d",temp);
		*val = temp + 1;
		hashtable_put(a, key, (void *) val);
		hashtable_get(a, key, (void** ) &item);
		if (*item != temp+1){
			printf("==autograde== {'name': 'test5', 'pass': False, 'reason': 'failed to retrieve %d with key %d; got %d. If you passed all previous tests, this may indicate that you cannot handle large amounts of entries.'\n", temp+1, temp,*item);
			return;
		}
		if (rand()%100==0){
			hashtable_remove(a,key);
			free(item);
			//Maybe test that item is actually removed here.
		}
		free(key);
	}

	hashtable_destroy(a);
	printf("==autograde== {'name': 'test5', 'pass': True, 'reason': 'success'\n");
}

//Corner Cases
void test6(){
	hashtable_t a = hashtable_new(0);
	int *item;
	int *val;
	char* key;
	int i = 0;
	int temp;

	srand(42);
	//Test key reuse
	key = (char *)malloc(15);
	val = (int *)malloc(sizeof(int));
	sprintf(key,"Key: %d",42);
	*val = 42;
	hashtable_put(a, key, (void *)val);
	hashtable_remove(a, key);
	free(val);
	val = (int*)malloc(sizeof(int));
	*val = 43;
	hashtable_put(a, key, (void *) val);
	hashtable_get(a, key, (void **)&item);
	if (*item!=43){
		printf("==autograde== {'name': 'test6', 'pass': False, 'reason': 'failed to retrieve %d with key %d;  got  %d.  Hashtable  does  not  seem  to  support  reusing  a  previously  deleted  key.'\n",  43, 43,*item);
		return;
	}
	
	/*val = (int*)malloc(sizeof(int));
	*val = 42;
	//Test reptitive key
	hashtable_put(a, key, (void *) val);
	hashtable_get(a, key,(void **)&item);
	if (*item!=42){
		printf("==autograde== {'name': 'test6', 'pass': False, 'reason': 'failed to retrieve %d with key %d; got %d. Hashtable does not seem to support updating key.'\n", 42, 42,*item);
		return;
	}
	free(key);*/

	//Test negative numbers
	for (i=0; i<100; i++){
		temp = -(rand() % 32768);
		key = (char *)malloc(15);
		val = (int *)malloc(sizeof(int));
		sprintf(key,"Key: %d",temp);
		*val = temp;

		hashtable_put(a, key, (void *)val);
		hashtable_get(a, key, (void **) &item);
		free(key);
		if (*item != temp){
			printf("==autograde== {'name': 'test6', 'pass': False, 'reason': 'failed to retrieve %d with key %d; got %d. Hashtable does not seem to support negative numbers.'\n", temp, temp,*item);
			return;
		}
	}
	
	
	hashtable_destroy(a);
	a = hashtable_new(0);
	
	//Test Largest numbers
	key = (char *) malloc(16);
	val = (int *)malloc(sizeof(int));
	sprintf(key,"Key: %d",INT_MAX);
	*val = INT_MAX;

	hashtable_put(a, key, (void *)val);
	hashtable_get(a, key, (void **)&item);

	if (*item!=INT_MAX){
		printf("==autograde== {'name': 'test6', 'pass': False, 'reason': 'failed to retrieve %d with key %d; got %d. Hashtable does not seem to support max int.'\n", INT_MAX, INT_MAX,*item);
		return;
	}

	free(key);

	hashtable_destroy(a);
	a = hashtable_new(0);

	//Test Smallest numbers
	key = (char *)malloc(17);
	val = (int *)malloc(sizeof(int));
	sprintf(key,"Key: %d",INT_MIN);
	*val = INT_MIN;
	hashtable_put(a, key, (void *)val);
	hashtable_get(a, key, (void **)&item);
	free(key);
	if (*item!=INT_MIN){
		printf("==autograde== {'name': 'test6', 'pass': False, 'reason': 'failed to retrieve %d with key %d; got %d. Hashtable does not seem to support min int.'\n", INT_MIN, INT_MIN,*item);
		return;
	}
	hashtable_destroy(a);
	printf("==autograde== {'name': 'test6', 'pass': True, 'reason': 'amazing success!'\n");
}

int main(int argc, char *argv[]){
	//Do all the tests
	test1();
	test2();
	test3();
	test4();
	test5();
	test6();
	return 0;
}
