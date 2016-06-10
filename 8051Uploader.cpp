/

#include "stdafx.h"
//define FTD2XXSTATIC
#include <windows.h>
#include "ftd2xx.h"
#include "ihex.h"

#define dummyData 0xAA

#define RST 0
#define MISO 1
#define MOSI 2
#define _CLK 3

#define PIN_TX  0x01
#define PIN_RX  0x02
#define PIN_RTS 0x04
#define PIN_CTS 0x08
#define PIN_DTR 0x10
#define PIN_DSR 0x20
#define PIN_DCD 0x40
#define PIN_RI  0x80

typedef union Pins {
	struct {
		unsigned rst : 1;       // TX
		unsigned mosi : 1;      // RX
		unsigned unused1 : 1;   // RTS 
		unsigned clk : 1;		// CTS
		unsigned miso : 1;      // DTR
		unsigned unused : 3;
	};
	byte pins;
};

Pins pins;

FT_HANDLE handle;

void digitalWrite(byte pin, byte value) {
	static DWORD b = 0;
	switch (pin) {
		case MISO: pins.miso = value; break;
		case MOSI: pins.mosi = value; break;
		case _CLK: pins.clk = value; break;
		case RST: pins.rst  = value; break;
	}
	FT_Write(handle, &pins, 1, &b);
}

byte digitalRead(byte pin) {
	static DWORD b = 0;
	byte buffer = 0;
	FT_GetBitMode(handle, &buffer);
	buffer = buffer | 0x0F;
	pins.pins |= buffer;
	pins.pins &= buffer;
	switch (pin)
	{
		case MISO: return pins.miso;
		case MOSI: return pins.mosi;
		case _CLK: return pins.clk;
		case RST: return pins.rst;
	}
	return 0;
	
}

void delayMicroseconds(DWORD n) {	
	Sleep(n);	
}

void delay(DWORD n) {
	Sleep(n);
}


unsigned char SendSPI(unsigned char data)
{
	byte retval = 0;
	byte intData = data;
	int t;

	for (int ctr = 0; ctr < 7; ctr++)
	{
		if (intData & 0x80) digitalWrite(MOSI, 1);
		else digitalWrite(MOSI, 0);

		digitalWrite(_CLK, 1);
		delayMicroseconds(1);

		t = digitalRead(MISO);
		digitalWrite(_CLK, 0);

		if (t) retval |= 1; else retval &= 0xFE;
		retval <<= 1;
		intData <<= 1;
		delayMicroseconds(1);
	}


	if (intData & 0x80) digitalWrite(MOSI, 1);
	else digitalWrite(MOSI, 0);

	digitalWrite(_CLK, 1);
	delayMicroseconds(1);

	t = digitalRead(MISO);
	digitalWrite(_CLK, 0);

	if (t) retval |= 1;
	else retval &= 0xFE;

	return retval;
}


byte progEnable()
{
	SendSPI(0xAC);
	SendSPI(0x53);
	SendSPI(dummyData);
	return SendSPI(dummyData);
}

void eraseChip()
{
	SendSPI(0xAC);
	SendSPI(0x9F);
	SendSPI(dummyData);
	SendSPI(dummyData);

	delay(520);
}

byte readProgmem(byte AH, byte AL)
{

	SendSPI(0x20);
	SendSPI(AH);
	SendSPI(AL);
	return SendSPI(dummyData);
}

void writeProgmem(byte AH, byte AL, byte data)
{
	SendSPI(0x40);
	SendSPI(AH);
	SendSPI(AL);
	SendSPI(data);
}

void writeLockBits(byte lockByte)
{
	SendSPI(0xAC);
	SendSPI(lockByte);
	SendSPI(dummyData);
	SendSPI(dummyData);
}

byte readLockBits()
{
	SendSPI(0x24);
	SendSPI(dummyData);
	SendSPI(dummyData);
	return SendSPI(dummyData);
}

byte readSign(byte AH, byte AL)
{
	SendSPI(0x28);
	SendSPI(AH);
	SendSPI(AL);
	return SendSPI(dummyData);
}


int main(int argc, char ** argv)
{
	
	DWORD devIndex = 0; // first device 
	char Buffer[255]; // more than enough room! 
	pins.pins = 0xF1;
	pins.clk = 1;
	DWORD p = 0;
	byte e1 = 0xff;
	byte e2 = 0;
	HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	SetConsoleTextAttribute(hConsole, 7);
	if (argc < 2) return 1;
	int bytes = load_file(argv[1]);
	byte b = memory[0];
	FT_STATUS ftStatus = FT_ListDevices((PVOID)devIndex, Buffer, FT_LIST_BY_INDEX | FT_OPEN_BY_SERIAL_NUMBER);
	if (ftStatus == FT_OK) {
		ftStatus = FT_OpenEx(Buffer, FT_OPEN_BY_SERIAL_NUMBER, &handle);
		FT_SetBitMode(handle, PIN_TX | PIN_RX | PIN_CTS, 1);
		FT_SetBaudRate(handle, 9600);
		digitalWrite(RST, 0);
		digitalWrite(MOSI, 0);
		digitalWrite(_CLK, 0);
		digitalWrite(RST, 1);
		Sleep(500);
		printf("%x\n", progEnable());
		Sleep(100);
		Sleep(1000);

		eraseChip();
		for (int i = from_addr; i < to_addr + 1; i++) {
			printf("%02X", readProgmem(0, i));
		}
		printf("\n");
		if (1)
		{
			for (int i = from_addr; i < to_addr + 1; i++) {
				printf("%02X", memory[i]);
			}
			printf("\n");
			for (int i = from_addr; i < to_addr+1; i++) {
				writeProgmem(0, i, memory[i]);
				printf("%02X", readProgmem(0, i));
			}
			printf("\n");
		}
		Sleep(1000);
		digitalWrite(RST, 0);
		Sleep(500);
		getchar();
		//FT_SetBitMode(handle, 0x00, 0xFF);
		//FT_ResetDevice(handle);
		FT_Close(handle);	
	}
	else {
		
	}

	return 0;
}

