/*
  ==============================================================================

	BLEManager.cpp
	Created: 1 Feb 2023 1:22:50pm
	Author:  bkupe

  ==============================================================================
*/

#include "Common/CommonIncludes.h"

#if BLE_SUPPORT

#include "simpleble/Utils.h"

juce_ImplementSingleton(BLEManager);

BLEManager::BLEManager() :
	Thread("BLE Manager")
{
	startThread();
}
BLEManager::~BLEManager()
{
	stopThread(5000);
}

void BLEManager::setup()
{
	if (!Adapter::bluetooth_enabled()) {
		NLOG("BLEManager", "Bluetooth is not enabled!");
		return;
	}

	auto adapters = Adapter::get_adapters();

	if (adapters.size() >= 1) {

		if (adapters.size() > 1)
		{
			NLOG("BLEManager", "Multiple Bluetooth adapters found, you can set the one you want to use in preferences");
			for (auto& a : adapters)
			{
				LOG(" > " << a.identifier() << " [" << a.address() << "]");
			}
		}

		adapter = adapters.at(0);
		adapter->set_callback_on_scan_found([&](SimpleBLE::Peripheral peripheral) {
			////NLOG("BLEManager", "Found device " << peripheral.identifier() << "[" << adapter->address() << "]");
			//if (String(peripheral.identifier()).isEmpty()) {
			//	//NLOG("BLEManager", "No identifier, ignoring device ...");
			//	return;
			//}
			//if (!peripheral.is_connected() && !peripheral.is_connectable()) 
			//{
			//	//NLOG("BLEManager", "Not connectable, ignoring device ...");
			//	return;
			//}
			//NLOG("BLEManager", "Found device " << peripheral.identifier() << "[" << adapter->address() << "]");
			//if (BLEDevice* d = getDevice(peripheral))
			//{
			//	d->setPeripheral(peripheral);
			//	NLOG("BLEManager", "Already existing, ignoring device ...");
			//	return;
			//}
			//addDevice(peripheral);
		});
		adapter->set_callback_on_scan_start([]() { NLOG("BLEManager", "Scan started."); });
		adapter->set_callback_on_scan_stop([&]() { 
			//Array<BLEDevice*> devicesToRemove;
			//for (auto& d : devices)
			//{
			//	bool found = false;
			//	for (auto& s : adapter->scan_get_results())
			//	{
			//		String newID = s.address();
			//		if (d->id == newID)
			//		{
			//			found = true;
			//			break;
			//		}
			//	}
			//	if (!found)
			//	{
			//		//create one so peripherals can delete the original
			//		devicesToRemove.add(d);
			//	}
			//}

			////check removed devices
			//for (auto& d : devicesToRemove) removeDevice(d);
			NLOG("BLEManager", "Scan stopped.");
			});
		
		NLOG("BLEManager", "Using adapter " << adapter->identifier() << "[" << adapter->address() << "]");

		return;
	}

	if (adapters.empty()) {
		NLOGWARNING("BLEManager", "No adapter was found.");
		return;
	}
}


void BLEManager::updateDeviceList()
{
	adapter->scan_for(5000);
	
	std::vector<SimpleBLE::Peripheral> scanResult = adapter->scan_get_results();

	
	for (auto& p : scanResult)
	{
		if (String(p.identifier()).isEmpty()) continue;
		if (!p.is_connected() && !p.is_connectable()) continue;
		if (BLEDevice* d = getDevice(p))
		{
			d->setPeripheral(p);
			continue;
		}

		addDevice(p);
	}

	//check removed
	Array<BLEDevice*> devicesToRemove;
	for (auto& d : devices)
	{
		bool found = false;
		for (auto& s : scanResult)
		{
			String newID = s.address();
			if (d->id == newID)
			{
				found = true;
				break;
			}
		}

		if (!found)
		{
			//create one so peripherals can delete the original
			devicesToRemove.add(d);
		}
	}

	//check removed devices
	for (auto& d : devicesToRemove) removeDevice(d);
	
}

BLEDevice* BLEManager::getDevice(Peripheral& p)
{
	String pid = p.address();
	for (auto& d : devices)if (d->id == pid) return d;
	return nullptr;
}

BLEDevice* BLEManager::getDevice(String deviceID)
{
	for (auto& d : devices) if (d->id == deviceID) return getDevice(d->peripheral);
	return nullptr;
}

void BLEManager::addDevice(Peripheral& p)
{
	BLEDevice* d = new BLEDevice(p);
	devices.add(d);

	NLOG("BLEManager", "Device Added : " + d->description);
	listeners.call(&BLEManagerListener::bleDeviceAdded, d);
}

void BLEManager::removeDevice(BLEDevice* device)
{
	if(device == nullptr) return;

	device->close();

	NLOG("BLEManager", "Device Removed : " + device->description);
	listeners.call(&BLEManagerListener::bleDeviceRemoved, device);

	devices.removeObject(device, true);
}

void BLEManager::run()
{
	setup();

	if (!adapter.has_value()) return;
	static int count = 0;
	while (!threadShouldExit())
	{
		//adapter->scan_for(5000);
		if(count <1){
			updateDeviceList();
			count++;
		}
		wait(3000);
	}
}

#endif