#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <errno.h>
#include <time.h>
#include <fcntl.h>
#include <strings.h>
#include <string.h>

#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include "utilities.h"

unsigned long long getTimeMS() {
    struct timeval t;
    gettimeofday(&t, NULL);
    return (unsigned long long)((t.tv_sec + (t.tv_usec / 1000000.0)) * 1000.0);
}

// ----------------------------------------------------------------------------
// Print hostname and port to stdout
// ----------------------------------------------------------------------------
void printNameInfo(struct addrinfo *addr) {
    if (addr == NULL) {
        fprintf(stderr, "Cannot print name info from null sockaddr\n");
        return;
    }

    char host[NI_MAXHOST], service[NI_MAXSERV];
    int s = getnameinfo((struct sockaddr *)addr->ai_addr,
                        addr->ai_addrlen, host, NI_MAXHOST,
                        service, NI_MAXSERV, NI_NUMERICSERV);
    if (s != 0)
        fprintf(stderr, "getnameinfo: %s\n", gai_strerror(s));
    else {
        printf("Connected to %s:%s\n", host, service);
    }
}

// ----------------------------------------------------------------------------
// Print error from errno and exit with a failure indication
// ----------------------------------------------------------------------------
void perrorExit(const char *msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}

// ----------------------------------------------------------------------------
// Print error to stderr and exit with a failure indication
// ----------------------------------------------------------------------------
void ferrorExit(const char *msg) {
    fputs(msg, stderr);
    exit(EXIT_FAILURE);
}

unsigned long nameToAddr(char *name) {
    struct addrinfo hints;
    bzero(&hints, sizeof(struct addrinfo));
    hints.ai_family   = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags    = 0;

    struct addrinfo *info;
    int errcode = getaddrinfo(name, NULL, &hints, &info);
    if (errcode != 0) {
        fprintf(stderr, "Utilities getaddrinfo: %s\n", gai_strerror(errcode));
        exit(EXIT_FAILURE);
    }

    // Loop through all the results of getaddrinfo and try to create a socket for requester
    // NOTE: this is done so that we can find which of the getaddrinfo results is the requester
	
	int sockfd;
    struct addrinfo *p;
    for(p = info; p != NULL; p = p->ai_next) {
        sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (sockfd == -1) {
            perror("Socket error");
            continue;
        }

        break;
    }
    if (p == NULL) perrorExit("Requester lookup failed to create socket");
    //else            printf("Requester socket created.\n\n");
    close(sockfd); // don't need this socket
	
    struct sockaddr_in *ip = (struct sockaddr_in *)info->ai_addr;
	unsigned long addr = ntohl(ip->sin_addr.s_addr);
	return addr;
}

char* addrToName(unsigned long addr) {
	struct addrinfo hints;
    bzero(&hints, sizeof(struct addrinfo));
    hints.ai_family   = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags    = 0;
	
	char ipStr[16];
	inet_ntop(AF_INET, &addr, ipStr, sizeof ipStr);
	
    struct addrinfo *info;
    int errcode = getaddrinfo(ipStr, NULL, &hints, &info);
    if (errcode != 0) {
        fprintf(stderr, "Utilities getaddrinfo: %s\n", gai_strerror(errcode));
        exit(EXIT_FAILURE);
    }

    // Loop through all the results of getaddrinfo and try to create a socket for requester
    // NOTE: this is done so that we can find which of the getaddrinfo results is the requester
	
	int sockfd;
    struct addrinfo *p;
    for(p = info; p != NULL; p = p->ai_next) {
        sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (sockfd == -1) {
            perror("Socket error");
            continue;
        }

        break;
    }
    if (p == NULL) perrorExit("Requester lookup failed to create socket");
    //else            printf("Requester socket created.\n\n");
    close(sockfd); // don't need this socket
	
	return info->ai_canonname;
}

void ipULtoStr(unsigned long ip, char *buffer)
{
    unsigned char bytes[4];
    bytes[0] = ip & 0xFF;
    bytes[1] = (ip >> 8) & 0xFF;
    bytes[2] = (ip >> 16) & 0xFF;
    bytes[3] = (ip >> 24) & 0xFF;	
    sprintf(buffer, "%d.%d.%d.%d", bytes[3], bytes[2], bytes[1], bytes[0]);   
}
