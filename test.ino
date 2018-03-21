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
        unsigned char params[1] = {0x00};
		module.send (0x06, params, 1);
		Serial.println ("Done.");
		Serial.read();
	}

	if (Serial2.available())
	{
		int data = Serial2.read();
		Serial1.write (data);
	}

	if (Serial1.available())
	{
		int data = Serial1.read();
		Serial2.write (data);
	}
}
