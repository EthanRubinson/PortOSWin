//Office hour questoins: Should we block if there is no more bounded port addresses, or just return NULL?

/*
 *	Implementation of minimsgs and miniports.
 */
#include "minimsg.h"
#include "queue.h"
#include "synch.h"

#define UNBOUNDED_PORT_START 0
#define UNBOUNDED_PORT_LIMIT 32767
#define BOUNDED_PORT_START 32768
#define BOUNDED_PORT_LIMIT 65535

typedef enum {UNBOUNDED,BOUNDED} port_t;

struct miniport { 
	port_t port_type; 
	int port_number; 

	union { 
		struct { 
			queue_t incoming_data; 
			semaphore_t datagrams_ready;
		} unbound_port; 

		struct { 
			network_address_t remote_address; 
			int remote_unbound_port; 
		} bound_port;

	} port_structure;
};


miniport_t unbounded_ports[UNBOUNDED_PORT_LIMIT - UNBOUNDED_PORT_START + 1];
miniport_t bounded_ports[BOUNDED_PORT_LIMIT - BOUNDED_PORT_START + 1];
int current_bounded_port_number;

/* performs any required initialization of the minimsg layer.
 */
void minimsg_initialize()
{
	current_bounded_port_number = BOUNDED_PORT_START;
	memset(unbounded_ports, 0, sizeof(unbounded_ports));
	memset(bounded_ports, 0, sizeof(bounded_ports));
}

/* Creates an unbound port for listening. Multiple requests to create the same
 * unbound port should return the same miniport reference. It is the responsibility
 * of the programmer to make sure he does not destroy unbound miniports while they
 * are still in use by other threads -- this would result in undefined behavior.
 * Unbound ports must range from 0 to 32767. If the programmer specifies a port number
 * outside this range, it is considered an error.
 */
miniport_t miniport_create_unbound(int port_number)
{
	miniport_t new_port;

	if(port_number > UNBOUNDED_PORT_LIMIT || port_number < UNBOUNDED_PORT_START){
		printf("[ERROR] Specified port number [%d] for an unbounded port is out of range.\n", port_number);
		return NULL;
	}

	if(unbounded_ports[port_number] == NULL){
		printf("[INFO] Specified port number [%d] for an unbounded port is already assigned.\n", port_number);
		return unbounded_ports[port_number];
	}
	
	new_port = (miniport_t) malloc(sizeof(struct miniport));
	
	/*If memory allocation fails, return NULL*/
	if (new_port == NULL) {
		printf("[ERROR] Memory allocation for unbounded port [%d] failed.\n", port_number);
		return NULL;
	}

	new_port->port_structure.unbound_port.incoming_data = queue_new();
	new_port->port_structure.unbound_port.datagrams_ready = semaphore_create();
	semaphore_initialize(new_port->port_structure.unbound_port.datagrams_ready,0);

	new_port->port_type = UNBOUNDED;
	new_port->port_number = port_number;
	

	unbounded_ports[port_number - UNBOUNDED_PORT_START] = new_port;

	return new_port;
}

//Returns the next available bounded port number on SUCCESS. -1 on FAILURE
int get_next_bounded_port_number(){
	int port_iter = current_bounded_port_number;

	do {

		if(bounded_ports[port_iter - BOUNDED_PORT_START] == NULL){
			current_bounded_port_number = port_iter + 1;
			return port_iter;
		}

		else if(port_iter > BOUNDED_PORT_LIMIT){
			port_iter = BOUNDED_PORT_START;
		}

		else{
			port_iter++;
		}

	}while(port_iter != current_bounded_port_number);

	return -1;
}

/* Creates a bound port for use in sending packets. The two parameters, addr and
 * remote_unbound_port_number together specify the remote's listening endpoint.
 * This function should assign bound port numbers incrementally between the range
 * 32768 to 65535. Port numbers should not be reused even if they have been destroyed,
 * unless an overflow occurs (ie. going over the 65535 limit) in which case you should
 * wrap around to 32768 again, incrementally assigning port numbers that are not
 * currently in use.
 */
miniport_t miniport_create_bound(network_address_t addr, int remote_unbound_port_number)
{
	miniport_t new_port;
	int bounded_port_num = get_next_bounded_port_number();

	if(bounded_port_num == -1){
		printf("[ERROR] All bounded ports in use. Can not create a new one.\n");
		return NULL;
	}

	if( remote_unbound_port_number > UNBOUNDED_PORT_LIMIT || remote_unbound_port_number < UNBOUNDED_PORT_LIMIT){
		printf("[ERROR] Specified remote port number [%d] is out of range.\n", remote_unbound_port_number);
		return NULL;
	}

	new_port = (miniport_t) malloc(sizeof(struct miniport));
	
	/*If memory allocation fails, return NULL*/
	if (new_port == NULL) {
		printf("[ERROR] Memory allocation for bounded port [%d] failed.\n", bounded_port_num);
		return NULL;
	}

	
	network_address_copy(addr, new_port->port_structure.bound_port.remote_address);
	new_port->port_structure.bound_port.remote_unbound_port = remote_unbound_port_number;

	new_port->port_type = BOUNDED;
	new_port->port_number = bounded_port_num;
	
	bounded_ports[bounded_port_num - BOUNDED_PORT_START] = new_port;

	return new_port;

}

/* Destroys a miniport and frees up its resources. If the miniport was in use at
 * the time it was destroyed, subsequent behavior is undefined.
 */
void miniport_destroy(miniport_t miniport)
{
	if(miniport == NULL){
		printf("[ERROR] Cannot destroy a NULL port");
		return;
	}


	if(miniport->port_type = UNBOUNDED){
		queue_free(miniport->port_structure.unbound_port.incoming_data); 
		semaphore_destroy(miniport->port_structure.unbound_port.datagrams_ready);
		unbounded_ports[miniport->port_number - UNBOUNDED_PORT_START] = NULL;
	}
	else{
		bounded_ports[miniport->port_number - BOUNDED_PORT_START] = NULL;
	}

	free(miniport);
}

/* Sends a message through a locally bound port (the bound port already has an associated
 * receiver address so it is sufficient to just supply the bound port number). In order
 * for the remote system to correctly create a bound port for replies back to the sending
 * system, it needs to know the sender's listening port (specified by local_unbound_port).
 * The msg parameter is a pointer to a data payload that the user wishes to send and does not
 * include a network header; your implementation of minimsg_send must construct the header
 * before calling network_send_pkt(). The return value of this function is the number of
 * data payload bytes sent not inclusive of the header.
 */
int minimsg_send(miniport_t local_unbound_port, miniport_t local_bound_port, minimsg_t msg, int len)
{

}

/* Receives a message through a locally unbound port. Threads that call this function are
 * blocked until a message arrives. Upon arrival of each message, the function must create
 * a new bound port that targets the sender's address and listening port, so that use of
 * this created bound port results in replying directly back to the sender. It is the
 * responsibility of this function to strip off and parse the header before returning the
 * data payload and data length via the respective msg and len parameter. The return value
 * of this function is the number of data payload bytes received not inclusive of the header.
 */
int minimsg_receive(miniport_t local_unbound_port, miniport_t* new_local_bound_port, minimsg_t msg, int *len)
{

}
