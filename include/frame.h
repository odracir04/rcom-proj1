#ifndef FRAME_H
#define FRAME_H

// Frame reception states
typedef enum
{
    START, 
    FLAG_RCV, 
    A_RCV, 
    C_RCV,
    C_RR, 
    C_REJ,
    C_DISC,
    BCC_OK,
    BCC_RR,
    BCC_REJ,
    BCC_DISC, 
    BCC2_OK,
    ERROR,
    STOP
} FrameState;

// Frame sizes
#define CTRL_FRAME_SIZE 5
#define IFRAME_FLAGS 6

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

#endif