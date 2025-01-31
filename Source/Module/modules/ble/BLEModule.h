/*
  ==============================================================================

    BLEModule.h
    Created: 1 Feb 2023 4:50:15pm
    Author:  bkupe

  ==============================================================================
*/

#pragma once
#if BLE_SUPPORT

class BLEModule :
	public Module, 
	public Thread
{
public:
	BLEModule(const String& name = "Bluetooth LE");
	virtual ~BLEModule();

	Trigger* scanDevicesTrigger;

	ControllableContainer devicesCC;

	virtual void run() override;

	static BLEModule* create() { return new BLEModule(); }
	virtual String getDefaultTypeString() const override { return "Bluetooth LE"; }

};

#endif