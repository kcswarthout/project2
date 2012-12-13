#include <netinet/in.h>
#include "packet.h"

#ifndef _FORWARDTABLE_H_
#define _FORWARDTABLE_H_

struct table_entry {
	unsigned long dest;
	unsigned int destPort;
	unsigned long nextHop;
	unsigned int nextHopPort;
} __attribute__((packed));

struct raw_entry {
	char *dest;
	unsigned int destPort;
	char *nextHop;
	unsigned int nextHopPort;
	unsigned int delay;
	int lossChance;
	struct raw_entry *nextEntry;
};

struct end_packet_list {
	struct ip_packet *pkt;
	struct end_packet_list *nextPkt;
};

#define TABLE_ENTRY_SIZE sizeof(struct table_entry)

// ----------------------------------------------------------------------------
// Parse the forwarding table file.
//   Builds a file_info struct for the specified file consisting of 
//   a linked list of file_part structures that contain the location 
//   and sequence information from the tracker for the specified file.
// ----------------------------------------------------------------------------
int createRoutes();

// ----------------------------------------------------------------------------
int nextHop(struct ip_packet *pkt, struct sockaddr_in *socket);

// ----------------------------------------------------------------------------


#endif
