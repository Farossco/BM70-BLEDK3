#ifndef BM70_H
#define BM70_H

#include <Arduino.h>

#define BM70_DEFAULT_TIMEOUT 10

class BM70
{
public:
	BM70();
	BM70(HardwareSerial * serial);
	BM70(HardwareSerial * serial, int baudrate);
	void send (uint8_t opCode, uint8_t * parameters, uint16_t parametersLength);
	int read (uint16_t timeout = BM70_DEFAULT_TIMEOUT);
	int read (uint8_t * data, uint16_t bufferSize, uint16_t &length, uint16_t timeout = BM70_DEFAULT_TIMEOUT);
	int sendAndRead (uint8_t opCode, uint8_t * parameters, uint16_t parametersLength, uint8_t * data, uint16_t bufferSize, uint16_t &length, uint16_t timeout = BM70_DEFAULT_TIMEOUT, uint16_t waitDelay = 10, uint16_t maxAttempts = 2);

	// Common commands
	int getInfos (uint32_t &fwVersion, uint64_t &btAddress, uint16_t timeout = BM70_DEFAULT_TIMEOUT);
	int reset (uint16_t timeout = BM70_DEFAULT_TIMEOUT);
	int getStatus (uint8_t &status, uint16_t timeout = BM70_DEFAULT_TIMEOUT);
	int getAdc (uint8_t channel, float &adcValue, uint16_t timeout = BM70_DEFAULT_TIMEOUT);
	int shutDown (uint16_t timeout = BM70_DEFAULT_TIMEOUT);
	int getName (char * name, uint16_t timeout = BM70_DEFAULT_TIMEOUT);
	int setName (char * name, uint16_t timeout = BM70_DEFAULT_TIMEOUT);
	int getPairingMode (uint8_t &setting, uint16_t timeout = BM70_DEFAULT_TIMEOUT);
	int setPairingMode (uint8_t setting, uint16_t timeout = BM70_DEFAULT_TIMEOUT);
	int getPaired (uint64_t * devices, uint8_t * priorities, uint8_t &quantity, uint16_t timeout = BM70_DEFAULT_TIMEOUT);
	int removePaired (uint8_t index, uint16_t timeout = BM70_DEFAULT_TIMEOUT);

private:
	HardwareSerial * serial;
};

#endif // ifndef BM70_H
