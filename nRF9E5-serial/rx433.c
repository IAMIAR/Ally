/*******************************************
˫�������շ����ն˳���
        �����߽��յ����ַ���ʾ��PC���ڡ�

�޸ģ������յ����ݸ�����ԭ�ȵĻ�������С�жϸĳ��ɽ��յ���һ���ֽ��жϡ�
******************************************/

#include <Nordic\reg9e5.h>

#define   HFREQ 0                     // 0=433MHz, 1=868/915MHz     (������ģ�
#define   UART_BUFFER_SIZE    10       //���崮�ڻ�������С 10 bytes

unsigned char UART_BUFFER[UART_BUFFER_SIZE+1];

char echo[6]="ATE1\r\n";
char mode[11]="AT+CMGF=1\r\n";
char recp[24]="AT+CMGS=\"+12014233865\"\r\n";
char message[11]="HelloWorld\n";

//���ڷ���1Byte
void PutChar(char c)
{
    while(!TI)			            //Wait until finished seri-port transmitting
        ;
	TI = 0;			               //Clear interrupt flag
    SBUF = c;			           //sending a byte
}

//���ڷ����ַ���
void PutString(const char *s,unsigned char strlen)
{
    unsigned char k;
	//EIE &= 0xF0;
    for(k=1;k<=strlen;k++)
       {
         PutChar(*(s+k));
       }
	//EIE |= 0x0F;
}
/*void PutString(const char *s)
{
    while(*s != '\n')
        PutChar(*s++);
}*/


unsigned char SpiReadWrite(unsigned char b)
{
    EXIF &= ~0x20;                  // Clear SPI interrupt        (EXIP.5=0)
    SPI_DATA = b;                   // Move byte to send to SPI data register
    while((EXIF & 0x20) == 0x00)    // Wait until SPI hs finished transmitting   (����EXIF.5��״̬)
          ;
    return SPI_DATA;
}


void Receiver(void)
{
    unsigned char i=0;
    TXEN = 0;
    TRX_CE = 1;                     //�������ģʽ
    while(DR == 0)                  //DR=0��ʾ����δ׼���ã��ȴ�
        ;

    TRX_CE=0;                       //�������ģʽ�ſɶ�дSPI
    RACSN = 0;                      //�ڲ�SPI��ʼ�ȴ����1��0�����䣩
    SpiReadWrite(RRP);  
/*
while(DR == 1)
{ 	UART_BUFFER[i] = SpiReadWrite(0); 
    i++;
}
*/            		
	UART_BUFFER[0] = SpiReadWrite(0); 
	for(i=1;i<=UART_BUFFER[0];i++)   //****�޸Ĵ�*****
	{
		UART_BUFFER[i] = SpiReadWrite(0); 		 
	}
	P04 = ~P04;
    RACSN = 1;
//    TRX_CE = 1;                     //��ʱMCU�Ŵ�SPI������
}

void UART_Init(void)                //nRF9E5���ڳ�ʼ��
{
    TH1 = 0xE6;                      // 9600@16MHz (when T1M=1 and SMOD=1)
    CKCON |= 0x30;                  // T1M=1 (/4 timer clock)
    PCON = 0x80;                    // SMOD=1 (double baud rate)
    SCON = 0x52;                    // Serial mode1, enable receiver
    TMOD = 0x20;                    // Timer1 8bit auto reload 
    TR1 = 1;                        // Start timer1  
	  TI = 1;
    P0_ALT |= 0x06;
    P0_DIR  = 0xA2;                // P0.1 (RxD) is input*/
}

void Init(void)                     //nRF9E5����ģ���ʼ��
{
    unsigned char tmp,cklf;
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
    tmp = SpiReadWrite(0) & 0xf1;   // Clear the power and frequency setting bits  
                                    //PA_PWR[1:0]=00(�����Դ-10dBm��Bit1��HFREQ_PLL=0��433MHz)
    RACSN = 1;

    RACSN = 0;
    SpiReadWrite(WRC | 0x01);           // Write RF config address 1
    SpiReadWrite(tmp | (HFREQ << 1));   // Change to 433 or 868/915MHz defined by HFREQ above:
    RACSN = 1;


    // ��ʱ��Ƶ�ʸ�Ϊ16MHz:
    RACSN = 0;
    SpiReadWrite(RRC | 0x09);
    cklf = SpiReadWrite(0) | 0x1c;
    RACSN = 1;
    RACSN = 0;
    SpiReadWrite(WRC | 0x09);
    SpiReadWrite(cklf);
    RACSN = 1;


    RACSN = 0;
    SpiReadWrite(WRC | 0x05);       //���ñ��������ɽ��ܵĵ�ַ,��tx433�������һ���ſ�ͨ��
    SpiReadWrite(0x18);
    SpiReadWrite(0x32);
    SpiReadWrite(0x40);
    SpiReadWrite(0xAF);
    RACSN = 1;

    RACSN = 0;
    SpiReadWrite(WRC | 0x03);       //���ñ��������ɽ��ܵ����ݿ��,��tx433�������һ���ſ�ͨ��
    SpiReadWrite(0x20);             //�������ݿ��Ϊ32���ֽ�
    RACSN = 1;


}


void Delay(volatile unsigned char n)
{
    unsigned char i;
    while(n--)
        for(i=0;i<100;i++)
            ;
}

void main(void)
{
    Init();
    UART_Init();   

      //Receiver();
	     PutChar('A'); 	// AT command to send SMS message
			 Delay(100);
			 PutChar('T'); 	// AT command to send SMS message
			 Delay(100);
			 PutChar('\r'); 	// AT command to send SMS message
			 Delay(100);
			 PutChar('\n'); 	// AT command to send SMS message
			 Delay(100);
			 
			 PutChar(echo[0]); 	// AT command to send SMS message
			 Delay(100);
			 PutChar(echo[1]); 	// AT command to send SMS message
			 Delay(100);
			 PutChar(echo[2]); 	// AT command to send SMS message
			 Delay(100);
			 PutChar(echo[3]);
			 Delay(100);
			 PutChar(echo[4]); 	// AT command to send SMS message
			 Delay(100);
			 PutChar(echo[5]); 	// AT command to send SMS message
			 Delay(100);
			 
			 PutChar(mode[0]); 	// AT command to send SMS message
			 Delay(100);
			 PutChar(mode[1]); 	// AT command to send SMS message
			 Delay(100);
			 PutChar(mode[2]); 	// AT command to send SMS message
			 Delay(100);
			 PutChar(mode[3]);
			 Delay(100);
			 PutChar(mode[4]); 	// AT command to send SMS message
			 Delay(100);
			 PutChar(mode[5]); 	// AT command to send SMS message
			 Delay(100);
			 PutChar(mode[6]); 	// AT command to send SMS message
			 Delay(100);
			 PutChar(mode[7]);
			 Delay(100);
			 PutChar(mode[8]); 	// AT command to send SMS message
			 Delay(100);
			 PutChar(mode[9]); 	// AT command to send SMS message
			 Delay(100);
			 PutChar(mode[10]); // AT command to send SMS message
			 Delay(100);

       PutChar(recp[0]); 	// AT command to send SMS message
			 Delay(100);
			 PutChar(recp[1]); 	// AT command to send SMS message
			 Delay(100);
			 PutChar(recp[2]); 	// AT command to send SMS message
			 Delay(100);
			 PutChar(recp[3]);
			 Delay(100);
			 PutChar(recp[4]); 	// AT command to send SMS message
			 Delay(100);
			 PutChar(recp[5]); 	// AT command to send SMS message
			 Delay(100);
			 PutChar(recp[6]); 	// AT command to send SMS message
			 Delay(100);
			 PutChar(recp[7]);
			 Delay(100);
			 PutChar(recp[8]); 	// AT command to send SMS message
			 Delay(100);
			 PutChar(recp[9]); 	// AT command to send SMS message
			 Delay(100);
			 PutChar(recp[10]); // AT command to send SMS message
			 Delay(100);
			 PutChar(recp[11]); 	// AT command to send SMS message
			 Delay(100);
			 PutChar(recp[12]); 	// AT command to send SMS message
			 Delay(100);
			 PutChar(recp[13]);
			 Delay(100);
			 PutChar(recp[14]); 	// AT command to send SMS message
			 Delay(100);
			 PutChar(recp[15]); 	// AT command to send SMS message
			 Delay(100);
			 PutChar(recp[16]); 	// AT command to send SMS message
			 Delay(100);
			 PutChar(recp[17]);
			 Delay(100);
			 PutChar(recp[18]); 	// AT command to send SMS message
			 Delay(100);
			 PutChar(recp[19]); 	// AT command to send SMS message
			 Delay(100);
			 PutChar(recp[20]); // AT command to send SMS message
			 Delay(100);
			 PutChar(recp[21]); 	// AT command to send SMS message
			 Delay(100);
			 PutChar(recp[22]); 	// AT command to send SMS message
			 Delay(100);
			 PutChar(recp[23]); // AT command to send SMS message
			 Delay(100);
			 
			 PutChar(message[0]); 	// AT command to send SMS message
			 Delay(100);
			 PutChar(message[1]); 	// AT command to send SMS message
			 Delay(100);
			 PutChar(message[2]); 	// AT command to send SMS message
			 Delay(100);
			 PutChar(message[3]);
			 Delay(100);
			 PutChar(message[4]); 	// AT command to send SMS message
			 Delay(100);
			 PutChar(message[5]); 	// AT command to send SMS message
			 Delay(100);
			 PutChar(message[6]); 	// AT command to send SMS message
			 Delay(100);
			 PutChar(message[7]);
			 Delay(100);
			 PutChar(message[8]); 	// AT command to send SMS message
			 Delay(100);
			 PutChar(message[9]); 	// AT command to send SMS message
			 Delay(100);
			 PutChar(message[10]); // AT command to send SMS message
			 Delay(100);

       PutChar((char)26);
       Delay(100);
			 PutChar('\n');
			 Delay(100);
			 PutChar('\r');
			 Delay(100);
			 PutChar('\n');
			 Delay(100);
	    
}
