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
    if (argc < 11) {
        printf("usage: emul -p <port> -g <requester port> ");
        printf("-r <rate> -q <seq_no> -l <length>\n");
        exit(1);
    }

    char *portStr    = NULL;
    char *queueStr 	 = NULL;
    char *filename   = NULL;
    char *logname	 = NULL;

    int cmd;
    while ((cmd = getopt(argc, argv, "p:q:f:l:")) != -1) {
        switch(cmd) {
            case 'p': portStr   = optarg; break;
            case 'q': queueStr  = optarg; break;
            case 'f': filename  = optarg; break;
            case 'l': logname    = optarg; break;
            case '?':
                if (optopt == 'p' || optopt == 'q' || optopt == 'f'
                 ||  optopt == 'l')
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
    printf("Queue       : %s\n", queueStr);
    printf("File Name   : %s\n", filename);
    printf("Log         : %s\n", logname);

    // Convert program args to values
    int emulPort  = atoi(portStr);
    int queueLength   = atoi(queueStr);

    // Validate the argument values
    if (emulPort <= 1024 ||emulPort >= 65536)
        ferrorExit("Invalid emul port");
    if (queueLength < 0) 
        ferrorExit("Invalid queueLength");
    puts("");

	srand(time(NULL));
	initLog(filename);
	
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

	parseFile(filename, sp->ai_canonname, emulPort);
	
	

    // ------------------------------------------------------------------------
	// The Big Loop of DOOM

	int queuePtr[3][2] = {{0}};
	int queueFull[3] = {0};
	struct ip_packet **queue = malloc(3 * queueLength * (sizeof (void *)));
	unsigned char  priority[3] = {HIGH_PRIORITY, MEDIUM_PRIORITY, LOW_PRIORITY};
	
	struct ip_packet *currPkt = NULL;
	struct sockaddr_in *nextSock = malloc(sizeof(struct sockaddr_in));
	struct table_entry *currEntry = NULL;
	
	fd_set fds;
	FD_ZERO(&fds);
	FD_SET(sockfd, &fds);
    struct timeval *tv = malloc(sizeof(struct tv));
	tv->tv_sec = 10000;
	tv->tv_usec = 0;
    int retval = 0;
	int i;
	struct ip_packet *pkt = malloc(sizeof(struct ip_packet));
	void *msg = malloc(sizeof(struct ip_packet));
    for (;;) {
		// ------------------------------------------------------------------------
		// receiving half
        
        bzero(msg, sizeof(struct ip_packet));

		retval = select(sockfd + 1, &fds, NULL, NULL, tv);
        
		if (retval > 0) {
			// Receive a message
			size_t bytesRecvd = recvfrom(sockfd, msg, sizeof(struct ip_packet), 0, NULL, NULL);
			if (bytesRecvd == -1) {
				perror("Recvfrom error");
				fprintf(stderr, "Failed/incomplete receive: ignoring\n");
				continue;
			}
			// Deserialize the message into a packet 
			pkt = malloc(sizeof(struct ip_packet));
			bzero(pkt, sizeof(struct ip_packet));
			deserializeIpPacket(msg, pkt);
			if (shouldForward(pkt))	{
				for (i = 0; i < 3; i++) {
					if (pkt->priority == priority[i]) {
						if (queueFull[i]) {
							char tmpStr[30] = {'\0'};
							sprintf(tmpStr, "Priority queue %d full", i);
							log(pkt, tmpStr);
						}
						else {
							queue[(i*queueLength) + queuePtr[i][1]] = pkt;
							queuePtr[i][1]++;
							if (queuePtr[i][1] == queueLength) {
								queuePtr[i][1] = 0;
							}
							if (queuePtr[i][1] == queuePtr[i][0]) {
								queueFull[i] = 1;
							}
						}
						break;
					}
				}
				if (i == 3) {
					perror("Invalid packet priority value");
					log(pkt, "Invalid packet priority value");
				}
			}
			else {
				log(pkt, "No forwarding entry found");
			}
			
		}
		else if (retval == 0) {
			// ------------------------------------------------------------------------
			// sending half
			if (currPkt != NULL) {
				int lossChance = currEntry->lossChance;
				// Determine if packet should be dropped
				if (currPkt->payload->type == 'E') {
					lossChance = 0;
				}
				if (lossChance > rand() % 100) {
					log(currPkt, "Loss event occurred");
				}
				else {
					sendIpPacketTo(sockfd, currPkt, (struct sockaddr*)nextSock);
					free(currPkt);
					currPkt = NULL;
				}
			}
	
		}
		else {
			perror("Select()");
		}
		
		if (currPkt == NULL) {
			for (i = 0; i < 3; i++) {
				if (queuePtr[i][0] != queuePtr[i][1] || queueFull[i]) {
					currPkt = queue[(i*queueLength) + queuePtr[i][1]];
					queue[(i*queueLength) + queuePtr[i][1]] = NULL;
					queuePtr[i][0]++;
					if (queuePtr[i][0] == queueLength) {
						queuePtr[i][0] = 0;
					}
					queueFull[i] = 0;
					currEntry = nextHop(currPkt, nextSock);
					tv->tv_sec = currEntry->delay / 1000;
					tv->tv_usec = (currEntry->delay % 1000) * 1000;
				}
			}
		}
    }
	// Cleanup packets
    free(pkt);
    free(msg);
}

