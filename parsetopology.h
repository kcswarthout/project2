#include <sys/socket.h>
#include <sys/types.h>

#ifndef _PARSETOPOLOGY_H_
#define _PARSETOPOLOGY_H_

struct neighbor_entry {
  unsigned long ip;
  unsigned int port;
  struct sockaddr_in *socket;
  int isAlive;
};

struct neighbor_listing {
  unsigned long ip;
  unsigned int port;
} __attribute__((packed));

int readtopology(const char *filename, struct sockaddr_in *local, struct neighbor_entry **neighbors);

int neighborsToPayload(struct neighbor_listing *neighbors, unsigned char **bytes, int size);

int payloadToNeighbors(struct neighbor_listing **neighbors, unsigned char *bytes, int size);

#endif
