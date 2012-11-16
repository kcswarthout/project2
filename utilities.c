#include <stdio.h>
#include <stdlib.h>

#include <sys/time.h>
#include <netinet/in.h>
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
    errcode = getaddrinfo(name, NULL, &hints, &info);
    if (errcode != 0) {
        fprintf(stderr, "Utilities getaddrinfo: %s\n", gai_strerror(errcode));
        exit(EXIT_FAILURE);
    }

    // Loop through all the results of getaddrinfo and try to create a socket for requester
    // NOTE: this is done so that we can find which of the getaddrinfo results is the requester
	
	int sockfd;
    struct addrinfo *p;
    for(p = requesterinfo; p != NULL; p = p->ai_next) {
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
	unsigned long addr = ip->sin_addr;
	return addr;
}