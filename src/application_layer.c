// Application layer protocol implementation

#include "../include/application_layer.h"
#include "../include/link_layer.h"
#include <stdio.h>
#include <sys/stat.h>
#include <string.h>

unsigned int getFileSize(char* filename) {
    struct stat st;
    stat(filename, &st);
    return st.st_size;
}

void sendControlPacket(unsigned char control_number, char* filename) {
    // build control packet
    unsigned int filename_size = strlen(filename);
    unsigned int packet_size = 3 + filename_size + 4;
    char packet[packet_size];

    packet[0] = control_number;
    packet[1] = FILE_SIZE;
    packet[2] = 2; // file size octets

    unsigned int file_size = getFileSize(filename);
    packet[3] = file_size / 256;
    packet[4] = file_size % 256;

    packet[5] = FILE_NAME;
    packet[6] = strlen(filename);
    strcpy(&packet[7], filename);

    llwrite(packet, packet_size);     
}

void sendDataPackets(char* filename) {
    unsigned char sequence_number = 0;
    char buffer[4 + MAX_PAYLOAD_SIZE];

    buffer[0] = 2; 

    FILE* file = fopen(filename, "rb");
    if (file == NULL) {
        perror("Could not open file");
        return;
    }

    int nbytes;
    while ((nbytes = fread(&buffer[4], 1, MAX_PAYLOAD_SIZE, file)) > 0) {
        // prep control fields
        buffer[1] = sequence_number;
        buffer[2] = nbytes / 256;
        buffer[3] = nbytes % 256;

        int llbytes = llwrite(buffer, nbytes + 4);
        if (llbytes == -1) {
            printf("ERROR: Connection lost - timeout\n");
            return;
        }
        else
            printf("\nDATA: Wrote %d bytes\n", llbytes);

        sequence_number = (sequence_number + 1) % 256;
    }
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

    switch (connectionParameters.role) {
        case LlTx:
            // sendControlPacket(1, filename);
            sendDataPackets(filename);
            // sendControlPacket(3, filename);
            break;
        case LlRx:
            unsigned char packet[1000] = {0};
            packet[999] = '\0';
            llread(packet);

            for (int i = 0; i < 1000; i++) {
                printf("byte: %02X\n", packet[i]);
            }
            break;
    }

    llclose(0);
}
