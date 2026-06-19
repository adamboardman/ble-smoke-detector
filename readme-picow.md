BLE Smoke Detector - PicoW
==========================

The smoke detector will use these UUID's:

```
PRIMARY_SERVICE, 298cfeca-a10d-49ee-8a74-e513547f7ef7
CHARACTERISTIC, a8d99167-e58c-4a0c-9565-e2f1a7fbc05d, READ | WRITE | DYNAMIC
```

The idea is that you set these where you wish to detect smoke.

They should be generally off, only booting when the smoke detector detects smoke.
It then single shot fires off a notification to any BLE devices it can scan for,
signals done and expects to be powered off.

This readme is for the PicoW and PicoW2 devices using the Pico C++ SDK version.
See also readme-xiao-esp32c3.md for details about using that device with the Arduino SDK.

Note: This variant uses btstack, cmake and compiles main.cpp.

Building
========

The Pico_w is perfectly sufficient, and as the cheaper device it would seem more likely to be used embedded with the 
insides of a smoke detector.

Command Line
------------

To build on the command line for the Pico_w you could type something along the lines of:

```
ble-smoke-detector$ mkdir build
ble-smoke-detector$ cd build
ble-smoke-detector/build$ cmake .. -DPICO_SDK_PATH=~/pico-pi/pico-sdk -DPICOTOOL_FETCH_FROM_GIT_PATH=~/pico-pi/picotool
ble-smoke-detector/build$ make
```

Or for a Pico2_w:

```
ble-smoke-detector$ mkdir build2350
ble-smoke-detector$ cd build2350
ble-smoke-detector/build2350$ cmake .. -DPICO_PLATFORM=rp2350-arm-s -DPICO_SDK_PATH=~/pico-pi/pico-sdk -DPICOTOOL_FETCH_FROM_GIT_PATH=~/pico-pi/picotool
ble-smoke-detector/build2350$ make
```

CLion
-----

If using CLion you'll need to set 'CMake options' to something along the lines of the following:

```
-DPICO_SDK_PATH=~/pico-pi/pico-sdk -DPICOTOOL_FETCH_FROM_GIT_PATH=~/pico-pi/picotool
```

For step through debugging 'Edit configurations...' to then add '+' and pick 'Open OCD Download & Run', select your
target 'ble_smoke_detector', Executable binary should be 'ble_smoke_detector' or 'ble_smoke_detector.elf' you may need to
select this directly from the cmake-build-debug directory, bundled GDB is fine, Board config file should be a new file
you'll probably need to create in your local copy of the raspberry pi openocd 'openocd/tcl/board/pico.cfg'. I set
Download to 'Always', the rest of the defaults were fine.

I created a new openocd/tcl/board/pico.cfg containing:

```
# SPDX-License-Identifier: GPL-2.0-or-later
# Attempt to make clion happy

source [find interface/cmsis-dap.cfg]
adapter speed 5000

set CHIPNAME rp2040
source [find target/rp2040.cfg]
```

I also had to set the openocd to my locally built version, 'Settings' -> 'Build, Execution, Deployment' -> 'Embedded
Development' -> Open OCD location: '/usr/local/bin/openocd'. Though it populated itself with that value once CLion
noticed it was missing.

Debugging
---------

When running attached to a linux computer via the USB port you can view what is going on with:

```
sudo minicom -D /dev/ttyACM0 -b 115200
```

or

```
screen -L /dev/ttyACM0 115200
```

Bluetooth debugging
-------------------

Useful tools for debugging BTLE things from a linux desktop:

Interactive tool:

```
bluetoothctl 
```

Commands such as 'scan on', 'connect XX:XX:XX:XX:XX:XX' can be useful to report info about a specific device. For
example both my android devices merrily report their names, even if their ble is using a random address.

Scan for BTLE devices:

```
sudo hcitool lescan
```

Interactively talk to BTLE device:

```
gatttool -b  XX:...:XX -I
[XX:...:XX][LE]> connect
[XX:...:XX][LE]> primary
[XX:...:XX][LE]> char-desc
```

* primary - lists UIDs supported
* char-desc lists handles and UIDs

