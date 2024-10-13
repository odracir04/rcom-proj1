// Link layer protocol implementation

#include <signal.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <termios.h>
#include <unistd.h>

#include "../../include/link_layer.h"
#include "../../include/serial_port.h"

// MISC
#define _POSIX_SOURCE 1 // POSIX compliant source

int alarmEnabled = FALSE;
int alarmCount = 0;

// Alarm function handler
void alarmHandler(int signal)
{
    alarmEnabled = FALSE;
    alarmCount++;

    printf("Alarm #%d\n", alarmCount);
}

////////////////////////////////////////////////
// LLOPEN
////////////////////////////////////////////////
int llopen(LinkLayer connectionParameters)
{
    int fd;

    if ((fd = openSerialPort(connectionParameters.serialPort,
                       connectionParameters.baudRate)) < 0)
    {
        return -1;
    }

    // Set alarm function handler
    (void)signal(SIGALRM, alarmHandler);

    // Create string to send
    unsigned char buf[BUF_SIZE] = {0};

    buf[0] = FLAG;
    buf[1] = ADDRESS;
    buf[2] = CONTROL_SET;
    buf[3] = BCC1;
    buf[4] = FLAG;

    UAState state = START; 
    while (state != STOP && alarmCount < connectionParameters.nRetransmissions)
    {
    
    if (!alarmEnabled) {
        int bytes = writeBytes(buf, BUF_SIZE);
        printf("%d bytes written\n", bytes);
        alarm(connectionParameters.timeout);
        alarmEnabled = TRUE;
        sleep(1);
    }    

    int bytes = readByte(buf);
    
    if (bytes > 0) {
        switch (state) {
            case START:
                printf("START State\n");
                state = buf[0] == FLAG ? FLAG_RCV : START;
                break;
            case FLAG_RCV:
                printf("FLAG_RCV State\n");
                if (buf[0] == FLAG) {
                    state = FLAG; 
                } else {
                    state = buf[0] == ADDRESS ? A_RCV : START;
                }
                break;
            case A_RCV:
                printf("A_RCV State\n");
                if (buf[0] == FLAG) {
                    state = FLAG; 
                } else {
                    state = buf[0] == CONTROL_UA ? C_RCV : START;
                }
                break;
            case C_RCV:
                printf("C_RCV State\n");
                if (buf[0] == FLAG) {
                    state = FLAG; 
                } else {
                    state = buf[0] == (ADDRESS ^ CONTROL_UA) ? BCC_OK : START;
                }
                break;
            case BCC_OK:
                printf("BCC_OK State\n");
                if (buf[0] == FLAG) {
                    state = STOP;
                    alarm(0);
                } else {
                    state = START;
                }
                break;
            case STOP:
                break;
            }        
        }
    }

    return 1;
}

////////////////////////////////////////////////
// LLWRITE
////////////////////////////////////////////////
int llwrite(const unsigned char *buf, int bufSize)
{
    // TODO

    return 0;
}

////////////////////////////////////////////////
// LLREAD
////////////////////////////////////////////////
int llread(unsigned char *packet)
{
    // TODO

    return 0;
}

////////////////////////////////////////////////
// LLCLOSE
////////////////////////////////////////////////
int llclose(int showStatistics)
{
    // TODO

    int clstat = closeSerialPort();
    return clstat;
}
