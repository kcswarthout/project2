#include <sys/socket.h>
#include <sys/types.h>

struct neighbor_entry {
  unsigned long ip;
  unsigned int port;
  struct sockaddr_in *socket;
};

int readtopology(const char *filename, struct sockaddr_in *local, struct neighbor_entry *neighbors);

