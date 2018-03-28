#include <Arduino.h>
#include "BM70.h"

void setup ()
{
	Serial.begin (250000);
	Serial1.begin (115200);
	Serial2.begin (115200);

	BM70.init (115200);
}

void loop ()
{
	BM70.receiveData();

	/*
	 * if (Serial1.available())
	 * {
	 *  char data = Serial1.read();
	 *  Serial2.write (data);
	 * }
	 *
	 * if (Serial2.available())
	 * {
	 *  char data = Serial2.read();
	 *  Serial1.write (data);
	 * }
	 */
} // loop

void testCommon ()
{
	int error;

	uint8_t setting;
	uint32_t fwVersion;
	uint64_t btAddress;
	float adcValue;
	uint64_t devices[8];
	uint8_t priorities[8];
	uint8_t size;

	char name[20];
	char newName[17] = "BM70 - Test Mode";


	Serial.println ("getInfos test");
	error = BM70.getInfos (fwVersion, btAddress);
	Serial.print ("Error : ");
	Serial.println (error);
	Serial.print ("fwVersion = 0x");
	Serial.print (fwVersion, HEX);
	Serial.print ("  -  btAddress = 0x");
	Serial.print ((uint32_t) (btAddress >> 32), HEX);
	Serial.println ((uint32_t) btAddress, HEX);
	Serial.println();

	delay (4);

	Serial.println ("reset test");
	error = BM70.reset();
	Serial.print ("Error : ");
	Serial.println (error);
	Serial.println();

	delay (4);

	Serial.println ("getStatus test");
	error = BM70.updateStatus();
	Serial.print ("Error : ");
	Serial.println (error);
	Serial.print ("Satus = 0x");
	Serial.println (BM70.getStatus(), HEX);
	Serial.println();

	delay (4);

	Serial.println ("getAdc test");
	error = BM70.getAdc (0x01, adcValue);
	Serial.print ("Error : ");
	Serial.println (error);
	Serial.print ("ADC value : ");
	Serial.println (adcValue, 3);
	Serial.println();

	delay (4);

	Serial.println ("shutDown test");
	error = BM70.shutDown();
	Serial.print ("Error : ");
	Serial.println (error);
	Serial.println();

	delay (1500);

	BM70.read();

	delay (4);

	Serial.println ("getName test");
	error = BM70.getName (name);
	Serial.print ("Error : ");
	Serial.println (error);
	Serial.print ("Name : ");
	Serial.println (name);
	Serial.println ("");

	delay (4);

	Serial.println ("setName test");
	error = BM70.setName (newName);
	Serial.print ("Error : ");
	Serial.println (error);
	Serial.println ("");

	delay (4);

	Serial.println ("getPairingMode test");
	error = BM70.getPairingMode (setting);
	Serial.print ("Error : ");
	Serial.println (error);
	Serial.print ("Setting : ");
	Serial.println (setting, HEX);
	Serial.println ("");

	delay (4);

	Serial.println ("setPairingMode test");
	error = BM70.setPairingMode (0x02);
	Serial.print ("Error : ");
	Serial.println (error);
	Serial.println ("");

	delay (4);

	Serial.println ("getPaired test");
	error = BM70.getPaired (devices, priorities, size);
	Serial.print ("Error : ");
	Serial.println (error);
	Serial.print ("Size: ");
	Serial.println (size);
	for (int i = 0; i < size; i++)
	{
		Serial.print ("Device ");
		Serial.print (i);
		Serial.print (" = 0x");
		Serial.print ((uint32_t) (devices[i] >> 32), HEX);
		Serial.println ((uint32_t) devices[i], HEX);
		Serial.print ("Prioriry: ");
		Serial.println (priorities[i]);
	}
	Serial.println ("");

	delay (4);

	Serial.println ("removePaired test");
	// error = BM70.removePaired (0xFF);
	Serial.print ("Error : ");
	Serial.println (error);
	Serial.println ("");

	delay (4);

	Serial.println ("enableScan test");
	error = BM70.enableScan();
	Serial.print ("Error : ");
	Serial.println (error);
	Serial.println ("");


	delay (4);

	Serial.println ("disableScan test");
	error = BM70.disableScan();
	Serial.print ("Error : ");
	Serial.println (error);
	Serial.println ("");


	delay (4);

	Serial.println ("connect test");
	error = BM70.connect (false, 0x001167500000);
	Serial.print ("Error : ");
	Serial.println (error);
	Serial.println ("");

	delay (4);

	Serial.println ("cancelConnect test");
	error = BM70.cancelConnect();
	Serial.print ("Error : ");
	Serial.println (error);
	Serial.println ("");

	delay (4);

	Serial.println ("disconnect test");
	error = BM70.disconnect();
	Serial.print ("Error : ");
	Serial.println (error);
	Serial.println ("");

	delay (4);

	Serial.println ("enableAdvert test");
	error = BM70.enableAdvert();
	Serial.print ("Error : ");
	Serial.println (error);
	Serial.println ("");

	delay (4);

	Serial.println ("disableAdvert test");
	error = BM70.disableAdvert();
	Serial.print ("Error : ");
	Serial.println (error);
	Serial.println ("\n");
} // testCommon

void testBuff (int a)
{
	uint8_t randomData1[] = { 0x58, 0x97, 0xFD, 0x98, 0x5D, 0xBD, 0xEA };
	uint8_t randomData2[] = { 0x48, 0x14, 0x45, 0x45, 0x58, 0x36, 0x87 };
	uint8_t randomData3[] = { 0xA8, 0x95, 0x22, 0x78, 0x46, 0x28, 0x16 };
	uint8_t randomData4[] = { 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77 };

	uint8_t response[8];

	uint16_t size;

	switch (a)
	{
		case 7:
			BM70.addResponse (0x08, randomData1, 7);
			break;

		case 8:
			BM70.addResponse (0x0A, randomData2, 5);
			break;

		case 9:
			BM70.getResponse (0x08, response, size);
			Serial.print ("\nSize: ");
			Serial.print (size);

			for (int i = 0; i < size; i++)
			{
				Serial.print ("  0x");
				Serial.print (response[i], HEX);
				Serial.print ("  ");
			}
			break;

		case 4:
			BM70.addResponse (0x0A, randomData3, 7);
			break;

		case 5:
			BM70.addResponse (0x0A, randomData4, 5);
			break;

		case 6:
			BM70.getResponse (0x0A, response, size);
			Serial.print ("\nSize: ");
			Serial.print (size);

			for (int i = 0; i < size; i++)
			{
				Serial.print ("  0x");
				Serial.print (response[i], HEX);
				Serial.print ("  ");
			}
			break;

		case 1:
			Serial.print (BM70.responseAvailable (0x08));
			break;

		case 2:
			Serial.print (BM70.responseAvailable (0x0A));
			break;
	}
} // testBuff

void serialEvent ()
{
	int byte = Serial.read();

	if (byte >= '1' && byte <= '9')
		testBuff (byte - '0');
	else if (byte == '0')
		testCommon();
}
