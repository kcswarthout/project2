#include "forwardtable.h"
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <errno.h>
#include <time.h>
#include <fcntl.h>
#include <strings.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include "utilities.h"
#include "packet.h"

#define TRACKER "tracker.txt"
#define TOK_PER_LINE 8
#define MAX_NUM_NODES 20

enum token { EMULATOR, EMUL_PORT, DESTINATION, DEST_PORT, NEXT_HOP, NEXT_HOP_PORT, DELAY, LOSS_CHANCE };

// this is the forward table
struct table_entry *table;
int tabSize = 0;
struct neighbor_entry *allNeighbors;
int allNeiNum;
struct neighbor_listing *neighbors;
int neiNum;
struct lsp_entry *lspList;
int lspSize = 0;
unsigned long lspSeq = 0;

int getIndexLSP(unsigned long ip, unsigned int port) {
  int i;
  for (i = 0; i < lspSize; i++) {
    if(lspList[i].ip == ip && lspList[i].port == port) {
      return i;
    }
  }
  return -1;
}

void updateNeighbors() {
  int i;
  neiNum = 0;
  free(neighbors);
  neighbors = malloc(20 * sizeof(struct neighbor_listing));
  for (i = 0; i < allNeiNum; i++) {
    int x = getIndexLSP(allNeighbors[i].ip, allNeighbors[i].port);
    if (x == -1) {
      allNeighbors[i].isAlive = 0;
      continue;
    }
    else if (lspList[x].time - getTimeMS() > 1500) {
      allNeighbors[i].isAlive = 0;
      continue;
    }
    else {
      allNeighbors[i].isAlive = 1;
      neighbors[neiNum].ip = allNeighbors[i].ip;
      neighbors[neiNum].port =allNeighbors[i].port;
      neiNum++;
    }
  }
}

int updateLSP(struct ip_packet *lsp) {
  int i = getIndexLSP(lsp->src, lsp->srcPort);
  struct packet *dpkt = (struct packet *)lsp->payload;
  if (i == -1) {
    if (lspSize >= 20) {
      return 0;
    }
    lspList[lspSize].ip = lsp->src;
    lspList[lspSize].port = lsp->srcPort;
    lspList[lspSize].time = getTimeMS();
    lspList[i].seq = dpkt->seq;
    lspList[lspSize].list = malloc(20 * sizeof(struct neighbor_listing));
    bzero(lspList[lspSize].list, 20 * sizeof(struct neighbor_listing));
    memcpy(lspList[lspSize].list, dpkt->payload, lsp->length - HEADER_SIZE);
    lspSize++;
    return 1;
  }
  else {
    
    if (dpkt->seq > lspList[i].seq) {
      lspList[i].ip = lsp->src;
      lspList[i].port = lsp->srcPort;
      lspList[i].time = getTimeMS();
      lspList[i].seq = dpkt->seq;
      bzero(lspList[i].list, 20 * sizeof(struct neighbor_listing));
      memcpy(lspList[i].list, dpkt->payload, lsp->length - HEADER_SIZE);
      return 1;
    }
    return 0;
  }
}

struct packet *createNeighborPkt() {
  struct packet *dpkt = malloc(sizeof(struct packet));
  unsigned char *buffer;
  int size = neighborsToPayload(neighbors, &buffer, neiNum);
  memcpy(dpkt->payload, buffer, size);
  dpkt->len = size;
  //dpkt->type = 's';
  return dpkt;
}

void floodLSP(int sockfd, struct sockaddr_in *local, struct ip_packet *pkt) {
  int i;
  if (pkt == NULL) {
    pkt = malloc(sizeof(struct ip_packet));
    bzero(pkt, sizeof(struct ip_packet));
    struct packet *dpkt = createNeighborPkt();
    pkt->length = HEADER_SIZE + dpkt->len;  
    dpkt->type = 'S';
    dpkt->seq  = lspSeq;
    dpkt->len  = (unsigned long) 20;
    memcpy(pkt->payload, dpkt, sizeof(struct packet));
    pkt->src = local->sin_addr.s_addr;
    pkt->srcPort = local->sin_port;
    pkt->priority = 0;
    updateLSP(pkt);
  }
  for (i = 0; i < allNeiNum; i++) {
    sendIpPacketTo(sockfd, pkt, (struct sockaddr *) allNeighbors[allNeiNum].socket);
  }
}

int nextHop(struct ip_packet *pkt, struct sockaddr_in *socket) {
	if (table == NULL) printf("No table entries\n");
	if (pkt == NULL) perrorExit("nextHop function: pkt null");
	int i;
	for (i = 0; i < tabSize; i++) {
		//printf("dest %lu  %lu\n", table[i].dest, pkt->dest);
		if (table[i].dest == pkt->dest) {
			//printf("destport %u  %u\n", table[i].destPort, pkt->destPort);
			if (table[i].destPort == pkt->destPort) {
				if (socket != NULL) {
					bzero(socket, sizeof(struct sockaddr_in));
					socket->sin_family = AF_INET;
					socket->sin_addr.s_addr = htonl(table[i].nextHop);
					socket->sin_port = htons(table[i].nextHopPort);
					//printf("socket is %lu  %u", ntohl(socket->sin_addr.s_addr), (unsigned long)ntohs(socket->sin_port));
				}
				printf("i = %d\n", i);
				return 1;
			}
		}
		//printf("no match\n\n");
	}
  return 0;
}



int createRoutes() {
  return 1;
}


int initTable(const char *filename, struct sockaddr_in *local) {
  table = malloc(20 * sizeof(struct table_entry));
  lspList = malloc(20 * sizeof(struct lsp_entry));
  allNeiNum = readtopology(filename, local, &allNeighbors);
  int i;
  printf("%d Neighbors\n", allNeiNum);
  char str[20];
  for (i = 0; i < allNeiNum; i++) {
    ipULtoStr(allNeighbors[i].ip, str);
    printf("IP: %s    Port: %u\n", str, allNeighbors[i].port);
  }
  //fflush(stdout);
  
  return 1;
}






// ----------------------------------------------------------------------------
// Parse the forwarding table file.
//   Builds a file_info struct for the specified file consisting of 
//   a linked list of file_entry structures that contain the location 
//   and sequence information from the tracker for the specified file.
// ----------------------------------------------------------------------------
/*
int parseFile(const char *filename, unsigned int port) {
    if (filename == NULL) ferrorExit("ParseTracker: invalid filename");
    char *hostname;
	
    // Setup the rawTable
    struct raw_entry *rawTable = malloc(sizeof(struct raw_entry));
    struct raw_entry *tmp = rawTable;
	
    // Open the forward table file
    FILE *file = fopen(filename, "r");
    if (file == NULL) perrorExit("Table open error");
    else              puts("\nTable file opened.");

    // Read in a line at a time from the forward table file
    char *line = NULL;
    size_t lineLen = 0;
    size_t bytesRead = getline(&line, &lineLen, file);
    if (bytesRead == -1) perrorExit("Getline error");
    while (bytesRead != -1) { 
		//printf("read a line\n");
        // Tokenize line
        int n = 0; // TODO: should this be 0 or -1?
        char *tokens[TOK_PER_LINE];
        char *tok = strtok(line, " ");
        while (tok != NULL) {
            tokens[n] = tok;
            tok  = strtok(NULL, " ");
			n++;
        }
		size_t len = strlen((const char *) tokens[EMULATOR]);
		hostname = malloc(len + 1);
		hostname[len] = '\0';
		gethostname(hostname, len);
		//printf("tokenized a line\n");
		//printf("EMUL %d\n", EMULATOR);
		//printf("EMULPORT %d\n", EMUL_PORT);
		//printf("hostname %s\n", hostname);
		//printf("port %d\n", port);
		//printf("tok[em] %s\n", tokens[EMULATOR]);
		//printf("tok[empo] %s\n", tokens[EMUL_PORT]);
        // Only process this line if it is for the specified emulator
        if (strcmp(tokens[EMULATOR], hostname) == 0 && atoi(tokens[EMUL_PORT]) == port) {
            //printf("for this emul\n");
            struct raw_entry *entry = malloc(sizeof(struct raw_entry));
            bzero(entry, sizeof(struct raw_entry));
            //printf("malloced entry\n");
            // TODO: type conversions likr atol or strtoul instead of atoi
            entry->dest         = strdup(tokens[DESTINATION]);
            entry->destPort     = atoi(tokens[DEST_PORT]);
            entry->nextHop      = strdup(tokens[NEXT_HOP]);
            entry->nextHopPort  = atoi(tokens[NEXT_HOP_PORT]);
            entry->delay 		    = 0;
            entry->lossChance 	= 0;
            //printf("set fields\n");
            // Link it in to the list of raw_entries
            tmp->nextEntry = entry;
            tmp = entry;
            ++size;
        }
		
        // Get the next forward table line
        free(line);
        line = NULL;
        bytesRead = getline(&line, &lineLen, file);
    }
    free(line);
    

    // Close the forward table file
    if (fclose(file) != 0) perrorExit("Forward table close error");
    else                   puts("Forward table file closed.\n");

    // ------------------------------------------------------------------------
    // Setup forward table
	table = malloc(sizeof(struct table_entry) * size);
	tmp = rawTable->nextEntry;
	free(rawTable);
	rawTable = tmp;
	int i = 0;
	for(i = 0; i < size; i++) {
		table[i].dest = nameToAddr(tmp->dest);
		table[i].destPort = tmp->destPort;
		table[i].nextHop = nameToAddr(tmp->nextHop);
		table[i].nextHopPort = tmp->nextHopPort;
		printf("dest: %s %u     nexthop: %s %u   delay: %u  loss: %d\n", 
    tmp->dest, tmp->destPort, tmp->nextHop, tmp->nextHopPort, tmp->delay, tmp->lossChance);
		rawTable = tmp;
		tmp = tmp->nextEntry;
		free(rawTable);
	}
    return 1; // success
}
*/


