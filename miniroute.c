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
	network_address_t* path[MAX_ROUTE_LENGTH];
	unsigned int path_length;
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

/* sends a miniroute packet, automatically discovering the path if necessary. See description in the
 * .h file.
 */
int miniroute_send_pkt(network_address_t dest_address, int hdr_len, char* hdr, int data_len, char* data)
{
	// [beging-synchronized]
	// check path cache for dest_address
	// if timestamp is greater than 3 seconds old 
	// [end-synchronized]
	// call miniroute_discover_path()

	// send data routing packet using path from hashtable cache

}

/* Broadcast packet to discover route, 
 * it seems like we still need a separate semamphore for each thread... ASK TA..
 *
 */
int miniroute_discover_path(network_address_t dest_address) {
	int i;
	int alarm_id;
	int interrupt_level;
	unsigned int *my_address;
	unsigned int discovery_id = get_route_discovery_id();
	cache_entry_t cached_path;
	semaphore_t cache_update = semaphore_create();
	
	semaphore_initialize(cache_update, 0);

	network_get_my_address(my_address);
	routing_header_t header = (routing_header_t) malloc(sizeof(struct routing_header));
	pack_address(header->destination, dest_address);
	pack_unsigned_int(header->id, discovery_id);
	memset(header->path, 0, sizeof(header->path));
	pack_address(header->path[0], my_address);
	pack_unsigned_int(header->path_len, 1);
	header->routing_packet_type = ROUTING_ROUTE_DISCOVERY;
	pack_unsigned_int(header->ttl, MAX_ROUTE_LENGTH);
	

	for(i = 0; i < 3; i++){
		// set alarm for 12 seconds
		if(network_bcast_pkt(sizeof(struct routing_header), (char*)header, 22, "Discovering route... \n") != 22) {
			printf("[ERROR] broadcast packet failed \n");
			free(header);
			return -1;
		}
		semaphore_P(cache_update);

		// use alarm to V cache_update

		interrupt_level = set_interrupt_level(DISABLED);
		if(hashtable_get(route_cache, (char*) dest_address, (void**) &cached_path) == 0
			&& cached_path->path > 0) {
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