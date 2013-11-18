/*
 *	Implementation of minisockets.
 */
#include "minisocket.h"
#include "miniheader.h"
#include "synch.h"
#include "minithread.h"
#include "queue.h"
#include "alarm.h"


/*enum designated the type of socket a minisocket is*/
typedef enum {SERVER,CLIENT} socket_t;
typedef enum {READY,NOT_READY} status_t;

struct minisocket
{
	semaphore_t data_lock;
	queue_t data_queue;

	int num_threads_blocked;
	int should_terminate;
	status_t port_status;

	unsigned short port_number;
	int ack_num;
	int seq_num;

	unsigned short remote_port_number;
	network_address_t remote_address;

	socket_t socket_type;
};

/*
 * Valid socket number constants.
 */
#define SERVER_SOCKET_START 0
#define SERVER_SOCKET_LIMIT 32767
#define CLIENT_SOCKET_START 32768
#define CLIENT_SOCKET_LIMIT 65535


// Socket arrays
minisocket_t server_sockets[SERVER_SOCKET_LIMIT - SERVER_SOCKET_START + 1];
minisocket_t client_sockets[CLIENT_SOCKET_LIMIT - CLIENT_SOCKET_START + 1];
int current_client_socket_number;

int get_next_client_socket_number(){
	interrupt_level_t interrupt_level = set_interrupt_level(DISABLED);
	int port_iter = current_client_socket_number;

	do {
		if(client_sockets[port_iter - CLIENT_SOCKET_START] == NULL){
			current_client_socket_number = port_iter + 1;
			set_interrupt_level(interrupt_level);
			return port_iter;
		} else if (port_iter > CLIENT_SOCKET_LIMIT){
			port_iter = CLIENT_SOCKET_START;
		} else {
			port_iter++;
		}

	} while(port_iter != current_client_socket_number);

	set_interrupt_level(interrupt_level);
	return -1;
}


//Broadcasts a terminate signal to all sockets that are blocked on the data_lock
void broadcast_socket_close_signal(minisocket_t socket_to_close){
	int i;

	if(socket_to_close == NULL){
		printf("[INFO] Cannot close sockt, socket is NULL");
		return;
	}

	//Ensure that the socket is not already closed
	if(socket_to_close->should_terminate == 1){
		printf("[INFO] Socket at port %d is already marked for closure", socket_to_close->port_number);
		return;
	}

	//Signal all remaining (blocked) threads to terminate
	socket_to_close->should_terminate = 1;
	for(i = 0; i< socket_to_close->num_threads_blocked; i++) {
		semaphore_V(socket_to_close->data_lock);
	}
}

void force_receive_to_exit(void* socket){
	semaphore_V(((minisocket_t)socket)->data_lock);
}

/* Initializes the minisocket layer. */
void minisocket_initialize()
{
	current_client_socket_number = CLIENT_SOCKET_START;
	// Initialize socket arrays
	memset(server_sockets, 0, sizeof(server_sockets));
	memset(client_sockets, 0, sizeof(client_sockets));
}

/* 
 * Listen for a connection from somebody else. When communication link is
 * created return a minisocket_t through which the communication can be made
 * from now on.
 *
 * The argument "port" is the port number on the local machine to which the
 * client will connect.
 *
 * Return value: the minisocket_t created, otherwise NULL with the errorcode
 * stored in the "error" variable.
 */
minisocket_t minisocket_server_create(int port, minisocket_error *error)
{
	interrupt_level_t interrupt_level = set_interrupt_level(DISABLED);
	minisocket_t new_socket;
	int should_terminate;
	int connection_established = 0;
	network_interrupt_arg_t *data_received;

	*error = SOCKET_NOERROR;

	if (port < SERVER_SOCKET_START || port > SERVER_SOCKET_LIMIT) {
		printf("[ERROR] Failed to create server socket, port [%d] out of range.\n", port);
		*error = SOCKET_INVALIDPARAMS;
		set_interrupt_level(interrupt_level);
		return NULL;
	}

	if (server_sockets[port - SERVER_SOCKET_START] != NULL) {
		printf("[ERROR] Failed to create server socket, port [%d] in use.\n", port);
		*error = SOCKET_PORTINUSE;
		set_interrupt_level(interrupt_level);
		return NULL;
	}

	new_socket = (minisocket_t) malloc(sizeof(struct minisocket));
	if (new_socket == NULL) {
		printf("[ERROR] Memory allocation for server socket [%d] failed.\n", port);
		*error = SOCKET_OUTOFMEMORY;
		set_interrupt_level(interrupt_level);
		return NULL;
	}

	new_socket->data_lock = semaphore_create();
	if (new_socket->data_lock == NULL) {
		printf("[ERROR] Server socket initialization failed (Out of memory).\n", port);
		*error = SOCKET_OUTOFMEMORY;
		free(new_socket);
		set_interrupt_level(interrupt_level);
		return NULL;
	}

	new_socket->data_queue = queue_new();
	if (new_socket->data_queue == NULL) {
		printf("[ERROR] Server socket initialization failed (Out of memory).\n", port);
		*error = SOCKET_OUTOFMEMORY;
		semaphore_destroy(new_socket->data_lock);
		free(new_socket);
		set_interrupt_level(interrupt_level);
		return NULL;
	}

	semaphore_initialize(new_socket->data_lock, 0);
	new_socket->num_threads_blocked = 0;
	new_socket->should_terminate = 0;
	new_socket->port_number = port;
	new_socket->socket_type = SERVER;
	new_socket->ack_num = 0;
	new_socket->seq_num = 1;
	new_socket->port_status = NOT_READY;
	server_sockets[port - SERVER_SOCKET_START] = new_socket;

	new_socket->num_threads_blocked++;
	set_interrupt_level(interrupt_level);

	//Get handshake
	//While recieved packet is not a SYN, keep waiting (as long as we should not terminate)
	while (connection_established == 0) {
		semaphore_P(new_socket->data_lock);

		//Process received packet		
		if(new_socket->should_terminate == 1){
			printf("[INFO] Socket was closed before the connection could be established");
			return NULL;
		}
		
		interrupt_level = set_interrupt_level(DISABLED);
		queue_dequeue(new_socket->data_queue, (void **) &data_received);
		set_interrupt_level(interrupt_level);


		//We got the SYN
		if(*(data_received->buffer + 21) == MSG_SYN){
			//Unpack the remote address/port/seq/ack
			if(unpack_unsigned_int(data_received->buffer + 26) == new_socket->ack_num + 1 && unpack_unsigned_int(data_received->buffer + 30) == 0) /*seq #*/{
				unpack_address(data_received->buffer + 1,new_socket->remote_address);
				new_socket->remote_port_number = unpack_unsigned_short(data_received->buffer + 9);
				new_socket->ack_num++;

				if( minisocket_send_packet_with_retransmission(new_socket, "This is a SYN_ACK\n", 18, MSG_SYNACK) > 0 ){
					new_socket->seq_num++;
					connection_established = 1;
					new_socket->port_status = READY;
					printf("[INFO] Connection established with client\n");
				}
				else{ //Reset the socket's ack
					new_socket->ack_num = 0;
				}
			}
		}	
		
	}

	new_socket->num_threads_blocked--;
	return new_socket;
	
}


/*
 * Initiate the communication with a remote site. When communication is
 * established create a minisocket through which the communication can be made
 * from now on.
 *
 * The first argument is the network address of the remote machine. 
 *
 * The argument "port" is the port number on the remote machine to which the
 * connection is made. The port number of the local machine is one of the free
 * port numbers.
 *
 * Return value: the minisocket_t created, otherwise NULL with the errorcode
 * stored in the "error" variable.
 */
minisocket_t minisocket_client_create(network_address_t addr, int port, minisocket_error *error)
{
	interrupt_level_t interrupt_level = set_interrupt_level(DISABLED);
	minisocket_t new_socket;
	int should_terminate;
	int connection_established = 0;
	network_interrupt_arg_t *data_received;
	int socket_port_num = get_next_client_socket_number();

	*error = SOCKET_NOERROR;

	if(socket_port_num == -1){
		printf("[ERROR] All client socket ports in use. Can not create a new one.\n");
		*error = SOCKET_NOMOREPORTS;
		set_interrupt_level(interrupt_level);
		return NULL;
	}

	if (client_sockets[port - CLIENT_SOCKET_START] != NULL) {
		printf("[ERROR] Failed to create client socket, port [%d] in use.\n", port);
		*error = SOCKET_PORTINUSE;
		set_interrupt_level(interrupt_level);
		return NULL;
	}

	new_socket = (minisocket_t) malloc(sizeof(struct minisocket));
	if (new_socket == NULL) {
		printf("[ERROR] Memory allocation for client socket [%d] failed.\n", port);
		*error = SOCKET_OUTOFMEMORY;
		set_interrupt_level(interrupt_level);
		return NULL;
	}

	new_socket->data_lock = semaphore_create();
	if (new_socket->data_lock == NULL) {
		printf("[ERROR] Client socket initialization failed (Out of memory).\n", port);
		*error = SOCKET_OUTOFMEMORY;
		free(new_socket);
		set_interrupt_level(interrupt_level);
		return NULL;
	}

	new_socket->data_queue = queue_new();
	if (new_socket->data_queue == NULL) {
		printf("[ERROR] Client socket initialization failed (Out of memory).\n", port);
		*error = SOCKET_OUTOFMEMORY;
		semaphore_destroy(new_socket->data_lock);
		free(new_socket);
		set_interrupt_level(interrupt_level);
		return NULL;
	}

	semaphore_initialize(new_socket->data_lock, 0);
	new_socket->num_threads_blocked = 0;
	new_socket->should_terminate = 0;
	new_socket->socket_type = CLIENT;

	new_socket->ack_num = 0;
	new_socket->seq_num = 1;

	network_address_copy(addr,new_socket->remote_address);
	new_socket->remote_port_number = port;
	new_socket->port_status = NOT_READY;
	client_sockets[port - CLIENT_SOCKET_START] = new_socket;

	new_socket->num_threads_blocked++;
	set_interrupt_level(interrupt_level);

	//Create handshake

	if( minisocket_send_packet_with_retransmission(new_socket, "This is a SYN\n", 14, MSG_SYN) > 0){
		new_socket->ack_num++;
		minisocket_send_packet_without_retransmission(new_socket, "This is an ACK\n", 15, MSG_ACK);

		new_socket->port_status = READY;
	}
	else{
		printf("[ERROR] A timeout occured, no response from server on port %d.\n", new_socket->port_number);
		return NULL;
	}


	return new_socket;
}


/* 
 * Send a message to the other end of the socket.
 *
 * The send call should block until the remote host has ACKnowledged receipt of
 * the message.  This does not necessarily imply that the application has called
 * 'minisocket_receive', only that the packet is buffered pending a future
 * receive.
 *
 * It is expected that the order of calls to 'minisocket_send' implies the order
 * in which the concatenated messages will be received.
 *
 * 'minisocket_send' should block until the whole message is reliably
 * transmitted or an error/timeout occurs
 *
 * Arguments: the socket on which the communication is made (socket), the
 *            message to be transmitted (msg) and its length (len).
 * Return value: returns the number of successfully transmitted bytes. Sets the
 *               error code and returns -1 if an error is encountered.
 */
int minisocket_send(minisocket_t socket, minimsg_t msg, int len, minisocket_error *error)
{
	// partition msg into MINIMSG_MAX_MSG_SIZE chunks
	// call send with retransmission on each sequential chunk
	// for each send that succeeds, increment count of number of bytes sent
	// if some send fails, return -1 and set error message

	char* marker = msg;
	int buffer = len;
	int bytes_sent_successfully = 0;
	int segment_size;

	while(buffer > 0) {
		if(buffer > MINIMSG_MAX_MSG_SIZE) {
			segment_size = minisocket_send_packet_with_retransmission(socket, marker, MINIMSG_MAX_MSG_SIZE, (char)0);
			marker += MINIMSG_MAX_MSG_SIZE;
			buffer -= MINIMSG_MAX_MSG_SIZE;
		} else {
			segment_size = minisocket_send_packet_with_retransmission(socket, marker, buffer, (char)0);
		}
		if(segment_size <= 0){
			return -1;
		} else {
			bytes_sent_successfully += segment_size;
		}
	}
	return bytes_sent_successfully;
}

int minisocket_send_packet_without_retransmission(minisocket_t socket, minimsg_t msg, int len, char type){
	int bytes_sent_successfully;
	mini_header_reliable_t packet_header;
	network_address_t local_addr;
	interrupt_level_t interrupt_level;
	network_interrupt_arg_t *data_received;
	int num_retries;

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
	if(socket == NULL || socket->port_status == NOT_READY){
		printf("[ERROR] Socket is not initialized\n");
		return 0;
	}
	
	interrupt_level = set_interrupt_level(DISABLED);
	if( (socket->socket_type == SERVER && server_sockets[socket->port_number - SERVER_SOCKET_START] != NULL) || (socket->socket_type == CLIENT && client_sockets[socket->port_number - CLIENT_SOCKET_START] != NULL)){
		printf("[ERROR] Socket is not initialized\n");
		set_interrupt_level(interrupt_level);
		return 0;
	}
	set_interrupt_level(interrupt_level);

	// Create packet header
	packet_header = (mini_header_reliable_t)malloc(sizeof(struct mini_header_reliable));	
	if(packet_header == NULL) {
		printf("[ERROR] Memory allocation for packet header failed\n");
		return 0;
	}

	// Initialize packet header
	packet_header->protocol = PROTOCOL_MINISTREAM;
	
	network_get_my_address(local_addr);
	pack_address(packet_header->source_address, local_addr);

	pack_unsigned_short(packet_header->source_port, socket->port_number);

	pack_address(packet_header->destination_address, socket->remote_address);

	pack_unsigned_short(packet_header->destination_port, socket->remote_port_number);

	packet_header->message_type = type;

	pack_unsigned_short(packet_header->seq_number, socket->seq_num);
	pack_unsigned_short(packet_header->ack_number, socket->ack_num);

	// Send packet
	bytes_sent_successfully = network_send_pkt(socket->remote_address, sizeof(struct mini_header_reliable), (char *)packet_header, len, msg) - sizeof(struct mini_header_reliable);
	bytes_sent_successfully = max(bytes_sent_successfully, 0);	


	free(packet_header);
	return bytes_sent_successfully;
}

int minisocket_send_packet_with_retransmission(minisocket_t socket, minimsg_t msg, int len, char type){
	int bytes_sent_successfully;
	mini_header_reliable_t packet_header;
	network_address_t local_addr;
	interrupt_level_t interrupt_level;
	network_interrupt_arg_t *data_received;
	int num_retries;

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
	if(socket == NULL || socket->port_status == NOT_READY){
		printf("[ERROR] Socket is not initialized\n");
		return 0;
	}
	
	interrupt_level = set_interrupt_level(DISABLED);
	if( (socket->socket_type == SERVER && server_sockets[socket->port_number - SERVER_SOCKET_START] != NULL) || (socket->socket_type == CLIENT && client_sockets[socket->port_number - CLIENT_SOCKET_START] != NULL)){
		printf("[ERROR] Socket is not initialized\n");
		set_interrupt_level(interrupt_level);
		return 0;
	}
	set_interrupt_level(interrupt_level);

	// Create packet header
	packet_header = (mini_header_reliable_t)malloc(sizeof(struct mini_header_reliable));	
	if(packet_header == NULL) {
		printf("[ERROR] Memory allocation for packet header failed\n");
		return 0;
	}

	// Initialize packet header
	packet_header->protocol = PROTOCOL_MINISTREAM;
	
	network_get_my_address(local_addr);
	pack_address(packet_header->source_address, local_addr);

	pack_unsigned_short(packet_header->source_port, socket->port_number);

	pack_address(packet_header->destination_address, socket->remote_address);

	pack_unsigned_short(packet_header->destination_port, socket->remote_port_number);

	packet_header->message_type = type;

	pack_unsigned_short(packet_header->seq_number, socket->seq_num);
	pack_unsigned_short(packet_header->ack_number, socket->ack_num);


	for(num_retries = 0; num_retries < 7; num_retries++){

		// Send packet
		bytes_sent_successfully = network_send_pkt(socket->remote_address, sizeof(struct mini_header_reliable), (char *)packet_header, len, msg) - sizeof(struct mini_header_reliable);
		bytes_sent_successfully = max(bytes_sent_successfully, 0);	
		
		register_alarm(100 * 2^num_retries, force_receive_to_exit,socket);
		semaphore_P(socket->data_lock);

		//Retrieve packet from queue
		interrupt_level = set_interrupt_level(DISABLED);
		queue_dequeue(socket->data_queue, (void **) &data_received);
		set_interrupt_level(interrupt_level);
		
		if(data_received == NULL){
			continue;
		}

		//Check for the appropriate ack
		if(*(data_received->buffer + 21) == MSG_ACK){
			//Check that the ack number matches our current socket's seq number
			if(unpack_unsigned_int(data_received->buffer + 26) != socket->seq_num){
				continue;
			}
			else{
				free(packet_header);
				return bytes_sent_successfully;
			}
		}		
	}

	free(packet_header);
	return -1;
}

/*
 * Receive a message from the other end of the socket. Blocks until
 * some data is received (which can be smaller than max_len bytes).
 *
 * Arguments: the socket on which the communication is made (socket), the memory
 *            location where the received message is returned (msg) and its
 *            maximum length (max_len).
 * Return value: -1 in case of error and sets the error code, the number of
 *           bytes received otherwise
 */
int minisocket_receive(minisocket_t socket, minimsg_t msg, int max_len, minisocket_error *error)
{
	network_interrupt_arg_t *data_received;
	unsigned int received_ack_number;
	unsigned int received_seq_number;
	int bytes_received = 0;

	interrupt_level_t interrupt_level = set_interrupt_level(DISABLED);
	if (socket == NULL ||
			socket->socket_type == SERVER && server_sockets[socket->port_number - SERVER_SOCKET_START] == NULL ||
			socket->socket_type == CLIENT && client_sockets[socket->port_number - CLIENT_SOCKET_START] == NULL) {
		printf("[ERROR] Cannot read from an uninitialized socket\n");
		*error = SOCKET_INVALIDPARAMS;
		set_interrupt_level(interrupt_level);
		return -1;
	}

	if(socket->port_status != READY){
		printf("[ERROR] Receive failed. A connection has not yet been established on this socket\n");
		*error = SOCKET_INVALIDPARAMS;
		set_interrupt_level(interrupt_level);
		return -1;
	}

	set_interrupt_level(interrupt_level);
	
	socket->num_threads_blocked++;
	while(bytes_received == 0){
		
		// Wait for packet arrival
		semaphore_P(socket->data_lock);
	
		//Check if we are supposed to terminate
		if(socket->should_terminate == 1){
			printf("[INFO] Socket was terminated before data could be received\n");
			*error = SOCKET_RECEIVEERROR;
			socket->num_threads_blocked--;
			return -1;
		}
	
		interrupt_level = set_interrupt_level(DISABLED);
		queue_dequeue(socket->data_queue, (void **) &data_received);
		set_interrupt_level(interrupt_level);

		//Check that their seq number is exactly one more than out ack number
		received_seq_number = unpack_unsigned_int(data_received->buffer + 22);

		if (received_seq_number != socket->ack_num + 1 || data_received->size - sizeof(struct mini_header_reliable) < 0){
			//Resend the ack for the last packet with our current ack number
			minisocket_send_packet_without_retransmission(socket, "This is an ACK\n", 15, MSG_ACK);
		}
		else{
			bytes_received = data_received->size - sizeof(struct mini_header_reliable);
			socket->ack_num++;
			minisocket_send_packet_without_retransmission(socket, "This is an ACK\n", 15, MSG_ACK);	
			memcpy(msg, data_received->buffer + sizeof(struct mini_header_reliable), bytes_received);
			
		}
		
		free(data_received);
	}

	socket->num_threads_blocked--;
	return bytes_received;
}

/* Close a connection. If minisocket_close is issued, any send or receive should
 * fail.  As soon as the other side knows about the close, it should fail any
 * send or receive in progress. The minisocket is destroyed by minisocket_close
 * function.  The function should never fail.
 */
void minisocket_close(minisocket_t socket)
{
	interrupt_level_t interrupt_level = set_interrupt_level(DISABLED);
	int current_blocked_threads = socket->num_threads_blocked;

	if(socket == NULL){
		printf("[ERROR] Can't close socket. Socket is NULL");
		return;
	}

	//Broadcast the stop signal
	broadcast_socket_close_signal(socket);	
	set_interrupt_level(interrupt_level);

	//Wait for the blocked threads to exit the send/recieve
	while(current_blocked_threads > 0){
		minithread_yield();

		interrupt_level = set_interrupt_level(DISABLED);
		current_blocked_threads = socket->num_threads_blocked;
		set_interrupt_level(interrupt_level);
	}


	//Destroy the socket / free corresponding structures
	interrupt_level = set_interrupt_level(DISABLED);
	if(socket->socket_type == SERVER){
		server_sockets[socket->port_number - SERVER_SOCKET_START] = NULL;
	}
	else{
		client_sockets[socket->port_number - SERVER_SOCKET_START] = NULL;
	}
	semaphore_destroy(socket->data_lock);
	free(socket);
	set_interrupt_level(interrupt_level);
	
}

void minisocket_process(network_interrupt_arg_t *data){
	unsigned short target_port = unpack_unsigned_short(data->buffer + 19);
	minisocket_t target_socket;
	mini_header_reliable_t header = (mini_header_reliable_t) data->buffer;
	network_address_t destination_address;
	network_address_t my_address;
	network_get_my_address(my_address);
	unpack_address(header->destination_address, destination_address);
	
	if (target_port > SERVER_SOCKET_START && target_port < SERVER_SOCKET_LIMIT) {
		target_socket = server_sockets[target_port - SERVER_SOCKET_START];
	} else if (target_port > CLIENT_SOCKET_START && target_port < CLIENT_SOCKET_LIMIT) {
		target_socket = client_sockets[target_port - CLIENT_SOCKET_START];
	} else {
		return;
	}

	// Validate packet
	if(target_socket == NULL){
		printf("[ERROR] Target socket %d is null \n", target_port);
		free(data);
	} else if (data->size < sizeof(struct mini_header_reliable)) {
		printf("[ERROR] Packet size is smaller than header \n");
		free(data);
	} else if (!network_address_same(destination_address, my_address)) {
		printf("[ERROR] Received packet not intended for us \n");
		free(data);
	} else {
		// Add packet to port queue
		queue_append(target_socket->data_queue, data);
		// Signal packet arrival
		semaphore_V(target_socket->data_lock);
	}
}
