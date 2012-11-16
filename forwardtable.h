#ifndef _FORWARDTABLE_H_
#define _FORWARDTABLE_H_

struct table_entry {
	unsigned long dest;
	unsigned int destPort;
	unsigned long nextHop;
	unsigned int nextHopPort;
	unsigned int delay;
	int lossChance;
}

struct raw_entry {
	char *dest;
	unsigned int destPort;
	char *nextHop;
	unsigned int nextHopPort
	unsigned int delay;
	int lossChance;
	struct rawEntry *nextEntry;
}


// ----------------------------------------------------------------------------
// Parse the forwarding table file.
//   Builds a file_info struct for the specified file consisting of 
//   a linked list of file_part structures that contain the location 
//   and sequence information from the tracker for the specified file.
// ----------------------------------------------------------------------------
bool parseFile(const char *filename, char *hostname, unsigned int port);

// ----------------------------------------------------------------------------
struct sockaddr_in *nextHop(ip_packet *pkt);

// ----------------------------------------------------------------------------
bool shouldForward(ip_packet *pkt);

// ----------------------------------------------------------------------------


#endif