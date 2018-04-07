#include <Arduino.h>
#include "BM70.h"

BM70 module;

void setup ()
{
	Serial.begin (250000);

	module = BM70 (&Serial1);

	module.updateStatus();

	Log.info ("Connecting to Bluetooth module... ");

	while (module.getStatus() != BM70_STATUS_IDLE)
	{
		// module.reset();
	}

	Log.info ("Done" dendl);
}

void loop ()
{
	module.action();
}
