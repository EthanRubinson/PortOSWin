/*
 *	Implementation of minimsgs and miniports.
 */
#include "minimsg.h"
#include "miniheader.h"
#include "miniroute.h"
#include "queue.h"
#include "synch.h"

/*
 * Valid port number constants.
 */
#define UNBOUNDED_PORT_START 0
#define UNBOUNDED_PORT_LIMIT 32767
#define BOUNDED_PORT_START 32768
#define BOUNDED_PORT_LIMIT 65535

/*enum designated the type of port a miniport is*/
typedef enum {UNBOUNDED,BOUNDED} port_t;

/*
 * Miniport structure. 
 */
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

// Port arrays
miniport_t unbounded_ports[UNBOUNDED_PORT_LIMIT - UNBOUNDED_PORT_START + 1];
miniport_t bounded_ports[BOUNDED_PORT_LIMIT - BOUNDED_PORT_START + 1];
int current_bounded_port_number;

/* 
 * Performs any required initialization of the minimsg layer.
 */
void minimsg_initialize()
{
	// Initialize port arrays
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
	
	// Validate port number range
	if(port_number > UNBOUNDED_PORT_LIMIT || port_number < UNBOUNDED_PORT_START){
		printf("[ERROR] Specified port number [%d] for an unbounded port is out of range.\n", port_number);
		return NULL;
	}

	// Synchronized: Check if port is aleady assigned
	interrupt_level = set_interrupt_level(DISABLED);
	if(unbounded_ports[port_number - UNBOUNDED_PORT_START] != NULL){
		printf("[INFO] Specified port number [%d] for an unbounded port is already assigned.\n", port_number);
		temp_port = unbounded_ports[port_number - UNBOUNDED_PORT_START];
		set_interrupt_level(interrupt_level);

		return temp_port;
	}
	set_interrupt_level(interrupt_level);
	
	// Create new port
	new_port = (miniport_t) malloc(sizeof(struct miniport));
	if (new_port == NULL) {
		printf("[ERROR] Memory allocation for unbounded port [%d] failed.\n", port_number);
		return NULL;
	}

	// Initialize port
	new_port->port_structure.unbound_port.incoming_data = queue_new();
	new_port->port_structure.unbound_port.datagrams_ready = semaphore_create();
	semaphore_initialize(new_port->port_structure.unbound_port.datagrams_ready,0);
	new_port->port_type = UNBOUNDED;
	new_port->port_number = port_number;
	
	// Synchronized: record new port in array
	interrupt_level = set_interrupt_level(DISABLED);
	unbounded_ports[port_number - UNBOUNDED_PORT_START] = new_port;
	set_interrupt_level(interrupt_level);

	return new_port;
}

/*
 * Returns the next available bounded port number on SUCCESS. -1 on FAILURE
 */
int get_next_bounded_port_number(){
	interrupt_level_t interrupt_level = set_interrupt_level(DISABLED);
	int port_iter = current_bounded_port_number;

	do {
		if(port_iter > BOUNDED_PORT_LIMIT){
			port_iter = BOUNDED_PORT_START;
		} else if (bounded_ports[port_iter - BOUNDED_PORT_START] == NULL){
			current_bounded_port_number = port_iter + 1;
			set_interrupt_level(interrupt_level);
			return port_iter;
		} else {
			port_iter++;
		}

	} while(port_iter != current_bounded_port_number);

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
	
	// Sanity checks
	if(bounded_port_num == -1){
		printf("[ERROR] All bounded ports in use. Can not create a new one.\n");
		return NULL;
	}
	if(remote_unbound_port_number > UNBOUNDED_PORT_LIMIT || remote_unbound_port_number < UNBOUNDED_PORT_START){
		printf("[ERROR] Specified remote port number [%d] is out of range.\n", remote_unbound_port_number);
		return NULL;
	}

	// Create port
	new_port = (miniport_t) malloc(sizeof(struct miniport));	
	if (new_port == NULL) {
		printf("[ERROR] Memory allocation for bounded port [%d] failed.\n", bounded_port_num);
		return NULL;
	}

	// Initialize port
	network_address_copy(addr, new_port->port_structure.bound_port.remote_address);
	new_port->port_structure.bound_port.remote_unbound_port = remote_unbound_port_number;
	new_port->port_type = BOUNDED;
	new_port->port_number = bounded_port_num;
	
	// Synchronized: record new port in array
	interrupt_level = set_interrupt_level(DISABLED);
	bounded_ports[bounded_port_num - BOUNDED_PORT_START] = new_port;
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
		printf("[ERROR] Cannot destroy a NULL port \n");
		return;
	}

	interrupt_level = set_interrupt_level(DISABLED);

	if(miniport->port_type == UNBOUNDED) {
		queue_free(miniport->port_structure.unbound_port.incoming_data); 
		semaphore_destroy(miniport->port_structure.unbound_port.datagrams_ready);
		unbounded_ports[miniport->port_number - UNBOUNDED_PORT_START] = NULL;
	} else { // Bounded port
		bounded_ports[miniport->port_number - BOUNDED_PORT_START] = NULL;
	}	

	free(miniport);
	set_interrupt_level(interrupt_level);
	//printf("[DEBUG] port destroyed \n");
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
	int interrupt_level;

	// Validate message and port to send through
	if(msg == NULL){
		printf("[ERROR] Cannot send a NULL message\n");
		return 0;
	}
	if(len == 0){
		printf("[ERROR] Cannot send a message of length 0\n");
		return 0;
	}
	if(len > MINIMSG_MAX_MSG_SIZE){
		printf("[ERROR] Size of message cannot exceed %d bytes\n", MINIMSG_MAX_MSG_SIZE);
		return 0;
	}

	//interrupt_level = set_interrupt_level(DISABLED);
	if(local_bound_port == NULL || bounded_ports[local_bound_port->port_number - BOUNDED_PORT_START] == NULL){
		printf("[ERROR] Local bound (sending) port is not initialized \n");
		return 0;
	}
	if(local_unbound_port == NULL || unbounded_ports[local_unbound_port->port_number - UNBOUNDED_PORT_START] == NULL){
		printf("[ERROR] Local unbounded (listening) port is not initialized \n");
		return 0;
	}
	//set_interrupt_level(interrupt_level);
	
	
	// Create packet header
	packet_header = (mini_header_t)malloc(sizeof(struct mini_header));	
	if(packet_header == NULL) {
		printf("[ERROR] Memory allocation for packet header failed\n");
		return 0;
	}

	// Initialize packet header
	packet_header->protocol = PROTOCOL_MINIDATAGRAM;
	pack_unsigned_short(packet_header->source_port, local_unbound_port->port_number);
	pack_unsigned_short(packet_header->destination_port, local_bound_port->port_structure.bound_port.remote_unbound_port);
	network_get_my_address(local_addr);
	pack_address(packet_header->source_address, local_addr);
	pack_address(packet_header->destination_address, local_bound_port->port_structure.bound_port.remote_address);

	// Send packet
	bytes_sent_successfully = miniroute_send_pkt(local_bound_port->port_structure.bound_port.remote_address, sizeof(struct mini_header), (char *)packet_header, len, msg) - sizeof(struct mini_header);
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
	
	if (local_unbound_port == NULL || unbounded_ports[local_unbound_port->port_number - UNBOUNDED_PORT_START] == NULL) {
		printf("[ERROR] Cannot read from an uninitialized port");
		return 0;
	}
	//printf("[DEBUG] Waiting for packet to arrive ... \n");
	// Wait for packet arrival
	semaphore_P(local_unbound_port->port_structure.unbound_port.datagrams_ready);
	interrupt_level = set_interrupt_level(DISABLED);
	// Synchronized: Retrieve packet from queue
	queue_dequeue(local_unbound_port->port_structure.unbound_port.incoming_data, (void **) &data_received);
	set_interrupt_level(interrupt_level);

	// Get address and port number
	unpack_address(data_received->buffer + 1, response_address);
	response_port = unpack_unsigned_short(data_received->buffer + 9);

	// port to send response through
	*new_local_bound_port = miniport_create_bound(response_address, response_port);

	// Extract message
	*len = min(max(data_received->size - sizeof(struct mini_header),0),*len);
	memcpy(msg, data_received->buffer + sizeof(struct mini_header), *len);

	free(data_received);
	return *len;

}

void minimsg_process(unsigned short unbound_port_num, network_interrupt_arg_t *data){
	miniport_t target_port = unbounded_ports[unbound_port_num - UNBOUNDED_PORT_START];
	mini_header_t header = (mini_header_t) data->buffer;
	network_address_t destination_address;
	network_address_t my_address;
	network_get_my_address(my_address);
	unpack_address(header->destination_address, destination_address);

	// Validate packet
	if(target_port == NULL){
		printf("[ERROR] Target port %d is null \n", unbound_port_num);
		free(data);
	} else if (data->size < sizeof(struct mini_header)) {
		printf("[ERROR] Packet size is smaller than header \n");
		free(data);
	} else if (header->protocol != PROTOCOL_MINIDATAGRAM) {
		printf("[ERROR] Invalid packet protocol \n");
		free(data);
	} else if (!network_address_same(destination_address, my_address)) {
		printf("[ERROR] Received packet not intended for us \n");
		free(data);
	} else {
		// Add packet to port queue
		queue_append(target_port->port_structure.unbound_port.incoming_data, data);
		// Signal packet arrival
		semaphore_V(target_port->port_structure.unbound_port.datagrams_ready);
	}
}
