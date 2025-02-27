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
	public BLEDevice::BLEDeviceListener,
	public BLEManager::BLEManagerListener
{
public:
	BLEModule(const String& name = "Bluetooth LE");
	virtual ~BLEModule();

	//Device info
	StringParameter* nameFilter;
	StringParameter* serviceUUID;
	StringParameter* readUUID;
	StringParameter* writeUUID;

	String deviceID;
	String lastOpenedDeviceID; //for ghosting

	BLEDeviceParameter* deviceParam;
	BLEDevice* device;
	BoolParameter* isConnected;
	Trigger* connectDevice;
	Trigger* disconnectDevice;

	
	class CharacteristicTrigger {
		
	public:
		enum CharacteristicCapability {
			Write,
			Read,
			Notify,
			Indicate
		};
		CharacteristicInfo characteristic_info;
		CharacteristicCapability characteristic_capability;
		Trigger* characteristic_trigger;
		
	};

	std::vector<CharacteristicTrigger> characteristicTriggers;

	void bleServicesAndCharacteristicsInventory();

	virtual void setCurrentDevice(BLEDevice* device, bool force = false);
	
	void onContainerParameterChanged(Parameter* p) override;
	void onControllableFeedbackUpdate(ControllableContainer* cc, Controllable* c) override;


	// Inherited via BLEDeviceListener
	virtual void deviceOpened(BLEDevice*) override;
	virtual void deviceClosed(BLEDevice*) override;
	virtual void deviceRemoved(BLEDevice*) override;
	virtual void bleDataReceived(const var& data, CharacteristicInfo characteristicInfo) override;
	virtual void bleDeviceConnected() override;
	virtual void bleDeviceDisconnected() override;

	// Inherited via BLEManagerListener
	virtual void bleDeviceAdded(BLEDevice* device) override;
	virtual void bleDeviceRemoved(BLEDevice* device) override;

	virtual var getJSONData(bool includeNonOverriden = false) override;
	virtual void loadJSONDataInternal(var data) override;

	virtual void setupModuleFromJSONData(var data) override;

	class BLEModuleListener
	{
	public:
		virtual ~BLEModuleListener() {}
		virtual void deviceOpened() {}
		virtual void deviceClosed() {}
		virtual void currentDeviceChanged() {}
	};

	ListenerList<BLEModuleListener> bleModuleListeners;
	void addBLEModuleListener(BLEModuleListener* newListener) { bleModuleListeners.add(newListener); }
	void removeBLEModuleListener(BLEModuleListener* listener) { bleModuleListeners.remove(listener); }

	static BLEModule* create() { return new BLEModule(); }
	virtual String getDefaultTypeString() const override { return "Bluetooth LE"; }

};

#endif