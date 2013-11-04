/* network test program 6

   Allows two processes to take turns sending chat messages to each other

   USAGE: ./minithread <souceport> <destport> [<hostname>]

   sourceport = udp port to listen on.
   destport   = udp port to send to.

   if no hostname is supplied, will wait for a packet before sending the first
   packet; if a hostname is given, will send and then receive. 
   the receive-first copy must be started first!
*/

#include "defs.h"
#include "minithread.h"
#include "minimsg.h"
#include "synch.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define BUFFER_SIZE 256
#define MAX_COUNT 100


char* hostname; // the remote host to connect to

/*
 * Gets a string from the command line.
 */
char * getline(void) {
    char *line = malloc(100);
	char *linep = line;
	char *linen;
    size_t lenmax = 100, len = lenmax;
    int c;

    if(line == NULL)
        return NULL;

    for(;;) {
        c = fgetc(stdin);
        if(c == EOF)
            break;

        if(--len == 0) {
            len = lenmax;
            linen = realloc(linep, lenmax *= 2);

            if(linen == NULL) {
                free(linep);
                return NULL;
            }
            line = linen + (line - linep);
            linep = linen;
        }

        if((*line++ = c) == '\n')
            break;
    }
    *line = '\0';
    return linep;
}

/*
 * First to receive a message.
 */
int receive_first(int* arg) {
  char buffer[BUFFER_SIZE];
  int length;
  int i;
  miniport_t port;
  miniport_t from;

  port = miniport_create_unbound(1);

  for (i=0; i<MAX_COUNT; i++) {
    length = BUFFER_SIZE;
	printf("Receiving message... \n");
    minimsg_receive(port, &from, buffer, &length);
    printf("%s", buffer);
    printf("Enter message to send: \n");
    sprintf(buffer, getline());
    length = strlen(buffer) + 1;
    minimsg_send(port, from, buffer, length);
    miniport_destroy(from);
  }

  return 0;
}

/*
 * First to send a message.
 */
int transmit_first(int* arg) {
  char buffer[BUFFER_SIZE];
  int length = BUFFER_SIZE;
  int i;
  network_address_t addr;
  miniport_t port;
  miniport_t dest;
  miniport_t from;

  AbortOnCondition(network_translate_hostname(hostname, addr) < 0,
		   "Could not resolve hostname, exiting.");

  port = miniport_create_unbound(0);
  dest = miniport_create_bound(addr, 1);

  for (i=0; i<MAX_COUNT; i++) {
    printf("Enter message to send: \n");
    sprintf(buffer, getline());
    length = strlen(buffer) + 1;
    minimsg_send(port, dest, buffer, length);
    length = BUFFER_SIZE;
	printf("Waiting for message: \n");
    minimsg_receive(port, &from, buffer, &length);
    printf("%s", buffer);
    miniport_destroy(from);
  }

  return 0;
}

#ifdef WINCE
void main(void){
  READCOMMANDLINE
#else /* WINNT code */
main(int argc, char** argv) {
#endif
  short fromport, toport;
  fromport = atoi(argv[1]);
  toport = atoi(argv[2]);
  network_udp_ports(fromport,toport); 

  if (argc > 3) {
    hostname = argv[3];
		minithread_system_initialize(transmit_first, NULL);
  }
  else {
		minithread_system_initialize(receive_first, NULL);
  }
}
