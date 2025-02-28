/*
  ==============================================================================

	BLEDevice.cpp
	Created: 1 Feb 2023 1:22:59pm
	Author:  bkupe

  ==============================================================================
*/

#include "Common/CommonIncludes.h"
#include "BLEDevice.h"

#if BLE_SUPPORT

BLEDevice::BLEDevice(Peripheral& p) :
	peripheral(p),
	thread("BLE " + p.identifier(), this)
{
	name = p.identifier();
	id = p.address();
	description = name + " (" + id + ")";

	// configure peripheral callbacks
	peripheral.set_callback_on_connected([&]() {
		NLOG("BLEDevice", "Device connected : " << description);
		listeners.call(&BLEDevice::BLEDeviceListener::deviceOpened, this);
		listeners.call(&BLEDevice::BLEDeviceListener::bleDeviceConnected);
		if (thread.isThreadRunning()) return;
		thread.startThread();
		});
	
	peripheral.set_callback_on_disconnected([&]() {
		NLOG("BLEDevice", "Device disconnected : " << description);
		/*listeners.call(&BLEDevice::BLEDeviceListener::bleDeviceDisconnected);
		
		listeners.call(&BLEDevice::BLEDeviceListener::deviceClosed, this);*/
		thread.stopThread(1000);
		});

	
}

BLEDevice::~BLEDevice()
{
	//cleanup
	close();
	listeners.call(&BLEDeviceListener::deviceRemoved, this);
}
int byteArrayToIntBE(const std::string& byteArray) {
	int result = 0;
	for (unsigned char byte : byteArray) {  // Ensure correct type handling
		result = (result << 8) | static_cast<unsigned char>(byte);
	}
	return result;
}

std::string byteArrayToHexString(char* data, size_t length) {
	std::ostringstream oss;
	oss << std::hex << std::setfill('0');

	for (size_t i = 0; i < length; ++i) {
		oss << std::setw(2) << static_cast<int>(data[i]);
	}

	return oss.str();
}

void BLEDevice::setPeripheral(Peripheral& p)
{
	GenericScopedLock lock(peripheralLock);
	peripheral = p;
}

void BLEDevice::open()
{
	peripheral.connect();
}

void BLEDevice::close()
{
	try
	{
		peripheral.disconnect();
		
	}
	catch (std::exception e)
	{
		NLOGWARNING("BLE", "Error closing device : " << description);
	}
	listeners.call(&BLEDevice::BLEDeviceListener::bleDeviceDisconnected);
	listeners.call(&BLEDevice::BLEDeviceListener::deviceClosed, this);
	thread.removeBLEListener(this);
	thread.stopThread(1000);
}


bool BLEDevice::isOpen() {
	GenericScopedLock lock(peripheralLock);
	return peripheral.is_connected();
}

void BLEDevice::dataReceived(const var& data) {
	listeners.call(&BLEDeviceListener::bleDataReceived,data);

}

void BLEDevice::addBLEDeviceListener(BLEDeviceListener* newListener) { listeners.add(newListener); }

void BLEDevice::removeBLEDeviceListener(BLEDeviceListener* listener) { listeners.remove(listener); }

BLEReadThread::BLEReadThread(String name, BLEDevice* _device) :
	Thread(name + "_thread"),
	device(_device)
{

}

BLEReadThread::~BLEReadThread()
{
	stopThread(1000);
}


 
void BLEDevice::readCharacteristic(CharacteristicInfo characteristicInfo)
{
	try {
		auto readResult = peripheral.read(characteristicInfo.service_uuid, characteristicInfo.characteristic_uuid);
		auto resultObject = new juce::DynamicObject();
		resultObject->setProperty("service_uuid", (juce::var)characteristicInfo.service_uuid);
		resultObject->setProperty("char_uuid", (juce::var)characteristicInfo.characteristic_uuid);
		resultObject->setProperty("data", juce::String(std::to_string(byteArrayToIntBE(readResult))));
		dataReceived(resultObject);
		//dataReceived((juce::var)readResult);
	}
	catch (std::exception e)
	{
		NLOGWARNING("BLEDevice", "Error reading characteristic : " << characteristicInfo.characteristic_uuid);
	}
	
}

void BLEDevice::notifyCharacteristic(CharacteristicInfo characteristicInfo)
{
	peripheral.notify(characteristicInfo.service_uuid, characteristicInfo.characteristic_uuid, [&](SimpleBLE::ByteArray bytes) {
		juce::DynamicObject::Ptr resultObject = new juce::DynamicObject();
		resultObject->setProperty("service_uuid", juce::String(static_cast<std::string>( characteristicInfo.service_uuid)));
		resultObject->setProperty("char_uuid", juce::String(static_cast<std::string>(characteristicInfo.characteristic_uuid)));
		resultObject->setProperty("data", juce::String(std::to_string(byteArrayToIntBE(bytes))));
		juce::var objectVar = resultObject;
		dataReceived(objectVar);
	});
}

void BLEDevice::writeCharacteristic(CharacteristicInfo characteristicInfo, SimpleBLE::ByteArray bytes)
{
	try {
		peripheral.write_command(characteristicInfo.service_uuid, characteristicInfo.characteristic_uuid,bytes);
	}
	catch (std::exception e)
	{
		NLOGWARNING("BLEDevice", "Error reading characteristic : " << characteristicInfo.characteristic_uuid);
	}
}

void BLEReadThread::run() {
	NLOG("BLEDevice:" << device->name, "Thread started !");

	while (!threadShouldExit()) {
		sleep(1000);
		//NLOG("BLEDevice:" << device->name, "Thread running ... ");
	}
	NLOG("BLEDevice:" << device->name, "Thread stopped !");
}
/*
void BLEReadThread::run()
{
	NLOG("BLEDevice", "Connecting to BLE device " << device->description << "...");

	device->peripheral.connect();

	if (!device->isOpen())
	{
		NLOGERROR("BLEDevice", "Error opening device " << device->description);
		return;
	}

	NLOG("BLEDevice", "Device opened : " << device->description);
	device->listeners.call(&BLEDevice::BLEDeviceListener::deviceOpened, device);

	std::vector<uint8_t> byteBuffer; //for cobs and data255

	// check if device has characteritic
	// check if device characteristic is notify enabled
	// 
	//auto services = device->peripheral.services();
	//bool found = false;
	//SimpleBLE::Service batteryService;
	//SimpleBLE::Characteristic batteryCharacteristic;
	//for (auto& service : services)
	//{
	//	/*NLOG("BLEDEVICE", "Service found " + service.uuid());
	//	auto characteristics = service.characteristics();
	//	for (auto& characteristic : characteristics) {
	//		NLOG("BLEDEVICE", "Characteristic found " + characteristic.uuid());
	//	}
	//	if (service.uuid() == "0000180f-0000-1000-8000-00805f9b34fb") {
	//		NLOG("BLEDEVICE", "Service found !");
	//		batteryService = service;
	//		auto characteristics = service.characteristics();
	//		for (auto& characteristic : characteristics) {
	//			NLOG("BLEDEVICE", "Characteristic found " + service.uuid());
	//			if (characteristic.uuid() == "00002a19-0000-1000-8000-00805f9b34fb" && characteristic.can_notify()) {
	//				NLOG("BLEDEVICE", "Characteristic found !");
	//				found = true;
	//				batteryCharacteristic = characteristic;
	//				break;
	//			}
	//		}
	//	}
	//}
	//if (!found) {
	//	NLOG("BLEDEVICE", "No service nor characteristic found !");
	//	device->peripheral.disconnect();
	//	device->listeners.call(&BLEDevice::BLEDeviceListener::deviceClosed, device);
	//	return;
	//}

	//// register to notifications
	//device->peripheral.notify(batteryService.uuid(), batteryCharacteristic.uuid(), [&](SimpleBLE::ByteArray bytes) {
	//	//NLOG("BLEDEVICE", "Received data : " + bytes + " as int -> " + std::to_string(byteArrayToIntBE(bytes)).c_str());
	//	device->dataReceived((juce::var)byteArrayToIntBE(bytes));
	//	});
	static int iter = 0;
	while (!threadShouldExit())
	{
		sleep(2); //500fps

		if (device == nullptr) return;
		if (!device->isOpen())
		{
			NLOG("BLEDevice Thread", "Device not opened : " << device->description);
			return;
		}

		try
		{

		}
		catch (...)
		{
			DBG("### BLE Problem ");
		}
	}

	device->peripheral.disconnect();
	device->listeners.call(&BLEDevice::BLEDeviceListener::deviceClosed, device);

	DBG("END BLE THREAD");
}
*/
#endif