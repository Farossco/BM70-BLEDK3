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
		{
			int error;
			float voltage;

			uint8_t setting;
			uint32_t fwVersion;
			uint64_t btAddress;

			char name[20];
			char newName[17] = "BM70 - Test Mode";

			error = module.readLocalInformation (fwVersion, btAddress);
			Serial.print ("Error : ");
			Serial.println (error);
			Serial.print ("fwVersion = 0x");
			Serial.print (fwVersion, HEX);
			Serial.print ("  -  btAddress = 0x");
			Serial.print ((uint32_t) (btAddress >> 32), HEX);
			Serial.println ((uint32_t) btAddress, HEX);
			Serial.println();

			delay (2);

			error = module.reset();
			Serial.print ("Error : ");
			Serial.println (error);
			Serial.print ("Satus = 0x");
			Serial.println (module.status, HEX);
			Serial.println();

			delay (2);

			error = module.readStatus();
			Serial.print ("Error : ");
			Serial.println (error);
			Serial.print ("Satus = 0x");
			Serial.println (module.status, HEX);
			Serial.println();

			delay (2);

			voltage = module.readAdcValue (0x01);
			error   = (voltage < 0) ? voltage : 0;
			Serial.print ("Error : ");
			Serial.println (error);
			Serial.print ("ADC value : ");
			Serial.println (voltage, 3);
			Serial.println();

			delay (2);

			error = module.getName (name);
			Serial.print ("Error : ");
			Serial.println (error);
			Serial.print ("Name : ");
			Serial.println (name);
			Serial.println ("");

			delay (2);

			error = module.setName (newName);
			Serial.print ("Error : ");
			Serial.println (error);
			Serial.println ("");

			delay (2);

			error = module.eraseAllPaired();
			Serial.print ("Error : ");
			Serial.println (error);
			Serial.println ("");

			delay (2);

			error = module.readPairingModeSetting (setting);
			Serial.print ("Error : ");
			Serial.println (error);
			Serial.print ("Setting : ");
			Serial.println (setting, HEX);
			Serial.println ("");

			delay (2);

			error = module.writePairingModeSetting (0x02);
			Serial.print ("Error : ");
			Serial.println (error);
			Serial.println ("\n");
		}
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
