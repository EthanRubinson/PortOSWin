/* Miniport Functionality Test

  This local loopback test comprehensively tests miniport.c functionality.

USAGE: ./minithread <test_port>

*/

#include "minithread.h"
#include "minimsg.h"
#include "synch.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>


#define BUFFER_SIZE 256


miniport_t test_port;
miniport_t listen_port;
miniport_t send_port;

char text[] = "Hello World!\n";
int textlen=14;

int thread(int* arg) {
  char buffer[BUFFER_SIZE];
  int length = BUFFER_SIZE;
  miniport_t from;
  network_address_t my_address;
  int i;
  
  network_get_my_address(&my_address);

  printf("\n###############################################\n");
  printf("#####     MINIPORT FUNCTIONALITY TEST     #####\n");
  printf("#####      Ethan Rubinson & Kevin Li      #####\n");
  printf("###############################################\n\n");

  printf(" -----------------------------------\n");
  printf("|     Begin Port Creation Tests     |\n");
  printf(" -----------------------------------\n");

  //UNBOUND PORT TESTS
  printf("* Attempting to create unbound ports 0-32767 *\n");

  listen_port = miniport_create_unbound(0); 
  if(listen_port == NULL){
		  printf("[ERROR] Failed to create unbound port %d\n", i);
		  return 1;
  }
  for(i = 1; i <= 32767; i++) {
	  if(miniport_create_unbound(i) == NULL){
		  printf("[ERROR] Failed to create unbound port %d\n", i);
		  return 1;
	  }
  }
  printf("* Finished creating unbound ports! *\n\n");

  printf("* Trying to create out of range unbound port -1 *\n");
  if(miniport_create_unbound(-1) != NULL){ printf("* We successfully bound port '-1' | Test Failed *\n"); return 1;}
  else{ printf("* Test Passed *\n\n"); }

  printf("* Trying to create out of range unbound port 32768 *\n");
  if(miniport_create_unbound(32768) != NULL){ printf("* We successfully bound port '3768' | Test Failed *\n"); return 1;}
  else{ printf("* Test Passed *\n\n"); }

  printf("* Trying to create out of range unbound port 65536 *\n");
  if(miniport_create_unbound(65536) != NULL){ printf("* We successfully bound port '65536' | Test Failed *\n"); return 1;}
  else{ printf("* Test Passed *\n\n"); }

  printf("* Trying to create a port (100) that is already assigned *\n");
  test_port = miniport_create_unbound(100);
  if(test_port == NULL){ printf("* Port is NULL | Test Failed *\n"); return 1; }
  else{ printf("* Test Passed *\n\n"); }

  printf("* Trying to destroy a port (100) and recreate it *\n");
  miniport_destroy(test_port);
  printf("* Port should now be destroyed... Recreating *\n");
  if(miniport_create_unbound(100) == NULL){ printf("* Port is NULL | Test Failed *\n"); return 1;}
  else{ printf("* Test Passed *\n\n\n\n"); }


  // BOUND PORT TESTS
   printf("* Attempting to create bound ports 32768-65535 and bind them to 0-32768 *\n");

  for(i = 0; i <= 32767; i++) {
	  if(i == 500){ 
		  test_port = miniport_create_bound(my_address, i); 
		  if(test_port == NULL){
			  printf("[ERROR] Failed to create bound port %d\n", 500);
			  return 1;
		  }
	  }
	  else{
		  if(miniport_create_bound(my_address, i) == NULL){
			  printf("[ERROR] Failed to create bound port %d\n", i + 32768);
			  return 1;
		  }
	  }
	  
  }
  printf("* Finished creating bound ports! *\n\n");

  printf("* Trying to create another bound port [There are none left] *\n");
  if(miniport_create_bound(my_address, 0) != NULL){ printf("* A bound port was somehow created | Test Failed *\n"); return 1; }
  else{ printf("* Test Passed *\n\n"); }

  printf("* Trying to destroy a bound port (500) and recreate it *\n");
  miniport_destroy(test_port);
  printf("* Port should now be destroyed... Recreating *\n");
  test_port = miniport_create_bound(my_address,0);
  if(test_port == NULL){ printf("* Port is NULL | Test Failed *\n"); return 1;}
  else{ printf("* Test Passed *\n\n"); }

   printf("* Trying to create another bound port [There are none left again] *\n");
  if(miniport_create_bound(my_address, 0) != NULL){ printf("* A bound port was somehow created | Test Failed *\n"); return 1; }
  else{ printf("* Test Passed *\n"); }

  printf(" -----------------------------------\n");
  printf("|       End Port Creation Tests     |\n");
  printf(" -----------------------------------\n\n\n");

  miniport_destroy(listen_port);
  listen_port = miniport_create_unbound(0);
  
  printf(" -----------------------------------\n");
  printf("|   Start Message Sending Tests     |\n");
  printf(" -----------------------------------\n");

  printf("* Try to send message *\n");
  printf("Sent %d bytes successfully (Should be 14)\n", minimsg_send(listen_port, test_port, text, textlen));
  printf("* End Test *\n\n");


  printf("* Try to receive message (Should fail to create reply port but still work) *\n");
  minimsg_receive(listen_port, &from, buffer, &length);
  printf("Received %s", buffer);
  printf("* End Test *\n\n");


  printf("* Try to send message *\n");
  printf("Sent %d bytes successfully (Should be 14)\n", minimsg_send(listen_port, test_port, text, textlen));
  printf("* End Test *\n\n");

  miniport_destroy(test_port);


  printf("* Try to receive message (Should succeed) *\n");
  minimsg_receive(listen_port, &from, buffer, &length);
  printf("Received %s", buffer);
  printf("* End Test *\n");

  printf(" -----------------------------------\n");
  printf("|     End Message Sending Tests     |\n");
  printf(" -----------------------------------\n\n\n");


  printf("###############################################\n");
  printf("#####     MINIPORT FUNCTIONALITY TEST     #####\n");
  printf("#####             ~ COMPLETE ~            #####\n");
  printf("###############################################\n\n");
  return 0;
}

main(int argc, char** argv) {
  short fromport;
  fromport = atoi(argv[1]);
  network_udp_ports(fromport,fromport); 
  textlen = strlen(text) + 1;
  minithread_system_initialize(thread, NULL);
}
