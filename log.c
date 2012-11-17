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

#include "log.h"

File *file = NULL;

bool initLog(char *filename) {
	file = fopen(filename, "a");
    if (file == NULL) {
		perror("Log file open error");
		return false;
	}
	
	puts("\nLog file opened.");
	return true;
}

void log(ip_packet pkt, char *str) {
	fprintf("Packet dropped at time %ull ms\n     Priority: %uhh     Size: %ul\n     Dest: %s %ui\n     Src:  %s %ui\n     Cause: %s\n",
		getTimeMS(), pkt->priority, pkt->length, addrToName(pkt->dest), pkt->destPort, addrToName(pkt->src), pkt->srcPort, str);
}