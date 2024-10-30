// Link layer header.
// NOTE: This file must not be changed.

#ifndef _LINK_LAYER_H_
#define _LINK_LAYER_H_

typedef enum
{
    LlTx,
    LlRx,
} LinkLayerRole;

typedef enum
{
    START, 
    FLAG_RCV, 
    A_RCV, 
    C_RCV, 
    C_REJ,
    C_DISC,
    BCC_OK,
    BCC_REJ,
    BCC_DISC, 
    BCC2_OK,
    STOP
} FrameState;

typedef struct
{
    char serialPort[50];
    LinkLayerRole role;
    int baudRate;
    int nRetransmissions;
    int timeout;
} LinkLayer;

typedef struct {
    int totalFrames;
    int rejectedFrames;
    int retransmissions;
    int timeouts;
} LinkLayerStats;

// SIZE of maximum acceptable payload.
// Maximum number of bytes that application layer should send to link layer
#define MAX_PAYLOAD_SIZE 1000

// MISC
#define FALSE 0
#define TRUE 1

// Control frame size
#define CTRL_FRAME_SIZE 0x05

// Frame control fields
#define FLAG 0x7E
#define ADDRESS_TX 0x03
#define ADDRESS_RX 0x01
#define CONTROL_SET 0x03
#define CONTROL_UA 0x07
#define CTRL_I0 0x00
#define CTRL_I1 0x80
#define ESCAPE 0x7D
#define RR0 0xAA
#define RR1 0xAB
#define REJ0 0x54
#define REJ1 0x55
#define DISC 0X0B

// Open a connection using the "port" parameters defined in struct linkLayer.
// Return "1" on success or "-1" on error.
int llopen(LinkLayer connectionParameters);

// Send data in buf with size bufSize.
// Return number of chars written, or "-1" on error.
int llwrite(const unsigned char *buf, int bufSize);

// Receive data in packet.
// Return number of chars read, or "-1" on error.
int llread(unsigned char *packet);

// Close previously opened connection.
// if showStatistics == TRUE, link layer should print statistics in the console on close.
// Return "1" on success or "-1" on error.
int llclose(int showStatistics);

#endif // _LINK_LAYER_H_
