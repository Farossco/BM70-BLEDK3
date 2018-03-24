#include <Arduino.h>
#include "BM70.h"

BM70 module;

void setup ()
{
	Serial.begin (9600);
	Serial1.begin (115200);
	Serial2.begin (115200);

	module = BM70 (&Serial1);
}

void loop ()
{
	if (Serial.available())
	{
		int voltage = module.readAdcValue (Serial.read() - '0');

		Serial.print ("Voltage: ");
		Serial.println (voltage);
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
