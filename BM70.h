#ifndef BM70_H
#define BM70_H

#include <Arduino.h>

#define BM70_DEFAULT_TIMEOUT          20
#define BM70_RESPONSE_BUFF_SIZE       1  // 3
#define BM70_TRANSPARENT_BUFF_SIZE    1  // 10
#define BM70_RESPONSE_MAX_SIZE        30 // 50
#define BM70_TRANSPARENT_MAX_SIZE     20 // 50
#define BM70_BUFFER_EMPTY_DELAY       1000

#define BM70_STATUS_UNKNOWN           0x00 // Status is unknown
#define BM70_STATUS_SCANNING          0x01 // Scanning mode
#define BM70_STATUS_CONNECTING        0x02 // Connecting mode
#define BM70_STATUS_STANDBY           0x03 // Standby mode
#define BM70_STATUS_BROADCAST         0x05 // Broadcast mode
#define BM70_STATUS_TRANSCOM          0x08 // Transparent Service enabled mode
#define BM70_STATUS_IDLE              0x09 // Idle mode
#define BM70_STATUS_SHUTDOWN          0x0A // Shutdown mode
#define BM70_STATUS_CONFIG            0x0B // Configure mode
#define BM70_STATUS_CONNECTED         0x0C // BLE Connected mode

#define BM70_TYPE_RESPONSE            0x80 // Command complete event
#define BM70_TYPE_STATUS              0x81 // Status report event
#define BM70_TYPE_ADVERT_REPORT       0x70 // Advertising report event
#define BM70_TYPE_CONNECTION_COMPLETE 0x71 // LE connection complete event
#define BM70_TYPE_PAIR_COMPLETE       0x61 // Pair complete event
#define BM70_TYPE_PASSKEY_REQUEST     0x60 // Passkey entry request event
#define BM70_TYPE_TRANSPARENT_IN      0x9A // Received transparent data event

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
	uint8_t processAdvReport (uint8_t p1, uint8_t addressType, uint8_t * address, uint8_t dataLength, uint8_t * data, int8_t rssi);
	void configureAutoConnect (bool host, uint64_t address, char * passKey);
	void sendPassKey (char * passKey);
	void addTransparent (uint8_t * datas, uint16_t size);
	int transparentRead (char * data);
	int transparentWrite (char * data);
	bool transparentAvailable ();

	// Low level functions
	void send (uint8_t opCode, uint8_t * parameters, uint16_t parametersLength);
	int read ();
	int read (uint8_t * data, uint16_t bufferSize, uint16_t &length, uint16_t timeout = BM70_DEFAULT_TIMEOUT);
	int sendAndRead (uint8_t opCode, uint8_t * parameters, uint16_t parametersLength, uint8_t * response, uint16_t &length, uint16_t timeout = BM70_DEFAULT_TIMEOUT);

	bool isPaired ();

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

	int passKeySendDigit (uint8_t digit);
	int passKeyEraseDigit ();
	int passKeyClearDigit ();
	int passKeyComplete ();

	int enableTransparent (bool writeCmd = true);
	int disableTransparent ();

private:
	HardwareSerial * serial;

	uint8_t status;
	uint32_t lastStatusUpdate;
	uint8_t responseBuffer[BM70_RESPONSE_BUFF_SIZE][BM70_RESPONSE_MAX_SIZE], transparentBuffer[BM70_TRANSPARENT_BUFF_SIZE][BM70_TRANSPARENT_MAX_SIZE];
	uint8_t responseIndex, transparentIndex;
	uint32_t lastResponseBufferAccess;
	uint64_t connectAddress;
	bool autoConnect;
	bool isHost;
	bool paired;
	uint8_t connectionHandle;
	char passKey[7];
	bool actionCalled;
};

#endif // ifndef BM70_H
