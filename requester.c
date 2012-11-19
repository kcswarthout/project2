#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <errno.h>
#include <time.h>
#include <strings.h>
#include <string.h>
#include <assert.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include "utilities.h"
#include "tracker.h"
#include "packet.h"


int main(int argc, char **argv) {
    // ------------------------------------------------------------------------
    // Handle commandline arguments
    if (argc != 11) {
        printf("usage: requester -p <port> -o <file option>\n");
        exit(1);
    }

    char *portStr    = NULL;
    char *fHostName  = NULL;
    char *fPortStr   = NULL;
    char *fileOption = NULL;
    char *windowStr  = NULL;

    int cmd;
    while ((cmd = getopt(argc, argv, "p:f:h:o:w:")) != -1) {
        switch(cmd) {
            case 'p': portStr    = optarg; break;
            case 'f': fHostName  = optarg; break;
            case 'h': fPortStr   = optarg; break;
            case 'o': fileOption = optarg; break;
            case 'w': windowStr  = optarg; break;
            case '?':
                if (optopt == 'p' || optopt == 'f' || optopt == 'h' || optopt == 'o' || optopt == 'w')
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
    printf("File: %s\n", fileOption);

    // Convert program args to values
    int requesterPort = atoi(portStr);
    int emulPort = atoi(fPortStr);
    int window = atoi(windowStr);

    // Validate the argument values
    if (requesterPort <= 1024 || requesterPort >= 65536)
        ferrorExit("Invalid requester port");
	if (emulPort <= 1024 || emulPort >= 65536)
        ferrorExit("Invalid emulator port");
		
    // ------------------------------------------------------------------------

    // Parse the tracker file for parts corresponding to the specified file
    struct file_info *fileParts = parseTracker(fileOption);
    assert(fileParts != NULL && "Invalid file_info struct");

    // ------------------------------------------------------------------------
    // Setup requester address info 
    struct addrinfo rhints;
    bzero(&rhints, sizeof(struct addrinfo));
    rhints.ai_family   = AF_INET;
    rhints.ai_socktype = SOCK_DGRAM;
    rhints.ai_flags    = AI_PASSIVE;

    // Get the requester's address info
	char hostname[256];
	hostname[255] = '\0';
	gethostname(hostname, 255);

    struct addrinfo *requesterinfo;
    int errcode = getaddrinfo(hostname, portStr, &rhints, &requesterinfo);
    if (errcode != 0) {
        fprintf(stderr, "requester getaddrinfo: %s\n", gai_strerror(errcode));
        exit(EXIT_FAILURE);
    }

    // Loop through all the results of getaddrinfo and try to create a socket for requester
    int sockfd;
    struct addrinfo *rp;
    for(rp = requesterinfo; rp != NULL; rp = rp->ai_next) {
        // Try to create a new socket
        sockfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (sockfd == -1) {
            perror("Socket error");
            continue;
        }

        // Try to bind the socket
        if (bind(sockfd, rp->ai_addr, rp->ai_addrlen) == -1) {
            perror("Bind error");
            close(sockfd);
            continue;
        }

        break;
    }
    if (rp == NULL) perrorExit("Request socket creation failed");
    else            { printf("Requester socket: "); printNameInfo(rp); }
	
	struct sockaddr_in *tmp = (struct sockaddr_in *)rp->ai_addr;
	unsigned long rIpAddr = ntohl(tmp->sin_addr.s_addr);
	printf("req ip %s      %lu\n", inet_ntoa(tmp->sin_addr), rIpAddr);
	

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
        esockfd = socket(esp->ai_family, esp->ai_socktype, esp->ai_protocol);
        if (esockfd == -1) {
            perror("Socket error");
            continue;
        }
        break;
    }
    if (esp == NULL) perrorExit("emul socket creation failed");
    else            printf("emul socket created.\n");
	
	
	tmp = (struct sockaddr_in *)esp->ai_addr;
	unsigned long eIpAddr = ntohl(tmp->sin_addr.s_addr);
	printf("emul ip %s      %lu\n", inet_ntoa(tmp->sin_addr), eIpAddr);
	close(esockfd);
	
    // ------------------------------------------------------------------------
    // Sender hints TODO: move into part loop
    struct addrinfo shints;
    bzero(&shints, sizeof(struct addrinfo));
    shints.ai_family   = AF_INET;
    shints.ai_socktype = SOCK_DGRAM;
    shints.ai_flags    = 0;
    
    FILE *file = fopen("recvd.txt", "at");
    if (file == NULL) perrorExit("File open error");
    
	unsigned long sIpAddr;
    struct file_part *part = fileParts->parts;
	printf("got to loop\n");
    while (part != NULL) {
        // Convert the sender's port # to a string
        char senderPortStr[6] = "\0\0\0\0\0\0";
        sprintf(senderPortStr, "%d", part->sender_port);
    
        // Get the sender's address info
        struct addrinfo *senderinfo;
        errcode = getaddrinfo(part->sender_hostname, senderPortStr, &shints, &senderinfo);
        if (errcode != 0) {
            fprintf(stderr, "sender getaddrinfo: %s\n", gai_strerror(errcode));
            exit(EXIT_FAILURE);
        }
    
        // Loop through all the results of getaddrinfo and try to create a socket for sender
        // NOTE: this is done so that we can find which of the getaddrinfo results is the sender
        int sendsockfd;
        struct addrinfo *sp;
        for(sp = senderinfo; sp != NULL; sp = sp->ai_next) {
            sendsockfd = socket(sp->ai_family, sp->ai_socktype, sp->ai_protocol);
            if (sendsockfd == -1) {
                perror("Socket error");
                continue;
            }
    
            break;
        }
        if (sp == NULL) perrorExit("Send socket creation failed");
        //else            printf("Sender socket created.\n\n");
        close(sendsockfd); // don't need this socket
		
		tmp = (struct sockaddr_in *)sp->ai_addr;
		sIpAddr = ntohl(tmp->sin_addr.s_addr);
		printf("send ip %s      %lu\n", inet_ntoa(tmp->sin_addr), sIpAddr);
        // ------------------------------------------------------------------------
    
        // Setup variables for statistics
        unsigned long numPacketsRecvd = 0;
        unsigned long numBytesRecvd = 0;
        time_t startTime = time(NULL);
    
        // ------------------------------------------------------------------------
        // Construct a REQUEST packet
		printf("request packet constructing\n");
		struct ip_packet *pkt = NULL;
		struct packet *dpkt = NULL;
        pkt = malloc(sizeof(struct ip_packet));
        bzero(pkt, sizeof(struct ip_packet));
		dpkt = malloc(sizeof(struct packet));
        bzero(dpkt, sizeof(struct packet));
        dpkt->type = 'R';
        dpkt->seq  = 0;
        dpkt->len  = (unsigned long) window;
		printf("cpy pkt to ip pkt\n");
        strcpy(dpkt->payload, fileOption);
		memcpy(pkt->payload, dpkt, sizeof(struct packet));
		pkt->src = rIpAddr;
		pkt->srcPort = requesterPort;
		pkt->dest = sIpAddr;
		pkt->destPort = part->sender_port;
		pkt->priority = HIGH_PRIORITY;
		pkt->length = HEADER_SIZE + strlen(fileOption) + 1;
		printf("send r pkt\n");
        sendIpPacketTo(sockfd, pkt, esp->ai_addr);
    
        free(pkt);
		free(dpkt);
        // Create the file to write data to
        if (access(fileOption, F_OK) != -1) // if it already exists
            remove(fileOption);             // delete it

        // ------------------------------------------------------------------------
        // Connect to senders one at a time to get all parts
    
        // TODO: sometimes the requester stops receiving packets 
        //       even though the sender sends them properly... 
        //       requester sits blocked in recvfrom below, but doesn't recv 
        // NOTE: this isn't happening anymore with the rate limit betw 1-1000 pkt/sec
    
		printf("setup for recv loop\n");
        struct sockaddr_storage emulAddr;
        bzero(&emulAddr, sizeof(struct sockaddr_storage));
        socklen_t len = sizeof(emulAddr);
		printf("buffer\n");
		struct packet **buffer = malloc(window * (sizeof (void *)));
		int buffIndex;
		for (buffIndex = 0; buffIndex < window; buffIndex++) {
			buffer[buffIndex] = NULL;
		}
		unsigned long start = 1;
		struct ip_packet *rpkt;
		struct packet *drpkt;
        // Start a recv loop here to get all packets for the given part
		printf("recv loop\n");
        for (;;) {
            void *msg = malloc(sizeof(struct ip_packet));
            bzero(msg, sizeof(struct ip_packet));
    
            // Receive a message 
            size_t bytesRecvd = recvfrom(sockfd, msg, sizeof(struct ip_packet), 0,
                (struct sockaddr *)&emulAddr, &len);
            if (bytesRecvd == -1) perrorExit("Receive error");
			printf("received");
            // Deserialize the message into a packet
            rpkt = malloc(sizeof(struct ip_packet));
            bzero(rpkt, sizeof(struct ip_packet));
            deserializeIpPacket(msg, rpkt);
			drpkt = (struct packet *)rpkt->payload;
			
            printIpPacketInfo(rpkt, &emulAddr);
			if (rpkt->dest != rIpAddr || rpkt->destPort != requesterPort) {
				printf("ip mismatch\n");
				continue;
			}
			
            // Handle DATA packet
            if (drpkt->type == 'D') {
				if ( (drpkt->seq - start) >= window) {
					int i;
					for (i = 0; i < window; i++) {
						if (buffer[i] != NULL) {
							size_t bytesWritten = fprintf(file, "%s", buffer[i]->payload);
							if (bytesWritten != buffer[i]->len) {
								fprintf(stderr,
									"Incomplete file write: %d bytes written, %lu pkt len",
									(int)bytesWritten, buffer[i]->len);
							} else {
								fflush(file);
							}
							buffer[i] = NULL;
						}
					}			
					start += window;
				}
				else if (buffer[drpkt->seq - start] != NULL) {
					printf("duplicate packet\n");
				}
                // Update statistics
                ++numPacketsRecvd;
                numBytesRecvd += drpkt->len;
				printf("<- *** [Received data packet] ***\n");
                /* FOR DEBUG
                printf("[Packet Details]\n------------------\n");
                printf("type : %c\n", pkt->type);
                printf("seq  : %lu\n", pkt->seq);
                printf("len  : %lu\n", pkt->len);
                printf("payload: %s\n\n", pkt->payload);
                */
    
                //Print details about the received packet
                //printf("<- [Received DATA packet] ");
                //printPacketInfo(rpkt->payload, (struct sockaddr_storage *)&emulAddr);
    
                // Save the data to a buffer
					
				buffer[drpkt->seq - start] =  malloc(sizeof(struct packet));
				memcpy(buffer[drpkt->seq - start], drpkt, sizeof(struct packet));
				buffer[drpkt->seq - start] = drpkt;
					
					
				/*pkt = malloc(sizeof(struct ip_packet));
				bzero(pkt, sizeof(struct ip_packet));
				dpkt = (struct packet *)pkt->payload;
				dpkt->type = 'A';
				dpkt->seq  = drpkt->seq;
				dpkt->len  = window;
				pkt->src = rIpAddr;
				pkt->srcPort = requesterPort;
				pkt->dest = rpkt->src;
				pkt->dest = sIpAddr;
				pkt->destPort = part->sender_port;
				pkt->length = HEADER_SIZE;
				
				sendIpPacketTo(sockfd, pkt, esp->ai_addr);*/
				unsigned long long t = getTimeMS();
				while (t == getTimeMS()){}
				pkt = malloc(sizeof(struct ip_packet));
				bzero(pkt, sizeof(struct ip_packet));
				dpkt = malloc(sizeof(struct packet));
				bzero(dpkt, sizeof(struct packet));
				dpkt->type = 'A';
				dpkt->seq  = drpkt->seq;
				dpkt->len  = 0;
				printf("cpy pkt to ip pkt\n");
				dpkt->payload[0] = '\0';
				memcpy(pkt->payload, dpkt, sizeof(struct packet));
				pkt->src = rIpAddr;
				pkt->srcPort = requesterPort;
				pkt->dest = sIpAddr;
				pkt->destPort = part->sender_port;
				pkt->priority = HIGH_PRIORITY;
				pkt->length = HEADER_SIZE;
				printf("send a pkt\n");
				sendIpPacketTo(sockfd, pkt, esp->ai_addr);
				free(rpkt);
    
				free(pkt);
				free(dpkt);
            }
    
            // Handle END packet
            if (drpkt->type == 'E') {
                printf("<- *** [Received END packet] ***");
                double dt = difftime(time(NULL), startTime);
                if (dt <= 1) dt = 1;
    
                // Print statistics
                printf("\n---------------------------------------\n");
                printf("Total packets recvd: %lu\n", numPacketsRecvd);
                printf("Total payload bytes recvd: %lu\n", numBytesRecvd);
                printf("Average packets/second: %d\n", (int)(numPacketsRecvd / dt));
                printf("Duration of test: %f sec\n\n", dt);
				
				int i;
				for (i = 0; i < window; i++) {
					if (buffer[i] != NULL) {
						size_t bytesWritten = fprintf(file, "%s", buffer[i]->payload);
						if (bytesWritten != buffer[i]->len) {
							fprintf(stderr,
								"Incomplete file write: %d bytes written, %lu pkt len",
								(int)bytesWritten, buffer[i]->len);
						} else {
							fflush(file);
						}
						buffer[i] = NULL;
					}
				}
                break;
            }
        }
        part = part->next_part;
        freeaddrinfo(senderinfo);
    }

    fclose(file);

    // Got what we came for, shut it down
    if (close(sockfd) == -1) perrorExit("Close error");
    else                        puts("Connection closed.\n");

    // Cleanup address and file info data 
    freeaddrinfo(requesterinfo);
    freeFileInfo(fileParts);

    // All done!
    exit(EXIT_SUCCESS);
}

