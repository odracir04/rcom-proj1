// Application layer protocol implementation

#include "../../include/application_layer.h"
#include "../../include/link_layer.h"
#include <string.h>

void applicationLayer(const char *serialPort, const char *role, int baudRate,
                      int nTries, int timeout, const char *filename)
{
    LinkLayer connectionParameters;
    strcpy(connectionParameters.serialPort, serialPort);
    connectionParameters.role = LlTx;
    connectionParameters.baudRate = baudRate;
    connectionParameters.nRetransmissions = nTries;
    connectionParameters.timeout = timeout;

    llopen(connectionParameters);

    unsigned char buf[12] = {1,2,0x7E,4,5,6,7,8,9,10,11,12};
    llwrite(buf, 12);
    llclose(0);
}
