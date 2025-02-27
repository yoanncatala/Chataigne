/*
  ==============================================================================

	BLEDevice.h
	Created: 1 Feb 2023 1:22:59pm
	Author:  bkupe

  ==============================================================================
*/

#pragma once

#if BLE_SUPPORT

class BLEDevice;

class BLEReadThread :
	public Thread
{
public:

	BLEReadThread(String name, BLEDevice* _device);
	virtual ~BLEReadThread();

	BLEDevice* device;


	virtual void run() override;


	class BLEThreadListener {
	public:
		virtual ~BLEThreadListener() {};
		virtual void dataReceived(const var&) {};
	};

	ListenerList<BLEThreadListener> serialThreadListeners;

	void addBLEListener(BLEThreadListener* newListener) { serialThreadListeners.add(newListener); }
	void removeBLEListener(BLEThreadListener* listener) { serialThreadListeners.remove(listener); }
};

class CharacteristicInfo {
public:
	SimpleBLE::BluetoothUUID service_uuid;
	SimpleBLE::BluetoothUUID characteristic_uuid;
	String niceName = characteristic_uuid;
};

class BLEDevice :
	public BLEReadThread::BLEThreadListener
{
public:
	BLEReadThread thread;

	BLEDevice(Peripheral& peripheral);

	virtual ~BLEDevice();

	Peripheral peripheral;

	String name;
	String id;
	String description;

	SpinLock peripheralLock;

	void setPeripheral(Peripheral& p);

	void notifyCharacteristic(CharacteristicInfo characteristicInfo);

	void open();
	void close();

	//void disconnect();

	bool isOpen();

	void dataReceived(const var& data, CharacteristicInfo characteristicInfo);

	class BLEDeviceListener
	{
	public:
		virtual ~BLEDeviceListener() {}

		//serial data here
		virtual void deviceOpened(BLEDevice*) {};
		virtual void deviceClosed(BLEDevice*) {};
		virtual void deviceRemoved(BLEDevice* p) {};
		virtual void bleDataReceived(const var&, CharacteristicInfo characteristicInfo) {};
		virtual void bleDeviceConnected() {};
		virtual void bleDeviceDisconnected() {};
		
	};

	ListenerList<BLEDeviceListener> listeners;
	void addBLEDeviceListener(BLEDeviceListener* newListener);
	void removeBLEDeviceListener(BLEDeviceListener* listener);
};

#endif