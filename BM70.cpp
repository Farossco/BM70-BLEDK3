#include "BM70.h"

BM70::BM70()
{ }

BM70::BM70(HardwareSerial * output) : BM70 (output, 115200)
{ }

BM70::BM70(HardwareSerial * output, int baudrate)
{
	this->output = output;
	output->begin (baudrate);
}

void BM70::send (unsigned char opCode, unsigned char * parameters, short int parametersLength)
{
	unsigned char start    = 0xAA;
	short int length       = 1 + parametersLength;
	unsigned char lengthH  = (unsigned char) (length >> 8);
	unsigned char lengthL  = (unsigned char) length;
	unsigned char checksum = 0 - lengthH - lengthL - opCode;

	for (int i = 0; i < parametersLength; i++)
		checksum -= parameters[i];

	output->write (start);
	output->write (lengthH);
	output->write (lengthL);
	output->write (opCode);
	for (int i = 0; i < parametersLength; i++)
		output->write (parameters[i]);
	output->write (checksum);

	Serial2.write (start);
	Serial2.write (lengthH);
	Serial2.write (lengthL);
	Serial2.write (opCode);
	for (int i = 0; i < parametersLength; i++)
		Serial2.write (parameters[i]);
	Serial2.write (checksum);
}

void BM70::read ()
{ }
