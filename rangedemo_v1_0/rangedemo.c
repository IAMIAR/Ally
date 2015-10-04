/*= rangedemo.c ====================================================================================
 *
 * Copyright (C) 2004 Nordic Semiconductor
 *
 * This file is distributed in the hope that it will be useful, but WITHOUT
 * WARRANTY OF ANY KIND.
 *
 * Author(s): Ole Saether
 *
 * DESCRIPTION:
 *
 *  nRF9E5/nRF24E1 range demo. Select receiver by shorting P03 and P05 and transmitter by shorting
 *  P05 and P06. Uncomment/comment the appropriate #define below to select nRF9E5 or nRF24E1.
 *
 *  The transmitter continously send one byte packets. Each time the receiver receives a packet the
 *  P00 pin is set low (LED1 is turned on on the 9E5 eval board). At the same time a 20ms timer is
 *  started and if a new packets is not received before the 20ms time-out the P00 pin is set high
 *  (LED1 is turned off). If a new packet is received before the time-out a new 20ms time-out period
 *  is started.
 *
 *  Please remember to turn off the RS232 switch on the receiver and transmitter boards. On the
 *  nRF9E5 board turn off all dip-switches on the transmitter and turn on only the LED1 switch on
 *  the receiver.
 *
 * COMPILER:
 *  
 *  This program has been tested with Keil C51 V7.09
 *
 * $Revision: 1 $
 *
 *==================================================================================================
*/
// Comment out the following line for nRF24E1
#define NRF9E5 1


#ifdef NRF9E5
#include <Nordic\reg9e5.h>
#define POWER      3                // 0=min power...3 = max power
#define HFREQ      1                // 0=433MHz, 1=868/915MHz
#define CHANNEL  351                // Channel number: f(MHz) = (422.4+CHANNEL/10)*(1+HFREQ) 
#else
#include <nordic\reg24e1.h>
#endif

#define TIMEOUT    20               // 20ms time-out on LED
#define ON          1
#define OFF         0

static volatile unsigned char timer;
static volatile unsigned char t0lrel, t0hrel;

#ifndef NRF9E5

struct RFConfig
{
    unsigned char n;
    unsigned char buf[15];
};

typedef struct RFConfig RFConfig;

#define ADDR_INDEX  8               // Index to address bytes in RFConfig.buf
#define ADDR_COUNT  4               // Number of address bytes

const RFConfig tconf =
{
    15,
    0x08,                           // Payload size transmitter Rx #2 (not used in this example)
    0x08,                           // Payload size transmitter Rx #1 (not used in this example)
    0x00, 0x00, 0x00, 0x00, 0x00,   // Address of transmitter Rx #2 (not used in this example)
    0x00, 0x12, 0x34, 0x56, 0x78,   // Address of transmitter Rx #1 (not used in this example)
    0x81, 0x6f, 0x04
};

const RFConfig rconf =
{
    15,
    0x08,                           // Payload size receiver Rx #2 (not used in this example)
    0x08,                           // Payload size receiver Rx #1
    0x00, 0x00, 0x00, 0x00, 0x00,   // Address receiver Rx #2 (not used in this example)
    0x00, 0x12, 0x34, 0x56, 0x78,   // Address receiver Rx #1 (four lower bytes used here)
    0x81, 0x6f, 0x05
};

#endif

void Delay100us(volatile unsigned char n)
{
    unsigned char i;
    while(n--)
        for(i=0;i<35;i++)
            ;
}

unsigned char SpiReadWrite(unsigned char b)
{
    EXIF &= ~0x20;                  // Clear SPI interrupt
    SPI_DATA = b;                   // Move byte to send to SPI data register
    while((EXIF & 0x20) == 0x00)    // Wait until SPI hs finished transmitting
        ;
    return SPI_DATA;
}

unsigned char ReceivePacket()
{
    unsigned char b;

#ifdef NRF9E5
    TRX_CE = 1;
    while(DR == 0)
        ;
    RACSN = 0;
    SpiReadWrite(RRP);
    b = SpiReadWrite(0);
    RACSN = 1;
    TRX_CE = 0;
#else
    CE = 1;
    while(DR1 == 0)
        ;
    b = SpiReadWrite(0);
    CE = 0;
#endif
    return b;
}

void TransmitPacket(unsigned char b)
{
#ifdef NRF9E5
    RACSN = 0;
    SpiReadWrite(WTP);
    SpiReadWrite(b);
    RACSN = 1;
    TRX_CE = 1;
    Delay100us(1);
    TRX_CE = 0;
    while(DR == 0)
        ;
#else
    unsigned char i;

    CE = 1;
    Delay100us(0);
    // All packets start with the receiver address:
    for(i=0;i<ADDR_COUNT;i++)
        SpiReadWrite(rconf.buf[ADDR_INDEX+i]);
    SpiReadWrite(b);
    CE = 0;
    Delay100us(3);                  // Wait ~300us
#endif
}

void Led(unsigned char on)
{
    if (on)
    {
        P0 &= ~0x01;
        timer = 0;  
        TR0 = 1;                    // Start Timer0
    } else
        P0 |= 0x01;
}

void InitTimer(void)
{
    TR0 = 0;
    TMOD &= ~0x03;
    TMOD |= 0x01;                   // mode 1
    CKCON |= 0x08;                  // T0M = 1 (/4 timer clock)
    t0lrel = 0x60;                  // 1KHz tick...
    t0hrel = 0xF0;                  // ... = 65536-16e6/(4*1e3) = F060h
    TF0 = 0;                        // Clear any pending Timer0 interrupts
    ET0 = 1;                        // Enable Timer0 interrupt
}

void Timer0ISR (void) interrupt 1
{
    TF0 = 0;                        // Clear Timer0 interrupt
    TH0 = t0hrel;                   // Reload Timer0 high byte
    TL0 = t0lrel;                   // Reload Timer0 low byte
    timer++;
    if (timer == TIMEOUT)
    {
        P0 |= 0x01;                 // Led off
        TR0 = 0;                    // Stop timer
    }
}

void Receiver(void)
{
    unsigned char b, bo, err;

#ifdef NRF9E5
    TXEN = 0;
#else
    CS = 1;
    Delay100us(0);
    for(b=0;b<rconf.n;b++)
    {
        SpiReadWrite(rconf.buf[b]);
    }
    CS = 0;
#endif
    bo = err = 0;
    for(;;)
    {
        b = ReceivePacket();
        Led(ON);
    }
}

void Transmitter(void)
{
    unsigned char b;
    
#ifdef NRF9E5
    TXEN = 1;
#else
    CS = 1;
    Delay100us(0);
    for(b=0;b<tconf.n;b++)
    {
        SpiReadWrite(tconf.buf[b]);
    }
    CS = 0;
#endif
    b = 0;
    for(;;)
    {
        TransmitPacket(b++);        // Transmit data
    }
}

void Init(void)
{
    unsigned char tmp;

    P0_DIR = 0x20;                  // P0.5 is input, the rest output
    P0 = 0x89;                      // P0.3 = 1 and P0.6=0 for the rec/tran selection
                                    // LED off
    SPICLK = 1;
    SPI_CTRL = 0x02;                // Connect internal SPI controller to Radio

#ifdef NRF9E5
    // Switch to 16MHz clock:
    RACSN = 0;
    SpiReadWrite(RRC | 0x09);
    tmp = SpiReadWrite(0) | 0x04;
    RACSN = 1;

    RACSN = 0;
    SpiReadWrite(WRC | 0x09);
    SpiReadWrite(tmp);
    RACSN = 1;

    // Configure Radio:
    RACSN = 0;
    SpiReadWrite(WRC | 0x03);       // Write to RF config address 3 (RX payload)
    SpiReadWrite(0x01);             // One byte RX payload
    SpiReadWrite(0x01);             // One byte TX payload
    RACSN = 1;

    RACSN = 0;
    SpiReadWrite(RRC | 0x01);       // Read RF config address 1
    tmp = SpiReadWrite(0) & 0xf0;   // Clear the power and frequency setting bits
    RACSN = 1;

    RACSN = 0;
    SpiReadWrite(WRC);              // Write RF config address 0
    SpiReadWrite(CHANNEL & 0xff);   // CHANNEL bit7..0
    // Change power defined by POWER above, to 433 or 868/915MHz defined by HFREQ and
    // bit8 of CHANNEL:
    SpiReadWrite(tmp | (POWER<<2) | (HFREQ << 1) | ((CHANNEL >> 8) & 0x01));
    RACSN = 1;
#else
    PWR_UP = 1;                     // Turn on Radio on 24E1
    Delay100us(30);                 // Wait > 3ms
#endif

    InitTimer();
    EA = 1;
}

void main(void)
{
    Init();
    if(P0 & 0x20)
    {
        Receiver();
    }
    else
    {
        Transmitter();
    }
}
