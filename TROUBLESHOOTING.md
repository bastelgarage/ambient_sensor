# Bastelgarage.ch Ambient Sensor Troubleshooting

## Setup the IDE
For the setup of the Arduino IDE which is needed to program the ESP32 with the provided code, please check the following resources.

- [Windows](https://www.youtube.com/watch?v=TwOqgXSROHI&index=4&list=PLeW4WDHiYMZHtWh2dYQC2lGYQVVQXSan3)
- Linux: [Fedora](https://github.com/espressif/arduino-esp32/blob/master/docs/arduino-ide/fedora.md), [Debian/Ubuntu](https://github.com/espressif/arduino-esp32/blob/master/docs/arduino-ide/debian_ubuntu.md)
- [macOS](https://github.com/espressif/arduino-esp32/blob/master/docs/arduino-ide/mac.md)

## Soldering
Please check the [instruction video](https://www.youtube.com/watch?v=16id2UgzC2s&index=3&list=PLeW4WDHiYMZHbmb2ZuTdQ1KZCNdrLls8V) in German for the basics about soldering.

## BME280 Sensor
The BME280 is connected to the I2C bus. To check if the right pins are used, the [https://github.com/mcauser/i2cdetect](`i2cdetect`) library can help.

## Other options for the ESP32
There are different system and programming language for ESP32 available.

- [Micropython](https://micropython.org/)
- [MangooseOS](https://mongoose-os.com/)
- [Lua](https://github.com/Nicholas3388/LuaNode)
- [Zephyr](https://www.zephyrproject.org/)

## Using Bluetooth for sending values
The ESP32 has support for Bluetooth connections. The provided default code is limited to sending values over WiFi.

## Charging the battery
The battery (LiPo) can be charged over USB. A standard USB cable with a Micro-B plug is required. The status LED (red) is indicating if the battery is loading.

## Something is broken
In case a part is broken, please contact `info@bastelgarage.ch`.

## Different sensors
For other sensor or actuators, visit [Bastelgarage's online shop](https://www.bastelgarage.ch/). 

