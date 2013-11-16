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

//Broadcasts a terminate signal to all sockets that are blocked on the data_lock
void broadcast_socket_close_signal(minisocket_t socket_to_close){
	
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
	for(int i = 0; i< socket_to_close->num_threads_blocked; i++) {
		semaphore_V(socket_to_close->data_lock);
	}
}

/* Initializes the minisocket layer. */
void minisocket_initialize()
{
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
	new_socket->num_threads_blocked++;
	server_sockets[port - SERVER_SOCKET_START] = new_socket;
	
	should_terminate = new_socket->should_terminate;
	set_interrupt_level(interrupt_level);

	//Get handshake
	
	//While recieved is not a SYN, keep waiting (as long as we should not terminate)
	while (connection_established == 0 && should_terminate == 0) {
		semaphore_P(new_socket->data_lock);

		//Process received packet
		interrupt_level = set_interrupt_level(DISABLED);
		queue_dequeue(new_socket->data_queue, (void **) &data_received);
		
		
		//We got the SYN
		if(*(data_received->buffer + 21) == MSG_SYN){

			/*for(int i = 0; i < 7; i++) {
				send(synack(1,1));
				alarm_id = create_alarm(timeout = 2^i, signal(data_lock));
				if(receive(ack(1,1)) within timeout) {
					//disable interrupts
					if(alarm_id != NULL){
						deregister(alarm_id)
					}
					//re-enable interrupts

					//proccess data
					connection_established = 1;
					break;	
				}
									
			} 
			*/

		}	

		//else (do nothing)
		
		should_terminate = new_socket->should_terminate;
		set_interrupt_level(interrupt_level);
	}

	//When we do recieve a SYN, send a SYN_ACK, wait for ACK, and return
	interrupt_level = set_interrupt_level(DISABLED);
	new_socket->num_threads_blocked--;
	
	if(should_terminate == 1){
		printf("[INFO] Socket was closed before the connection could be established");
		set_interrupt_level(interrupt_level);
		return NULL;
	}

	else{
		set_interrupt_level(interrupt_level);
		return new_socket;
	}
	
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
		
		register_alarm(100 * 2^num_retries, force_receive_to_exit, socket);
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

void force_receive_to_exit(void* socket){
	semaphore_V(((minisocket_t)socket)->data_lock);
}

/*
typedef struct mini_header_reliable
{
	char protocol;

	char source_address[8];
	char source_port[2];

	char destination_address[8];
	char destination_port[2];
	
	char message_type;
	char seq_number[4];
	char ack_number[4];

} *mini_header_reliable_t;
*/




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
