 // Link layer protocol implementation

#include <signal.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <termios.h>
#include <unistd.h>
#include <time.h>

#include "../include/link_layer.h"
#include "../include/serial_port.h"
#include "../include/alarm.h"
#include "../include/stats.h"
#include "../include/frame.h"

// MISC
#define _POSIX_SOURCE 1 // POSIX compliant source

extern int alarmEnabled;
extern int alarmCount;

clock_t start_time = 0;
int current_frame = 0;
LinkLayer connection;
LinkLayerStats stats = {0};

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
    FrameState state;
    start_time = clock();
    unsigned char buf[CTRL_FRAME_SIZE] = {0};

    (void)signal(SIGALRM, alarmHandler);

    switch (connectionParameters.role) {
        case LlTx:
            buf[0] = FLAG;
            buf[1] = ADDRESS_TX;
            buf[2] = CONTROL_SET;
            buf[3] = ADDRESS_TX ^ CONTROL_SET;
            buf[4] = FLAG;

            state = START; 
            while (state != STOP && alarmCount < connectionParameters.nRetransmissions)
            {
            
            if (!alarmEnabled) {
                stats.sentFrames++;

                int bytes = writeBytes(buf, CTRL_FRAME_SIZE);
                alarm(connectionParameters.timeout);
                alarmEnabled = TRUE;
                sleep(1);
            }    

            int bytes = readByte(buf);
            
            if (bytes > 0) {
                switch (state) {
                    case START:
                        state = buf[0] == FLAG ? FLAG_RCV : START;
                        break;
                    case FLAG_RCV:
                        if (buf[0] == FLAG) {
                            state = FLAG_RCV; 
                        } else {
                            state = buf[0] == ADDRESS_TX ? A_RCV : START;
                        }
                        break;
                    case A_RCV:
                        if (buf[0] == FLAG) {
                            state = FLAG_RCV; 
                        } else {
                            state = buf[0] == CONTROL_UA ? C_RCV : START;
                        }
                        break;
                    case C_RCV:
                        if (buf[0] == FLAG) {
                            state = FLAG_RCV; 
                        } else {
                            state = buf[0] == (ADDRESS_TX ^ CONTROL_UA) ? BCC_OK : START;
                        }
                        break;
                    case BCC_OK:
                        if (buf[0] == FLAG) {
                            state = STOP;
                            alarm(0);
                            alarmEnabled = FALSE;
                            stats.receivedFrames++;
                        } else {
                            state = START;
                        }
                        break;
                    case STOP:
                        break;
                    }        
                }
            }
            break;
        case LlRx:
            state = START; 
            while (state != STOP)
            {
            
            int bytes = readByte(buf);
            
            if (bytes > 0) {
                switch (state) {
                    case START:
                        state = buf[0] == FLAG ? FLAG_RCV : START;
                        break;
                    case FLAG_RCV:
                        if (buf[0] == FLAG) {
                            state = FLAG_RCV; 
                        } else {
                            state = buf[0] == ADDRESS_TX ? A_RCV : START;
                        }
                        break;
                    case A_RCV:
                        if (buf[0] == FLAG) {
                            state = FLAG_RCV; 
                        } else {
                            state = buf[0] == CONTROL_SET ? C_RCV : START;
                        }
                        break;
                    case C_RCV:
                        if (buf[0] == FLAG) {
                            state = FLAG_RCV; 
                        } else {
                            state = buf[0] == (ADDRESS_TX ^ CONTROL_SET) ? BCC_OK : START;
                        }
                        break;
                    case BCC_OK:
                        if (buf[0] == FLAG) {
                            state = STOP;
                            alarm(0);
                            stats.receivedFrames++;
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
            stats.sentFrames++;
            sleep(1);
            break;
        }

    return alarmCount == connection.nRetransmissions ? -1 : 1;
}

////////////////////////////////////////////////
// LLWRITE
////////////////////////////////////////////////
int llwrite(const unsigned char *buf, int bufSize)
{
    alarmCount = 0;

    char bcc2 = 0;
    unsigned char frame[2 * bufSize + IFRAME_FLAGS];   

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
        
    if (bcc2 == FLAG || bcc2 == ESCAPE) {
            frame[current_position] = ESCAPE;
            current_position++;
            frame[current_position] = bcc2 ^ 0x20;
            current_position++;
        } else {
            frame[current_position] = bcc2;
            current_position++;
    }

    frame[current_position] = FLAG;

    unsigned char rr[2]; 
    FrameState state = START;
        
    while (state != STOP && alarmCount < connection.nRetransmissions) {
        
        if (!alarmEnabled) {
            stats.sentFrames++;
            int nbytes = writeBytes(frame, current_position + 1); 
            alarm(connection.timeout);
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
                        state = C_REJ;
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
                case C_REJ:
                    if (rr[0] == FLAG) {
                        state = FLAG_RCV; 
                    } else {
                        if ((current_frame == 0 && rr[0] == (ADDRESS_TX ^ REJ0)) || (current_frame == 1 && rr[0] == (ADDRESS_TX ^ REJ1))) {
                            state = BCC_REJ;
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
                        stats.receivedFrames++;
                    } else {
                        state = START;
                    }
                    break;
                case BCC_REJ:
                    if (rr[0] == FLAG) {
                        stats.rejectedFrames++;
                        stats.retransmissions++;
                        stats.receivedFrames++;
                        state = START;
                        alarm(0);
                        alarmEnabled = FALSE;
                    } else {
                        state = START;
                    }
                    break;
                case STOP:
                    break;
            }        
        }
    }

    return alarmCount == connection.nRetransmissions ? -1 : current_position;
}

////////////////////////////////////////////////
// LLREAD
////////////////////////////////////////////////
int llread(unsigned char *packet)
{
    unsigned char rr[2];     
    FrameState state = START;
    int packet_position = 0;
    unsigned char is7d = FALSE;
    unsigned char bbc2 = 0;
    unsigned char keep = 0;
    unsigned char buf[CTRL_FRAME_SIZE];
        
    while (state != STOP) {
        int bytes = readByte(rr);
        int bcc2 = 0;
    
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
                        if (current_frame == 0 && rr[0] == CTRL_I0) {
                            state = C_RR;
                        } else if (current_frame == 1 && rr[0] == CTRL_I1) {
                            state = C_RR;
                        } else if (rr[0] == DISC) {
                            state = C_DISC;
                        } else if (rr[0] == CONTROL_SET) {
                            state = C_RCV;
                        }
                        else {
                            if ((current_frame == 0 && rr[0] == CTRL_I1) || (current_frame == 1 && rr[0] == CTRL_I0)) {
                                stats.receivedFrames++;
                            }
                            state = START;
                        }
                    }
                    break;
                case C_RCV:
                    if (rr[0] == (ADDRESS_TX ^ CONTROL_SET)) {
                        state = BCC_OK;
                    } else {
                        state = START;
                    }
                    break;
                case C_RR:
                    if (rr[0] == FLAG) {
                        state = FLAG_RCV; 
                    } else if ((rr[0] == (ADDRESS_TX ^ CTRL_I0) && current_frame == 0) || (rr[0] == (ADDRESS_TX ^ CTRL_I1) && current_frame == 1)) {
                        state = BCC_RR;
                    } else {
                        state = ERROR;
                    }
                    break;
                case C_DISC:
                    if (rr[0] == (ADDRESS_TX ^ DISC)) {
                        state = BCC_DISC;
                    } else {
                        state = ERROR;
                    }
                    break;
                case BCC_OK:
                    if (rr[0] == FLAG) {
                        unsigned char buf[CTRL_FRAME_SIZE];
                        buf[0] = FLAG;
                        buf[1] = ADDRESS_TX;
                        buf[2] = CONTROL_UA;
                        buf[3] = ADDRESS_TX ^ CONTROL_UA;
                        buf[4] = FLAG;

                        int bytes = writeBytes(buf, CTRL_FRAME_SIZE);
                        stats.sentFrames++;
                        stats.receivedFrames++;
                        stats.retransmissions++;
                        sleep(1);
                        state = START;
                    }
                    break;
                case BCC_RR:
                    if (rr[0] == bbc2 && !is7d) {
                        keep = rr[0];
                        state = BCC2_OK;
                    } else if (rr[0] == FLAG) {
                        if (bbc2 == 0) {
                            stats.receivedFrames++;
                            state = STOP;
                        } else {
                            state = ERROR;
                        }
                        break;
                    } else {
                        if (rr[0] == 0x7d) {
                            is7d = TRUE;
                            continue;
                        } else if (is7d) {
                            if (rr[0] == 0x5e) {
                                packet[packet_position] = 0x7e;
                                bbc2 ^= 0x7e;
                                packet_position++;
                            } else if (rr[0] == 0x5d) {
                                packet[packet_position] = 0x7d;
                                bbc2 ^= 0x7d;
                                packet_position++;
                            } else return -1;
                            is7d = FALSE;
                            continue;
                        }
                        packet[packet_position] = rr[0];
                        bbc2 ^= rr[0];
                        packet_position++;
                    }
                    break;
                case BCC2_OK:
                    if (rr[0] == FLAG) {
                        if ((bbc2 ^ keep) == 0) {
                            stats.receivedFrames++;
                            state = STOP;
                        } else {
                            state = ERROR;
                        }  
                    } else {
                        state = BCC_RR;
                        if (keep == 0x7d) {
                            if (rr[0] == 0x5e) {
                                packet[packet_position] = 0x7e;
                                bbc2 ^= 0x7e;
                                packet_position++;
                            } else if (rr[0] == 0x5d) {
                                packet[packet_position] = 0x7d;
                                bbc2 ^= 0x7d;
                                packet_position++;
                            } else return -1;
                        } else {
                            bbc2 ^= keep;
                            packet[packet_position] = keep;
                            packet_position++;
                            if (rr[0] == 0x7d) {
                                is7d = TRUE;
                                continue;
                            } if (rr[0] == bbc2) {
                                bbc2 ^= rr[0];
                                continue;
                            } else {
                                packet[packet_position] = rr[0];
                                bbc2 ^= rr[0];
                                packet_position++;
                            }
                        }
                    }
                    break;
                case BCC_DISC:
                    if (rr[0] == FLAG) {
                        stats.receivedFrames++;
                        return 0;
                    } else {
                        state = ERROR;
                    }
                    break;
                case ERROR:
                    stats.rejectedFrames++;
                    stats.receivedFrames++;
                    stats.sentFrames++;
                    buf[0] = FLAG;
                    buf[1] = ADDRESS_TX;
                    if (current_frame == 0) {
                        buf[2] = REJ0;
                    } else {
                        buf[2] = REJ1;
                    }
                    buf[3] = ADDRESS_TX ^ buf[2];
                    buf[4] = FLAG;

                    int bytes = writeBytes(buf, CTRL_FRAME_SIZE);
                    sleep(1);
                    state = START;

                    packet_position = 0;
                    is7d = FALSE;
                    bbc2 = 0;
                    keep = 0;
                    break;
                case STOP:
                    break;
                }        
        }
    }

    buf[0] = FLAG;
    buf[1] = ADDRESS_TX;
    if (current_frame == 0) {
        buf[2] = RR1;
        current_frame = 1;
    } else {
        buf[2] = RR0;
        current_frame = 0;
    }
    buf[3] = ADDRESS_TX ^ buf[2];
    buf[4] = FLAG;

    int bytes = writeBytes(buf, CTRL_FRAME_SIZE);
    stats.sentFrames++;
    sleep(1);
    return packet_position;
}

////////////////////////////////////////////////
// LLCLOSE
////////////////////////////////////////////////
int llclose(int showStatistics)
{
    FrameState state;
    unsigned char disconnect_frame[CTRL_FRAME_SIZE];

    switch (connection.role) {
        case LlTx:
            disconnect_frame[0] = FLAG;
            disconnect_frame[1] = ADDRESS_TX;
            disconnect_frame[2] = DISC;
            disconnect_frame[3] = ADDRESS_TX ^ DISC;
            disconnect_frame[4] = FLAG;

            state = START; 
            alarmCount = 0;

            while (state != STOP && alarmCount < connection.nRetransmissions)
            {
            
            if (!alarmEnabled) {
                stats.sentFrames++;
                int bytes = writeBytes(disconnect_frame, CTRL_FRAME_SIZE);
                alarm(connection.timeout);
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
                            state = FLAG_RCV; 
                        } else {
                            state = disconnect_frame[0] == ADDRESS_RX ? A_RCV : START;
                        }
                        break;
                    case A_RCV:
                        if (disconnect_frame[0] == FLAG) {
                            state = FLAG_RCV; 
                        } else {
                            state = disconnect_frame[0] == DISC ? C_RCV : START;
                        }
                        break;
                    case C_RCV:
                        if (disconnect_frame[0] == FLAG) {
                            state = FLAG_RCV; 
                        } else {
                            state = disconnect_frame[0] == (ADDRESS_RX ^ DISC) ? BCC_OK : START;
                        }
                        break;
                    case BCC_OK:
                        if (disconnect_frame[0] == FLAG) {
                            stats.receivedFrames++;
                            state = STOP;
                            alarm(0);
                            alarmEnabled = FALSE;
                        } else {
                            state = START;
                        }
                        break;
                    case STOP:
                        break;
                    }        
                }
            }

            unsigned char ua[CTRL_FRAME_SIZE];
            ua[0] = FLAG;
            ua[1] = ADDRESS_RX;
            ua[2] = CONTROL_UA;
            ua[3] = ADDRESS_RX ^ CONTROL_UA;
            ua[4] = FLAG;

            writeBytes(ua, CTRL_FRAME_SIZE);
            stats.sentFrames++;
            break;
        case LlRx:
            disconnect_frame[0] = FLAG;
            disconnect_frame[1] = ADDRESS_RX;
            disconnect_frame[2] = DISC;
            disconnect_frame[3] = ADDRESS_RX ^ DISC;
            disconnect_frame[4] = FLAG;

            state = START; 
            alarmCount = 0;

            while (state != STOP && alarmCount < connection.nRetransmissions)
            {
            
            if (!alarmEnabled) {
                stats.sentFrames++;
                int bytes = writeBytes(disconnect_frame, CTRL_FRAME_SIZE);
                alarm(connection.timeout);
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
                            state = FLAG_RCV; 
                        } else {
                            state = disconnect_frame[0] == ADDRESS_RX ? A_RCV : START;
                        }
                        break;
                    case A_RCV:
                        if (disconnect_frame[0] == FLAG) {
                            state = FLAG_RCV; 
                        } else {
                            state = disconnect_frame[0] == CONTROL_UA ? C_RCV : START;
                        }
                        break;
                    case C_RCV:
                        if (disconnect_frame[0] == FLAG) {
                            state = FLAG_RCV; 
                        } else {
                            state = disconnect_frame[0] == (ADDRESS_RX ^ CONTROL_UA) ? BCC_OK : START;
                        }
                        break;
                    case BCC_OK:
                        if (disconnect_frame[0] == FLAG) {
                            stats.receivedFrames++;
                            state = STOP;
                            alarm(0);
                            alarmEnabled = FALSE;
                        } else {
                            state = START;
                        }
                        break;
                    case STOP:
                        break;
                    }        
                }
            }
            break;
    }

    stats.elapsedTime = (double)(clock() - start_time) / CLOCKS_PER_SEC;

    if (showStatistics) {
        printf("\nCommunication Summary:\n");
        printf("------------------------------------\n");
        printf("Sent Frames: %d\n", stats.sentFrames);
        printf("Received Frames: %d\n", stats.receivedFrames);
        printf("Rejected Frames: %d\n", stats.rejectedFrames);
        printf("Timeouts: %d\n", stats.timeouts);
        printf("Retransmissions: %d\n", stats.retransmissions);
        printf("%% of retransmissions in sent: %.2lf%%\n", stats.retransmissions * 1.0 / stats.sentFrames * 100);
        printf("Total elapsed time: %.2lf seconds\n", stats.elapsedTime);
    }

    int clstat = closeSerialPort();
    return clstat;
}
