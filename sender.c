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
    // ------------------------------------------------------------------------
    // Handle commandline arguments
    if (argc < 11) {
        printf("usage: sender -p <port> -g <requester port> ");
        printf("-r <rate> -q <seq_no> -l <length>\n");
        exit(1);
    }

    char *portStr    = NULL;
    char *reqPortStr = NULL;
    char *rateStr    = NULL;
    char *seqNumStr  = NULL;
    char *lenStr     = NULL;
    char *fHostName  = NULL;
    char *fPortStr   = NULL;
    char *priorStr   = NULL;
    char *timeoutStr = NULL;

    int cmd;
    while ((cmd = getopt(argc, argv, "p:g:r:q:l:f:h:i:t:")) != -1) {
        switch(cmd) {
            case 'p': portStr    = optarg; break;
            case 'g': reqPortStr = optarg; break;
            case 'r': rateStr    = optarg; break;
            case 'q': seqNumStr  = optarg; break;
            case 'l': lenStr     = optarg; break;
            case 'f': fHostName  = optarg; break;
            case 'h': fPortStr   = optarg; break;
            case 'i': priorStr   = optarg; break;
            case 't': timeoutStr = optarg; break;
            case '?':
                if (optopt == 'p' || optopt == 'g' || optopt == 'r' || optopt == 'q' || optopt == 'l' 
								  || optopt == 'f' || optopt == 'h' || optopt == 'i' || optopt == 't')
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

    printf("Port           : %s\n", portStr);
    printf("Requester Port : %s\n", reqPortStr);
    printf("Rate           : %s\n", rateStr);
    printf("Sequence #     : %s\n", seqNumStr);
    printf("Length         : %s\n", lenStr);

    // Convert program args to values
    int senderPort    		= atoi(portStr);
    int requesterPort 		= atoi(reqPortStr);
    int emulPort      		= atoi(fPortStr);
    int sequenceNum			= atoi(seqNumStr);
    int payloadLen    		= atoi(lenStr);
    unsigned sendRate 		= (unsigned) atoi(rateStr);
	unsigned char priority 	= priorityBin(atoi(priorStr));
	unsigned int timeout    = (unsigned) atoi(timeoutStr);	

	sequenceNum = 1;
	
    // Validate the argument values
    if (senderPort <= 1024 || senderPort >= 65536)
        ferrorExit("Invalid sender port");
    if (requesterPort <= 1024 || requesterPort >= 65536)
        ferrorExit("Invalid requester port");
    if (sendRate > 1000 || sendRate < 1) 
        ferrorExit("Invalid sendrate");
    puts("");

    // ------------------------------------------------------------------------
    // Setup sender address info 
    struct addrinfo shints;
    bzero(&shints, sizeof(struct addrinfo));
    shints.ai_family   = AF_INET;
    shints.ai_socktype = SOCK_DGRAM;
    shints.ai_flags    = AI_PASSIVE;

    // Get the sender's address info
    struct addrinfo *senderinfo;
    int errcode = getaddrinfo(NULL, portStr, &shints, &senderinfo);
    if (errcode != 0) {
        fprintf(stderr, "sender getaddrinfo: %s\n", gai_strerror(errcode));
        exit(EXIT_FAILURE);
    }

    // Loop through all the results of getaddrinfo and try to create a socket for sender
    int sockfd;
    struct addrinfo *sp;
    for(sp = senderinfo; sp != NULL; sp = sp->ai_next) {
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
    else            printf("Sender socket created.\n");

	struct sockaddr_in *tmp = (struct sockaddr_in *)sp->ai_addr;
	unsigned long sIpAddr = tmp->sin_addr.s_addr;
	
	// ------------------------------------------------------------------------
    // Setup emul address info 
    struct addrinfo ehints;
    bzero(&ehints, sizeof(struct addrinfo));
    ehints.ai_family   = AF_INET;
    ehints.ai_socktype = SOCK_DGRAM;
    ehints.ai_flags    = 0;

    // Get the emul's address info
    struct addrinfo *emulinfo;
    errcode = getaddrinfo(fHostName, fPortStr, &ehints, &emulinfo);
    if (errcode != 0) {
        fprintf(stderr, "emul getaddrinfo: %s\n", gai_strerror(errcode));
        exit(EXIT_FAILURE);
    }

    // Loop through all the results of getaddrinfo and try to create a socket for emul
    int esockfd;
    struct addrinfo *esp;
    for(esp = emulinfo; esp != NULL; esp = esp->ai_next) {
        // Try to create a new socket
        sockfd = socket(esp->ai_family, esp->ai_socktype, esp->ai_protocol);
        if (sockfd == -1) {
            perror("Socket error");
            continue;
        }

        // Try to bind the socket
        if (bind(sockfd, esp->ai_addr, esp->ai_addrlen) == -1) {
            perror("Bind error");
            close(sockfd);
            continue;
        }

        break;
    }
    if (esp == NULL) perrorExit("Send socket creation failed");
    else            printf("emul socket created.\n");
	
	
    // -----------------------------===========================================
    // REQUESTER ADDRESS INFO
    struct addrinfo rhints;
    bzero(&rhints, sizeof(struct addrinfo));
    rhints.ai_family   = AF_INET;
    rhints.ai_socktype = SOCK_DGRAM;
    rhints.ai_flags    = 0;

    struct addrinfo *requesterinfo;
    errcode = getaddrinfo(NULL, reqPortStr, &rhints, &requesterinfo);
    if (errcode != 0) {
        fprintf(stderr, "requester getaddrinfo: %s\n", gai_strerror(errcode));
        exit(EXIT_FAILURE);
    }

    // Loop through all the results of getaddrinfo and try to create a socket for requester
    // NOTE: this is done so that we can find which of the getaddrinfo results is the requester
    int requestsockfd;
    struct addrinfo *rp;
    for(rp = requesterinfo; rp != NULL; rp = rp->ai_next) {
        requestsockfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (requestsockfd == -1) {
            perror("Socket error");
            continue;
        }

        break;
    }
    if (sp == NULL) perrorExit("Requester lookup failed to create socket");
    //else            printf("Requester socket created.\n\n");
    close(requestsockfd); // don't need this socket

	struct sockaddr_in *tmp = (struct sockaddr_in *)rp->ai_addr;
	unsigned long rIpAddr = tmp->sin_addr.s_addr;
	
    // ------------------------------------------------------------------------
    puts("Sender waiting for request packet...\n");

    // Receive and discard packets until a REQUEST packet arrives
	unsigned long window;
    char *filename = NULL;
    for (;;) {
        void *msg = malloc(sizeof(struct ip_packet));
        bzero(msg, sizeof(struct ip_packet));

        // Receive a message
        size_t bytesRecvd = recvfrom(sockfd, msg, sizeof(struct ip_packet), 0,
            (struct sockaddr *)esp->ai_addr, &esp->ai_addrlen);
        if (bytesRecvd == -1) {
            perror("Recvfrom error");
            fprintf(stderr, "Failed/incomplete receive: ignoring\n");
            continue;
        }

        // Deserialize the message into a ip_packet 
        struct ip_packet *pkt = malloc(sizeof(struct ip_packet));
        bzero(pkt, sizeof(struct ip_packet));
        deserializeIpPacket(msg, pkt);

        // Check for REQUEST packet
        if (pkt->payload->type == 'R') {
            // Print some statistics for the recvd packet
            printf("<- [Received REQUEST]: ");
            printPacketInfo(pkt->payload, (struct sockaddr_storage *)esp->ai_addr);

            // Grab a copy of the filename
            filename = strdup(pkt->payload->payload);
			window = pkt->payload->len;
            // Cleanup packets
            free(pkt);
            free(msg);
            break;
        }

        // Cleanup packets
        free(pkt);
        free(msg);
    }

    // ------------------------------------------------------------------------
    // Got REQUEST packet, start sending DATA packets
    // ------------------------------------------------------------------------

    // Open file for reading
    FILE *file = fopen(filename, "r");
    if (file == NULL) perrorExit("File open error");
    else              printf("Opened file \"%s\" for reading.\n", filename);

	fd_set fds;
	FD_ZERO(&fds);
	FD_SET(sockfd, &fds);
	struct timeval *tv = malloc(sizeof(struct timeval));
	tv->tv_sec = 0;
	tv->tv_usec = 0;
	int retval = 0;
	
	int numSent = 0;
	int numLost = 0;
	int windowStart = 1;
	int windowDone = 1;
	int fileDone = 0;
	int loopCont = 1;
	struct packet **buffer = malloc(window * (sizeof (void *)));
	unsigned long long *buffTimer = malloc(window * (sizeof (int)));
	int *buffTOCount = malloc(window * (sizeof (int)));
    unsigned long long start = getTimeMS();
	unsigned long long timeoutEnd;
    struct packet *pkt;
	struct ip_packet *spkt = malloc(sizeof(struct ip_packet));
	struct ip_packet *msg = malloc(sizeof(struct ip_packet));
    while (loopCont) {
		retval = select(sockfd + 1, &fds, NULL, NULL, tv);
	
		// ------------------------------------------------------------------------
		// receiving half
        
		if (retval > 0) {
			// Receive a message
			bzero(msg, sizeof(struct ip_packet));
			size_t bytesRecvd = recvfrom(sockfd, msg, sizeof(struct ip_packet), 0, NULL, NULL);
			if (bytesRecvd == -1) {
				perror("Recvfrom error");
				fprintf(stderr, "Failed/incomplete receive: ignoring\n");
				continue;
			}
			// Deserialize the message into a packet 
			bzero(spkt, sizeof(struct ip_packet));
			deserializeIpPacket(msg, spkt);
			free(buffer[spkt->payload->seq - start]);
			buffer[spkt->payload->seq - start] = NULL;
		}
		else {
			// ------------------------------------------------------------------------
			// sending half
			if (sequenceNum >= window + windowStart) {
				int buffIndex;
				windowDone = 1;
				for (buffIndex = 0; buffIndex < window; buffIndex++) {	
					if (buffer[buffIndex] != NULL) {
						if (buffTimer[buffIndex] <= getTimeMS()) {
							numLost++;
							if ([buffIndex] >= 5) {
								free(buffer[buffIndex]);
								buffer[buffIndex] = NULL;
							}
							else {
								buffTOCount[buffIndex]++;
								pkt = buffer[buffIndex];
								windowDone = 0;
								if (timeoutEnd > buffTimer[buffIndex]) {
									timeoutEnd = buffTimer[buffIndex];
								}
							}
						}
						else {
							windowDone = 0;
						}
					}
				}
				if (windowDone) {
					windowStart += window;
				}
				else if ((1000 * sendRate) < (timeoutEnd - getTimeMS())) { 
					tv->tv_usec = timeoutEnd - getTimeMS();
				}
				else {
					tv->tv_usec = 1000 * sendRate;
				}
			}
			else if (!fileDone) {
				// Is file part finished?
				if (feof(file) != 0) {
					sequenceNum = window + windowStart;
					fileDone = 1;
					continue;
				}
				else {
					// Create DATA packet
					pkt = malloc(sizeof(struct packet));
					bzero(pkt, sizeof(struct packet));
					pkt->type = 'D';
					pkt->seq  = sequenceNum;
					pkt->len  = payloadLen;

					// Chunk the next batch of file data into this packet
					char buf[payloadLen];
					bzero(buf, payloadLen);
					fread(buf, 1, payloadLen, file); // TODO: check return value
					memcpy(pkt->payload, buf, sizeof(buf));
					buffer[pkt->seq - start] = pkt;
					buffTimer[pkt->seq - start] = timeout + getTimeMS();
					buffTOCount[pkt->seq - start] = 0;
					
					// Update sequence number for next packet
					sequenceNum++;
					
					/*
					printf("[Packet Details]\n------------------\n");
					printf("type : %c\n", pkt->type);
					printf("seq  : %lu\n", pkt->seq);
					printf("len  : %lu\n", pkt->len);
					printf("payload: %s\n\n", pkt->payload);
					*/
				}
				tv->tv_usec = 1000 * sendRate;
			}
			else {
				// Create END packet and send it
				pkt = malloc(sizeof(struct packet));
				bzero(pkt, sizeof(struct packet));
				pkt->type = 'E';
				pkt->seq  = 0;
				pkt->len  = 0;
				loopCont = 0;
			}
			// Send the packet to the requester 
			if (pkt != NULL) {
				bzero(spkt, sizeof(struct ip_packet));
				spkt->src = sIpAddr;
				spkt->srcPort = senderPort;
				spkt->dest = rIpAddr;
				spkt->destPort = requesterPort;
				spkt->priority = priority;
				spkt->length = HEADER_SIZE + pkt->len;
				memcpy(spkt->payload, pkt, sizeof(struct packet));
				sendIpPacketTo(sockfd, spkt, (struct sockaddr *)esp->ai_addr);
				numSent++;
				pkt = NULL;
			}
        }
    }

	free(spkt);
	free(msg);
	
    // Cleanup the file
    if (fclose(file) != 0) fprintf(stderr, "Failed to close file \"%s\"\n", filename);
    else                   printf("File \"%s\" closed.\n", filename);
    free(filename);


    // Got what we came for, shut it down
    if (close(sockfd) == -1) perrorExit("Close error");
    else                     puts("Connection closed.\n");

    // Cleanup address info data
    freeaddrinfo(senderinfo);

    // All done!
    exit(EXIT_SUCCESS);
}

