// Application layer protocol implementation

#include "../include/application_layer.h"
#include "../include/link_layer.h"
#include <stdio.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>

#define FILE_SIZE 0x00
#define FILE_NAME 0x01
#define MAX_FILENAME 256
#define MAX_FILESIZE 65535

#define START_PACKET 1
#define END_PACKET 3
#define FILE_SIZE_OCTETS 2
#define CONTROL_OCTETS 7
#define DATA_PACKET 2

unsigned int getFileSize(char* filename) {
    struct stat st;
    stat(filename, &st);
    return st.st_size;
}

void sendControlPacket(unsigned char control_number, char* filename) {
    unsigned int filename_size = strlen(filename);
    unsigned int packet_size = filename_size + CONTROL_OCTETS;
    char packet[packet_size];

    packet[0] = control_number;
    packet[1] = FILE_SIZE;
    packet[2] = FILE_SIZE_OCTETS;

    unsigned int file_size = getFileSize(filename);
    packet[3] = file_size / 256;
    packet[4] = file_size % 256;

    if (file_size > MAX_FILESIZE) {
        printf("ERROR: File too large!\n");
        llclose(FALSE);
        exit(EXIT_FAILURE);
    }

    packet[5] = FILE_NAME;
    packet[6] = strlen(filename);
    strcpy(&packet[7], filename);

    llwrite(packet, packet_size);     
}

void sendDataPackets(char* filename) {
    unsigned char sequence_number = 0;
    char buffer[MAX_PAYLOAD_SIZE];

    buffer[0] = DATA_PACKET; 

    FILE* file = fopen(filename, "rb");
    if (file == NULL) {
        printf("ERROR: Could not open file\n");
        return;
    }

    int nbytes;
    while ((nbytes = fread(&buffer[4], 1, MAX_PAYLOAD_SIZE - 4, file)) > 0) {
        buffer[1] = sequence_number;
        buffer[2] = nbytes / 256;
        buffer[3] = nbytes % 256;

        int llbytes = llwrite(buffer, nbytes + 4);
        if (llbytes == -1) {
            printf("ERROR: Connection lost - timeout\n");
            return;
        }

        sequence_number = (sequence_number + 1) % 256;
    }

    fclose(file);
}

void receivePackets(char* filename) {
    FILE *out;
    unsigned char packet[2000] = {0};
    int sequence_number = 0;
    int file_size = 0;
    char write_filename[MAX_FILENAME + 1];

    out = fopen(filename, "w");
    int llbytes;
    while ((llbytes = llread(packet)) > 0) {
        if (packet[0] == 1) {
            int filename_length = packet[6];
            memcpy(write_filename, packet + 7, filename_length);
            write_filename[filename_length] = '\0';
            printf("------------------------------------\n");
            printf("Received file: %s\n", write_filename);
            file_size = packet[3] * 256 + packet[4];
            printf("Total file size: %d bytes\n", file_size);
        } else if (packet[0] == 2) {
            int bytes = fwrite(&packet[4], 1, llbytes - 4, out);
            if (packet[1] == sequence_number) {
                sequence_number = (sequence_number + 1) % 256;
            } else {
                printf("ERROR: Out of order packet\n");
            }
        } else if (packet[0] == 3) {
            int filename_length = packet[6];
            char filename[filename_length + 1];
            memcpy(filename, packet + 7, filename_length);
            filename[filename_length] = '\0';
            if (strcmp(write_filename, filename) != 0) {
                printf("ERROR: Wrong file name\n");
            }
            if (file_size != packet[3] * 256 + packet[4]) {
                printf("ERROR: Wrong file size\n");
            } else {
                printf("CONTROL PACKET: CORRECT TRANSMISSION\n");
            }
        }
    }
    fclose(out);
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
            sendControlPacket(START_PACKET, filename);
            sendDataPackets(filename);
            sendControlPacket(END_PACKET, filename);
            break;
        case LlRx:
            receivePackets(filename);
            break;
    }
    llclose(TRUE);
}
