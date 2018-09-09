//---------------------------------------------------------------------------------------------
//                  Відправляємо данні на LCD
void Data(unsigned char c, unsigned char rs)
{
	if(rs==0) LCDcontrolPORT&=~(1<<RS); else LCDcontrolPORT|=(1<<RS);
	LCDcontrolPORT|=(1<<E);
	_delay_ms(1);
	LCDdataPORT=0x00;
	LCDdataPORT=c;
	LCDcontrolPORT&=~(1<<E);
	_delay_ms(1);
}
//---------------------------------------------------------------------------------------------
//                    Виводимо один символ
void Set_Char(unsigned char c)
{
	Data(c, 1);
}

//---------------------------------------------------------------------------------------------
//                     Виводимо строку символів
void Set_String(char str[])
{
	for(unsigned char i=0; str[i]!='\0'; i++)
	{
		Set_Char(str[i]);
	}
}
//---------------------------------------------------------------------------------------------
//                     Виводимо двоцифрове число
void Set_Number(unsigned char c)
{
	unsigned char l=0, r=0;
	while (c>9)
	{
		l++;
		c-=10;
	}
	r=c;
	Set_Char(48+l);
	Set_Char(48+r);
}
//---------------------------------------------------------------------------------------------
//                    Міняємо позицію курсора
void GotoXY(unsigned char x, unsigned char y)
{
	char adress=0;
	adress = (0x40*y+x)|0b10000000;
	Data(adress, 0);
}
//---------------------------------------------------------------------------------------------
//                    Ініціалізуємо LCD
void LCD_ini()
{
	_delay_ms(15);
	Data(0b00111000, 0);
	_delay_ms(4);
	Data(0b00111000, 0);
	_delay_us(15);
	Data(0b00111000, 0);
	_delay_ms(1);
	Data(0b00111000, 0);
	_delay_ms(1);
	Data(0b00001100, 0);
	_delay_ms(1);
	Data(0b00000110, 0);
	_delay_ms(1);
}

//---------------------------------------------------------------------------------------------
//                    Очищуємо екран
void Clear_LCD()
{
	GotoXY(0, 0);
	Set_String("                ");
	GotoXY(0, 1);
	Set_String("                ");
}
