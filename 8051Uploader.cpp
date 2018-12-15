#include "stdafx.h"
//define FTD2XXSTATIC
#include <windows.h>
#include "ftd2xx.h"
#include "ihex.h"

#define COMMAND_END 0xAA

#define BAUD_RATE 9600

#define RST_8051 0
#define MISO_8051 1
#define MOSI_8051 2
#define CLK_8051 3

#define PIN_TX  0x01
#define PIN_RX  0x02
#define PIN_RTS 0x04
#define PIN_CTS 0x08
#define PIN_DTR 0x10
#define PIN_DSR 0x20
#define PIN_DCD 0x40
#define PIN_RI  0x80

#define LOW 0
#define HIGH 1

typedef union pinmap_t {
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

pinmap_t ftdi_pins;

FT_HANDLE ftdi_chip;

void setPin(byte pin, byte value) {
	static DWORD b = 0;
	switch (pin) {
		case MISO_8051: ftdi_pins.miso = value; break;
		case MOSI_8051: ftdi_pins.mosi = value; break;
		case CLK_8051: ftdi_pins.clk = value; break;
		case RST_8051: ftdi_pins.rst  = value; break;
	}
	FT_Write(ftdi_chip, &ftdi_pins, 1, &b);
}

byte getPin(byte pin) {
	static DWORD b = 0;
	byte buffer = 0;
	FT_GetBitMode(ftdi_chip, &buffer);
	buffer = buffer | 0x0F;
	ftdi_pins.pins |= buffer;
	ftdi_pins.pins &= buffer;
	switch (pin)
	{
		case MISO_8051: return ftdi_pins.miso;
		case MOSI_8051: return ftdi_pins.mosi;
		case CLK_8051: return ftdi_pins.clk;
		case RST_8051: return ftdi_pins.rst;
	}
	return 0;
	
}

unsigned char SendSPI(unsigned char data)
{
	byte retval = 0;
	byte intData = data;
	int t;

	for (int ctr = 0; ctr < 7; ctr++)
	{
		if (intData & 0x80) setPin(MOSI_8051, HIGH);
		else setPin(MOSI_8051, LOW);

		setPin(CLK_8051, HIGH);
		Sleep(1);

		t = getPin(MISO_8051);
		setPin(CLK_8051, LOW);

		if (t) retval |= 1; else retval &= 0xFE;
		retval <<= 1;
		intData <<= 1;
		Sleep(1);
	}


	if (intData & 0x80) setPin(MOSI_8051, HIGH);
	else setPin(MOSI_8051, LOW);

	setPin(CLK_8051, HIGH);
	Sleep(1);

	t = getPin(MISO_8051);
	setPin(CLK_8051, LOW);

	if (t) retval |= 1;
	else retval &= 0xFE;

	return retval;
}


byte progEnable()
{
	SendSPI(0xAC);
	SendSPI(0x53);
	SendSPI(COMMAND_END);
	return SendSPI(COMMAND_END);
}

void eraseChip()
{
	SendSPI(0xAC);
	SendSPI(0x9F);
	SendSPI(COMMAND_END);
	SendSPI(COMMAND_END);

	Sleep(520);
}

byte readProgmem(byte AH, byte AL)
{

	SendSPI(0x20);
	SendSPI(AH);
	SendSPI(AL);
	return SendSPI(COMMAND_END);
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
	SendSPI(COMMAND_END);
	SendSPI(COMMAND_END);
}

byte readLockBits()
{
	SendSPI(0x24);
	SendSPI(COMMAND_END);
	SendSPI(COMMAND_END);
	return SendSPI(COMMAND_END);
}

byte readSign(byte AH, byte AL)
{
	SendSPI(0x28);
	SendSPI(AH);
	SendSPI(AL);
	return SendSPI(COMMAND_END);
}


int main(int argc, char ** argv)
{
	
	DWORD devIndex = 0; 
	char Buffer[255];
	ftdi_pins.pins = 0xF1;
	ftdi_pins.clk = HIGH;
	DWORD p = 0;
	byte e1 = 0xff;
	byte e2 = 0;

	HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	SetConsoleTextAttribute(hConsole, 7);
	if (argc < 2) {
		printf("Please specify the hex file\n");
		return 1;
	}

	
	FT_STATUS ftStatus = FT_ListDevices((PVOID)devIndex, Buffer, FT_LIST_BY_INDEX | FT_OPEN_BY_SERIAL_NUMBER);
	if (ftStatus == FT_OK) {

		int bytes = load_file(argv[1]);

		ftStatus = FT_OpenEx(Buffer, FT_OPEN_BY_SERIAL_NUMBER, &ftdi_chip);
		FT_SetBitMode(ftdi_chip, PIN_TX | PIN_RX | PIN_CTS, 1); // BIT BANG MODE
		FT_SetBaudRate(ftdi_chip, BAUD_RATE);						 

		setPin(RST_8051, LOW);
		setPin(MOSI_8051, LOW);
		setPin(CLK_8051, LOW);
		setPin(RST_8051, HIGH);
		
		Sleep(500);
		printf("%x\n", progEnable());		
		Sleep(1100);

		eraseChip();

		for (int i = from_addr; i < to_addr + 1; i++) {
			printf("%02X", readProgmem(0, i));
		}
		printf("\n");


		for (int i = from_addr; i < to_addr + 1; i++) {
			printf("%02X", memory[i]);
		}
		printf("\n");
		
		// write memory
		for (int i = from_addr; i < to_addr+1; i++) {
			writeProgmem(0, i, memory[i]);
			printf("%02X", readProgmem(0, i));
		}
		printf("\n");
		

		Sleep(1000);
		setPin(RST_8051, LOW);
		Sleep(500);

		FT_Close(ftdi_chip);

		getchar();
	}
	else {
		printf("Can't open FTDI chip\n");
		return 1;
	}

	return 0;
}

