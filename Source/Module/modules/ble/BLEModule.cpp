/*
  ==============================================================================

	BLEModule.cpp
	Created: 1 Feb 2023 4:50:15pm
	Author:  bkupe

  ==============================================================================
*/

#include "Module/ModuleIncludes.h"

#if BLE_SUPPORT

BLEModule::BLEModule(const String& name):
	Module(name),
	Thread(name + "_thread"),
	devicesCC("Devices")
{
	scanDevicesTrigger = moduleParams.addTrigger("Scan devices", "Scan for new devices");
	NLOG(niceName,"Module started");
	moduleParams.addChildControllableContainer(&devicesCC);
	startThread();
}

BLEModule::~BLEModule()
{
	NLOG(niceName, "Stopping thread");
	stopThread(3000);
}

void BLEModule::run() {
	NLOG(niceName, "Thread started");
	
	while (!threadShouldExit())
	{
		
		
		wait(3000);
	}
}


#endif