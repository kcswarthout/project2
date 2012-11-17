#include "utilities.h"
#include "packet.h"
#include "forwardtable.h"


int initLog(char *filename);

void log(struct ip_packet *pkt, char *str);