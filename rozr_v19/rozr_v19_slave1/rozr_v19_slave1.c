//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
#define	LCDdataPORT	PORTA // Данні LCD
#define	LCDdataPIN	PINA
#define	LCDdataDDR	DDRA
#define	LCDcontrolPORT	PORTB // Контроль LCD
#define	LCDcontrolPIN PINB
#define	LCDcontrolDDR DDRB
#define	RS 0
#define	RW 1
#define	E 2
#define F_CPU 9216000L // Частота мікроконтролера
#define BAUD 9600 // Швидкість передачі
#define UBRRcalc (F_CPU/(BAUD*16L)-1)
#define	BUF_SIZE 4 // Розмір буфера
#define	BUF_MASK (BUF_SIZE-1)
//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
#include <avr/io.h>
#include <util/delay.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>
#include <math.h> 
#include <stdlib.h> 
#include "LCD_8.h" // Бібліотека LCD
#include "DS18B20.h" // Бібліотека давача температури
//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
OneWire OW1={&DDRD, &PIND, &PORTD, PD7}; // перший давач температури
OneWire OW2={&DDRB, &PINB, &PORTB, PB7}; // другий давач температури
char timer=0, address;
unsigned char  BufferOUT[BUF_SIZE],  StartBufOUT = 0, EndBufOUT = 0, Boof[4];
volatile unsigned char  waitread = 0, write = 0;

//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
ISR(USART_RXC_vect) // Переривання при прийнятому байті                   
{ 
	if (UDR == address) // Якщо адреси співпадають
	{
		WriteBufOUT(Boof[0]); // Записуємо збережені байти температури в буфер
		WriteBufOUT(Boof[1]);
		WriteBufOUT(Boof[2]);
		WriteBufOUT(Boof[3]);
	 	UCSRA &= ~(1<<MPCM); // Відключення мультиплексорного режиму
	 	UCSRB |= 1<<UDRIE; // Включення переривання при спорожненому UDR
		write = 1;
	}                         
}           
//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
ISR(USART_UDRE_vect ) // Переривання при спорожненому UDR              
{
	PORTD |= 1<<PD2; // Переходимо у режим передачі  
    asm("nop");
    UDR = BufferOUT[StartBufOUT++]; // Відправляємо значення з буферу
    StartBufOUT &= BUF_MASK;
    if( StartBufOUT == EndBufOUT ) // Якщо буфер порожній
	{
    	UCSRB &= ~(1<<UDRIE); // Відключення переривання при спорожненому UDR
		UCSRA |= (1<<MPCM); // Включення мультиплексорного режиму
		write=0;
	}
}
//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
ISR(USART_TXC_vect ) // Переривання при відправленому байті                 
{
	if( StartBufOUT == EndBufOUT ) PORTD &= ~(1<<PD2); // Переходимо у режим прийому
}  
//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
ISR(TIMER0_COMP_vect) // таймер  на опитування давачів
{
	timer++;
	if (timer==40)
	{
		timer=0;
		unsigned int tempHB, tempLB, temp;
		unsigned char tempDigital,tempDecimal, minus = 0;
		char Sbuf[4];
		Clear_LCD(); // очищення екрану LCD
		if(OneWireReset(OW1)) // Ініціалізація 1-wire шини
		{
			OneWireWriteByte(OW1, SKIP_ROM); // Команда ROM для звертання до давача
			OneWireWriteByte(OW1, READ_SCRATCHPAD); // Функціональна команда на зчитування
			tempLB = (unsigned int)OneWireReadByte(OW1); // Зчитування двох байтів з температурою
			tempHB = (unsigned int)OneWireReadByte(OW1);
			Boof[0]=(unsigned char) tempLB; // Запамятовування двох байтів з температурою
			Boof[1]=(unsigned char) tempHB;
			temp = (tempLB)|(tempHB<<8); // Формування стічки зі значенням температури
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
			tempDecimal = (tempDecimal<<1) + (tempDecimal<<3); // Множимо на 10
			tempDecimal = (tempDecimal>>4);	// Ділимо на 16 
			Set_String( utoa(tempDigital,Sbuf,10) );
			Set_Char('.');
			Set_String( utoa(tempDecimal,Sbuf,10) );
			Set_Char('*');
			Set_Char('C');
		}
		if(OneWireReset(OW1)) // Ініціалізація 1-wire шини
		{
			OneWireWriteByte(OW1, SKIP_ROM); // Команда ROM для звертання до давача
			OneWireWriteByte(OW1, CONVERT_TEMP); // Функціональна команда запуску конвертації темпери
		}
		
		minus = 0;
		if(OneWireReset(OW2)) // Ініціалізація 1-wire шини 
		{
			OneWireWriteByte(OW2, SKIP_ROM); // Команда ROM для звертання до давача
			OneWireWriteByte(OW2, READ_SCRATCHPAD); // Функціональна команда на зчитування
			tempLB = (unsigned int)OneWireReadByte(OW2); // Зчитування двох байтів з температурою
			tempHB = (unsigned int)OneWireReadByte(OW2);
			Boof[2]=(unsigned char) tempLB; // Запамятовування двох байтів з температурою
			Boof[3]=(unsigned char) tempHB;
			temp = (tempLB)|(tempHB<<8); // Формування стічки зі значенням температури
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
			tempDecimal = (tempDecimal<<1) + (tempDecimal<<3); // Множимо на 10
			tempDecimal = (tempDecimal>>4);	// Ділимо на 16
			Set_String( utoa(tempDigital,Sbuf,10) );
			Set_Char('.');
			Set_String( utoa(tempDecimal,Sbuf,10) );
			Set_Char('*');
			Set_Char('C');
		}
		if(OneWireReset(OW2)) // Ініціалізація 1-wire шини
		{
			OneWireWriteByte(OW2, SKIP_ROM); // Команда ROM для звертання до давача
			OneWireWriteByte(OW2, CONVERT_TEMP); // Функціональна команда запуску конвертації темпери
		}
	}
}
//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void WriteBufOUT(unsigned char value) // Записування значення в буфер
{	
	if (write != 1) // якщо не передаються данні
	{
    	BufferOUT[EndBufOUT++] = value;
       	EndBufOUT &= BUF_MASK;
	}
}
//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
int main()
{
	Initial(); // Ініціалізація заліза
	address = ~PINC;
	if(OneWireReset(OW1) ) // Перша конвертація температури
	{
		OneWireWriteByte(OW1, SKIP_ROM); // Команда ROM для звертання до давача
		OneWireWriteByte(OW1, CONVERT_TEMP); // Функціональна команда запуску конвертації темпери
	}
	_delay_ms(1000);

	if(OneWireReset(OW2) ) // Перша конвертація температури
	{
		OneWireWriteByte(OW2, SKIP_ROM); // Команда ROM для звертання до давача
		OneWireWriteByte(OW2, CONVERT_TEMP); // Функціональна команда запуску конвертації темпери
	}
	_delay_ms(1000);

	sei();
	while (1)
	{	}
}
//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void Initial()
{
	DDRA  = 0xFF; // Ініціалізація портів
	PORTA = 0x00;
	DDRB  = 0xFF;
	PORTB = 0x00;
	DDRC  = 0x00;
	PORTC = 0xFF;
	DDRD=0b11111110; 
	PORTD=0b00000001;
	UBRRL = (unsigned char)(UBRRcalc); // Ініціалізація USART
    UBRRH = (unsigned char)(UBRRcalc>>8); // Швидкість передачі
    UCSRA = (1<<MPCM); // Мультипроцесорний режим
    UCSRC = (1<<URSEL)|(1<<UCSZ1)|(1<<UCSZ0)|(1<<USBS);  // Формат кадру 9n2 без перевірки парності
    UCSRB = (1<<UCSZ2)|(1<<RXEN)|(1<<TXEN)|(1<<RXCIE)|(1<<TXCIE); // Дозвіл прийому-передачі+перерив.прийому,заверш.перед.+9n
	LCD_ini(); // Ініціалізація LCD
	TCCR0 = (1<<WGM01)|(1<<CS02)|(1<<CS00); // Ініціалізація таймера
	OCR0  = 0x77;
	TIMSK = (1<<OCIE0);
	OneWireInit(OW1); // Ініціалізація 1-Wire шини
	OneWireInit(OW2); // Ініціалізація 1-Wire шини
}
//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
