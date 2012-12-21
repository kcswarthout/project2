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

int main(int argc, char **argv) {
    if (argc != 13) {
        printf("usage: trace -a <routetrace port> -b < source hostname> -c <source port>");
        printf("-d <destination hostname> -e <destination port> -f <debug option>\n");
        exit(1);
    }

    char *portStr     = NULL;
    char *sHostName   = NULL;
    char *sPortStr    = NULL;
    char *dHostName   = NULL;
    char *dPortStr    = NULL;
    char *debugStr    = NULL;

    int cmd;
    while ((cmd = getopt(argc, argv, "a:b:c:d:e:f:")) != -1) {
        switch(cmd) {
            case 'a': portStr     = optarg; break;
            case 'b': sHostName   = optarg; break;
            case 'c': sPortStr    = optarg; break;
            case 'd': dHostName   = optarg; break;
            case 'e': dPortStr    = optarg; break;
            case 'f': debugStr    = optarg; break;
            case '?':
                if (optopt == 'a' || optopt == 'b' ||
                    optopt == 'c' || optopt == 'd' || optopt == 'e' || optopt == 'f')
                    fprintf(stderr, "Option -%c requires an argument.\n", optopt);
                else if (isprint(optopt))
                    fprintf(stderr, "Unknown option -%c.\n", optopt);
                else
                    fprintf(stderr, "Unknown option character '\\x%x'.\n", optopt);
                exit(EXIT_FAILURE);
            break;
            default: 
                printf("Unhandled argument: %d\n", cmd);
                exit(EXIT_FAILURE); 
        }
    }

    // DEBUG
    printf("Port: %s\n", portStr);

    // Convert program args to values
    int tracePort     = atoi(portStr);
    int srcPort       = atoi(sPortStr);
    int destPort      = atoi(dPortStr);
    int debug         = atoi(debugStr);

    // Validate the argument values
    if (tracePort <= 1024   ||  tracePort  >= 65536)
        ferrorExit("Invalid requester port");
    if (srcPort   <= 1024   ||  srcPort    >= 65536)
        ferrorExit("Invalid trace port");
    if (destPort  <= 1024   ||  destPort   >= 65536)
        ferrorExit("Invalid trace port");
        
    struct addrinfo thints;
    bzero(&thints, sizeof(struct addrinfo));
    thints.ai_family   = AF_INET;
    thints.ai_socktype = SOCK_DGRAM;
    thints.ai_flags    = AI_PASSIVE;


    // Get the trace's address info
    struct addrinfo *traceinfo;
    int errcode = getaddrinfo(NULL, portStr, &thints, &traceinfo);
    if (errcode != 0) {
        fprintf(stderr, "trace getaddrinfo: %s\n", gai_strerror(errcode));
        exit(EXIT_FAILURE);
    }

    // Loop through all the results of getaddrinfo and try to create a socket for trace
    int sockfd;
    struct addrinfo *p;
    for(p = traceinfo; p != NULL; p = p->ai_next) {
      // Try to create a new socket
      sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
      if (sockfd == -1) {
        perror("Socket error");
        continue;
      }
		
      // Try to bind the socket
      if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
        perror("Bind error");
        close(sockfd);
        continue;
      }
		
      break;
    }
    if (p == NULL) perrorExit("Send socket creation failed");
    else            printf("trace socket created.\n");
    
    struct sockaddr_in *tmp = (struct sockaddr_in *)p->ai_addr;
    unsigned long ipAddr = ntohl(tmp->sin_addr.s_addr);
    
    // ------------------------------------------------------------------------
    // Setup emul address info 
    struct addrinfo shints;
    bzero(&shints, sizeof(struct addrinfo));
    shints.ai_family   = AF_INET;
    shints.ai_socktype = SOCK_DGRAM;
    shints.ai_flags    = 0;

    // Get the emul's address info
    struct addrinfo *srcinfo;
    errcode = getaddrinfo(sHostName, sPortStr, &shints, &srcinfo);
    if (errcode != 0) {
        fprintf(stderr, "emul getaddrinfo: %s\n", gai_strerror(errcode));
        exit(EXIT_FAILURE);
    }

    // Loop through all the results of getaddrinfo and try to create a socket for emul
    int ssockfd;
    struct addrinfo *sp;
    for(sp = srcinfo; sp != NULL; sp = sp->ai_next) {
        // Try to create a new socket
        ssockfd = socket(sp->ai_family, sp->ai_socktype, sp->ai_protocol);
        if (ssockfd == -1) {
            perror("Socket error");
            continue;
        }
        break;
    }
    if (sp == NULL) perrorExit("emul socket creation failed");
    else            printf("emul socket created.\n");
    close(ssockfd);
	
    tmp = (struct sockaddr_in *)sp->ai_addr;
    //unsigned long sIpAddr = ntohl(tmp->sin_addr.s_addr);
    //printf("emul ip %s      %lu\n", inet_ntoa(tmp->sin_addr), eIpAddr);
    
    
    //trace packet construction
    printf("trace packet constructing\n");
		struct ip_packet *pkt = NULL;
    pkt = malloc(sizeof(struct ip_packet));
    bzero(pkt, sizeof(struct ip_packet));
    struct packet *dpkt = (struct packet *)pkt->payload;
    dpkt->type = 'T';
    dpkt->seq  = 0;
    dpkt->len  = (unsigned long) 0;
		pkt->src = ipAddr;
		pkt->srcPort = tracePort;
		pkt->dest = nameToAddr(dHostName);
		pkt->destPort = destPort;
		pkt->priority = 0;
		pkt->length = HEADER_SIZE + 1;
    
    struct ip_packet *rpkt = malloc(sizeof(struct ip_packet));
    void *msg = malloc(sizeof(struct ip_packet));
    char destS[20];
    char srcS[20];
    while (dpkt->len < 25) {
      if (debug) {
        ipULtoStr(pkt->src, srcS);
        ipULtoStr(pkt->dest, destS);
        printf("Trace pkt sent - TTL=%lu   Src=%s:%u   Dest=%s:%u\n", dpkt->len, srcS, pkt->srcPort, destS, pkt->destPort);
      }
      sendIpPacketTo(sockfd, pkt, sp->ai_addr);
      
      bzero(msg, sizeof(struct ip_packet));
			size_t bytesRecvd = recvfrom(sockfd, msg, sizeof(struct ip_packet), 0, NULL, NULL);
			if (bytesRecvd == -1) {
				perror("Recvfrom error");
				fprintf(stderr, "Failed/incomplete receive: ignoring\n");
				continue;
			}
      
      deserializeIpPacket(msg, rpkt);
      ipULtoStr(rpkt->src, srcS);
      if (debug) {
        ipULtoStr(rpkt->dest, destS);
        printf("Trace pkt recv - TTL=0   Src=%s:%u   Dest=%s:%u\n", srcS, rpkt->srcPort, destS, rpkt->destPort);
      }
      printf("%s:%u", srcS, rpkt->srcPort);
      if (rpkt->srcPort == destPort && rpkt->src == nameToAddr(dHostName)) {
        break;
      }
      dpkt->len++;
    }
    
    free(pkt);
		free(dpkt);
}
