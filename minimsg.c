//Office hour questoins: Should we block if there is no more bounded port addresses, or just return NULL?

/*
 *	Implementation of minimsgs and miniports.
 */
#include "minimsg.h"
#include "miniheader.h"
#include "queue.h"
#include "synch.h"

#define UNBOUNDED_PORT_START 0
#define UNBOUNDED_PORT_LIMIT 32767
#define BOUNDED_PORT_START 32768
#define BOUNDED_PORT_LIMIT 65535

typedef enum {UNBOUNDED,BOUNDED} port_t;

struct miniport { 
	port_t port_type; 
	unsigned short port_number; 

	union { 
		struct { 
			queue_t incoming_data; 
			semaphore_t datagrams_ready;
		} unbound_port; 

		struct { 
			network_address_t remote_address; 
			unsigned short remote_unbound_port; 
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
	interrupt_level_t interrupt_level;
	miniport_t new_port;
	miniport_t temp_port;

	
	if(port_number > UNBOUNDED_PORT_LIMIT || port_number < UNBOUNDED_PORT_START){
		printf("[ERROR] Specified port number [%d] for an unbounded port is out of range.\n", port_number);
		return NULL;
	}

	interrupt_level = set_interrupt_level(DISABLED);
	if(unbounded_ports[port_number] != NULL){
		printf("[INFO] Specified port number [%d] for an unbounded port is already assigned.\n", port_number);
		temp_port = unbounded_ports[port_number];
		set_interrupt_level(interrupt_level);

		return temp_port;
	}
	set_interrupt_level(interrupt_level);
	
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
	
	interrupt_level = set_interrupt_level(DISABLED);
	unbounded_ports[port_number - UNBOUNDED_PORT_START] = new_port;
	//printf("[INFO] Created unbound port # %d\n", new_port->port_number);
	set_interrupt_level(interrupt_level);

	return new_port;
}

//Returns the next available bounded port number on SUCCESS. -1 on FAILURE
int get_next_bounded_port_number(){
	interrupt_level_t interrupt_level = set_interrupt_level(DISABLED);

	int port_iter = current_bounded_port_number;

	do {

		if(bounded_ports[port_iter - BOUNDED_PORT_START] == NULL){
			current_bounded_port_number = port_iter + 1;
			set_interrupt_level(interrupt_level);
			return port_iter;
		}

		else if(port_iter > BOUNDED_PORT_LIMIT){
			port_iter = BOUNDED_PORT_START;
		}

		else{
			port_iter++;
		}

	}while(port_iter != current_bounded_port_number);

	set_interrupt_level(interrupt_level);
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
	interrupt_level_t interrupt_level;
	miniport_t new_port;
	int bounded_port_num = get_next_bounded_port_number();

	
	if(bounded_port_num == -1){
		printf("[ERROR] All bounded ports in use. Can not create a new one.\n");
		return NULL;
	}

	if( remote_unbound_port_number > UNBOUNDED_PORT_LIMIT || remote_unbound_port_number < UNBOUNDED_PORT_START){
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
	
	interrupt_level = set_interrupt_level(DISABLED);
	bounded_ports[bounded_port_num - BOUNDED_PORT_START] = new_port;
	//printf("[INFO] Created bound port # %d\n", new_port->port_number);
	set_interrupt_level(interrupt_level);

	return new_port;

}

/* Destroys a miniport and frees up its resources. If the miniport was in use at
 * the time it was destroyed, subsequent behavior is undefined.
 */
void miniport_destroy(miniport_t miniport)
{
	interrupt_level_t interrupt_level;

	if(miniport == NULL) {
		printf("[ERROR] Cannot destroy a NULL port");
		return;
	}

	interrupt_level = set_interrupt_level(DISABLED);
	
	if(miniport->port_type = UNBOUNDED) {
		queue_free(miniport->port_structure.unbound_port.incoming_data); 
		semaphore_destroy(miniport->port_structure.unbound_port.datagrams_ready);
		unbounded_ports[miniport->port_number - UNBOUNDED_PORT_START] = NULL;
	} else {
		bounded_ports[miniport->port_number - BOUNDED_PORT_START] = NULL;
	}
	
	set_interrupt_level(interrupt_level);

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
	int bytes_sent_successfully;
	mini_header_t packet_header;
	network_address_t local_addr;

	if(msg == NULL){
		printf("[ERROR] Cannot send a NULL message\n");
		return 0;
	}
	if(len == 0){
		printf("[ERROR] Cannot send a message of length 0\n");
		return 0;
	}
	if(local_bound_port == NULL){
		printf("[ERROR] Local bound (sending) port cannot be NULL\n");
		return 0;
	}
	if(local_unbound_port == NULL){
		printf("[ERROR] Local unbounded (listening) port cannot be NULL\n");
		return 0;
	}
	if(len > MINIMSG_MAX_MSG_SIZE){
		printf("[ERROR] Size of message cannot exceed %d bytes\n", MINIMSG_MAX_MSG_SIZE);
		return 0;
	}

	packet_header = (mini_header_t)malloc(sizeof(struct mini_header));
	
	if(packet_header == NULL) {
		printf("[ERROR] Memory allocation for packet header failed\n");
		return 0;
	}

	packet_header->protocol = PROTOCOL_MINIDATAGRAM;

	pack_unsigned_short(packet_header->source_port, local_unbound_port->port_number);
	pack_unsigned_short(packet_header->destination_port, local_bound_port->port_structure.bound_port.remote_unbound_port);
	

	network_get_my_address(local_addr);
	pack_address(packet_header->source_address, local_addr);
	pack_address(packet_header->destination_address, local_bound_port->port_structure.bound_port.remote_address);

	
	//printf("[INFO] Sending message from port %d, to port # %d || reply to port %d\n", local_bound_port->port_number, local_bound_port->port_structure.bound_port.remote_unbound_port,local_unbound_port->port_number);
	bytes_sent_successfully = network_send_pkt(local_bound_port->port_structure.bound_port.remote_address, sizeof(struct mini_header), (char *)packet_header, len, msg) - sizeof(struct mini_header);
	bytes_sent_successfully = max(bytes_sent_successfully, 0);
	
	free(packet_header);

	return len;
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
	network_interrupt_arg_t *data_received;
	network_address_t response_address;
	unsigned int response_port;
	interrupt_level_t interrupt_level;
	//printf("[INFO] Entered receive method for port # %d. Blocking until data is available\n", local_unbound_port->port_number);

	semaphore_P(local_unbound_port->port_structure.unbound_port.datagrams_ready);
	
	
	interrupt_level = set_interrupt_level(DISABLED);
	queue_dequeue(local_unbound_port->port_structure.unbound_port.incoming_data, (void **) &data_received);
	set_interrupt_level(interrupt_level);

	unpack_address(data_received->buffer + 1, response_address);
	response_port = unpack_unsigned_short(data_received->buffer + 9);

	//printf("[INFO] Recieved message on port # %d. Reply back to port # %d\n",local_unbound_port->port_number,response_port);

	*new_local_bound_port = miniport_create_bound(response_address, response_port);

	*len = data_received->size - sizeof(struct mini_header);
	//msg = data_received->buffer + sizeof(struct mini_header);
	
	memcpy(msg, data_received->buffer + sizeof(struct mini_header), *len);
	//printf("[INFO] Message recieved |BEGIN DATA|%s|END DATA|\n", msg);
	free(data_received);
	return *len;

}

void minimsg_process(unsigned short unbound_port_num, network_interrupt_arg_t *data){
	miniport_t target_port = unbounded_ports[unbound_port_num];
	
	if(target_port == NULL){
		printf("[ERROR] Target port %d is null", unbound_port_num);
	}
	else{
		queue_append(target_port->port_structure.unbound_port.incoming_data, data);
		semaphore_V(target_port->port_structure.unbound_port.datagrams_ready);
	}


}