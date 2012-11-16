#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "forwardtable.h"
#include "utilities.h"

#define TRACKER "tracker.txt"
#define TOK_PER_LINE 8

enum token {EMULATOR, EMUL_PORT, DESTINATION, DEST_PORT, NEXT_HOP, NEXT_HOP_PORT, DELAY, LOSS_CHANCE};

table_entry *table = NULL;
int size = 0;

struct sockaddr_in *nextHop(ip_packet *pkt) {
	if (table == NULL) perrorExit("No table entries");
	if (pkt == NULL) perrorExit("nextHop function: pkt null");	
	sockaddr_in *nextHop = malloc(sizeof(struct sockaddr_in));
    bzero(nextHop, sizeof(struct sockaddr_in));
	nextHop->sin_family = AF_INET;
	int i;
	for (i = 0; i < size; i++) {
		if (table[i]->dest == pkt->sin_addr) {
			if (table[i]->destPort == pkt->sin_port) P
					nextHop->sin_addr = table[i]->nextHop;
					nextHop->sin_port = table[i]->nextHopPort;
					return nextHop;
			}
		}
	}
	nextHop = NULL;
    return nextHop;
}

bool shouldForward(ip_packet *pkt) {
	ip_packet *p = nextHop(pkt);
	if (p == NULL) return false;
	return true;
} 

// ----------------------------------------------------------------------------
// Parse the forwarding table file.
//   Builds a file_info struct for the specified file consisting of 
//   a linked list of file_entry structures that contain the location 
//   and sequence information from the tracker for the specified file.
// ----------------------------------------------------------------------------
bool *parseFile(const char *filename, char *hostname, unsigned int port) {
    if (filename == NULL) ferrorExit("ParseTracker: invalid filename");

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
        // Tokenize line
        int n = -1; // TODO: should this be 0 or -1?
        char *tokens[TOK_PER_LINE];
        char *tok = strtok(line, " ");
        while (tok != NULL) {
            tokens[n++] = tok;
            tok  = strtok(NULL, " ");
        }

        // Only process this line if it is for the specified emulator
        if (strcmp(tokens[EMULATOR], hostname) == 0 && atoi(tokens[EMUL_PORT]) == port) {
            struct raw_entry *entry = malloc(sizeof(struct file_entry));
            bzero(entry, sizeof(struct file_entry));
			// TODO: type conversions likr atol or strtoul instead of atoi
            entry->dest         = strdup(tokens[DESTINATION]);
            entry->destPort     = atoi(tokens[DEST_PORT]);
			entry->nextHop      = strdup(tokens[NEXT_HOP]);
            entry->nextHopPort  = atoi(tokens[NEXT_HOP_PORT]);
            entry->delay 		= atoi(tokens[DELAY]);
            entry->lossChance 	= atoi(tokens[LOSS_CHANCE]);

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
		rawTable = tmp;
		tmp = tmp->nextEntry;
		free(rawTable);
	}
	free(rawTable);
    return info; // success
}

// ----------------------------------------------------------------------------
void printFileInfo(struct file_info *info) {
    if (info == NULL) {
        fprintf(stderr, "Cannot print null file_info.\n");
        return;
    }

    printf("file_info for \"%s\" [%p]:\n", info->filename, info);
    printf("--------------------------------------\n");
    struct file_entry *entry = info->entrys;
    while (entry != NULL) {
        printFileentryInfo(entry);
        entry = entry->next_entry;
    }

    puts("");
}

// ----------------------------------------------------------------------------
void printFileentryInfo(struct file_entry *entry) {
    if (entry == NULL) {
        fprintf(stderr, "Cannot print null file_entry info.\n");
        return;
    }

    printf("  file_entry info   : [%p]\n", entry);
    printf("    id             : %d\n", entry->id);
    printf("    sender hostname: %s\n", entry->sender_hostname);
    printf("    sender port    : %d\n", entry->sender_port);
    printf("    next entry      : [%p]\n", entry->next_entry);
}

// ----------------------------------------------------------------------------
void freeFileInfo(struct file_info *info) {
    if (info == NULL) return;

    // Walk the entrys list, freeing each entry
    struct file_entry *entry = info->entrys;
    while (entry != NULL) {
        struct file_entry *p = entry;
        entry = entry->next_entry;
        free(p->sender_hostname);
        free(p);
    }

    free(info->filename);
    free(info);
}
