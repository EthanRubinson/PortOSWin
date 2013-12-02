#include "miniroute.h"
#include "minimsg.h"
#include "miniheader.h"
#include "synch.h"
#include "hashtable.h"
#include "queue.h"
#include "alarm.h"

typedef struct routing_header* routing_header_t;
typedef struct cache_entry* cache_entry_t;
unsigned int route_discovery_id;
semaphore_t route_id_lock;
hashtable_t route_cache;

struct cache_entry
{
	network_address_t dest_address;
	network_address_t path[MAX_ROUTE_LENGTH];
	unsigned int path_length;
	semaphore_t cache_update;
	unsigned int discovery_id;
	unsigned int num_threads_waiting;
};

/* Performs any initialization of the miniroute layer, if required. */
void miniroute_initialize()
{
	route_discovery_id = 0;
	route_id_lock = semaphore_create();
	semaphore_initialize(route_id_lock, 1);
	route_cache = hashtable_new(SIZE_OF_ROUTE_CACHE);
}

unsigned int get_route_discovery_id() {
	semaphore_P(route_id_lock);
	route_discovery_id++;
	return route_discovery_id;
	semaphore_V(route_id_lock);
}

void wake_up_route_discovery(void* cache_update) {
	semaphore_t update = (semaphore_t) cache_update;
	semaphore_V(update);
}

/* sends a miniroute packet, automatically discovering the path if necessary. See description in the
 * .h file.
 */
int miniroute_send_pkt(network_address_t dest_address, int hdr_len, char* hdr, int data_len, char* data)
{
	int interrupt_level;
	int i;
	int bytes_sent_successfully;
	unsigned int *my_address;
	cache_entry_t cached_path;
	routing_header_t header;
	char* user_data;

	interrupt_level = set_interrupt_level(DISABLED);
	if(hashtable_get(route_cache, (char*) dest_address, (void**) &cached_path) == -1) {
		set_interrupt_level(interrupt_level);
		if(miniroute_discover_path(dest_address) < 0) { // may block for up to 36 seconds
			return -1;
		}
	} else if (hashtable_get(route_cache, (char*) dest_address, (void**) &cached_path) == 0 && cached_path->path_length < 0) {
		cached_path->num_threads_waiting++;
		semaphore_P(cached_path->cache_update);
	}
	set_interrupt_level(interrupt_level);

	header = (routing_header_t) malloc(sizeof(struct routing_header));

	if (header == NULL) {
		printf("[ERROR] Routing header allocation failed \n");
		return -1;
	}

	pack_address(header->destination, dest_address);
	pack_unsigned_int(header->id, 0);
	pack_unsigned_int(header->path_len, cached_path->path_length);
	header->routing_packet_type = ROUTING_DATA;
	pack_unsigned_int(header->ttl, MAX_ROUTE_LENGTH);

	// routing path
	for(i = 0; i < MAX_ROUTE_LENGTH; i++) {
		pack_address(header->path[i], cached_path->path[i]);
	}

	user_data = (char*) malloc(hdr_len + data_len);
	memcpy(user_data, hdr, hdr_len);
	memcpy(user_data + hdr_len, data, data_len);

	bytes_sent_successfully = network_send_pkt(dest_address, sizeof(struct routing_header), (char*)header, hdr_len + data_len, user_data);
	free(header);
	free(user_data);
	return bytes_sent_successfully;
}

/* Broadcast packet to discover route, 
 * it seems like we still need a separate semamphore for each thread... ASK TA..
 *
 */
int miniroute_discover_path(network_address_t dest_address) {
	int i;
	int alarm_id;
	int interrupt_level;
	network_address_t my_address;
	unsigned int discovery_id = get_route_discovery_id();
	cache_entry_t cached_path;
	routing_header_t header;

	network_get_my_address(my_address);
	header = (routing_header_t) malloc(sizeof(struct routing_header));
	pack_address(header->destination, dest_address);
	pack_unsigned_int(header->id, discovery_id);
	memset(header->path, 0, sizeof(header->path));
	pack_address(header->path[0], my_address);
	pack_unsigned_int(header->path_len, 1);
	header->routing_packet_type = ROUTING_ROUTE_DISCOVERY;
	pack_unsigned_int(header->ttl, MAX_ROUTE_LENGTH);
	
	interrupt_level = set_interrupt_level(DISABLED);
	if(hashtable_get(route_cache, (char*) dest_address, (void**) &cached_path) == -1) {
		cached_path = (cache_entry_t) malloc(sizeof(struct cache_entry));
		cached_path->cache_update = semaphore_create();
		semaphore_initialize(cached_path->cache_update, 0);
		network_address_copy(dest_address, cached_path->dest_address);
		cached_path->discovery_id = discovery_id;
		cached_path->path_length = -1;
		network_address_copy(my_address, cached_path->path[0]);
		if(hashtable_put(route_cache, (char*) dest_address, (void**) &cached_path) == -1) {
			free(cached_path);
			free(header);
			printf("[ERROR] Could not put entry into hashtable \n");
			return -1;
		}
	}
	set_interrupt_level(interrupt_level);

	for(i = 0; i < 3; i++){
		// set alarm for 12 seconds (alarm will signal cache_update)
		alarm_id = register_alarm(12000, wake_up_route_discovery, (void*) cached_path->cache_update);
		if(network_bcast_pkt(sizeof(struct routing_header), (char*)header, 22, "Discovering route... \n") != 22) {
			printf("[ERROR] Broadcast discovery packet failed to send \n");
			free(header);
			return -1;
		}

		// use alarm to V cache_update
		cached_path->num_threads_waiting++;
		semaphore_P(cached_path->cache_update);

		interrupt_level = set_interrupt_level(DISABLED);
		deregister_alarm(alarm_id);

		if(hashtable_get(route_cache, (char*) dest_address, (void**) &cached_path) == 0 && cached_path->path_length > 0) {
			set_interrupt_level(interrupt_level);
			return 0;
		}
		set_interrupt_level(interrupt_level);
	}
	return -1;
}

/* hashes a network_address_t into a 16 bit unsigned int */
unsigned short hash_address(network_address_t address)
{
	unsigned int result = 0;
	int counter;

	for (counter = 0; counter < 3; counter++)
		result ^= ((unsigned short*)address)[counter];

	return result % 65521;
}