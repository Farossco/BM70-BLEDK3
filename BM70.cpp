#include "BM70.h"

int BM70_Class::receiveData (uint16_t timeout)
{
	time_t timeoutCounter;

	timeoutCounter = millis();

	while (!Serial1.available())
	{
		if (millis() - timeoutCounter >= timeout)
			return -1;
	}

	uint8_t data[100]; // TODO : dynamic allocation
	uint16_t length;

	while (Serial1.available())
	{
		if (read (data, sizeof(data), length) == 0)
		{
			if (data[3] == 0x80)
			{
				addResponse (data[4], data + 5, length - 6);
			}
			else if (data[3] == 0x81)
			{
				status           = data[4];
				lastStatusUpdate = millis();
			}
		}
	}

	return 0;
} // BM70_Class::receiveData

BM70_Class::BM70_Class()
{ }

void BM70_Class::init (int baudrate)
{
	Serial1.begin (baudrate);
	responseIndex = 0;
}

uint8_t BM70_Class::getStatus ()
{
	return status;
}

void BM70_Class::setStatus (uint8_t status)
{
	this->status = status;
}

// Test taille buff + remontée

void BM70_Class::addResponse (uint8_t opCode, uint8_t * datas, uint16_t size)
{
	while (responseIndex >= BM70_BUFF_SIZE)
	{
		responseIndex--;

		for (int i = 0; i < responseIndex; i++)
		{
			uint16_t lineSize = ((uint16_t) (responseBuffer[i + 1][1] << 8)) + ((uint16_t) responseBuffer[i + 1][2]) + 3;

			for (int l = 0; l < lineSize; l++)
				responseBuffer[i][l] = responseBuffer[i + 1][l];
		}
	}

	responseBuffer[responseIndex][0] = opCode;
	responseBuffer[responseIndex][1] = (uint8_t) (size >> 8);
	responseBuffer[responseIndex][2] = (uint8_t) size;

	for (int i = 0; i < size; i++)
	{
		responseBuffer[responseIndex][i + 3] = datas[i];
	}

	responseIndex++;
}

int BM70_Class::getResponse (uint8_t opCode, uint8_t * response, uint16_t &size)
{
	for (int i = 0; i < responseIndex; i++)
	{
		if (responseBuffer[i][0] == opCode)
		{
			size = ((uint16_t) (responseBuffer[i][1] << 8)) + ((uint16_t) responseBuffer[i][2]);

			for (int j = 0; j < size; j++)
				response[j] = responseBuffer[i][j + 3];

			responseIndex--;

			for (int k = i; k < responseIndex; k++)
			{
				uint16_t lineSize = ((uint16_t) (responseBuffer[k + 1][1] << 8)) + ((uint16_t) responseBuffer[k + 1][2]) + 3;

				for (int l = 0; l < lineSize; l++)
					responseBuffer[k][l] = responseBuffer[k + 1][l];
			}

			responseBuffer[responseIndex][0] = 0x00;

			return 0;
		}
	}

	size = 0;

	return -1;
}

int BM70_Class::responseAvailable (uint8_t opCode)
{
	int n = 0;

	for (int i = 0; i < responseIndex; i++)
		if (responseBuffer[i][0] == opCode)
			n++;

	return n;
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
void BM70_Class::send (uint8_t opCode, uint8_t * parameters, uint16_t parametersLength)
{
	uint16_t flag    = 2;
	uint8_t lengthH  = (uint8_t) ((1 + parametersLength) >> 8);
	uint8_t lengthL  = (uint8_t) (1 + parametersLength);
	uint8_t checksum = 0 - lengthH - lengthL - opCode;

	HardwareSerial * serial0 = &Serial1;

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
int BM70_Class::read ()
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
int BM70_Class::read (uint8_t * data, uint16_t bufferSize, uint16_t &length, uint16_t timeout)
{
	uint8_t checksum;
	time_t timeoutCounter;

	timeoutCounter = millis();

	Serial.print ("[DEBUG] start reading : ");

	do
	{
		if (Serial1.available())
		{
			data[0] = Serial1.read();
			Serial.print (data[0], HEX);
			Serial.print (" ");
		}

		if (millis() - timeoutCounter >= timeout)
			return -3;

		delay (2);
	}
	while (data[0] != 0xAA);

	Serial.println();

	data[1] = Serial1.read();
	data[2] = Serial1.read();

	length = (((short) data[1]) << 8 ) + data[2] + 4;

	if (length > bufferSize)
	{
		Serial.print ("[DEBUG] data emptying : ");

		// Emptying the UART buffer
		for (int i = 3; Serial1.available() && i < length; i++)
		{
			Serial.print (Serial1.read(), HEX);
			Serial.print (" ");
			Serial1.read();
			delay (2);
		}

		Serial.println();
		return -6;
	}

	Serial.print ("[DEBUG] data reading : ");

	for (int i = 3; Serial1.available() && i < length; i++)
	{
		data[i] = Serial1.read();
		Serial.print (data[i], HEX);
		Serial.print (" ");
		delay (2);
	}

	Serial.println();

	checksum = 0;

	for (int i = 2; i <= length - 1; i++)
		checksum += data[i];

	// Incorrect checksum
	if (checksum != 0)
		return -2;

	Serial2.write (0xFB);
	Serial2.write (data, length);

	return 0;
} // BM70_Class::read

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
int BM70_Class::sendAndRead (uint8_t opCode, uint8_t * parameters, uint16_t parametersLength, uint8_t * response, uint16_t &length, uint16_t timeout)
{
	uint32_t timeoutCounter;

	send (opCode, parameters, parametersLength);

	timeoutCounter = millis();

	Serial.print ("Available ressources: ");
	Serial.println (responseAvailable (opCode));

	while (!responseAvailable (opCode))
	{
		receiveData();

		if (millis() - timeoutCounter >= 300)
			return -3;
	}

	if (response[1] != 0x00)
		return response[1];

	getResponse (opCode, response, length);

	return 0;
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
int BM70_Class::getInfos (uint32_t &fwVersion, uint64_t &btAddress)
{
	uint8_t response[13]; // Response parameters should not exceed 10 bytes
	uint16_t length;

	fwVersion = btAddress = 0;

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
int BM70_Class::reset ()
{
	send (0x02, NULL, 0);

	receiveData (500);

	return 0;
}

/**
 * Ask module fot status update
 *
 **/
int BM70_Class::updateStatus ()
{
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
int BM70_Class::getAdc (uint8_t channel, float &adcValue)
{
	uint8_t response[7], stepSize; // Response parameters should not exceed 4 bytes
	uint16_t length, value;
	float stepVolt;

	value = stepSize = 0;

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
} // BM70_Class::readAdcValue

/**
 * Puts the module into shutdown mode
 *
 **/
int BM70_Class::shutDown ()
{
	uint8_t response[4]; // Response parameters should not exceed 1 byte
	uint16_t length;

	if (sendAndRead (0x05, NULL, 0, response, length) != 0)
		return -1;

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
int BM70_Class::getName (char * name)
{
	uint8_t response[36]; // Response parameters should not exceed 33 bytes
	uint16_t length;

	if (sendAndRead (0x07, NULL, 0, response, length) != 0)
		return -1;

	for (int i = 1; i < length - 1; i++)
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
int BM70_Class::setName (char * name)
{
	uint8_t response[4], parameters[16]; // Response parameters should not exceed 1 bytes
	uint16_t length, parametersLength;

	parametersLength = strlen (name);

	if (parametersLength <= 0)
		return -2;
	else if (parametersLength > 16)
		return -3;

	parameters[0] = 0x00;

	for (int i = 0; i < parametersLength; i++)
		parameters[i + 1] = name[i];

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
int BM70_Class::getPairingMode (uint8_t &setting)
{
	uint8_t response[5]; // Response parameters should not exceed 2 bytes
	uint16_t length;

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
int BM70_Class::setPairingMode (uint8_t setting)
{
	uint8_t response[4], parameters[2]; // Response parameters should not exceed 1 byte
	uint16_t length;

	if (setting > 0x04)
		return -2;

	parameters[0] = 0x00;
	parameters[1] = setting;

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
int BM70_Class::getPaired (uint64_t * devices, uint8_t * priorities, uint8_t &size)
{
	uint8_t response[69]; // Response parameters should not exceed 66 bytes
	uint16_t length;

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
}

/**
 * Erase given paired device from the module memory
 *
 * Input	: Index - The index of the device to erase - 0xFF for all
 *
 * Returns	: 0 if succeeded
 *           -1 if failed to receive answer or bad answer
 *
 **/
int BM70_Class::removePaired (uint8_t index)
{
	uint8_t response[4]; // Response parameters should not exceed 1 byte
	uint16_t length;

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
int BM70_Class::enableScan (boolean showDuplicate)
{
	uint8_t response[4], arguments[2]; // Response parameters should not exceed 1 byte
	uint16_t length;

	arguments[0] = 0x01;
	arguments[1] = (uint8_t) !showDuplicate;

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
int BM70_Class::disableScan ()
{
	uint8_t response[4], arguments[2]; // Response parameters should not exceed 1 byte
	uint16_t length;

	arguments[0] = 0x00;
	arguments[1] = 0x00;

	if (sendAndRead (0x16, arguments, 2, response, length) != 0)
		return -1;

	return 0;
}

/**
 * Connect to the specified device
 *
 **/
int BM70_Class::connect (boolean randomAddress, uint64_t address)
{
	uint8_t arguments[8];

	arguments[0] = 0x00;
	arguments[1] = (uint8_t) randomAddress;

	for (int i = 0; i < 6; i++)
		arguments[2 + i] = (uint8_t) (address >> (8 * i));

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
int BM70_Class::cancelConnect ()
{
	uint8_t response[4]; // Response parameters should not exceed 1 byte
	uint16_t length;

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
int BM70_Class::disconnect ()
{
	uint8_t response[4], argument; // Response parameters should not exceed 1 byte
	uint16_t length;

	argument = 0x00;

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
int BM70_Class::enableAdvert ()
{
	uint8_t response[4], argument; // Response parameters should not exceed 1 byte
	uint16_t length;

	argument = 0x01;

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
int BM70_Class::disableAdvert ()
{
	uint8_t response[4], argument; // Response parameters should not exceed 1 byte
	uint16_t length;

	argument = 0x00;

	if (sendAndRead (0x1C, &argument, 1, response, length) != 0)
		return -1;

	return 0;
}

BM70_Class BM70 = BM70_Class();
