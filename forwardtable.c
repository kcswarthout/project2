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
#include "forwardtable.h"
#include "packet.h"

#define TRACKER "tracker.txt"
#define TOK_PER_LINE 8

enum token { EMULATOR, EMUL_PORT, DESTINATION, DEST_PORT, NEXT_HOP, NEXT_HOP_PORT, DELAY, LOSS_CHANCE };

struct table_entry *table = NULL;
int size = 0;

struct table_entry *nextHop(struct ip_packet *pkt, struct sockaddr_in *socket) {
	if (table == NULL) printf("No table entries\n");
	if (pkt == NULL) perrorExit("nextHop function: pkt null");
	struct table_entry *nextEntry = NULL;
	int i;
	for (i = 0; i < size; i++) {
		printf("dest %lu  %lu\n", table[i].dest, pkt->dest);
		if (table[i].dest == pkt->dest) {
			printf("destport %li  %li\n", table[i].destPort, pkt->destPort);
			if (table[i].destPort == pkt->destPort) {
				if (socket != NULL) {
					bzero(socket, sizeof(struct sockaddr_in));
					socket->sin_family = AF_INET;
					socket->sin_addr.s_addr = table[i].nextHop;
					socket->sin_port = table[i].nextHopPort;
				}
				nextEntry = &table[i];
				break;
			}
		}
		printf("no match\n\n");
	}
    return nextEntry;
}

int shouldForward(struct ip_packet *pkt) {
	struct table_entry *p = nextHop(pkt, NULL);
	if (p == NULL) return 0;
	return 1;
} 

// ----------------------------------------------------------------------------
// Parse the forwarding table file.
//   Builds a file_info struct for the specified file consisting of 
//   a linked list of file_entry structures that contain the location 
//   and sequence information from the tracker for the specified file.
// ----------------------------------------------------------------------------
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
		printf("read a line\n");
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
		/*printf("tokenized a line\n");
		printf("EMUL %d\n", EMULATOR);
		printf("EMULPORT %d\n", EMUL_PORT);
		printf("hostname %s\n", hostname);
		printf("port %d\n", port);
		printf("tok[em] %s\n", tokens[EMULATOR]);
		printf("tok[empo] %s\n", tokens[EMUL_PORT]);*/
        // Only process this line if it is for the specified emulator
        if (strcmp(tokens[EMULATOR], hostname) == 0 && atoi(tokens[EMUL_PORT]) == port) {
			printf("for this emul\n");
            struct raw_entry *entry = malloc(sizeof(struct raw_entry));
            bzero(entry, sizeof(struct raw_entry));
			printf("malloced entry\n");
			// TODO: type conversions likr atol or strtoul instead of atoi
            entry->dest         = strdup(tokens[DESTINATION]);
            entry->destPort     = atoi(tokens[DEST_PORT]);
			entry->nextHop      = strdup(tokens[NEXT_HOP]);
            entry->nextHopPort  = atoi(tokens[NEXT_HOP_PORT]);
            entry->delay 		= atoi(tokens[DELAY]);
            entry->lossChance 	= atoi(tokens[LOSS_CHANCE]);
			printf("set fields\n");
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
		table[i].delay = tmp->delay;
		table[i].lossChance = tmp->lossChance;
		printf("dest: %s %u     nexthop: %s %u   delay: %u  loss: %d\n", 
				tmp->dest, tmp->destPort, tmp->nextHop, tmp->nextHopPort, tmp->delay, tmp->lossChance);
		rawTable = tmp;
		tmp = tmp->nextEntry;
		free(rawTable);
	}
    return 1; // success
}


