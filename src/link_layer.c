 // Link layer protocol implementation

#include <signal.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <termios.h>
#include <unistd.h>

#include "../include/link_layer.h"
#include "../include/serial_port.h"
#include "../include/alarm.h"

// MISC
#define _POSIX_SOURCE 1 // POSIX compliant source

extern int alarmEnabled;
extern int alarmCount;

int current_frame = 0;
LinkLayer connection;

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

    connection = connectionParameters;
    UAState state;

    switch (connectionParameters.role) {
        case LlTx:
            // Set alarm function handler
            (void)signal(SIGALRM, alarmHandler);

            // Create SET frame to send
            unsigned char set_frame[CTRL_FRAME_SIZE] = {0};

            set_frame[0] = FLAG;
            set_frame[1] = ADDRESS_TX;
            set_frame[2] = CONTROL_SET;
            set_frame[3] = ADDRESS_TX ^ CONTROL_SET;
            set_frame[4] = FLAG;

            // Transmit SET and wait for UA
            state = START; 
            while (state != STOP && alarmCount < connectionParameters.nRetransmissions)
            {
            
            if (!alarmEnabled) {
                int bytes = writeBytes(set_frame, CTRL_FRAME_SIZE);
                printf("SET-FRAME: %d bytes written\n", bytes);
                alarm(connectionParameters.timeout);
                alarmEnabled = TRUE;
                sleep(1);
            }    

            int bytes = readByte(set_frame);
            
            if (bytes > 0) {
                switch (state) {
                    case START:
                        state = set_frame[0] == FLAG ? FLAG_RCV : START;
                        break;
                    case FLAG_RCV:
                        if (set_frame[0] == FLAG) {
                            state = FLAG_RCV; 
                        } else {
                            state = set_frame[0] == ADDRESS_TX ? A_RCV : START;
                        }
                        break;
                    case A_RCV:
                        if (set_frame[0] == FLAG) {
                            state = FLAG_RCV; 
                        } else {
                            state = set_frame[0] == CONTROL_UA ? C_RCV : START;
                        }
                        break;
                    case C_RCV:
                        if (set_frame[0] == FLAG) {
                            state = FLAG_RCV; 
                        } else {
                            state = set_frame[0] == (ADDRESS_TX ^ CONTROL_UA) ? BCC_OK : START;
                        }
                        break;
                    case BCC_OK:
                        if (set_frame[0] == FLAG) {
                            state = STOP;
                            alarm(0);
                            alarmEnabled = FALSE;
                        } else {
                            state = START;
                        }
                        break;
                    case STOP:
                        printf("Received UA frame\n");
                        break;
                    }        
                }
            }
            break;
        case LlRx:
            // Create SET frame
            unsigned char buf[CTRL_FRAME_SIZE] = {0};

            state = START; 
            while (state != STOP)
            {
            
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
                            state = buf[0] == ADDRESS_TX ? A_RCV : START;
                        }
                        break;
                    case A_RCV:
                        printf("A_RCV State\n");
                        if (buf[0] == FLAG) {
                            state = FLAG; 
                        } else {
                            state = buf[0] == CONTROL_SET ? C_RCV : START;
                        }
                        break;
                    case C_RCV:
                        printf("C_RCV State\n");
                        if (buf[0] == FLAG) {
                            state = FLAG; 
                        } else {
                            state = buf[0] == (ADDRESS_TX ^ CONTROL_SET) ? BCC_OK : START;
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

            buf[0] = FLAG;
            buf[1] = ADDRESS_TX;
            buf[2] = CONTROL_UA;
            buf[3] = ADDRESS_TX ^ CONTROL_UA;
            buf[4] = FLAG;

            int bytes = writeBytes(buf, CTRL_FRAME_SIZE);
            printf("%d bytes written\n", bytes);
            sleep(1);
            break;
        }

    return 1;
}

////////////////////////////////////////////////
// LLWRITE
////////////////////////////////////////////////
int llwrite(const unsigned char *buf, int bufSize)
{
    alarmCount = 0;

    // Prepare I frame to send with byte stuffing and control fields
    char bcc2 = 0;
    unsigned char frame[2 * bufSize + 6];   

    frame[0] = FLAG;
    frame[1] = ADDRESS_TX;
    frame[2] = current_frame == 0 ? CTRL_I0 : CTRL_I1;
    frame[3] = frame[1] ^ frame[2];

    int current_position = 4; 
    for (int i = 0; i < bufSize; i++) {
        bcc2 ^= buf[i];
        if (buf[i] == FLAG || buf[i] == ESCAPE) {
            frame[current_position] = ESCAPE;
            current_position++;
            frame[current_position] = buf[i] ^ 0x20;
            current_position++;
        } else {
            frame[current_position] = buf[i];
            current_position++;
        }
    }
        
    frame[current_position] = bcc2;
    frame[current_position + 1] = FLAG;

    // Send I frame and await RR or REJ frame
    unsigned char rr[2]; 
    UAState state = START;
        
    while (state != STOP && alarmCount < 3) { // 3 is HARD-CODED 
        
        if (!alarmEnabled) {
            int nbytes = writeBytes(frame, current_position + 2);
            for (int i = 0; i < nbytes; i++) {
                printf(":%x", frame[i]);
            }   
            alarm(5); // This is HARD-CODED!
            alarmEnabled = TRUE;
            sleep(1);
        }

        int bytes = readByte(rr);
        if (bytes > 0) {
            switch (state) {
                case START:
                    state = rr[0] == FLAG ? FLAG_RCV : START;
                    break;
                case FLAG_RCV:
                    if (rr[0] == FLAG) {
                        state = FLAG_RCV; 
                    } else {
                        state = rr[0] == ADDRESS_TX ? A_RCV : START;
                    }
                    break;
                case A_RCV:
                    if (rr[0] == FLAG) {
                        state = FLAG_RCV; 
                    } else {
                    if (current_frame == 0 && rr[0] == RR1) {
                        state = C_RCV;
                        current_frame = 1;
                    } else if (current_frame == 1 && rr[0] == RR0) {
                        state = C_RCV;
                        current_frame = 0;
                    } else if ((current_frame == 0 && rr[0] == REJ0) || (current_frame == 1 && rr[0] == REJ1)) {
                        state = START;
                    }
                    }
                    break;
                case C_RCV:
                    if (rr[0] == FLAG) {
                        state = FLAG_RCV; 
                    } else {
                        if ((current_frame == 1 && rr[0] == (ADDRESS_TX ^ RR1)) || (current_frame == 0 && rr[0] == (ADDRESS_TX ^ RR0))) {
                            state = BCC_OK;
                        } else {
                            state = START;
                        }
                    }
                    break;
                case BCC_OK:
                    if (rr[0] == FLAG) {
                        state = STOP;
                        alarm(0);
                        alarmEnabled = FALSE;
                    } else {
                        state = START;
                    }
                    break;
                case STOP:
                    printf("Received response frame\n");
                    break;
            }        
        }
    }

    return alarmCount == 3 ? -1 : current_position + 2;
}

////////////////////////////////////////////////
// LLREAD
////////////////////////////////////////////////
int llread(unsigned char *packet)
{
    unsigned char rr[2];     
    UAState state = START;
    int packet_position = 0;
    unsigned char is7d = FALSE;
    unsigned char bbc2 = 0;
        
    while (state != STOP) {
        int bytes = readByte(rr);
        int bcc2 = 0;
    
        // modify state machine for disconnects
        // currently not checking C field or BCC2 field
        // not processing byte stuffing
        // very shady disconnect mechanism
        if (bytes > 0) {
            printf("Received: %02X\n", rr[0]);
            switch (state) {
                case START:
                    printf("START State\n");
                    state = rr[0] == FLAG ? FLAG_RCV : START;
                    break;
                case FLAG_RCV:
                    printf("FLAG_RCV State\n");
                    if (rr[0] == FLAG) {
                        state = FLAG_RCV; 
                    } else {
                        state = rr[0] == ADDRESS_TX ? A_RCV : START;
                    }
                    break;
                case A_RCV:
                    printf("A_RCV State\n");
                    if (rr[0] == FLAG) {
                        state = FLAG_RCV; 
                    } else {
                        if (current_frame == 0 && rr[0] == CTRL_I0) {
                            state = C_RCV;
                            current_frame = 1;
                        } else if (current_frame == 1 && rr[0] == CTRL_I1) {
                            state = C_RCV;
                            current_frame = 0;
                        } else if (rr[0] == DISC) {
                            state = STOP;
                        }
                    }
                    break;
                case C_RCV:
                    printf("C_RCV State\n");
                    if (rr[0] == FLAG) {
                        state = FLAG_RCV; 
                    } else if ((rr[0] == (ADDRESS_TX ^ CTRL_I0) && current_frame == 1) || (rr[0] == (ADDRESS_TX ^ CTRL_I1) && current_frame == 0)) {
                        state = BCC_OK;
                    }
                    break;
                case BCC_OK:
                    printf("BCC_OK State: DATA FRAME\n");
                    if (rr[0] == bbc2 && !is7d) {
                        printf("e\n");
                        state = BCC2_OK;
                    } else {
                        if (rr[0] == 0x7d) {
                            printf("d\n");
                            is7d = TRUE;
                            continue;
                        } else if (is7d) {
                            if (rr[0] == 0x5e) {
                                printf("a\n");
                                packet[packet_position] = 0x7e;
                                bbc2 ^= 0x7e;
                                packet_position++;
                            } else if (rr[0] == 0x5d) {
                                printf("b\n");
                                packet[packet_position] = 0x7d;
                                bbc2 ^= 0x7d;
                                packet_position++;
                            } else return -1;
                            printf("c\n");
                            is7d = FALSE;
                            continue;
                        }
                        printf("f\n");
                        packet[packet_position] = rr[0];
                        bbc2 ^= rr[0];
                        packet_position++;
                    }
                    break;
                case BCC2_OK:
                    printf("BCC2_OK State\n");
                    if (rr[0] == FLAG) {
                        state = STOP;
                    } else {
                        state = BCC_OK;
                        if (rr[0] == 0x7d) {
                            is7d = TRUE;
                            continue;
                        } else {
                            packet[packet_position] = rr[0];
                            bbc2 ^= rr[0];
                            packet_position++;
                        }
                    }
                case STOP:
                    break;
                }        
        }
    }

    unsigned char buf[5];
    buf[0] = FLAG;
    buf[1] = ADDRESS_TX;
    buf[2] = current_frame == 0 ? RR0 : RR1;
    buf[3] = ADDRESS_TX ^ buf[2];
    buf[4] = FLAG;

    int bytes = writeBytes(buf, 5);
    printf("RR: %d bytes written\n", bytes);
    sleep(1);
    return 0;
}

////////////////////////////////////////////////
// LLCLOSE
////////////////////////////////////////////////
int llclose(int showStatistics)
{
    switch (connection.role) {
        case LlTx:
            unsigned char disconnect_frame[5];
            disconnect_frame[0] = FLAG;
            disconnect_frame[1] = ADDRESS_TX;
            disconnect_frame[2] = DISC;
            disconnect_frame[3] = ADDRESS_TX ^ DISC;
            disconnect_frame[4] = FLAG;

            UAState state = START; 
            alarmCount = 0;

            while (state != STOP && alarmCount < 3)
            {
            
            if (!alarmEnabled) {
                int bytes = writeBytes(disconnect_frame, 5);
                printf("DISCONNECT: %d bytes written\n", bytes);
                alarm(5);
                alarmEnabled = TRUE;
                sleep(1);
            }    

            int bytes = readByte(disconnect_frame);
            
            if (bytes > 0) {
                switch (state) {
                    case START:
                        state = disconnect_frame[0] == FLAG ? FLAG_RCV : START;
                        break;
                    case FLAG_RCV:
                        if (disconnect_frame[0] == FLAG) {
                            state = FLAG; 
                        } else {
                            state = disconnect_frame[0] == ADDRESS_RX ? A_RCV : START;
                        }
                        break;
                    case A_RCV:
                        if (disconnect_frame[0] == FLAG) {
                            state = FLAG; 
                        } else {
                            state = disconnect_frame[0] == DISC ? C_RCV : START;
                        }
                        break;
                    case C_RCV:
                        if (disconnect_frame[0] == FLAG) {
                            state = FLAG; 
                        } else {
                            state = disconnect_frame[0] == (ADDRESS_RX ^ DISC) ? BCC_OK : START;
                        }
                        break;
                    case BCC_OK:
                        if (disconnect_frame[0] == FLAG) {
                            state = STOP;
                            alarm(0);
                            alarmEnabled = FALSE;
                        } else {
                            state = START;
                        }
                        break;
                    case STOP:
                        printf("Received DISC frame\n");
                        break;
                    }        
                }
            }

            // send UA and close
            unsigned char ua[5];
            ua[0] = FLAG;
            ua[1] = ADDRESS_RX;
            ua[2] = CONTROL_UA;
            ua[3] = ADDRESS_RX ^ CONTROL_UA;
            ua[4] = FLAG;

            writeBytes(ua, 5);
            printf("DISC-UA: Disconnected\n");
            break;
        case LlRx:
            break;
    }

    int clstat = closeSerialPort();
    return clstat;
}
