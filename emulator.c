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
#include "utilities.h"
#include "packet.h"
#include "forwardtable.h"


int main(int argc, char **argv) {
    // ------------------------------------------------------------------------
    // Handle commandline arguments
    if (argc < 5) {
        printf("usage: emulator -p <port> -f <filename>\n");
        exit(1);
    }

    char *portStr    = NULL;
    const char *filename   = NULL;

    int cmd;
    while ((cmd = getopt(argc, argv, "p:f:")) != -1) {
        switch(cmd) {
            case 'p': portStr   = optarg; break;
            case 'f': filename  = optarg; break;
            case '?':
                if (optopt == 'p' || optopt == 'f')
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

    printf("Port        : %s\n", portStr);
    printf("File Name   : %s\n", filename);

    // Convert program args to values
    int emulPort  = atoi(portStr);
    int maxTime = 2500;
    int minTime = 500;

    // Validate the argument values
    if (emulPort <= 1024 ||emulPort >= 65536)
        ferrorExit("Invalid emul port");
    puts("");

    srand(time(NULL));
    initLog("log_file.txt");
	
    // ------------------------------------------------------------------------
    // Setup emul address info 
    struct addrinfo ehints;
    bzero(&ehints, sizeof(struct addrinfo));
    ehints.ai_family   = AF_INET;
    ehints.ai_socktype = SOCK_DGRAM;
    ehints.ai_flags    = AI_PASSIVE;

    // Get the emul's address info
    struct addrinfo *emulinfo;
    int errcode = getaddrinfo(NULL, portStr, &ehints, &emulinfo);
    if (errcode != 0) {
        fprintf(stderr, "emul getaddrinfo: %s\n", gai_strerror(errcode));
        exit(EXIT_FAILURE);
    }

    // Loop through all the results of getaddrinfo and try to create a socket for emul
    int sockfd;
    struct addrinfo *sp;
    for(sp = emulinfo; sp != NULL; sp = sp->ai_next) {
        // Try to create a new socket
        sockfd = socket(sp->ai_family, sp->ai_socktype, sp->ai_protocol);
        if (sockfd == -1) {
            perror("Socket error");
            continue;
        }
		
		// Try to bind the socket
        if (bind(sockfd, sp->ai_addr, sp->ai_addrlen) == -1) {
            perror("Bind error");
            close(sockfd);
            continue;
        }
		
        break;
    }
    if (sp == NULL) perrorExit("Send socket creation failed");
    else            printf("emul socket created.\n");

	struct sockaddr_in *tmp = (struct sockaddr_in *)sp->ai_addr;
	unsigned long eIpAddr = tmp->sin_addr.s_addr;
	
  initTable(filename, tmp);

exit(0);
  // ------------------------------------------------------------------------
	// The Big Loop of DOOM

	struct sockaddr_in *nextSock;
  int shouldForward;
	fd_set fds;
	
  struct timespec *tv = malloc(sizeof(struct timespec));
	tv->tv_sec = (long) 1000;
	tv->tv_nsec = 0;
  int retval = 0;
	int numRecv = 0;
	unsigned long long start;
  struct packet *dpkt;
  struct ip_packet *pkt = malloc(sizeof(struct ip_packet));
	void *msg = malloc(sizeof(struct ip_packet));
  for (;;) {
		FD_ZERO(&fds);
		FD_SET(sockfd, &fds);
		start = getTimeMS();
		retval = pselect(sockfd + 1, &fds, NULL, NULL, tv, NULL);
	
		// ------------------------------------------------------------------------
		// receiving half
        
		if (retval > 0) {
			// Receive and forward packet
			printf("retval > 0\n");
			bzero(msg, sizeof(struct ip_packet));
			size_t bytesRecvd = recvfrom(sockfd, msg, sizeof(struct ip_packet), 0, NULL, NULL);
			if (bytesRecvd == -1) {
				perror("Recvfrom error");
				fprintf(stderr, "Failed/incomplete receive: ignoring\n");
				continue;
			}
			
			// Deserialize the message into a packet 
			bzero(pkt, sizeof(struct ip_packet));
			deserializeIpPacket(msg, pkt);
			dpkt = (struct packet *)pkt->payload;
			printIpPacketInfo(pkt, NULL);
      
      // Check packet type to see if any action needs to be taken
      nextSock = malloc(sizeof(struct sockaddr_in));
      if (dpkt->type == 'T') {
        if (dpkt->len == 0) {
          bzero(nextSock, sizeof(struct sockaddr_in));
          shouldForward = 1;
          nextSock->sin_family = AF_INET;
					nextSock->sin_addr.s_addr = htonl(pkt->src);
					nextSock->sin_port = htons(pkt->srcPort);
          pkt->src = eIpAddr;
          pkt->srcPort = emulPort;
        }
        else {
          dpkt->len--;
          shouldForward = nextHop(pkt, nextSock);
        }
      }
      else if (dpkt->type == 'S') {
        
        shouldForward = nextHop(pkt, nextSock);
      }
      else {
        shouldForward = nextHop(pkt, nextSock);
      }
      // Forward the packet if there is an entry for it
			if (shouldForward)	{
        printf("send packet\n");
        //printf("socket is %lu  %u", nextSock->sin_addr.s_addr, nextSock->sin_port);
        sendIpPacketTo(sockfd, pkt, (struct sockaddr*)nextSock);
        free(nextSock);
			}
			else {
				logP(pkt, "No forwarding entry found");
			}
			
      // update timespec
			long sec = tv->tv_sec - (long)((getTimeMS() - start) / 1000);
			long nsec = tv->tv_nsec - (long)(1000000 * ((getTimeMS() - start) % 1000));
			if (nsec < 0) {
				nsec = 1000000 * 1000 + nsec;
				sec--;
				
			}
			if (sec < 0 || !numRecv) {
					sec = 0;
					nsec = 0;
			}
			tv->tv_sec = sec;
			tv->tv_nsec = nsec;
		}
		else if (retval == 0) {
			// ------------------------------------------------------------------------
			// refresh forward table
			printf("retval == 0\n");
			
      createRoutes();
      
      int delay = minTime + (rand() % (maxTime - minTime));
      tv->tv_sec = (long)delay / 1000;
      tv->tv_nsec = (long)(delay % 1000) * 1000000;
		}
		else {
			//printf("Sockfd = %d\n", sockfd);
			//printf("tv%d  delay=%li s   %li us\n", x, tv->tv_sec, tv->tv_nsec);
			perrorExit("Select()");
		}
  }
	// Cleanup packets
  free(pkt);
  free(msg);
}

