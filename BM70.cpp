#include "BM70.h"

BM70::BM70()
{ }

BM70::BM70(HardwareSerial * serial) : BM70 (serial, 115200)
{ }

BM70::BM70(HardwareSerial * serial, int baudrate)
{
	this->serial = serial;
	serial->begin (baudrate);
	status = 0x00;
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
	uint16_t flag    = 2;
	uint8_t lengthH  = (uint8_t) ((1 + parametersLength) >> 8);
	uint8_t lengthL  = (uint8_t) (1 + parametersLength);
	uint8_t checksum = 0 - lengthH - lengthL - opCode;

	HardwareSerial * serial0 = serial;

	for (int i = 0; i < parametersLength; i++)
		checksum -= parameters[i];

	while (flag)
	{
		if (flag == 1)
		{
			serial0->write (0xFA);
		}
		serial0->write (0xAA);
		serial0->write (lengthH);
		serial0->write (lengthL);
		serial0->write (opCode);
		for (int i = 0; i < parametersLength; i++)
			serial0->write (parameters[i]);
		serial0->write (checksum);

		flag--;
		serial0 = &Serial2;
	}
}

/**
 * Read incomming data and detect eventual error
 *
 * Output	: data		- The received data
 * Output	: length	- The length of the datas
 * Input	: timeout	- Timeout before giving up (in ms) - default is 10 ms
 *
 *
 * Returns	: 0 if succeeded
 *           -1 if no start received
 *           -2 if incorrect checksum
 *           -3 if reception timed out
 *
 **/
int BM70::read (uint8_t * data, uint16_t &length, uint16_t timeout)
{
	uint8_t checksum;
	time_t timeoutCounter;

	if (!serial->available())
		return -3;

	timeoutCounter = millis();

	do
	{
		data[0] = serial->read();

		if (millis() - timeoutCounter >= timeout || !serial->available())
			return -1;

		delay (1);
	}
	while (data[0] != 0xAA);

	for (int i = 1; serial->available() && i < 50; i++)
	{
		data[i] = serial->read();
		delay (1);
	}

	length = (((short) data[1]) << 8 ) + data[2];

	checksum = 0;

	for (int i = 2; i <= length + 3; i++)
		checksum += data[i];

	// Incorrect checksum
	if (checksum != 0)
		return -2;

	Serial2.write (0xFB);
	Serial2.write (data, length + 4);

	return 0;
} // BM70::read

/**
 * Send data and wait for an answer
 *
 * Input	: opCode					- The OP Code to send
 * Input	: parameters (char array)	- The parameters to send
 * Input	: parametersLength			- The length of the parameters array
 * Output	: data						- The received data
 * Output	: length					- The length of the datas
 * Input	: timeout					- Timeout before giving up (in ms) - default is 10 ms
 *
 *
 * Returns	: 0 if succeeded
 *           -1 if no start received
 *           -2 if incorrect checksum
 *           -3 if reception timed out
 *           -4 if answer is not an actual answer
 *           -5 if not the answer to the specified request
 *
 * Warning : execution can last up to 200 ms if no answer is received
 *
 **/
int BM70::sendAndRead (uint8_t opCode, uint8_t * parameters, uint16_t parametersLength, uint8_t * data, uint16_t &length, uint16_t timeout, uint16_t waitDelay, uint16_t maxAttempts)
{
	int16_t errorCode;
	uint16_t attempt;
	time_t timeoutCounter;

	errorCode = attempt = 0;

	do
	{
		attempt++;

		if (attempt > maxAttempts)
			return errorCode;

		send (opCode, parameters, parametersLength);

		delay (waitDelay);

		timeoutCounter = millis();

		while (!serial->available())
			if (millis() - timeoutCounter >= timeout)
				break;

		errorCode = read (data, length, timeout);

		if (errorCode == 0)
		{
			if (data[3] != 0x80 && data[3] != 0x81)
			{
				errorCode = -4;
			}
			else if (data[3] == 0x80)
			{
				if (data[4] != opCode)
				{
					errorCode = -5;
				}
				else
				{
					errorCode = data[5];
				}
			}
		}

		Serial.print ("Attempt ");
		Serial.print (attempt);
		Serial.print ("  -  Erreur ");
		Serial.println (errorCode);
	}
	while (errorCode != 0)
	;

	return 0;
} // BM70::sendAndRead

/**
 * Read Local informations from BM70 module
 *
 * Input	: timeout (optional)	- Timeout for receiving the answer (in ms) - default is 10 ms
 * Output	: fwVersion				- The Firmware version of the module
 * Output	: btAddress				- The Bluetooth address of the module
 *
 * Returns	: 0 if succeeded
 *           -1 if failed to receive answer
 *
 **/
int BM70::readLocalInformation (uint32_t &fwVersion, uint64_t &btAddress, uint16_t timeout)
{
	uint8_t data[20]; // Response should not exceed 17 bytes and parameters should not exceed 12
	uint16_t length;

	fwVersion = btAddress = 0;

	if (sendAndRead (0x01, { }, 0, data, length) != 0)
		return -1;

	fwVersion = (((uint32_t) data[6]) << 24) + (((uint32_t) data[7]) << 16) + (((uint32_t) data[8]) << 8) + data[9];
	btAddress = (((uint64_t) data[15]) << 40) + ( ((uint64_t) data[14]) << 32) + (((uint64_t) data[13]) << 24) + (((uint64_t) data[12]) << 16) + (((uint64_t) data[11]) << 8) + data[10];

	return 0;
}

/**
 * Reset module and store status if succeeded
 *
 * Input	: timeout (optional) - Timeout for receiving the answer (in ms) - default is 10 ms
 *
 * Returns	: 0 if succeeded
 *           -1 if failed to receive answer
 *
 **/
int BM70::reset (uint16_t timeout)
{
	uint8_t data[10]; // Response should not exceed 6 bytes
	uint16_t length;

	if (sendAndRead (0x02, { }, 0, data, length, timeout, 100) != 0)
		return -1;

	status = data[4];

	return 0;
}

/**
 * Read and store status if succeeded
 *
 * Input	: timeout (optional) - Timeout for receiving the answer (in ms) - default is 10 ms
 *
 * Returns	: 0 if succeeded
 *           -1 if failed to receive answer
 *
 **/
int BM70::readStatus (uint16_t timeout)
{
	uint8_t data[10]; // Response should not exceed 6 bytes
	uint16_t length;

	if (sendAndRead (0x03, { }, 0, data, length, timeout) != 0)
		return -1;

	status = data[4];

	return 0;
}

/**
 * Read and store status if succeeded
 *
 * Input	: channel (0x00 -> 0x0F or 0x10 for Battery and 0x11 for temperature)
 * Input	: timeout (optional) - Timeout for receiving the answer (in ms) - default is 10 ms
 *
 * Returns	: Voltage value in volt or temperature value in °C
 *           -1 if failed to receive answer
 *
 **/
float BM70::readAdcValue (uint8_t channel, uint16_t timeout)
{
	uint8_t data[15], stepSize; // Response should not exceed 10 bytes
	uint16_t length, value;
	float stepVolt;

	value = stepSize = 0;

	if (sendAndRead (0x04, &channel, 1, data, length, timeout) != 0)
		return -1;

	stepSize = data[6];
	value    = (((uint16_t) data[7]) << 8) + data[8];

	// stepVolt = 0.1 / (pow (2, stepSize - 1));

	switch (stepSize)
	{
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

	return stepVolt * value;
} // BM70::readAdcValue
