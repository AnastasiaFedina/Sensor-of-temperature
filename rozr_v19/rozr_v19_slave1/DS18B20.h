#define	READ_ROM		0x33
#define	SKIP_ROM		0xCC
#define	MATCH_ROM		0x55
#define	SEARCH_ROM		0xF0
#define	CONVERT_TEMP	0x44
#define	READ_SCRATCHPAD	0xBE

typedef struct 
{
	volatile unsigned char *OneWireDDR;
	volatile unsigned char *OneWirePIN;
	volatile unsigned char *OneWirePORT;
	unsigned char LegsNumber;
} OneWire;

void OneWireInit(OneWire OW)
{
	//виставляємо лінію на вхід без підтягуючого резистора і робимо паузу
	*OW.OneWirePORT &= ~(1<<OW.LegsNumber); 	//not pullup or "0"-out
	*OW.OneWireDDR  &= ~(1<<OW.LegsNumber); 	//Z-input
	_delay_us(480);							
}
//---------------------------------------------------------------------------
unsigned char OneWireReset(OneWire OW)
{
	unsigned char condition=1, res;
	//якщо прапор переривання виставлений, тоді скидуємо його
	if(SREG & (1<<7)) cli();
	else condition=0;
	*OW.OneWireDDR  |=  (1<<OW.LegsNumber); 		//output
	_delay_us(480);							

	*OW.OneWireDDR  &= ~(1<<OW.LegsNumber); 	//Z-input
	_delay_us(70);							

	if( (*OW.OneWirePIN)&(1<<OW.LegsNumber) )   res=0;
	else		res=1;
	_delay_us(410);							

	//якщо ми скидували прапор переривання, тоді відновлюємо його
	if(condition) sei();
	return res;						//res=1 OK
}
//---------------------------------------------------------------------------
unsigned char OneWireReadBit(OneWire OW)
{
	unsigned char bit=0;
	*OW.OneWireDDR  |=  (1<<OW.LegsNumber); 		//output
	_delay_us(6);							

	*OW.OneWireDDR  &= ~(1<<OW.LegsNumber); 	//Z-input
	_delay_us(9);							

	if( (*OW.OneWirePIN)&(1<<OW.LegsNumber) )    bit=1;
	_delay_us(55);							
	return bit;
}
//---------------------------------------------------------------------------
void OneWireWrite1(OneWire OW)
{
	*OW.OneWireDDR  |=  (1<<OW.LegsNumber); 		//output 
	_delay_us(6);							

	*OW.OneWireDDR  &= ~(1<<OW.LegsNumber); 	//Z-input
	_delay_us(64);							
}
//---------------------------------------------------------------------------
void OneWireWrite0(OneWire OW)
{
	*OW.OneWireDDR  |=  (1<<OW.LegsNumber); 		//output 
	_delay_us(60);							

	*OW.OneWireDDR  &= ~(1<<OW.LegsNumber); 	//Z-input
	_delay_us(10);							
}
//---------------------------------------------------------------------------
unsigned char OneWireReadByte(OneWire OW)
{
	unsigned char i=8,byte=0;
	while(i--)
	{
		byte >>= 1;
		byte |= OneWireReadBit(OW) << 7;
	}
	return byte;
}
//---------------------------------------------------------------------------
void OneWireWriteByte(OneWire OW, unsigned char byte)
{
	unsigned char i=8;
	while(i--)
	{
		if(byte&1)	OneWireWrite1(OW);
		else		OneWireWrite0(OW);
		byte>>=1;
	}
}
