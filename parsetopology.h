
struct neighbor_entry {
  unsigned long ip;
  unsigned int port;
  struct sockaddr_in *socket;
}

struct neighbor_entry *readtopology(const char *filename, unsigned long ip, unsigned int port);

