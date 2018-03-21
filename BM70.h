#ifndef BM70_H
#define BM70_H

#include <Arduino.h>

class BM70
{
public:
	BM70();
	BM70(HardwareSerial * output);
	BM70(HardwareSerial * output, int baudrate);
	void send  (unsigned char opCode, unsigned char * parameters, short int parametersLength);
	void read ();

private:
	HardwareSerial * output;
};

#endif // ifndef BM70_H
