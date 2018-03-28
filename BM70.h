#ifndef BM70_H
#define BM70_H

#include <Arduino.h>

#define BM70_DEFAULT_TIMEOUT 30

class BM70
{
public:
	BM70();
	BM70(HardwareSerial * serial);
	BM70(HardwareSerial * serial, int baudrate);

	// Low level functions
	void send (uint8_t opCode, uint8_t * parameters, uint16_t parametersLength);
	int read (uint16_t timeout = BM70_DEFAULT_TIMEOUT);
	int read (uint8_t * data, uint16_t bufferSize, uint16_t &length, uint16_t timeout = BM70_DEFAULT_TIMEOUT);
	int readResponse (uint8_t opCode, uint16_t timeout = BM70_DEFAULT_TIMEOUT);
	int sendAndRead (uint8_t opCode, uint8_t * parameters, uint16_t parametersLength, uint8_t * data, uint16_t bufferSize, uint16_t &length, uint16_t timeout = BM70_DEFAULT_TIMEOUT, uint16_t waitDelay = 10, uint16_t maxAttempts = 2);

	// Common commands
	int getInfos (uint32_t &fwVersion, uint64_t &btAddress);
	int reset ();
	int getStatus (uint8_t &status);
	int getAdc (uint8_t channel, float &adcValue);
	int shutDown ();
	int getName (char * name);
	int setName (char * name);
	int getPairingMode (uint8_t &setting);
	int setPairingMode (uint8_t setting);
	int getPaired (uint64_t * devices, uint8_t * priorities, uint8_t &quantity);
	int removePaired (uint8_t index);

	// GAP commands
	int enableScan (boolean showDuplicate = false);
	int disableScan ();
	int connect (boolean randomAddress, uint64_t address);
	int cancelConnect ();
	int disconnect ();
	int enableAdvert ();
	int disableAdvert ();

private:
	HardwareSerial * serial;
};

#endif // ifndef BM70_H
