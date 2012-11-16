#ifndef _PACKET_H_
#define _PACKET_H_

#include <netinet/in.h>

#define MAX_PAYLOAD 5120
#define HIGH_PRIORITY 0x01
#define MEDIUM_PRIORITY 0x02
#define LOW_PRIORITY 0x03

struct packet {
    char type;
    unsigned long seq;
    unsigned long len;
    char payload[MAX_PAYLOAD];
} __attribute__((packed));

#define PACKET_SIZE sizeof(struct packet)

void *serializePacket(struct packet *pkt);
void deserializePacket(void *msg, struct packet *pkt);

void *serializeIpPacket(struct ip_packet *pkt);
void deserializeIpPacket(void *msg, struct ip_packet *pkt);

void sendPacketTo(int sockfd, struct packet *pkt, struct sockaddr *addr);
//void recvPacket(int sockfd, struct packet *pkt);

void sendIpPacketTo(int sockfd, struct ip_packet *pkt, struct sockaddr *addr);

void printPacketInfo(struct packet *pkt, struct sockaddr_storage *saddr);

struct ip_packet {
	unsigned char priority;
	unsigned long src;
	unsigned int srcPort;
	unsigned long dest;
	unsigned int destPort;
	unsigned long length;
	struct packet *payload;
} __attribute__((packed));

#define IP_PACKET_SIZE sizeof(struct ip_packet)

#endif

