/* network test program 1

   local loopback test: sends and then receives one message on the same machine.

USAGE: ./minithread <port>

*/

#include "minithread.h"
#include "minimsg.h"
#include "synch.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>


#define BUFFER_SIZE 256


miniport_t listen_port;
miniport_t send_port;

char text[] = "Hello, world!\n";
int textlen=14;

int thread(int* arg) {
  char buffer[BUFFER_SIZE];
  int length = BUFFER_SIZE;
  int i;
  miniport_t from;
  network_address_t my_address;
  
  network_get_my_address(&my_address);

  for(i = 0; i <= 32768; i++) {
	  listen_port = miniport_create_unbound(i);
  }
  printf("Setting listening port back to valid unbound port 32767 \n");
  listen_port = miniport_create_unbound(32767);
  
  for(i = 0; i <= 32767; i++) {
	  send_port = miniport_create_bound(my_address, i);
  }
  printf("^Last port should have been used already \n");
  printf("Sending message.. \n");
  
  minimsg_send(listen_port, send_port, text, textlen);

  printf("Receiving message: should fail to create a new bound port \n");
  minimsg_receive(listen_port, &from, buffer, &length);
  printf("%s", buffer);

  printf("Destroying port 32767 \n");
  miniport_destroy(listen_port);

  listen_port = NULL;

  printf("Recreating port 32767 \n");
  listen_port = miniport_create_unbound(32767);

  printf("Trying to send to port 32767 \n");
  printf("%d\n", minimsg_send(listen_port, send_port, text, textlen));
  //printf("Trying to receive on port 32767, this should throw error \n");
  minimsg_receive(listen_port, &from, buffer, &length);
  
  return 0;
}

main(int argc, char** argv) {
  short fromport;
  fromport = atoi(argv[1]);
  network_udp_ports(fromport,fromport); 
  textlen = strlen(text) + 1;
  minithread_system_initialize(thread, NULL);
}
