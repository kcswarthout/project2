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

#ifndef _UTILITIES_H_
#define _UTILITIES_H_

#define LOOPBACK "127.0.0.1"

// ----------------------------------------------------------------------------
// Returns the current system time with millisecond granularity
// ----------------------------------------------------------------------------
unsigned long long getTimeMS();

// ----------------------------------------------------------------------------
// Print hostname and port to stdout
// ----------------------------------------------------------------------------
void printNameInfo(struct addrinfo *addr);

// ----------------------------------------------------------------------------
// Print error from errno and exit with a failure indication
// ----------------------------------------------------------------------------
void perrorExit(const char *msg);

// ----------------------------------------------------------------------------
// Print error to stderr and exit with a failure indication
// ----------------------------------------------------------------------------
void ferrorExit(const char *msg);

// ----------------------------------------------------------------------------
// Get ip address of the given host name
// ----------------------------------------------------------------------------
unsigned long nameToAddr(char *name);

#endif

