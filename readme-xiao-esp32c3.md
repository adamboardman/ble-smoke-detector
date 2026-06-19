BLE Smoke Detector - XIAO ESP32C3
=================================

The smoke detector will use these UUID's:

```
PRIMARY_SERVICE, 298cfeca-a10d-49ee-8a74-e513547f7ef7
CHARACTERISTIC, a8d99167-e58c-4a0c-9565-e2f1a7fbc05d, READ | WRITE | DYNAMIC
```

The idea is that you set these where you wish to detect smoke.

They should be generally off, only booting when the smoke detector detects smoke.
It then single shot fires off a notification to any BLE devices it can scan for,
signals done and expects to be powered off.

This readme is for the XIAO ESP32C3 devices on the Arduino SDK that come with external antenna and support CODED PHY for a longer range. 
See also readme-picow.md for details about using PicoW and PicoW2 devices with the Pico C++ SDK.

Note: This variant compiles ble-smoke-detector.ino, some of the files from src/ and ignores the rest of the files in the project root.

Building
========

Open the project in the Arduino IDE.
