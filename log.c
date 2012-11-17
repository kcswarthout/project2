#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>

#include "log.h"
#include "packet.h"

void *file = NULL;

int initLog(char *filename) {
	file = fopen(filename, "a");
    if (file == NULL) {
		perror("log file open error");
		return 0;
	}
	
	puts("\nlog file opened.");
	return 1;
}

void logP(struct ip_packet *pkt, char *str) {
	fprintf((FILE*)file, 
		"Packet dropped at time %llu ms\n     Priority: %hhu     Size: %lu \n     Dest: %s %iu \n     Src:  %s %iu \n     Cause: %s \n", 
		getTimeMS(), pkt->priority, pkt->length, addrToName(pkt->dest), pkt->destPort, addrToName(pkt->src), pkt->srcPort, str);
}