#ifndef BM70_H
#define BM70_H

#include <Arduino.h>

#define BM70_DEFAULT_TIMEOUT    20
#define BM70_BUFF_SIZE          5
#define BM70_RESPONSE_MAX_SIZE  50
#define BM70_BUFFER_EMPTY_DELAY 1000

class BM70
{
public:
	BM70();
	BM70(HardwareSerial * initSerial);
	BM70(HardwareSerial * initSerial, uint32_t baudrate);

	void action ();

	int receiveData (uint16_t timeout = 0);

	uint8_t getStatus ();
	void setStatus (uint8_t status);
	void addResponse (uint8_t opCode, uint8_t * datas, uint16_t size);
	int getResponse (uint8_t opCode, uint8_t * response, uint16_t &size);
	int responseAvailable (uint8_t opCode);

	// Low level functions
	void send (uint8_t opCode, uint8_t * parameters, uint16_t parametersLength);
	int read ();
	int read (uint8_t * data, uint16_t bufferSize, uint16_t &length, uint16_t timeout = BM70_DEFAULT_TIMEOUT);
	int sendAndRead (uint8_t opCode, uint8_t * parameters, uint16_t parametersLength, uint8_t * response, uint16_t &length, uint16_t timeout = BM70_DEFAULT_TIMEOUT);

	// Common commands
	int getInfos (uint32_t &fwVersion, uint64_t &btAddress);
	int reset ();
	int updateStatus ();
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

	uint8_t status;
	uint32_t lastStatusUpdate;
	uint8_t responseBuffer[BM70_BUFF_SIZE][BM70_RESPONSE_MAX_SIZE];
	uint8_t responseIndex;
	uint32_t lastBufferAccess;
};

#endif // ifndef BM70_H
