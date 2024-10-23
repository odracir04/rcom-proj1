// Application layer protocol implementation

#include "../include/application_layer.h"
#include "../include/link_layer.h"
#include <stdio.h>
#include <string.h>

void sendControlPacket(unsigned char control_number, char* filename) {
    
    //build control packet
    char packet[1+2+128+2+2];

    packet[0] = control_number;
    packet[1] = FILE_SIZE;
    packet[2] = 2; // file size octets

    unsigned int file_size = getFileSize();
    packet[3] = file_size / 256;
    packet[4] = file_size % 256;

    packet[5] = FILE_NAME;
    packet[6] = strlen(filename);
    strcpy(&packet[7], filename);

        
}


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

    unsigned char sequence_number = 0;

    switch (connectionParameters.role) {
        case LlTx:
            // sendStartPacket()
            llwrite(buf, 12);
            // sendEndPacket()

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
