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
		perror("Log file open error");
		return 0;
	}
	
	puts("\nLog file opened.");
	return 1;
}

void log(struct ip_packet *pkt, char *str) {
	fprintf((FILE*)file, "Packet dropped at time %ull ms\n     Priority: %uhh     Size: %ul\n     Dest: %s %ui\n     Src:  %s %ui\n     Cause: %s\n", getTimeMS(), pkt->priority, pkt->length, addrToName(pkt->dest), pkt->destPort, addrToName(pkt->src), pkt->srcPort, str);
}