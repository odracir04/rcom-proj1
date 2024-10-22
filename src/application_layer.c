// Application layer protocol implementation

#include "../include/application_layer.h"
#include "../include/link_layer.h"
#include <stdio.h>
#include <string.h>

void applicationLayer(const char *serialPort, const char *role, int baudRate,
                      int nTries, int timeout, const char *filename)
{
    LinkLayer connectionParameters;
    strcpy(connectionParameters.serialPort, serialPort);
    if (strcmp(role, "tx") == 0) {
        connectionParameters.role = LlTx;
    } else if (strcmp(role, "rx") == 0) {
        connectionParameters.role = LlRx;
    }

    connectionParameters.baudRate = baudRate;
    connectionParameters.nRetransmissions = nTries;
    connectionParameters.timeout = timeout;

    llopen(connectionParameters);

    switch (connectionParameters.role) {
        case LlTx:
            unsigned char buf[12] = {1,2,0x7E,4,5,6,7,8,9,10,11,12};
            llwrite(buf, 12);
            break;
        case LlRx:
            unsigned char packet[20] = {0};
            packet[19] = '\0';
            llread(packet);

            for (int i = 0; i < 20; i++) {
                printf("byte: %02X\n", packet[i]);
            }
            break;
    }

    llclose(0);
}
