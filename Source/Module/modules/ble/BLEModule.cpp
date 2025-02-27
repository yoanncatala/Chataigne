/*
  ==============================================================================

	BLEModule.cpp
	Created: 1 Feb 2023 4:50:15pm
	Author:  bkupe

  ==============================================================================
*/

#include "Module/ModuleIncludes.h"
#include "BLEModule.h"

#if BLE_SUPPORT

//BLEValueComparator BLEModule::bleValueComparator;

BLEModule::BLEModule(const String& name) :
	Module(name),
	device(nullptr)
{
	//valuesCC.customControllableComparator = &BLEModule::bleValueComparator;
	deviceParam = new BLEDeviceParameter("Device", "BLE Device to connect", true);
	moduleParams.addParameter(deviceParam);

	serviceUUID = moduleParams.addStringParameter("Service UUID", "UUID for service to use", "19B10010-E8F2-537E-4F6C-D104768A1214");
	writeUUID = moduleParams.addStringParameter("Write UUID", "UUID for service to use", "19B10011-E8F2-537E-4F6C-D104768A1214");
	readUUID = moduleParams.addStringParameter("Read UUID", "UUID for service to use", "19B10012-E8F2-537E-4F6C-D104768A1214");

	isConnected = moduleParams.addBoolParameter("Is Connected", "This is checked if a ble device is connected.", false);
	isConnected->setControllableFeedbackOnly(true);
	isConnected->isSavable = false;
	connectionFeedbackRef = isConnected;
	connectDevice = moduleParams.addTrigger("Connect", "Connect the device", false);
	disconnectDevice = moduleParams.addTrigger("Disconnect", "Disconnect the device", false);

	BLEManager::getInstance()->addBLEManagerListener(this);

}

BLEModule::~BLEModule()
{
	if (BLEManager::getInstanceWithoutCreating() != nullptr)
	{
		BLEManager::getInstance()->removeBLEManagerListener(this);
	}

	setCurrentDevice(nullptr);
}

std::string joinStrings(const std::vector<std::string>& words, const std::string& delimiter) {
	if (words.empty()) return "";  // Handle empty case

	return std::accumulate(words.begin() + 1, words.end(), words[0],
		[&delimiter](const std::string& a, const std::string& b) {
			return a + delimiter + b;
		});
}
void BLEModule::bleServicesAndCharacteristicsInventory()
{
	NLOG("BLEModule", "Starting inventory");
	ControllableContainer* cParentContainer = &moduleParams;
	ControllableContainer* deviceContainer = cParentContainer->getControllableContainerByName(device->name, true);
	if (deviceContainer == nullptr) {
		deviceContainer = new ControllableContainer(device->name);
		deviceContainer->saveAndLoadRecursiveData = true;
		deviceContainer->isRemovableByUser = true;
		cParentContainer->addChildControllableContainer(deviceContainer);
	}
	for (auto& service : device->peripheral.services())
	{
		ControllableContainer* serviceContainer = deviceContainer->getControllableContainerByName(service.uuid(), true);
		if (serviceContainer == nullptr) {
			serviceContainer = new ControllableContainer(service.uuid());
			serviceContainer->saveAndLoadRecursiveData = true;
			serviceContainer->isRemovableByUser = false;
			deviceContainer->addChildControllableContainer(serviceContainer);
		}
		for (auto& characteristic : service.characteristics()) {
			ControllableContainer* characteristicContainer = serviceContainer->getControllableContainerByName(characteristic.uuid(), true);
			if (characteristicContainer == nullptr) {
				characteristicContainer = new ControllableContainer(characteristic.uuid());
				characteristicContainer->saveAndLoadRecursiveData = true;
				characteristicContainer->isRemovableByUser = false;
				serviceContainer->addChildControllableContainer(characteristicContainer);
			}

			Parameter* capabilitiesParameter = dynamic_cast<Parameter*>(characteristicContainer->getControllableByName("Capabilities", true));
			if (capabilitiesParameter == nullptr) {
				capabilitiesParameter = new StringParameter("Capabilities", "Characteristic capabilities", "", false);
				capabilitiesParameter->setValue((juce::var)joinStrings(characteristic.capabilities(), ","));
				capabilitiesParameter->isRemovableByUser = false;
				capabilitiesParameter->saveValueOnly = false;
				characteristicContainer->addParameter(capabilitiesParameter);
				characteristicContainer->sortControllables();
			}
			if (characteristic.can_read()) {
				Controllable* canReadTrigger = dynamic_cast<Controllable*>(characteristicContainer->getControllableByName("Read", true));
				if (canReadTrigger == nullptr) {
					canReadTrigger = new Trigger("Read", "", true);
					characteristicContainer->addControllable(canReadTrigger);
					characteristicContainer->sortControllables();
				}
				CharacteristicInfo characteristicInfo = CharacteristicInfo{ service.uuid(), characteristic.uuid() };
				characteristicTriggers.push_back(CharacteristicTrigger{ characteristicInfo ,CharacteristicTrigger::CharacteristicCapability::Read ,(Trigger*)canReadTrigger });
			}

			if (characteristic.can_notify()) {
				Controllable* canNotifyTrigger = dynamic_cast<Controllable*>(characteristicContainer->getControllableByName("Notify", true));
				if (canNotifyTrigger == nullptr) {
					canNotifyTrigger = new Trigger("Notify", "", true);
					characteristicContainer->addControllable(canNotifyTrigger);
					characteristicContainer->sortControllables();
				}
				CharacteristicInfo characteristicInfo = CharacteristicInfo{ service.uuid(), characteristic.uuid() };
				characteristicTriggers.push_back(CharacteristicTrigger{ characteristicInfo ,CharacteristicTrigger::CharacteristicCapability::Notify , (Trigger*)canNotifyTrigger });
			}
		}
	}
	NLOG("BLEModule", "Inventory done");
}

void BLEModule::setCurrentDevice(BLEDevice* _device, bool force)
{
	if (device == _device && !force) return;

	if (device != nullptr)
	{
		device->close();
		device->removeBLEDeviceListener(this);
	}

	device = _device;

	if (device != nullptr)
	{
		device->addBLEDeviceListener(this);

		if (device->isOpen()) device->close();
		device->open();
	}

	bleModuleListeners.call(&BLEModuleListener::currentDeviceChanged);
}


void BLEModule::onControllableFeedbackUpdate(ControllableContainer* cc, Controllable* c)
{
	if (c == deviceParam)
	{
		BLEDevice* newDevice = deviceParam->getDevice();
		BLEDevice* prevDevice = device;
		setCurrentDevice(newDevice);
		
		if (device == nullptr && prevDevice != nullptr)
		{
			DBG("Manually set no ghost device");
			lastOpenedDeviceID = ""; //forces no ghosting when user chose to manually disable device
		}
		if (deviceParam->getValueKey() == "Not connected or disconnected") {
			connectDevice->setEnabled(false);
			disconnectDevice->setEnabled(false);
		}
	}
	if (c == connectDevice) {
		NLOG("BLEModule::onControllableFeedbackUpdate", "Connect triggered");
		device->open();
	}
	if (c == disconnectDevice) {
		NLOG("BLEModule::onControllableFeedbackUpdate", "Disconnect triggered");
		device->close();
	}

	for (auto& characteristicTrigger : characteristicTriggers) {
		if (c == characteristicTrigger.characteristic_trigger) {
			NLOG("BLEModule", "Service uuid : " << characteristicTrigger.characteristic_info.service_uuid);
			NLOG("BLEModule", "Characteristic uuid : " << characteristicTrigger.characteristic_info.characteristic_uuid);
			switch (characteristicTrigger.characteristic_capability) {
			case CharacteristicTrigger::CharacteristicCapability::Read: {

				break;
			}
			case CharacteristicTrigger::CharacteristicCapability::Write: {
				break;
			}
			case CharacteristicTrigger::CharacteristicCapability::Notify: {
				device->notifyCharacteristic(characteristicTrigger.characteristic_info);
				break;
			}
			case CharacteristicTrigger::CharacteristicCapability::Indicate: {
				break;
			}
			default:
				break;
			}
		}
	}
}

void BLEModule::onContainerParameterChanged(Parameter* p)
{
	Module::onContainerParameterChanged(p);
	if (p == enabled)
	{
		if (device != nullptr)
		{
			NLOG(niceName, "Module is " << (enabled->boolValue() ? "enabled" : "disabled") << ", " << (enabled->boolValue() ? "opening" : "closing ble device"));
			setCurrentDevice(enabled->boolValue() ? device : nullptr, true);
		}

	}
}

void BLEModule::deviceOpened(BLEDevice*)
{
	lastOpenedDeviceID = device->id;

	NLOG("BLEModule::deviceOpened", "Device " << device->description << " opened.");

	bleModuleListeners.call(&BLEModuleListener::deviceOpened);
}

void BLEModule::deviceClosed(BLEDevice*)
{
	NLOG("BLEModule::deviceClosed", "Device " << device->description << " closed.");
	bleModuleListeners.call(&BLEModuleListener::deviceClosed);
}

void BLEModule::deviceRemoved(BLEDevice*)
{
	setCurrentDevice(nullptr);
}

void BLEModule::bleDeviceAdded(BLEDevice* d)
{
	if (device == nullptr && lastOpenedDeviceID == d->id)
	{
		setCurrentDevice(d);
	}
}

void BLEModule::bleDeviceRemoved(BLEDevice* d)
{
	if (device != nullptr && device == d) setCurrentDevice(nullptr);
}

void BLEModule::bleDataReceived(const var& data, CharacteristicInfo characteristicInfo)
{
	ControllableContainer* cParentContainer = &valuesCC;

	ControllableContainer* characteristicContainer = valuesCC.getControllableContainerByName(serviceUUID->value, true);
	if (characteristicContainer == nullptr)
	{
		characteristicContainer = new ControllableContainer(serviceUUID->value);
		characteristicContainer->saveAndLoadRecursiveData = true;
		characteristicContainer->isRemovableByUser = true;
		valuesCC.addChildControllableContainer(characteristicContainer, true);
	}
	cParentContainer = characteristicContainer;

	Parameter* p = dynamic_cast<Parameter*>(cParentContainer->getControllableByName(readUUID->value, true));

	if (p == nullptr)
	{
			p = new IntParameter(readUUID->value, "", 0, 0, INT32_MAX);
			p->setValue(data);
			p->isRemovableByUser = true;
			p->saveValueOnly = false;
			cParentContainer->addParameter(p);
			cParentContainer->sortControllables();
	}
	else
	{
		p->setValue(data);
	}
}
void BLEModule::bleDeviceConnected()
{
	isConnected->setValue(true);
	disconnectDevice->setEnabled(true);
	deviceParam->setEnabled(false);
	connectDevice->setEnabled(false);
	bleServicesAndCharacteristicsInventory();
	NLOG("BLEModule::bleDeviceConnected", std::to_string(isConnected->boolValue()));
}

void BLEModule::bleDeviceDisconnected()
{
	isConnected->setValue(false);
	disconnectDevice->setEnabled(false);
	deviceParam->setEnabled(true);
	connectDevice->setEnabled(true);
	NLOG("BLEModule::bleDeviceDisconnected", std::to_string(isConnected->boolValue()));
}




var BLEModule::getJSONData(bool includeNonOverriden)
{
	/*var data = StreamingModule::getJSONData(includeNonOverriden);
	data.getDynamicObject()->setProperty("deviceID", lastOpenedDeviceID);
	return data;*/
	return 0;
}

void BLEModule::loadJSONDataInternal(var data)
{
	/*StreamingModule::loadJSONDataInternal(data);
	lastOpenedDeviceID = data.getProperty("deviceID", "");*/
}

void BLEModule::setupModuleFromJSONData(var data)
{
	if (data.hasProperty("nameFilter"))
	{
		var nameFilterData = data.getProperty("vidFilter", "");
		if (nameFilterData.isArray())
		{
			for (int i = 0; i < nameFilterData.size(); i++) deviceParam->nameFilters.add(nameFilterData[i].toString());
		}
		else if (nameFilterData.toString().isNotEmpty())
		{
			deviceParam->nameFilters.add(nameFilterData.toString());
		}
	}

	//StreamingModule::setupModuleFromJSONData(data);
}



#endif