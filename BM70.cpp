#include "BM70.h"

BM70::BM70()
{ }

BM70::BM70(HardwareSerial * initSerial) : BM70 (initSerial, 115200)
{ }

BM70::BM70(HardwareSerial * initSerial, uint32_t baudrate)
{
	serial = initSerial;

	serial->begin (baudrate);
	responseIndex    = 0;
	transparentIndex = 0;
	status           = BM70_STATUS_UNKNOWN;
	autoConnect      = false;
	connectionHandle = 0x00;
	paired           = false;
}

void BM70::action ()
{
	receiveData();

	if (millis() - lastResponseBufferAccess > BM70_BUFFER_EMPTY_DELAY && responseIndex != 0)
	{
		// serial.println ("[action] Emptying response buffer");
		responseIndex = 0;
	}
	if (status != BM70_STATUS_CONNECTED && status != BM70_STATUS_TRANSCOM)
		paired = false;
}

int BM70::receiveData (uint16_t timeout)
{
	uint32_t timeoutCounter;

	timeoutCounter = millis();

	while (!serial->available())
	{
		if (millis() - timeoutCounter >= timeout)
			return -1;
	}

	// serial.println ("[receiveData] ReceiveData called and data available");

	uint8_t data[100]; // TODO : dynamic allocation
	uint16_t length;

	while (serial->available())
	{
		if (read (data, sizeof(data), length) == 0)
		{
			if (data[3] == BM70_TYPE_RESPONSE)
			{
				// serial.println ("[receiveData] Data is response");
				addResponse (data[4], data + 5, length - 6);
			}
			else if (data[3] == BM70_TYPE_STATUS)
			{
				// serial.println ("[receiveData] Data is status");
				status           = data[4];
				lastStatusUpdate = millis();
			}
			else if (data[3] == BM70_TYPE_ADVERT_REPORT)
			{
				// serial.println ("[receiveData] Data is advertising report");
				processAdvReport (data[4], data[5], data + 6, data[12], data + 13, data[length - 2]);
			}
			else if (data[3] == BM70_TYPE_CONNECTION_COMPLETE)
			{
				// serial.println ("[receiveData] Data is connection complete");
				// if (data[4] == 0)
				// serial.println ("Connection successful");
				// else
				// {
				// serial.println ("Connection failed");
				// return 0;
				// }
				connectionHandle = data[5];
				// serial.print ("[receiveData] Connection handle is 0x");
				// serial.println (connectionHandle, HEX);

				uint64_t address = 0;
				for (uint8_t i = 0; i < 6; i++)
					address += (((uint64_t) data[i + 8]) << (8 * i));

				// serial.print ("[action] Connecting address is 0x");
				// serial.print ((uint32_t) (address >> 32), HEX);
				// serial.println ((uint32_t) (address), HEX);

				// serial.print ("[action] Wanted address is 0x");
				// serial.print ((uint32_t) (connectAddress >> 32), HEX);
				// serial.println ((uint32_t) (connectAddress), HEX);

				if (address == connectAddress)
				{
					// serial.println ("Addresses match");
					paired = false;
				}
				else
				{
					// serial.println ("Addresses don't match! Disconnecting");
					disconnect();
					paired = false;
				}
			}
			else if (data[3] == BM70_TYPE_PAIR_COMPLETE)
			{
				// serial.println ("[receiveData] Data is pair complete");
				if (data[4] != connectionHandle)
				{
					// serial.print ("[receiveData] Bad connection handle (0x");
					// serial.print (data[4], HEX);
					// serial.println (")");
				}
				else
				{
					// serial.print ("[receiveData] Connection handle is 0x");
					// serial.println (connectionHandle, HEX);
					if (data[5] == 0x00)
					{
						// serial.println ("Paired!");
						paired = true;
					}
					else
					{
						// serial.println ("Error, not paired!");
						paired = false;
					}
				}
			}
			else if (data[3] == BM70_TYPE_PASSKEY_REQUEST)
			{
				// serial.println ("[receiveData] Data is passkey request");
				sendPassKey (passKey);
				lastStatusUpdate = millis();
			}
			else if (data[3] == BM70_TYPE_TRANSPARENT_IN)
			{
				// serial.print ("[receiveData] Data is transparent data\n[receiveData] Datas: ");
				for (uint8_t i = 5; i < length - 1; i++)
				{
					// serial.print ((char) data[i]);
				}
				// serial.println();

				addTransparent (data + 5, length - 6);
			}
			else
			{
				// serial.println ("[receiveData] Data is other");
			}
		}
	}

	return 0;
} // BM70::receiveData

uint8_t BM70::getStatus ()
{
	action();
	return status;
}

void BM70::setStatus (uint8_t status)
{
	this->status = status;
}

// Test taille buff + remontée

void BM70::addResponse (uint8_t opCode, uint8_t * datas, uint16_t size)
{
	lastResponseBufferAccess = millis();

	// serial.print ("[addResponse] Adding data to bufer: 0x");
	// serial.println (opCode, HEX);

	while (responseIndex >= BM70_RESPONSE_BUFF_SIZE)
	{
		// serial.println ("[addResponse] Buffer full, cleaning last value");

		responseIndex--;

		for (uint16_t i = 0; i < responseIndex; i++)
		{
			uint16_t lineSize = ((uint16_t) (responseBuffer[i + 1][1] << 8)) + ((uint16_t) responseBuffer[i + 1][2]) + 3;

			for (uint16_t l = 0; l < lineSize; l++)
				responseBuffer[i][l] = responseBuffer[i + 1][l];
		}
	}

	responseBuffer[responseIndex][0] = opCode;
	responseBuffer[responseIndex][1] = (uint8_t) (size >> 8);
	responseBuffer[responseIndex][2] = (uint8_t) size;

	for (uint16_t i = 0; i < size; i++)
	{
		responseBuffer[responseIndex][i + 3] = datas[i];
	}

	responseIndex++;

	// serial.print ("[addResponse] Buffer size: ");
	// serial.println (responseIndex);
} // BM70::addResponse

int BM70::getResponse (uint8_t opCode, uint8_t * response, uint16_t &size)
{
	lastResponseBufferAccess = millis();

	// serial.print ("[getResponse] Reading buffer for opCode 0x");
	// serial.println (opCode, HEX);

	for (int i = 0; i < responseIndex; i++)
	{
		if (responseBuffer[i][0] == opCode)
		{
			// serial.print ("[getResponse] Value found at index ");
			// serial.println (i);

			size = ((uint16_t) (responseBuffer[i][1] << 8)) + ((uint16_t) responseBuffer[i][2]);

			for (uint16_t j = 0; j < size; j++)
				response[j] = responseBuffer[i][j + 3];

			responseIndex--;

			for (uint16_t k = i; k < responseIndex; k++)
			{
				uint16_t lineSize = ((uint16_t) (responseBuffer[k + 1][1] << 8)) + ((uint16_t) responseBuffer[k + 1][2]) + 3;

				for (uint16_t l = 0; l < lineSize; l++)
					responseBuffer[k][l] = responseBuffer[k + 1][l];
			}

			responseBuffer[responseIndex][0] = 0x00;

			return 0;
		}
	}

	size = 0;

	return -1;
} // BM70::getResponse

int BM70::responseAvailable (uint8_t opCode)
{
	int n = 0;

	for (int i = 0; i < responseIndex; i++)
		if (responseBuffer[i][0] == opCode)
			n++;

	// serial.print ("[responseAvailable] ");
	// serial.print (n);
	// serial.print (" response available for 0x");
	// serial.println (opCode, HEX);

	return n;
}

uint8_t BM70::processAdvReport (uint8_t p1, uint8_t addressType, uint8_t * address, uint8_t dataLength, uint8_t * data, int8_t rssi)
{
	if (autoConnect == false)
		return -1;

	if (p1 != 0x00 && p1 != 0x01 && p1 != 0x02)
		return -2;

	uint64_t connectAddress = 0;

	for (uint8_t i = 0; i < 6; i++)
		connectAddress += (((uint64_t) address[i]) << (8 * i));

	// serial.print ("[addAdvReport] Received adv address 0x");
	// serial.print ((uint32_t) (connectAddress >> 32), HEX);
	// serial.print ((uint32_t) (connectAddress), HEX);

	if (this->connectAddress != connectAddress)
		return -3;

	// serial.println ("[addAdvReport] Addresses match: connecting...");

	connect (addressType == 0x01, connectAddress);

	return 0;
}

void BM70::configureAutoConnect (bool host, uint64_t address, char * passKey)
{
	removePaired (0xFF);

	connectAddress = address;
	autoConnect    = true;
	isHost         = host;

	// serial.print ("[configureAutoConnect] Configuring auto connect as ");
	// if (host)
	// serial.print ("host");
	// else
	// serial.print ("slave");

	// serial.print (" with address 0x");
	// serial.print ((uint32_t) (address >> 32), HEX);
	// serial.print ((uint32_t) (address), HEX);

	// serial.print (" and passKey = ");
	// serial.println (passKey);

	for (uint8_t i = 0; i <= strlen (passKey); i++)
	{
		this->passKey[i] = passKey[i];
	}

	// serial.println();

	if (host)
		enableScan();
	else
		enableAdvert();
}

void BM70::sendPassKey (char * pass)
{
	if (!autoConnect)
		return;

	for (uint8_t i = 0; i < 6; i++)
	{
		// serial.print ("Sending digit ");
		// serial.println (pass[i]);

		passKeySendDigit (pass[i]);
		delay (50);
	}

	passKeyComplete();
}

void BM70::addTransparent (uint8_t * datas, uint16_t size)
{
	if (size >= BM70_TRANSPARENT_MAX_SIZE)
	{
		// serial.println ("[addTransparent] Datas are too long, ignoring");
		return;
	}

	// serial.print ("[addTransparent] Adding transparent data to buffer: ");
	for (uint8_t i = 0; i < size; i++)
	{
		// serial.print ((char) datas[i]);
	}
	// serial.println();

	while (transparentIndex >= BM70_TRANSPARENT_BUFF_SIZE)
	{
		// serial.println ("[addTransparent] Buffer full, cleaning last value");

		transparentIndex--;

		for (uint16_t i = 0; i < transparentIndex; i++)
		{
			int16_t lineSize = strlen ((char *) transparentBuffer[i + 1]);
			if (lineSize == -1 || lineSize >= BM70_TRANSPARENT_MAX_SIZE)
			{
				// serial.println ("[addTransparent] Failed to find \\0: copying all the line");
				lineSize = BM70_TRANSPARENT_MAX_SIZE;
			}

			for (int16_t l = 0; l < lineSize; l++)
				transparentBuffer[i][l] = transparentBuffer[i + 1][l];
		}
	}

	for (uint16_t i = 0; i < size; i++)
	{
		transparentBuffer[transparentIndex][i] = datas[i];
	}

	transparentBuffer[transparentIndex][size] = '\0';

	transparentIndex++;

	// serial.print ("[addTransparent] Buffer size: ");
	// serial.println (transparentIndex);
} // BM70::addTransparent

int BM70::transparentRead (char * data)
{
	if (status != BM70_STATUS_TRANSCOM)
		return -3;

	if (transparentIndex <= 0)
	{
		// serial.println ("[readTransparent] Transparent buffer empty");
		transparentIndex = 0;
		data[0]          = '\0';
		return -1;
	}

	// serial.println ("[readTransparent] Reading transparent buffer");

	int16_t size = (strlen ((char *) transparentBuffer[0]));

	transparentIndex--;

	if (size > 0)
	{
		for (int16_t i = 0; i < size; i++)
			data[i] = transparentBuffer[0][i];
		data[size] = '\0';
	}
	else
	{
		data[0] = '\0';
	}

	for (uint16_t i = 0; i < transparentIndex; i++)
	{
		int16_t lineSize = strlen ((char *) transparentBuffer[i + 1]);
		if (lineSize == -1)
		{
			// serial.println ("[addTransparent] Failed to find \\0: copying all the line");
			lineSize = BM70_TRANSPARENT_MAX_SIZE;
		}

		for (int16_t l = 0; l < lineSize; l++)
			transparentBuffer[i][l] = transparentBuffer[i + 1][l];
	}

	return 0;
} // BM70::transparentRead

int BM70::transparentWrite (char * data)
{
	if (status != BM70_STATUS_TRANSCOM)
		return -3;

	uint8_t response[4], arguments[51]; // Response parameters should not exceed 1 byte
	uint16_t length;
	int16_t dataLength;

	dataLength = strlen (data);
	if (dataLength < 0 || dataLength >= 50)
		return -2;

	arguments[0] = connectionHandle;

	for (uint8_t i = 0; i < dataLength; i++)
	{
		arguments[i + 1] = data[i];
	}

	receiveData();

	if (sendAndRead (0x3F, arguments, dataLength + 1, response, length) != 0)
		return -1;

	return 0;
}

bool BM70::transparentAvailable ()
{
	if (status != BM70_STATUS_TRANSCOM)
		return false;

	return transparentIndex > 0;
}

/**
 * Send raw datas to the Bluetooth module
 *
 * Input	: opCode					- The OP Code to send
 * Input	: parameters (char array)	- The parameters to send
 * Input	: parametersLength			- The length of the parameters array
 *
 *
 * Returns	: Nothing
 *
 **/
void BM70::send (uint8_t opCode, uint8_t * parameters, uint16_t parametersLength)
{
	uint8_t lengthH  = (uint8_t) ((1 + parametersLength) >> 8);
	uint8_t lengthL  = (uint8_t) (1 + parametersLength);
	uint8_t checksum = 0 - lengthH - lengthL - opCode;

	// Serial.print ("[send] Sending data with opCode 0x");
	// Serial.println (opCode, HEX);

	for (uint16_t i = 0; i < parametersLength; i++)
		checksum -= parameters[i];

	serial->write (0xAA);
	serial->write (lengthH);
	serial->write (lengthL);
	serial->write (opCode);
	for (uint16_t i = 0; i < parametersLength; i++)
		serial->write (parameters[i]);
	serial->write (checksum);
}

/**
 * Just read the value and throw it to the void
 *
 * Input	: timeout - Timeout before giving up (in ms) - default is 10 ms
 *
 *
 * Returns	: 0 if succeeded
 *           -1 if no start received
 *           -2 if incorrect checksum
 *           -3 if reception timed out
 *
 **/
int BM70::read ()
{
	uint8_t data[5];
	uint16_t length;

	return read (data, sizeof(data), length);
}

/**
 * Read incomming data and detect eventual error
 *
 * Output	: data			- The received data
 * Input	: bufferSize	- The size of the data buffer provided
 * Output	: length		- The length of the datas
 * Input	: timeout		- Timeout before giving up (in ms) - default is 10 ms
 *
 *
 * Returns	: 0 if succeeded
 *           -1 if no start received
 *           -2 if incorrect checksum
 *           -3 if reception timed out
 *           -6 if provided buffer is too small
 *
 **/
int BM70::read (uint8_t * data, uint16_t bufferSize, uint16_t &length, uint16_t timeout)
{
	uint8_t checksum;
	uint32_t timeoutCounter;

	timeoutCounter = millis();

	// Serial.print ("[read] Start reading : ");

	do
	{
		if (serial->available())
		{
			data[0] = serial->read();
			// Serial.print (data[0], HEX);
			// Serial.print (" ");
		}

		if (millis() - timeoutCounter >= timeout)
			return -3;

		delay (2);
	}
	while (data[0] != 0xAA);

	// serial.println();

	data[1] = serial->read();
	data[2] = serial->read();

	length = (((short) data[1]) << 8 ) + data[2] + 4;

	if (length > bufferSize)
	{
		// serial.print ("[read] data emptying : ");

		// Emptying the UART buffer
		for (uint16_t i = 3; serial->available() && i < length; i++)
		{
			// serial.print (serial->read(), HEX);
			// serial.print (" ");
			serial->read();
			delay (2);
		}

		// serial.println();
		return -6;
	}

	// serial.print ("[read] data reading : ");

	for (uint16_t i = 3; serial->available() && i < length; i++)
	{
		data[i] = serial->read();
		// serial.print (data[i], HEX);
		// serial.print (" ");
		delay (2);
	}

	// serial.println();

	checksum = 0;

	for (uint16_t i = 2; i <= length - 1; i++)
		checksum += data[i];

	// Incorrect checksum
	if (checksum != 0)
		return -2;

	return 0;
} // BM70::read

/**
 * Send data and wait for an answer
 *
 * Input	: opCode					- The OP Code to send
 * Input	: parameters (char array)	- The parameters to send
 * Input	: parametersLength			- The length of the parameters array
 * Output	: response					- The received response parameters without the command complete event and without checksum
 * Output	: length					- The length of the response
 * Input	: timeout					- Timeout before giving up (in ms) - default is 10 ms
 *
 *
 * Returns	: 0 if succeeded
 *           -1 if no start received
 *           -2 if incorrect checksum
 *           -3 if reception timed out
 *           -4 if answer is not an actual answer
 *           -5 if not the answer to the specified request
 *           -6 if provided buffer is too small
 *
 **/
int BM70::sendAndRead (uint8_t opCode, uint8_t * parameters, uint16_t parametersLength, uint8_t * response, uint16_t &length, uint16_t timeout)
{
	uint32_t timeoutCounter;

	// serial.print ("[sendAndRead] Send and read for 0x");
	// serial.println (opCode, HEX);

	send (opCode, parameters, parametersLength);

	timeoutCounter = millis();

	// serial.println ("[sendAndRead] Available ressources before update: ");
	responseAvailable (opCode);

	while (!responseAvailable (opCode))
	{
		receiveData();

		if (millis() - timeoutCounter >= 300)
		{
			// serial.println ("[sendAndRead] Error, no response available");
			return -3;
		}
	}

	// serial.println ("[sendAndRead] Getting response from buffer");
	getResponse (opCode, response, length);

	// serial.print ("[sendAndRead] Response:");

	// for (uint16_t i = 0; i < length; i++)
	// {
	// serial.print (" 0x");
	// if (response[i] < 16)
	// serial.print (0);
	// serial.print (response[i], HEX);
	// }

	// serial.println();

	if (response[0] != 0x00)
	{
		// serial.println ("[sendAndRead] Error, Bad response code (!= 0x00)");
		return response[1];
	}

	// serial.println ("[sendAndRead] Everything went well :)");

	return 0;
} // BM70::sendAndRead

bool BM70::isPaired ()
{
	action();
	return paired;
}

// ******************************************************************************************** //
// ************************************** Common commands ************************************* //
// ******************************************************************************************** //

/**
 * Read Local informations from BM70 module
 *
 * Output	: fwVersion				- The Firmware version of the module
 * Output	: btAddress				- The Bluetooth address of the module
 *
 * Returns	: 0 if succeeded
 *           -1 if failed to receive answer
 *
 **/
int BM70::getInfos (uint32_t &fwVersion, uint64_t &btAddress)
{
	uint8_t response[13]; // Response parameters should not exceed 10 bytes
	uint16_t length;

	fwVersion = btAddress = 0;

	receiveData();

	int error = sendAndRead (0x01, NULL, 0, response, length);

	if (error != 0)
		return error;

	fwVersion = (((uint32_t) response[1]) << 24) + (((uint32_t) response[2]) << 16) + (((uint32_t) response[3]) << 8) + response[4];
	btAddress = (((uint64_t) response[10]) << 40) + ( ((uint64_t) response[9]) << 32) + (((uint64_t) response[8]) << 24) + (((uint64_t) response[7]) << 16) + (((uint64_t) response[6]) << 8) + response[5];

	return 0;
}

/**
 * Reset module
 *
 **/
int BM70::reset ()
{
	receiveData();

	send (0x02, NULL, 0);

	receiveData (500);

	return 0;
}

/**
 * Ask module fot status update
 *
 **/
int BM70::updateStatus ()
{
	receiveData();

	send (0x03, NULL, 0);

	receiveData (100);

	return 0;
}

/**
 * Read voltage value from ADC
 *
 * Input	: channel (0x00 -> 0x0F or 0x10 for Battery and 0x11 for temperature)	- Channelto read from
 * Output	: adcValue																- Voltage value in volt or temperature value in °C
 *
 * Returns	: 0 if succeeded
 *           -1 if failed to receive answer or bad answer
 *
 * TODO : Temperature in °C
 *
 **/
int BM70::getAdc (uint8_t channel, float &adcValue)
{
	uint8_t response[7], stepSize; // Response parameters should not exceed 4 bytes
	uint16_t length, value;
	float stepVolt;

	value = stepSize = 0;

	receiveData();

	if (sendAndRead (0x04, &channel, 1, response, length) != 0)
		return -1;

	stepSize = response[1];
	value    = (((uint16_t) response[2]) << 8) + response[3];

	// stepVolt = 0.1 / (pow (2, stepSize - 1));

	switch (stepSize)
	{
		case 0:
			stepVolt = 1;
			break;

		case 1:
			stepVolt = 0.1;
			break;

		case 2:
			stepVolt = 0.05;
			break;

		case 3:
			stepVolt = 0.025;
			break;

		default:
			return -1;
	}

	adcValue = stepVolt * value;

	return 0;
} // BM70::readAdcValue

/**
 * Puts the module into shutdown mode
 *
 **/
int BM70::shutDown ()
{
	uint8_t response[4]; // Response parameters should not exceed 1 byte
	uint16_t length;

	receiveData();

	if (sendAndRead (0x05, NULL, 0, response, length) != 0)
		return -1;

	receiveData (100);

	return 0;
}

/**
 * Read the device name and copy it in a char array with a '\0' as end character
 *
 * Output	: name (char array)  - The name of the device
 *
 * Returns	: 0 if succeeded
 *           -1 if failed to receive answer or bad answer
 *
 **/
int BM70::getName (char * name)
{
	uint8_t response[36]; // Response parameters should not exceed 33 bytes
	uint16_t length;

	receiveData();

	if (sendAndRead (0x07, NULL, 0, response, length) != 0)
		return -1;

	for (uint16_t i = 1; i < length - 1; i++)
	{
		name[i - 1] = response[i];
	}

	name [length - 1] = '\0';

	return 0;
}

/**
 * Write the device name with the one given
 *
 * Input	: name (char array)		- The name of the device (16 char max - Ending with '\0' !)
 *
 * Returns	: 0 if succeeded
 *           -1 if failed to receive answer or bad answer
 *           -2 if name empty or doesn't contains '\0'
 *           -3 if name too long
 *
 **/
int BM70::setName (char * name)
{
	uint8_t response[4], parameters[16]; // Response parameters should not exceed 1 bytes
	uint16_t length, parametersLength;

	parametersLength = strlen (name);

	if (parametersLength <= 0)
		return -2;
	else if (parametersLength > 16)
		return -3;

	parameters[0] = 0x00;

	for (uint16_t i = 0; i < parametersLength; i++)
		parameters[i + 1] = name[i];

	receiveData();

	if (sendAndRead (0x08, parameters, parametersLength + 1, response, length) != 0)
		return -1;

	return 0;
}

/**
 * Get the current pairing mode setting of the device
 *
 * Output	: setting - The pairing mode from 0x00 to 0x04 :	0x00 = DisplayOnly
 *																0x01 = DisplayYesNo
 *																0x02 = KeyboardOnly
 *																0x03 = NoInputNoOutput
 *																0x04 = KeyboardDisplay
 *
 * Returns	: 0 if succeeded
 *           -1 if failed to receive answer or bad answer
 *
 **/
int BM70::getPairingMode (uint8_t &setting)
{
	uint8_t response[5]; // Response parameters should not exceed 2 bytes
	uint16_t length;

	receiveData();

	if (sendAndRead (0x0A, NULL, 0, response, length) != 0)
		return -1;

	setting = response[1];

	return 0;
}

/**
 * Set the given pairing mode setting to the device
 *
 * Input	: setting - The pairing mode from 0x00 to 0x04 :	0x00 = DisplayOnly
 *																0x01 = DisplayYesNo
 *																0x02 = KeyboardOnly
 *																0x03 = NoInputNoOutput
 *																0x04 = KeyboardDisplay
 *
 * Returns	: 0 if succeeded
 *           -1 if failed to receive answer or bad answer
 *           -2 if setting is incorrect
 *
 **/
int BM70::setPairingMode (uint8_t setting)
{
	uint8_t response[4], parameters[2]; // Response parameters should not exceed 1 byte
	uint16_t length;

	if (setting > 0x04)
		return -2;

	parameters[0] = 0x00;
	parameters[1] = setting;

	receiveData();

	if (sendAndRead (0x0B, parameters, 2, response, length) != 0)
		return -1;

	return 0;
}

/**
 * Read all paired devices addresses and put them in an array
 *
 * Output	: devices (Array of size 8)		- Paired devices from most recent to oldest (0 is most recent)
 * Output	: priorities (Array of size 8)	- The priority of the device (Priority of device i is priorities[i])
 * Output	: size (Size of the arrays)		- The number of devices that are paired and copied to the array
 *
 * Note		: "size" is what you should consider as the size of the array, evrything beyond this is
 *			  unpredictable and unwanted datas, the "devices" array should always have a size of 8.
 *
 * Returns	: 0 if succeeded
 *           -1 if failed to receive answer or bad answer
 *           -2 if bad length
 *           -3 if bad index positionning
 *           -4 if bad number of paired devices
 *
 **/
int BM70::getPaired (uint64_t * devices, uint8_t * priorities, uint8_t &size)
{
	uint8_t response[69]; // Response parameters should not exceed 66 bytes
	uint16_t length;

	receiveData();

	if (sendAndRead (0x0C, NULL, 0, response, length) != 0)
		return -1;

	if ((length - 2) % 8 != 0)
		return -2;

	size = (length - 2) / 8;

	if (size != response[1])
		return -4;

	for (int i = 0; i < size; i++)
	{
		if (response[2 + 8 * i] != i) // If the index is not the one expected
			return -3;

		devices[i] = 0;

		priorities[i] = response[3 + 8 * i];

		for (int j = 0; j < 6; j++)
		{
			devices[i] += ((uint64_t) response[4 + 8 * i + j]) << (8 * j);
		}
	}

	return 0;
} // BM70::getPaired

/**
 * Erase given paired device from the module memory
 *
 * Input	: Index - The index of the device to erase - 0xFF for all
 *
 * Returns	: 0 if succeeded
 *           -1 if failed to receive answer or bad answer
 *
 **/
int BM70::removePaired (uint8_t index)
{
	uint8_t response[4]; // Response parameters should not exceed 1 byte
	uint16_t length;

	receiveData();

	if (sendAndRead (index == 0xFF ? 0x09 : 0x0D, index == 0xFF ? NULL : &index, index == 0xFF ? 0 : 1, response, length) != 0)
		return -1;

	return 0;
}

// ******************************************************************************************** //
// *************************************** GAP commands *************************************** //
// ******************************************************************************************** //


/**
 * Activate scanning mode
 *
 * Input	: showDuplicate - Tell if you want to receive updated infos of the same devices during the scan
 *
 * Returns	: 0 if succeeded
 *           -1 if failed to receive answer or bad answer
 *           -2 if not in the right status after the command
 *
 **/
int BM70::enableScan (boolean showDuplicate)
{
	uint8_t response[4], arguments[2]; // Response parameters should not exceed 1 byte
	uint16_t length;

	arguments[0] = 0x01;
	arguments[1] = (uint8_t) !showDuplicate;

	receiveData();

	if (sendAndRead (0x16, arguments, 2, response, length) != 0)
		return -1;

	return 0;
}

/**
 * Leave scanning mode
 *
 * Returns	: 0 if succeeded
 *           -1 if failed to receive answer or bad answer
 *           -2 if not in the right status after the command
 *
 **/
int BM70::disableScan ()
{
	uint8_t response[4], arguments[2]; // Response parameters should not exceed 1 byte
	uint16_t length;

	arguments[0] = 0x00;
	arguments[1] = 0x00;

	receiveData();

	if (sendAndRead (0x16, arguments, 2, response, length) != 0)
		return -1;

	return 0;
}

/**
 * Connect to the specified device
 *
 **/
int BM70::connect (boolean randomAddress, uint64_t address)
{
	uint8_t arguments[8];

	arguments[0] = 0x00;
	arguments[1] = (uint8_t) randomAddress;

	for (int i = 0; i < 6; i++)
		arguments[2 + i] = (uint8_t) (address >> (8 * i));

	receiveData();

	send (0x17, arguments, 8);

	receiveData (500);

	return 0;
}

/**
 * Cancel connection attempting
 *
 * Returns	: 0 if succeeded
 *           -1 if failed to receive answer or bad answer
 **/
int BM70::cancelConnect ()
{
	uint8_t response[4]; // Response parameters should not exceed 1 byte
	uint16_t length;

	receiveData();

	if (sendAndRead (0x18, NULL, 0, response, length) != 0)
		return -1;

	return 0;
}

/**
 * Disconnect Bluetooth
 *
 * Returns	: 0 if succeeded
 *           -1 if failed to receive answer or bad answer
 *
 **/
int BM70::disconnect ()
{
	uint8_t response[4], argument; // Response parameters should not exceed 1 byte
	uint16_t length;

	argument = 0x00;

	receiveData();

	if (sendAndRead (0x1B, &argument, 1, response, length) != 0)
		return -1;

	return 0;
}

/**
 * Enable Bluetooth advertising (Goes into standby mode)
 *
 * Returns	: 0 if succeeded
 *           -1 if failed to receive answer or bad answer
 *
 **/
int BM70::enableAdvert ()
{
	uint8_t response[4], argument; // Response parameters should not exceed 1 byte
	uint16_t length;

	argument = 0x01;

	receiveData();

	if (sendAndRead (0x1C, &argument, 1, response, length) != 0)
		return -1;

	return 0;
}

/**
 * Disable Bluetooth advertising (Leaves standby mode)
 *
 * Returns	: 0 if succeeded
 *           -1 if failed to receive answer or bad answer
 *
 **/
int BM70::disableAdvert ()
{
	uint8_t response[4], argument; // Response parameters should not exceed 1 byte
	uint16_t length;

	argument = 0x00;

	receiveData();

	if (sendAndRead (0x1C, &argument, 1, response, length) != 0)
		return -1;

	return 0;
}

/**
 * Disable Bluetooth advertising (Leaves standby mode)
 *
 * Returns	: 0 if succeeded
 *           -1 if failed to receive answer or bad answer
 *
 **/
int BM70::passKeySendDigit (uint8_t digit)
{
	uint8_t arguments[3];

	arguments[0] = connectionHandle;
	arguments[1] = 0x01;
	arguments[2] = digit;

	send (0x40, arguments, 3);

	return 0;
}

int BM70::passKeyEraseDigit ()
{
	uint8_t arguments[2];

	arguments[0] = connectionHandle;
	arguments[1] = 0x02;

	send (0x40, arguments, 3);

	return 0;
}

int BM70::passKeyClearDigit ()
{
	uint8_t arguments[2];

	arguments[0] = connectionHandle;
	arguments[1] = 0x03;

	send (0x40, arguments, 3);

	return 0;
}

int BM70::passKeyComplete ()
{
	uint8_t arguments[2];

	arguments[0] = connectionHandle;
	arguments[1] = 0x04;

	send (0x40, arguments, 3);

	return 0;
}

int BM70::enableTransparent (bool writeCmd)
{
	uint8_t response[4], arguments[3]; // Response parameters should not exceed 1 byte
	uint16_t length;

	arguments[0] = connectionHandle;
	arguments[1] = 0x01;
	arguments[2] = 0x00;

	receiveData();

	if (sendAndRead (0x35, arguments, 1, response, length) != 0)
		return -1;

	return 0;
}

int BM70::disableTransparent ()
{
	uint8_t response[4], arguments[3]; // Response parameters should not exceed 1 byte
	uint16_t length;

	arguments[0] = connectionHandle;
	arguments[1] = 0x00;
	arguments[2] = 0x00;

	receiveData();

	if (sendAndRead (0x1C, arguments, 1, response, length) != 0)
		return -1;

	return 0;
}
