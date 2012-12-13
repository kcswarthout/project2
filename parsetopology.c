#include "parsetopology.h"
#include <string.h>
#include <stdlib.h>

#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>

#include "utilities.h"

#define MAX_TOK_PER_LINE 20

int readtopology(const char *filename, struct sockaddr_in *local, struct neighbor_entry *neighbors) {
  if (filename == NULL) ferrorExit("ReadTopology: invalid filename");
  if (local == NULL) ferrorExit("ReadTopology: invalid sockaddr");
  char ipStr [INET_ADDRSTRLEN];
  inet_ntop(AF_INET, local, ipStr, INET_ADDRSTRLEN);
  
  // Open the topology file
  FILE *file = fopen(filename, "r");
  if (file == NULL) perrorExit("ReadTopology open error");
  else              puts("\nReadTopology file opened.");
  
  
  // Read in a line at a time from the topology file
  char *line = NULL;
  size_t lineLen = 0;
  size_t bytesRead = getline(&line, &lineLen, file);
  if (bytesRead == -1) perrorExit("Getline error");
  char *tok;
  while (bytesRead != -1) { 
    //printf("read a line\n");
    // Tokenize line
    tok = strtok(line, ",");
    // Only process this line if it is for the specified node
    if (strcmp(tok, ipStr) == 0) continue;
    //printf("for this emul\n");
    
    free(line);
    line = NULL;
    bytesRead = getline(&line, &lineLen, file);
  }
  
  int n = -1;
  if (bytesRead != -1) {
    char *tokens[MAX_TOK_PER_LINE];
    n = 0;
    tok = strtok(line, ",");
    while (tok != NULL) {
      tokens[n] = tok;
      tok  = strtok(NULL, " ");
      n++;
    }
    neighbors = malloc(n * (sizeof(struct neighbor_entry)));
    int i;
    //printf("set fields\n");
    struct addrinfo hints;
    bzero(&hints, sizeof(struct addrinfo));
    hints.ai_family   = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags    = 0;
    for (i = 0; i < n; i++) {
      struct addrinfo *info;
      int errcode = getaddrinfo(strtok(tokens[i], ","), strtok(NULL, "\0"), &hints, &info);
      if (errcode != 0) {
        fprintf(stderr, "ReadTopology getaddrinfo: %s\n", gai_strerror(errcode));
        exit(EXIT_FAILURE);
      }
      neighbors[i].socket = (struct sockaddr_in *)info->ai_addr;
      neighbors[i].ip = ntohl(neighbors[i].socket->sin_addr.s_addr);
      neighbors[i].port = ntohs(neighbors[i].socket->sin_port);
    }
  }
  
  free(line);
  
  // Close the forward table file
  if (fclose(file) != 0) perrorExit("ReadTopology close error");
  else                   puts("ReadTopology file closed.\n");
  
  return n;
}
