/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright © 2016-2018 The TokTok team.
 * Copyright © 2013 Tox project.
 */

/**
 * An implementation of the DHT as seen in docs/updates/DHT.md
 */
#ifndef C_TOXCORE_TOXCORE_DHT_H
#define C_TOXCORE_TOXCORE_DHT_H

#include <stdbool.h>

#include "attributes.h"
#include "crypto_core.h"
#include "logger.h"
#include "mono_time.h"
#include "network.h"
#include "ping_array.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Maximum number of clients stored per friend. */
#define MAX_FRIEND_CLIENTS 8

#define LCLIENT_NODES MAX_FRIEND_CLIENTS
#define LCLIENT_LENGTH 128

/** A list of the clients mathematically closest to ours. */
#define LCLIENT_LIST (LCLIENT_LENGTH * LCLIENT_NODES)

#define MAX_CLOSE_TO_BOOTSTRAP_NODES 8

/** The max number of nodes to send with send nodes. */
#define MAX_SENT_NODES 4

/** Ping timeout in seconds */
#define PING_TIMEOUT 5

/** size of DHT ping arrays. */
#define DHT_PING_ARRAY_SIZE 512

/** Ping interval in seconds for each node in our lists. */
#define PING_INTERVAL 60

/** The number of seconds for a non responsive node to become bad. */
#define PINGS_MISSED_NODE_GOES_BAD 1
#define PING_ROUNDTRIP 2
#define BAD_NODE_TIMEOUT (PING_INTERVAL + PINGS_MISSED_NODE_GOES_BAD * (PING_INTERVAL + PING_ROUNDTRIP))

/** The number of "fake" friends to add (for optimization purposes and so our paths for the onion part are more random) */
#define DHT_FAKE_FRIEND_NUMBER 2

#define MAX_CRYPTO_REQUEST_SIZE 1024

#define CRYPTO_PACKET_FRIEND_REQ    32  // Friend request crypto packet ID.
#define CRYPTO_PACKET_DHTPK         156
#define CRYPTO_PACKET_NAT_PING      254 // NAT ping crypto packet ID.

/** Create a request to peer.
 * send_public_key and send_secret_key are the pub/secret keys of the sender.
 * recv_public_key is public key of receiver.
 * packet must be an array of MAX_CRYPTO_REQUEST_SIZE big.
 * Data represents the data we send with the request with length being the length of the data.
 * request_id is the id of the request (32 = friend request, 254 = ping request).
 *
 * return -1 on failure.
 * return the length of the created packet on success.
 */
non_null()
int create_request(
    const uint8_t *send_public_key, const uint8_t *send_secret_key, uint8_t *packet,
    const uint8_t *recv_public_key, const uint8_t *data, uint32_t length, uint8_t request_id);

/** Puts the senders public key in the request in public_key, the data from the request
 * in data if a friend or ping request was sent to us and returns the length of the data.
 * packet is the request packet and length is its length.
 *
 *  return -1 if not valid request.
 */
non_null()
int handle_request(
    const uint8_t *self_public_key, const uint8_t *self_secret_key, uint8_t *public_key, uint8_t *data,
    uint8_t *request_id, const uint8_t *packet, uint16_t length);

typedef struct IPPTs {
    IP_Port     ip_port;
    uint64_t    timestamp;
} IPPTs;

typedef struct IPPTsPng {
    IP_Port     ip_port;
    uint64_t    timestamp;
    uint64_t    last_pinged;

    /* Returned by this node */
    IP_Port     ret_ip_port;
    uint64_t    ret_timestamp;
    /* true if this ip_port is ours */
    bool        ret_ip_self;
} IPPTsPng;

typedef struct Client_data {
    uint8_t     public_key[CRYPTO_PUBLIC_KEY_SIZE];
    IPPTsPng    assoc4;
    IPPTsPng    assoc6;
} Client_data;

/*----------------------------------------------------------------------------------*/

typedef struct NAT {
    /* 1 if currently hole punching, otherwise 0 */
    uint8_t     hole_punching;
    uint32_t    punching_index;
    uint32_t    tries;
    uint32_t    punching_index2;

    uint64_t    punching_timestamp;
    uint64_t    recv_nat_ping_timestamp;
    uint64_t    nat_ping_id;
    uint64_t    nat_ping_timestamp;
} NAT;

#define DHT_FRIEND_MAX_LOCKS 32

typedef struct Node_format {
    uint8_t     public_key[CRYPTO_PUBLIC_KEY_SIZE];
    IP_Port     ip_port;
} Node_format;

typedef struct DHT_Friend DHT_Friend;

non_null() const uint8_t *dht_friend_public_key(const DHT_Friend *dht_friend);
non_null() const Client_data *dht_friend_client(const DHT_Friend *dht_friend, size_t index);

/** Return packet size of packed node with ip_family on success.
 * Return -1 on failure.
 */
int packed_node_size(Family ip_family);

/** Packs an IP_Port structure into data of max size length.
 *
 * Packed_length is the offset of data currently packed.
 *
 * Returns size of packed IP_Port data on success
 * Return -1 on failure.
 */
non_null()
int pack_ip_port(uint8_t *data, uint16_t length, const IP_Port *ip_port);

/** Unpack IP_Port structure from data of max size length into ip_port.
 *
 * len_processed is the offset of data currently unpacked.
 *
 * Return size of unpacked ip_port on success.
 * Return -1 on failure.
 */
non_null()
int unpack_ip_port(IP_Port *ip_port, const uint8_t *data, uint16_t length, bool tcp_enabled);

/** Pack number of nodes into data of maxlength length.
 *
 * return length of packed nodes on success.
 * return -1 on failure.
 */
non_null()
int pack_nodes(uint8_t *data, uint16_t length, const Node_format *nodes, uint16_t number);

/** Unpack data of length into nodes of size max_num_nodes.
 * Put the length of the data processed in processed_data_len.
 * tcp_enabled sets if TCP nodes are expected (true) or not (false).
 *
 * return number of unpacked nodes on success.
 * return -1 on failure.
 */
non_null(1, 4) nullable(3)
int unpack_nodes(Node_format *nodes, uint16_t max_num_nodes, uint16_t *processed_data_len, const uint8_t *data,
                 uint16_t length, bool tcp_enabled);


/*----------------------------------------------------------------------------------*/
/* struct to store some shared keys so we don't have to regenerate them for each request. */
#define MAX_KEYS_PER_SLOT 4
#define KEYS_TIMEOUT 600

typedef struct Shared_Key {
    uint8_t public_key[CRYPTO_PUBLIC_KEY_SIZE];
    uint8_t shared_key[CRYPTO_SHARED_KEY_SIZE];
    uint32_t times_requested;
    bool stored;
    uint64_t time_last_requested;
} Shared_Key;

typedef struct Shared_Keys {
    Shared_Key keys[256 * MAX_KEYS_PER_SLOT];
} Shared_Keys;

/*----------------------------------------------------------------------------------*/

typedef int cryptopacket_handler_cb(void *object, const IP_Port *ip_port, const uint8_t *source_pubkey,
                                    const uint8_t *data, uint16_t len, void *userdata);

typedef struct DHT DHT;

non_null() const uint8_t *dht_get_self_public_key(const DHT *dht);
non_null() const uint8_t *dht_get_self_secret_key(const DHT *dht);
non_null() void dht_set_self_public_key(DHT *dht, const uint8_t *key);
non_null() void dht_set_self_secret_key(DHT *dht, const uint8_t *key);

non_null() Networking_Core *dht_get_net(const DHT *dht);
non_null() struct Ping *dht_get_ping(const DHT *dht);
non_null() const Client_data *dht_get_close_clientlist(const DHT *dht);
non_null() const Client_data *dht_get_close_client(const DHT *dht, uint32_t client_num);
non_null() uint16_t dht_get_num_friends(const DHT *dht);

non_null() DHT_Friend *dht_get_friend(DHT *dht, uint32_t friend_num);
non_null() const uint8_t *dht_get_friend_public_key(const DHT *dht, uint32_t friend_num);

/*----------------------------------------------------------------------------------*/

/** Shared key generations are costly, it is therefore smart to store commonly used
 * ones so that they can be re-used later without being computed again.
 *
 * If a shared key is already in shared_keys, copy it to shared_key.
 * Otherwise generate it into shared_key and copy it to shared_keys
 */
non_null()
void get_shared_key(
    const Mono_Time *mono_time, Shared_Keys *shared_keys, uint8_t *shared_key,
    const uint8_t *secret_key, const uint8_t *public_key);

/** Copy shared_key to encrypt/decrypt DHT packet from public_key into shared_key
 * for packets that we receive.
 */
non_null()
void dht_get_shared_key_recv(DHT *dht, uint8_t *shared_key, const uint8_t *public_key);

/** Copy shared_key to encrypt/decrypt DHT packet from public_key into shared_key
 * for packets that we send.
 */
non_null()
void dht_get_shared_key_sent(DHT *dht, uint8_t *shared_key, const uint8_t *public_key);

/** Sends a getnodes request to `ip_port` with the public key `public_key` for nodes
 * that are close to `client_id`.
 *
 * Return true on success.
 */
non_null()
bool dht_getnodes(DHT *dht, const IP_Port *ip_port, const uint8_t *public_key, const uint8_t *client_id);

typedef void dht_ip_cb(void *object, int32_t number, const IP_Port *ip_port);

typedef void dht_get_nodes_response_cb(const DHT *dht, const Node_format *node, void *userdata);

/** Sets the callback to be triggered on a getnodes response. */
non_null(1) nullable(2)
void dht_callback_get_nodes_response(DHT *dht, dht_get_nodes_response_cb *function);

/** Add a new friend to the friends list.
 * public_key must be CRYPTO_PUBLIC_KEY_SIZE bytes long.
 *
 * ip_callback is the callback of a function that will be called when the ip address
 * is found along with arguments data and number.
 *
 * lock_count will be set to a non zero number that must be passed to dht_delfriend()
 * to properly remove the callback.
 *
 *  return 0 if success.
 *  return -1 if failure (friends list is full).
 */
non_null(1, 2) nullable(3, 4, 6)
int dht_addfriend(DHT *dht, const uint8_t *public_key, dht_ip_cb *ip_callback,
                  void *data, int32_t number, uint16_t *lock_count);

/** Delete a friend from the friends list.
 * public_key must be CRYPTO_PUBLIC_KEY_SIZE bytes long.
 *
 *  return 0 if success.
 *  return -1 if failure (public_key not in friends list).
 */
non_null()
int dht_delfriend(DHT *dht, const uint8_t *public_key, uint16_t lock_count);

/** Get ip of friend.
 *  public_key must be CRYPTO_PUBLIC_KEY_SIZE bytes long.
 *  ip must be 4 bytes long.
 *  port must be 2 bytes long.
 *
 *  return -1, -- if public_key does NOT refer to a friend
 *  return  0, -- if public_key refers to a friend and we failed to find the friend (yet)
 *  return  1, ip if public_key refers to a friend and we found him
 */
non_null()
int dht_getfriendip(const DHT *dht, const uint8_t *public_key, IP_Port *ip_port);

/** Compares pk1 and pk2 with pk.
 *
 *  return 0 if both are same distance.
 *  return 1 if pk1 is closer.
 *  return 2 if pk2 is closer.
 */
non_null()
int id_closest(const uint8_t *pk, const uint8_t *pk1, const uint8_t *pk2);

/***
 * Add node to the node list making sure only the nodes closest to cmp_pk are in the list.
 *
 * @return true iff the node was added to the list.
 */
non_null()
bool add_to_list(
    Node_format *nodes_list, uint32_t length, const uint8_t *pk, const IP_Port *ip_port, const uint8_t *cmp_pk);

/** Return 1 if node can be added to close list, 0 if it can't.
 */
non_null()
bool node_addable_to_close_list(DHT *dht, const uint8_t *public_key, const IP_Port *ip_port);

/** Get the (maximum MAX_SENT_NODES) closest nodes to public_key we know
 * and put them in nodes_list (must be MAX_SENT_NODES big).
 *
 * sa_family = family (IPv4 or IPv6) (0 if we don't care)?
 * is_LAN = return some LAN ips (true or false)
 * want_good = do we want tested nodes or not? (TODO(irungentoo))
 *
 * return the number of nodes returned.
 *
 */
non_null()
int get_close_nodes(
    const DHT *dht, const uint8_t *public_key, Node_format *nodes_list, Family sa_family, bool is_LAN);


/** Put up to max_num nodes in nodes from the random friends.
 *
 * return the number of nodes.
 */
non_null()
uint16_t randfriends_nodes(const DHT *dht, Node_format *nodes, uint16_t max_num);

/** Put up to max_num nodes in nodes from the closelist.
 *
 * return the number of nodes.
 */
non_null()
uint16_t closelist_nodes(const DHT *dht, Node_format *nodes, uint16_t max_num);

/** Run this function at least a couple times per second (It's the main loop). */
non_null()
void do_dht(DHT *dht);

/*
 *  Use these two functions to bootstrap the client.
 */
/** Sends a "get nodes" request to the given node with ip, port and public_key
 *   to setup connections
 */
non_null()
void dht_bootstrap(DHT *dht, const IP_Port *ip_port, const uint8_t *public_key);

/** Resolves address into an IP address. If successful, sends a "get nodes"
 *   request to the given node with ip, port and public_key to setup connections
 *
 * address can be a hostname or an IP address (IPv4 or IPv6).
 * if ipv6enabled is 0 (zero), the resolving sticks STRICTLY to IPv4 addresses
 * if ipv6enabled is not 0 (zero), the resolving looks for IPv6 addresses first,
 *   then IPv4 addresses.
 *
 *  returns 1 if the address could be converted into an IP address
 *  returns 0 otherwise
 */
non_null()
int dht_bootstrap_from_address(DHT *dht, const char *address, uint8_t ipv6enabled,
                               uint16_t port, const uint8_t *public_key);

/** Start sending packets after DHT loaded_friends_list and loaded_clients_list are set.
 *
 * returns 0 if successful
 * returns -1 otherwise
 */
non_null()
int dht_connect_after_load(DHT *dht);

/* ROUTING FUNCTIONS */

/** Send the given packet to node with public_key.
 *
 *  return -1 if failure.
 */
non_null()
int route_packet(const DHT *dht, const uint8_t *public_key, const uint8_t *packet, uint16_t length);

/**
 * Send the following packet to everyone who tells us they are connected to friend_id.
 *
 *  return ip for friend.
 *  return number of nodes the packet was sent to. (Only works if more than (MAX_FRIEND_CLIENTS / 4).
 */
non_null()
uint32_t route_to_friend(const DHT *dht, const uint8_t *friend_id, const Packet *packet);

/** Function to handle crypto packets.
 */
non_null(1) nullable(3, 4)
void cryptopacket_registerhandler(DHT *dht, uint8_t byte, cryptopacket_handler_cb *cb, void *object);

/* SAVE/LOAD functions */

/** Get the size of the DHT (for saving). */
non_null()
uint32_t dht_size(const DHT *dht);

/** Save the DHT in data where data is an array of size dht_size(). */
non_null()
void dht_save(const DHT *dht, uint8_t *data);

/** Load the DHT from data of size size.
 *
 *  return -1 if failure.
 *  return 0 if success.
 */
non_null()
int dht_load(DHT *dht, const uint8_t *data, uint32_t length);

/** Initialize DHT. */
non_null()
DHT *new_dht(const Logger *log, Mono_Time *mono_time, Networking_Core *net, bool holepunching_enabled);

non_null()
void kill_dht(DHT *dht);

/**  return false if we are not connected to the DHT.
 *  return true if we are.
 */
non_null()
bool dht_isconnected(const DHT *dht);

/**  return false if we are not connected or only connected to lan peers with the DHT.
 *  return true if we are.
 */
non_null()
bool dht_non_lan_connected(const DHT *dht);


non_null()
uint32_t addto_lists(DHT *dht, const IP_Port *ip_port, const uint8_t *public_key);

/** Copies our own ip_port structure to `dest`. WAN addresses take priority over LAN addresses.
 *
 * This function will zero the `dest` buffer before use.
 *
 * Return 0 if our ip port can't be found (this usually means we're not connected to the DHT).
 * Return 1 if IP is a WAN address.
 * Return 2 if IP is a LAN address.
 */
non_null()
unsigned int ipport_self_copy(const DHT *dht, IP_Port *dest);

#ifdef __cplusplus
}  // extern "C"
#endif

#endif
