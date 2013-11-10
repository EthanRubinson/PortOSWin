/*
 *	Implementation of minisockets.
 */
#include "minisocket.h"
#include "minimsg.h"
#include "synch.h"
#include "minithread.h"


struct mini_socket_lock{
	semaphore_t data_lock;
	int num_threads_blocked;
	int should_terminate;
};

struct minisocket
{
  struct mini_socket_lock* socket_lock;
  int port_number;
  socket_t socket_type;
};

/*
 * Valid socket number constants.
 */
#define SERVER_SOCKET_START 0
#define SERVER_SOCKET_LIMIT 32767
#define CLIENT_SOCKET_START 32768
#define CLIENT_SOCKET_LIMIT 65535

/*enum designated the type of socket a minisocket is*/
typedef enum {SERVER,CLIENT} socket_t;

// Socket arrays
minisocket_t server_sockets[SERVER_SOCKET_LIMIT - SERVER_SOCKET_START + 1];
minisocket_t client_sockets[CLIENT_SOCKET_LIMIT - CLIENT_SOCKET_START + 1];
semaphore_t server_socket_modification_locks[SERVER_SOCKET_LIMIT - SERVER_SOCKET_START + 1];
semaphore_t client_socket_modification_locks[CLIENT_SOCKET_LIMIT - CLIENT_SOCKET_START + 1];

void broadcast_socket_close_signal(minisocket_t socket_to_close){
	semaphore_t minisocket_lock;

	interrupt_level_t interrupt_level = set_interrupt_level(DISABLED);
	if(socket_to_close->socket_type == SERVER){
		minisocket_lock = server_socket_modification_locks[socket_to_close->port_number - SERVER_SOCKET_START];	
	}
	else{
		minisocket_lock = client_socket_modification_locks[socket_to_close->port_number - SERVER_SOCKET_START];	
	}
	set_interrupt_level(interrupt_level);

	semaphore_P(minisocket_lock);
	
	//Ensure that the socket is not already closed
	if(socket_to_close->socket_lock->should_terminate == 1){
		printf("[INFO] Socket at port %d is already marked for closure", socket_to_close->port_number);
		semaphore_V(minisocket_lock);
		return;
	}

	socket_to_close->socket_lock->should_terminate = 1;
	for(int i = 0; i< socket_to_close->socket_lock->num_threads_blocked; i++) {
		semaphore_V(socket_to_close->socket_lock->data_lock);
	}

	semaphore_V(minisocket_lock);

}

/* Initializes the minisocket layer. */
void minisocket_initialize()
{

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
	semaphore_t minisocket_lock;

	if(socket == NULL){
		printf("[ERROR] Can't close socket. Socket is NULL");
		return;
	}


	//Broadcast the stop signal
	broadcast_socket_close_signal(socket);

	
	interrupt_level_t interrupt_level = set_interrupt_level(DISABLED);
	if(socket->socket_type == SERVER){
		minisocket_lock = server_socket_modification_locks[socket->port_number - SERVER_SOCKET_START];	
	}
	else{
		minisocket_lock = client_socket_modification_locks[socket->port_number - SERVER_SOCKET_START];	
	}
	set_interrupt_level(interrupt_level);
	
	
	semaphore_P(minisocket_lock);
	socket->socket_lock->should_terminate = 1;	

	//Wait for the blocked threads to exit the send/recieve
	while(socket->socket_lock->num_threads_blocked > 0){
		semaphore_V(minisocket_lock);
		minithread_yield();
		semaphore_P(minisocket_lock);
	}


	//Destroy the socket / free corresponding structures
	interrupt_level_t interrupt_level = set_interrupt_level(DISABLED);

	if(socket->socket_type == SERVER){
		server_sockets[socket->port_number - SERVER_SOCKET_START] = NULL;	
	}
	else{
		server_sockets[socket->port_number - CLIENT_SOCKET_START] = NULL;		
	}
	set_interrupt_level(interrupt_level);
	
	semaphore_destroy(socket->socket_lock->data_lock);
	free(socket->socket_lock);
	free(socket);

	semaphore_V(minisocket_lock);
	
}
