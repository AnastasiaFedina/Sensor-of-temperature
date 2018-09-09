//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
#define	LCDdataPORT	PORTA // ���� LCD
#define	LCDdataPIN	PINA
#define	LCDdataDDR	DDRA
#define	LCDcontrolPORT	PORTB // �������� LCD
#define	LCDcontrolPIN PINB
#define	LCDcontrolDDR DDRB
#define	RS 0
#define	RW 1
#define	E 2
#define F_CPU 9216000L // ������� ��������������
#define BAUD 9600 // �������� ��������
#define UBRRcalc (F_CPU/(BAUD*16L)-1)
#define	BUF_SIZE 4 // ����� ������
#define	BUF_MASK (BUF_SIZE-1)
//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
#include <avr/io.h>
#include <util/delay.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>
#include <math.h> 
#include <stdlib.h> 
#include "LCD_8.h" // ��������� LCD
#include "DS18B20.h" // ��������� ������ �����������
//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
OneWire OW1={&DDRD, &PIND, &PORTD, PD7}; // ������ ����� �����������
OneWire OW2={&DDRB, &PINB, &PORTB, PB7}; // ������ ����� �����������
char timer=0, address;
unsigned char  BufferOUT[BUF_SIZE],  StartBufOUT = 0, EndBufOUT = 0, Boof[4];
volatile unsigned char  waitread = 0, write = 0;

//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
ISR(USART_RXC_vect) // ����������� ��� ���������� ����                   
{ 
	if (UDR == address) // ���� ������ ����������
	{
		WriteBufOUT(Boof[0]); // �������� �������� ����� ����������� � �����
		WriteBufOUT(Boof[1]);
		WriteBufOUT(Boof[2]);
		WriteBufOUT(Boof[3]);
	 	UCSRA &= ~(1<<MPCM); // ³��������� ����������������� ������
	 	UCSRB |= 1<<UDRIE; // ��������� ����������� ��� ������������ UDR
		write = 1;
	}                         
}           
//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
ISR(USART_UDRE_vect ) // ����������� ��� ������������ UDR              
{
	PORTD |= 1<<PD2; // ���������� � ����� ��������  
    asm("nop");
    UDR = BufferOUT[StartBufOUT++]; // ³���������� �������� � ������
    StartBufOUT &= BUF_MASK;
    if( StartBufOUT == EndBufOUT ) // ���� ����� �������
	{
    	UCSRB &= ~(1<<UDRIE); // ³��������� ����������� ��� ������������ UDR
		UCSRA |= (1<<MPCM); // ��������� ����������������� ������
		write=0;
	}
}
//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
ISR(USART_TXC_vect ) // ����������� ��� ������������ ����                 
{
	if( StartBufOUT == EndBufOUT ) PORTD &= ~(1<<PD2); // ���������� � ����� �������
}  
//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
ISR(TIMER0_COMP_vect) // ������  �� ���������� �������
{
	timer++;
	if (timer==40)
	{
		timer=0;
		unsigned int tempHB, tempLB, temp;
		unsigned char tempDigital,tempDecimal, minus = 0;
		char Sbuf[4];
		Clear_LCD(); // �������� ������ LCD
		if(OneWireReset(OW1)) // ����������� 1-wire ����
		{
			OneWireWriteByte(OW1, SKIP_ROM); // ������� ROM ��� ��������� �� ������
			OneWireWriteByte(OW1, READ_SCRATCHPAD); // ������������� ������� �� ����������
			tempLB = (unsigned int)OneWireReadByte(OW1); // ���������� ���� ����� � ������������
			tempHB = (unsigned int)OneWireReadByte(OW1);
			Boof[0]=(unsigned char) tempLB; // ��������������� ���� ����� � ������������
			Boof[1]=(unsigned char) tempHB;
			temp = (tempLB)|(tempHB<<8); // ���������� ����� � ��������� �����������
			if(temp&0x8000) 
			{
				temp = ~temp + 1;
				minus = 1;
			}
			GotoXY(0, 0);
			if(minus)	Set_Char('-');
			else		Set_Char('+');
			tempDigital = temp >> 4;
			tempDecimal = temp & 0xF;
			tempDecimal = (tempDecimal<<1) + (tempDecimal<<3); // ������� �� 10
			tempDecimal = (tempDecimal>>4);	// ĳ���� �� 16 
			Set_String( utoa(tempDigital,Sbuf,10) );
			Set_Char('.');
			Set_String( utoa(tempDecimal,Sbuf,10) );
			Set_Char('*');
			Set_Char('C');
		}
		if(OneWireReset(OW1)) // ����������� 1-wire ����
		{
			OneWireWriteByte(OW1, SKIP_ROM); // ������� ROM ��� ��������� �� ������
			OneWireWriteByte(OW1, CONVERT_TEMP); // ������������� ������� ������� ����������� �������
		}
		
		minus = 0;
		if(OneWireReset(OW2)) // ����������� 1-wire ���� 
		{
			OneWireWriteByte(OW2, SKIP_ROM); // ������� ROM ��� ��������� �� ������
			OneWireWriteByte(OW2, READ_SCRATCHPAD); // ������������� ������� �� ����������
			tempLB = (unsigned int)OneWireReadByte(OW2); // ���������� ���� ����� � ������������
			tempHB = (unsigned int)OneWireReadByte(OW2);
			Boof[2]=(unsigned char) tempLB; // ��������������� ���� ����� � ������������
			Boof[3]=(unsigned char) tempHB;
			temp = (tempLB)|(tempHB<<8); // ���������� ����� � ��������� �����������
			if(temp&0x8000) 
			{
				temp = ~temp + 1;
				minus = 1;
			}
			GotoXY(0, 1);
			if(minus)	Set_Char('-');
			else		Set_Char('+');
			tempDigital = temp >> 4;
			tempDecimal = temp & 0xF;
			tempDecimal = (tempDecimal<<1) + (tempDecimal<<3); // ������� �� 10
			tempDecimal = (tempDecimal>>4);	// ĳ���� �� 16
			Set_String( utoa(tempDigital,Sbuf,10) );
			Set_Char('.');
			Set_String( utoa(tempDecimal,Sbuf,10) );
			Set_Char('*');
			Set_Char('C');
		}
		if(OneWireReset(OW2)) // ����������� 1-wire ����
		{
			OneWireWriteByte(OW2, SKIP_ROM); // ������� ROM ��� ��������� �� ������
			OneWireWriteByte(OW2, CONVERT_TEMP); // ������������� ������� ������� ����������� �������
		}
	}
}
//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void WriteBufOUT(unsigned char value) // ����������� �������� � �����
{	
	if (write != 1) // ���� �� ����������� ����
	{
    	BufferOUT[EndBufOUT++] = value;
       	EndBufOUT &= BUF_MASK;
	}
}
//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
int main()
{
	Initial(); // ����������� �����
	address = ~PINC;
	if(OneWireReset(OW1) ) // ����� ����������� �����������
	{
		OneWireWriteByte(OW1, SKIP_ROM); // ������� ROM ��� ��������� �� ������
		OneWireWriteByte(OW1, CONVERT_TEMP); // ������������� ������� ������� ����������� �������
	}
	_delay_ms(1000);

	if(OneWireReset(OW2) ) // ����� ����������� �����������
	{
		OneWireWriteByte(OW2, SKIP_ROM); // ������� ROM ��� ��������� �� ������
		OneWireWriteByte(OW2, CONVERT_TEMP); // ������������� ������� ������� ����������� �������
	}
	_delay_ms(1000);

	sei();
	while (1)
	{	}
}
//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void Initial()
{
	DDRA  = 0xFF; // ����������� �����
	PORTA = 0x00;
	DDRB  = 0xFF;
	PORTB = 0x00;
	DDRC  = 0x00;
	PORTC = 0xFF;
	DDRD=0b11111110; 
	PORTD=0b00000001;
	UBRRL = (unsigned char)(UBRRcalc); // ����������� USART
    UBRRH = (unsigned char)(UBRRcalc>>8); // �������� ��������
    UCSRA = (1<<MPCM); // ����������������� �����
    UCSRC = (1<<URSEL)|(1<<UCSZ1)|(1<<UCSZ0)|(1<<USBS);  // ������ ����� 9n2 ��� �������� �������
    UCSRB = (1<<UCSZ2)|(1<<RXEN)|(1<<TXEN)|(1<<RXCIE)|(1<<TXCIE); // ����� �������-��������+�������.�������,������.�����.+9n
	LCD_ini(); // ����������� LCD
	TCCR0 = (1<<WGM01)|(1<<CS02)|(1<<CS00); // ����������� �������
	OCR0  = 0x77;
	TIMSK = (1<<OCIE0);
	OneWireInit(OW1); // ����������� 1-Wire ����
	OneWireInit(OW2); // ����������� 1-Wire ����
}
//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
