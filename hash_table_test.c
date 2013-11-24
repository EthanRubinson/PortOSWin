#include <assert.h>
#include <limits.h>
#include "hashtable.c"

//Original tests
void test1(){
	int *item;
	int x = 99;
	hashtable_t a = hashtable_new(0);

	hashtable_put(a, "ge", (void *) &x);
	hashtable_get(a, "ge", (void **) &item);
	
	if (*item != 99){
		printf("==autograde== {'name': 'test1', 'pass': False, 'reason': 'key 0 not 99. This does not bode well.'\n");
		return;
	}


	printf("==autograde== {'name': 'test1', 'pass': True, 'reason': 'success'\n");
}

//Test insertions
void test2(){
	hashtable_t a = hashtable_new(0);
	int *item;
	int *val;
	char* key;
	int i = 0;

	for (i=0; i<1000; i+=1){
		key = (char *)malloc(sizeof(char) * 6);
		val = (int *)malloc(sizeof(int));
		*val = i+1;
		sprintf(key, "Key: %d", i);
		hashtable_put(a, key, (void *) val);
		hashtable_get(a, key, (void **) &item);
		if (*item != i+1){
			printf("==autograde== {'name': 'test2', 'pass': False, 'reason': 'failed to retrieve %d with key %d; got %d. Insertions do not appear to work.'\n", i+1, i,*item);
			return;
		}
	}

	for (i=0; i<1000; i++){
		key = (char *)malloc(sizeof(char) * 6);
		sprintf(key, "Key: %d", i);
		hashtable_get(a, key, (void **) &item);
		if (*item != i+1){ 
			printf("==autograde== {'name': 'test2', 'pass': False, 'reason': 'failed to retrieve %d with key %d; got %d. fyguy do not appear to work.'\n", i+1, i,*item);
			return;
		}
	}
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
		key = (char *)malloc(sizeof(char) * 6);
		val = (int *)malloc(sizeof(int));
		*val = i+1;
		sprintf(key, "Key: %d", i);
		hashtable_put(a, key, (void *) val);
		hashtable_get(a, key, (void ** ) &item);
		if (*item != i+1){
			
			printf("==autograde== {'name': 'test3', 'pass': False, 'reason': 'failed to retrieve %d with key %d;  got  %d.  Deletions  appear  to  interfere  with  operation  if  test2  succeeded.'\n",  i+1, i,*item);
			return;
		}
		if (i%2){
			hashtable_remove(a, key);
			printf("s");
		}
		
	}
	printf("half done");
	for (i=0; i<100; i++){
		key = (char *)malloc(sizeof(char) * 6);
		printf(key, "Key: %d", i);
		hashtable_get(a, key,(void **) &item);
		
		if ((i%2)==0){
			if (*item != i+1){
				printf("==autograde== {'name': 'test3', 'pass': False, 'reason': 'failed to retrieve %d with key %d; got %d. Deletions appear to interfere with operation if test2 succeeded.'\n", i+1, i,*item);
				return;
			}
		}
	}

	printf("==autograde== {'name': 'test3', 'pass': True, 'reason': 'success'\n");
}

//Test collisions, doubles as a memory leak test
/*void test4(){
	struct hashtable a;
	int i = 0;

	hashtable_create(&a);
	hashtable_put(&a, 1001, 88);
	//This pretty much guarantees a collision, although slow
	for (i=0; i < 1000; i++){
		hashtable_put(&a, i, i+1);
		if (hashtable_get(&a, i) != i+1){
			printf("==autograde== {'name': 'test4', 'pass': False, 'reason': 'failed to retrieve %d with key %d; got %d. This may indicate a problem with your collision handling. Check your hash function if previous tests passes.'\n", i+1, i,hashtable_get(&a, i));
			return;
		}
		hashtable_remove(&a, i);
	}
	if (hashtable_get(&a, 1001) != 88){
		printf("==autograde== {'name': 'test4', 'pass': False, 'reason': 'failed to retrieve %d with key %d; got %d. This may indicate a problem with your collision handling. Check your hash function if previous tests passes.'\n", 88, 1001,hashtable_get(&a, 1001));
		return; 
	}
	printf("==autograde== {'name': 'test4', 'pass': True, 'reason': 'success'\n");
}

//General use case with smallish positive numbers
void test5(){
	struct hashtable a;
	int i = 0;
	int temp = 0;
	srand(42);

	hashtable_create(&a);

	for (i=0; i<5000; i++){
		temp = rand() % 32768;
		hashtable_put(&a, temp, temp+1);
		if (hashtable_get(&a, temp) != temp+1){
			printf("==autograde== {'name': 'test5', 'pass': False, 'reason': 'failed to retrieve %d with key %d; got %d. If you passed all previous tests, this may indicate that you cannot handle large amounts of entries.'\n", temp+1, temp,hashtable_get(&a, temp));
			return;
		}
		if (rand()%100==0){
			hashtable_remove(&a,temp);
			//Maybe test that item is actually removed here.
		}
	}
	printf("==autograde== {'name': 'test5', 'pass': True, 'reason': 'success'\n");
}

//Corner Cases
void test6(){
	struct hashtable a;
	int i = 0;
	int temp;

	hashtable_create(&a);
	srand(42);
	//Test key reuse
	hashtable_put(&a, 42, 42);
	hashtable_remove(&a, 42);
	hashtable_put(&a, 42, 43);
	if (hashtable_get(&a, 42)!=43){
		printf("==autograde== {'name': 'test6', 'pass': False, 'reason': 'failed to retrieve %d with key %d;  got  %d.  Hashtable  does  not  seem  to  support  reusing  a  previously  deleted  key.'\n",  43, 43,hashtable_get(&a, 43));
		return;
	}

	//Test reptitive key
	hashtable_put(&a, 42, 42);
	if (hashtable_get(&a, 42)!=42){
		printf("==autograde== {'name': 'test6', 'pass': False, 'reason': 'failed to retrieve %d with key %d; got %d. Hashtable does not seem to support updating key.'\n", 42, 42,hashtable_get(&a, 42));
		return;
	}

	//Test negative numbers
	for (i=0; i<100; i++){
		temp = ­ (rand() % 32768);
		hashtable_put(&a, temp, temp);
		if (hashtable_get(&a, temp) != temp){
			printf("==autograde== {'name': 'test6', 'pass': False, 'reason': 'failed to retrieve %d with key %d; got %d. Hashtable does not seem to support negative numbers.'\n", temp, temp,hashtable_get(&a, temp));
			return;
		}
	}

	hashtable_create(&a);
	//Test Largest numbers
	hashtable_put(&a, INT_MAX, INT_MAX);
	if (hashtable_get(&a, INT_MAX)!=INT_MAX){
		printf("==autograde== {'name': 'test6', 'pass': False, 'reason': 'failed to retrieve %d with key %d; got %d. Hashtable does not seem to support max int.'\n", INT_MAX, INT_MAX,hashtable_get(&a, INT_MAX));
		return;
	}

	hashtable_create(&a);
	//Test Smallest numbers
	hashtable_put(&a, INT_MIN, INT_MIN);
	if (hashtable_get(&a, INT_MIN)!=INT_MIN){
		printf("==autograde== {'name': 'test6', 'pass': False, 'reason': 'failed to retrieve %d with key %d; got %d. Hashtable does not seem to support min int.'\n", INT_MIN, INT_MIN,hashtable_get(&a, INT_MIN));
		return;
	}

	printf("==autograde== {'name': 'test6', 'pass': True, 'reason': 'amazing success!'\n");
}*/

int main(int argc, char *argv[]){
	//Do all the tests
	test1();
	test2();
	test3();
	//test4();
	//test5();
	//test6();
	return 0;
}
