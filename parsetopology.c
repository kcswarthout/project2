#include "parsetopology.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>


#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>

#include "utilities.h"

#define MAX_TOK_PER_LINE 20

int readtopology(const char *filename, struct sockaddr_in *local, struct neighbor_entry **neighbors) {
  if (filename == NULL) ferrorExit("ReadTopology: invalid filename");
  if (local == NULL) ferrorExit("ReadTopology: invalid sockaddr");
  char ipStr[20];
  char ac[80];
  gethostname(ac, sizeof(ac));
  printf("Host name is %s", ac);

  struct hostent *phe = gethostbyname(ac);
  if (phe == 0) {
    printf("bad");
    return 1;
  }
  for (int i = 0; phe->h_addr_list[i] != 0; ++i) {
    //struct in_addr addr;
    //memcpy(&addr, phe->h_addr_list[i], sizeof(struct in_addr));
    ipULtoStr(nameToAddr(ac), ipStr);
    //printf("\nAddress %d: %s", i, ipStr);
  }
  
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
    //printf("\nbefore   %s", line);
    tok = strtok(line, ",");
    //printf("after   %s\n", line);
    puts(tok);
    // Only process this line if it is for the specified node
    if (strcmp(tok, ipStr) == 0) {
      printf("Match: %s", tok);
      break;
    }
    //printf("for this emul\n");
    
    free(line);
    line = NULL;
    bytesRead = getline(&line, &lineLen, file);
  }
  
  int n = -1;
  if (bytesRead != -1) {
    puts(line);
    char *tokens[MAX_TOK_PER_LINE];
    n = 0;
    tok  = strtok(NULL, " ");
    tok  = strtok(NULL, " ");
    while (tok != NULL) {
      //puts(tok);
      tokens[n] = tok;
      tok  = strtok(NULL, " ");
      n++;
    }
    *neighbors = malloc(n * (sizeof(struct neighbor_entry)));
    int i;
    //printf("set fields\n");
    struct addrinfo hints;
    bzero(&hints, sizeof(struct addrinfo));
    hints.ai_family   = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags    = 0;
    //puts("starting neigh loop\n");
    for (i = 0; i < n; i++) {
      struct addrinfo *info;
      //printf("/n%s", tokens[i]);
      char *tmp1 = strtok(tokens[i], ",");
      char *tmp2 = strtok(NULL, "\n");
      //puts(tmp1);
      //puts(tmp2);
      int errcode = getaddrinfo(tmp1, tmp2, &hints, &info);//strtok(tokens[i], ","), strtok(NULL, "\0"), &hints, &info);
      if (errcode != 0) {
        fprintf(stderr, "ReadTopology getaddrinfo: %s\n", gai_strerror(errcode));
        exit(EXIT_FAILURE);
      }
      (*neighbors)[i].socket = (struct sockaddr_in *)info->ai_addr;
      (*neighbors)[i].ip = ntohl((*neighbors)[i].socket->sin_addr.s_addr);
      (*neighbors)[i].port = ntohs((*neighbors)[i].socket->sin_port);
    }
  }
  
  free(line);
  
  // Close the forward table file
  if (fclose(file) != 0) perrorExit("ReadTopology close error");
  else                   puts("ReadTopology file closed.\n");
  
  return n;
}

int neighborsToPayload(struct neighbor_listing *neighbors, unsigned char **bytes, int size) {
  size_t newSize = size * sizeof(struct neighbor_listing);
  *bytes = malloc(newSize);
  bzero(*bytes, newSize);
  memcpy(*bytes, neighbors, newSize);
  return (int) newSize;
}

int payloadToNeighbors(struct neighbor_listing **neighbors, unsigned char *bytes, int size) {
  int newSize = size / sizeof(struct neighbor_listing);
  *neighbors = malloc((size_t) size);
  bzero(*neighbors, (size_t) size);
  memcpy(*neighbors, bytes, (size_t) size);
  return newSize;
}
