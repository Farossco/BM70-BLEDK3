#include <Arduino.h>
#include "BM70.h"

BM70 module;

void setup ()
{
	Serial.begin (250000);
	Serial1.begin (115200);
	Serial2.begin (115200);

	module = BM70 (&Serial1);
}

void loop ()
{
	if (Serial.available())
	{
		int byte = Serial.read();

		if (byte >= '0' && byte <= '9')
			testCommon();
	}

	if (Serial1.available())
	{
		char data = Serial1.read();
		Serial2.write (data);
	}

	if (Serial2.available())
	{
		char data = Serial2.read();
		Serial1.write (data);
	}
} // loop

void testCommon ()
{
	int error;

	uint8_t setting;
	uint32_t fwVersion;
	uint64_t btAddress;
	uint8_t status;
	float adcValue;
	uint64_t devices[8];
	uint8_t priorities[8];
	uint8_t size;

	char name[20];
	char newName[17] = "BM70 - Test Mode";


	Serial.println ("getInfos test");
	error = module.getInfos (fwVersion, btAddress);
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
	error = module.reset();
	Serial.print ("Error : ");
	Serial.println (error);
	Serial.println();

	delay (4);

	Serial.println ("getStatus test");
	error = module.getStatus (status);
	Serial.print ("Error : ");
	Serial.println (error);
	Serial.print ("Satus = 0x");
	Serial.println (status, HEX);
	Serial.println();

	delay (4);

	Serial.println ("getAdc test");
	error = module.getAdc (0x01, adcValue);
	Serial.print ("Error : ");
	Serial.println (error);
	Serial.print ("ADC value : ");
	Serial.println (adcValue, 3);
	Serial.println();

	delay (4);

	Serial.println ("shutDown test");
	error = module.shutDown();
	Serial.print ("Error : ");
	Serial.println (error);
	Serial.println();

	delay (1500);

	module.read();

	delay (4);

	Serial.println ("getName test");
	error = module.getName (name);
	Serial.print ("Error : ");
	Serial.println (error);
	Serial.print ("Name : ");
	Serial.println (name);
	Serial.println ("");

	delay (4);

	Serial.println ("setName test");
	error = module.setName (newName);
	Serial.print ("Error : ");
	Serial.println (error);
	Serial.println ("");

	delay (4);

	Serial.println ("getPairingMode test");
	error = module.getPairingMode (setting);
	Serial.print ("Error : ");
	Serial.println (error);
	Serial.print ("Setting : ");
	Serial.println (setting, HEX);
	Serial.println ("");

	delay (4);

	Serial.println ("setPairingMode test");
	error = module.setPairingMode (0x02);
	Serial.print ("Error : ");
	Serial.println (error);
	Serial.println ("");

	delay (4);

	Serial.println ("getPaired test");
	error = module.getPaired (devices, priorities, size);
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
	// error = module.removePaired (0xFF);
	Serial.print ("Error : ");
	Serial.println (error);
	Serial.println ("");

	delay (4);

	Serial.println ("enableScan test");
	error = module.enableScan();
	Serial.print ("Error : ");
	Serial.println (error);
	Serial.println ("");


	delay (4);

	Serial.println ("disableScan test");
	error = module.disableScan();
	Serial.print ("Error : ");
	Serial.println (error);
	Serial.println ("");


	delay (4);

	Serial.println ("connect test");
	error = module.connect (false, 0x001167500000);
	Serial.print ("Error : ");
	Serial.println (error);
	Serial.println ("");

	delay (4);

	Serial.println ("cancelConnect test");
	error = module.cancelConnect();
	Serial.print ("Error : ");
	Serial.println (error);
	Serial.println ("");

	delay (4);

	Serial.println ("disconnect test");
	error = module.disconnect();
	Serial.print ("Error : ");
	Serial.println (error);
	Serial.println ("");

	delay (4);

	Serial.println ("enableAdvert test");
	error = module.enableAdvert();
	Serial.print ("Error : ");
	Serial.println (error);
	Serial.println ("");

	delay (4);

	Serial.println ("disableAdvert test");
	error = module.disableAdvert();
	Serial.print ("Error : ");
	Serial.println (error);
	Serial.println ("\n");
} // testCommon
