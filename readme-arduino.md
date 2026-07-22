BLE Smoke Detector - Arduino
============================

The smoke detector will use these UUID's:

```
PRIMARY_SERVICE, 298cfeca-a10d-49ee-8a74-e513547f7ef7
CHARACTERISTIC, a8d99167-e58c-4a0c-9565-e2f1a7fbc05d, READ | WRITE | DYNAMIC
```

The idea is that you set these where you wish to detect smoke.

They should be generally off, only booting when the smoke detector detects smoke.
It then single shot fires off a notification to any BLE devices it can scan for,
signals done and expects to be powered off.

This readme is for devices on the Arduino SDK that come with external antenna and support CODED PHY for a longer range. 

See also readme-picow.md for details about using PicoW and PicoW2 devices with the Pico C++ SDK.

Arduino SDK
===========

This variant compiles ble-smoke-detector.ino and some of the files from src/
It ignores the rest of the files in the project root. The needed modules should be installed via the Arduino IDE.

Preferences Storage
===================

You can configure the smoke detector to report its location on each message to aid in finding out where it is.

You need to run the 'ble-store-preferences' program first on your device to store your local settings that will then persist.
Alternatively you can use a modified app with the 'deploy preferences' feature to set them at detector installation
via a BLE Packet(type_micro_mesh_preferences).

Building
========

Open the project in the Arduino IDE.

Install the needed packages:
 * NimBLE-Arduino by h2zero - https://github.com/h2zero/NimBLE-Arduino

Install the relevant boards package and select your device from the menus.

Device Support
==============

You need an Arduino device that supports BLE.

Tested so far:
 * XIAO ESP32C3 - comes with an external aerial that gives a good range.

Device Sleeps - Difficult to flash
==================================

If you are having trouble getting to flash updates to a smoke detector you can avoid its going to sleep by putting a jumper wire between GND and D10.
This will give you five minutes of uptime for each reboot.
