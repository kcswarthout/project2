#include "utilities.h"
#include "packet.h"
#include "forwardtable.h"


int initLog(char *filename);

void logP(struct ip_packet *pkt, char *str);