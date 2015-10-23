
#include <Nordic\reg9e5.h>   
   
#define    HFREQ    0                              
#define    POWER    3                               
#define    UART_BUFFER_SIZE    10                   
   
unsigned char  UART_BUFFER[UART_BUFFER_SIZE+1];   
unsigned char  countt=0;                            
   
unsigned int  Tnum=400;                             
bit    Overtime_Flag;                              
   
   
//????   
void Delay400us(volatile unsigned char n)   
{   
    unsigned char i;   
    while(n--)   
        for(i=0;i<35;i++)   
            ;   
}   
   
//???????T0???   
void Time0_Init(void)   
{   
    TMOD &= 0xF0;          
    TMOD |= 0x02;   
    CKCON |= 0x08;         
    TH0=0x38;              
    ET0=1;                 
    TR0=1;                 
    Overtime_Flag=0;       
}   
   
//???T0???????   
void Timer0_ISR (void) interrupt 1   
{   
     //TF0=0;               
     Tnum--;                 
     if(Tnum==0)            
      {   
       TR0=0;                  
       Overtime_Flag=1;   
       P06=~P06;                 
      }   
}   
   
//????????    
void serial_ISR (void) interrupt 4   
{   
    if(RI)   
      {   
          unsigned char ch;             
          RI = 0;   
          ch = SBUF;   
          //TR0=1;   
          if(Overtime_Flag==0)               
             {                               
                if(countt==UART_BUFFER_SIZE)     
                        countt=0;   
                countt++;   
                UART_BUFFER[0]=countt;   
                UART_BUFFER[countt]=ch;   
             }     
      }   
    if(TI)    
      {   
          //TI=0;   
          return;   
      }   
}   
   
//SPI??   
unsigned char SpiReadWrite(unsigned char b)   
{   
    EXIF &= ~0x20;                  // Clear SPI interrupt   
    SPI_DATA = b;                   // Move byte to send to SPI data register   
    while((EXIF & 0x20) == 0x00)    // Wait until SPI hs finished transmitting   
           ;   
    return SPI_DATA;   
}   
   
//????   
void Transmitter(unsigned char *str,unsigned char strlen)   
{   
    unsigned char j=0;   
    TRX_CE=0;                      
    //Delay400us(10);   
    RACSN = 0;                     
    SpiReadWrite(WTP);          
    for(j=0;j<strlen;j++)   
      {   
       SpiReadWrite(*(str + j));    
      }   
    
    RACSN = 1;   
    TXEN = 1;   
    TRX_CE = 1;                 
    Delay400us(2);   
    TRX_CE = 0;                 
    P04=~P04;   
//    countt=0;   
}   
   
//???   
void Init(void)   
{   
    unsigned char tmp,cklf,tmpa,tmpb;   
       
    TH1 = 0xE6;                      // 9600@16MHz (when T1M=1 and SMOD=1)   
    CKCON |= 0x10;                  // T1M=1 (/4 timer clock)   
    PCON = 0x80;                    // SMOD=1 (double baud rate)   
    SCON = 0x50;                    // Serial mode1, enable receiver   
    TMOD = 0x20;                    // Timer1 8bit auto reload    
    TR1 = 1;                        // Start timer1     
//    P0_ALT |= 0x06;   
//    P0_DIR |= 0x02;                 // P0.1 (RxD) is input   
    tmpa= P0_ALT | 0x06;                 // Select alternate functions on pins P0.1 and P0.2   
    P0_ALT = tmpa & 0x06;   
    tmpb = P0_DIR | 0x02;                 // P0.1 (RxD) is input   
    P0_DIR =tmpb & 0x02;   
   
    SPICLK = 0;                     // Max SPI clock   
    SPI_CTRL = 0x02;                // Connect internal SPI controller to Radio   
   
    // Configure Radio:   
    RACSN = 0;   
    SpiReadWrite(WRC | 0x03);       // Write to RF config address 3 (RX payload)   
    SpiReadWrite(0x01);             // One byte RX payload   
    SpiReadWrite(0x01);             // One byte TX payload   
    RACSN = 1;   
   
    RACSN = 0;   
    SpiReadWrite(RRC | 0x01);       // Read RF config address 1   
    tmp = SpiReadWrite(0) & 0xf1;   //  PA_PWR[1:0]=00(????-10dBm) HFREQ_PLL=0(433MHz)  Clear the power and frequency setting bits   
    RACSN = 1;   
   
    RACSN = 0;   
    SpiReadWrite(WRC | 0x01);      // Write RF config address 1   
    SpiReadWrite(tmp | (POWER <<2) | (HFREQ << 1));   // Change power defined by POWER and to 433 or 868/915MHz defined by HFREQ above:   
    RACSN = 1;   
   
   
    // Switch to 16MHz clock:   
    RACSN = 0;   
    SpiReadWrite(RRC | 0x09);   
    cklf = SpiReadWrite(0) | 0x1c;   
    RACSN = 1;   
    RACSN = 0;   
    SpiReadWrite(WRC | 0x09);   
    SpiReadWrite(cklf);   
    RACSN = 1;   
   
    RACSN = 0;   
    SpiReadWrite(WTA);                
    SpiReadWrite(0x18);   
    SpiReadWrite(0x32);   
    SpiReadWrite(0x40);   
    SpiReadWrite(0xAF);   
    RACSN = 1;   
   
    RACSN = 0;   
    SpiReadWrite(WRC | 0x04);       
    SpiReadWrite(0x20);            
    RACSN = 1;   
}   
   
void main(void)   
{   
    Init();   
    Time0_Init();     
    EA=1;   
    ES=1;   
    //PT0=1;       
      
    while(1)   
     {   
        if(Overtime_Flag)     
        {   
            ES=0;               
            //UART_BUFFER[0]=countt;   
            if(countt != 0)     
                   Transmitter(UART_BUFFER,(UART_BUFFER[0]+1));   
            countt=0;     
            Overtime_Flag=0;      
            Tnum=400;   
            TL0=0x38;           
            TR0=1;           
            ES=1;   
        }            
    }     
   
}