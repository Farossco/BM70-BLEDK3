#ifndef BM70_H
#define BM70_H

#include <Arduino.h>

#define BM70_DEFAULT_TIMEOUT 10

class BM70
{
public:
	uint8_t status;

	BM70();
	BM70(HardwareSerial * serial);
	BM70(HardwareSerial * serial, int baudrate);
	void send (uint8_t opCode, uint8_t * parameters, uint16_t parametersLength);
	int read (uint8_t * data, uint16_t &length, uint16_t timeout = BM70_DEFAULT_TIMEOUT);
	int sendAndRead (uint8_t opCode, uint8_t * parameters, uint16_t parametersLength, uint8_t * data, uint16_t &length, uint16_t timeout = BM70_DEFAULT_TIMEOUT, uint16_t waitDelay = 10, uint16_t maxAttempts = 2);


	// Common
	int readLocalInformation (uint32_t &fwVersion, uint64_t &btAddress, uint16_t timeout = BM70_DEFAULT_TIMEOUT);
	int reset (uint16_t timeout = BM70_DEFAULT_TIMEOUT);
	int readStatus (uint16_t timeout = BM70_DEFAULT_TIMEOUT);
	float readAdcValue (uint8_t channel, uint16_t timeout = BM70_DEFAULT_TIMEOUT);
	int shutDown (uint16_t = BM70_DEFAULT_TIMEOUT);
	int readMemory (uint16_t address, uint8_t length, uint timeout  = BM70_DEFAULT_TIMEOUT);
	int writeMemory (uint16_t timeout = BM70_DEFAULT_TIMEOUT);
	int readFlash (uint16_t timeout   = BM70_DEFAULT_TIMEOUT);
	int writeFlash (uint16_t timeout  = BM70_DEFAULT_TIMEOUT);

private:
	HardwareSerial * serial;
};

#endif // ifndef BM70_H
