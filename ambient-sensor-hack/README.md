# Bastelgarage.ch Ambient Sensor Hacks

This directory contains simplifications or combinations of features from all Ambient Sensors.

## Outdoor Ambient Sensor with OLED
This simple sketch show how a DS18b20 can be used with an OLED display. No network connection, only the display is used for showing the temperature.


## Simple Ambient Sensor with BMP280
The BMP280 is a cheap I2C capable sensor that is reporting the temperature and the pressure. It requires to use a different library than for the BME 280.
 
## Simple Ambient Sensor with MQ-7 and MQ-2 (work-in-progress)
Both sensors are measuring the concentration of certain particles in the air. The MQ-7 is detecting CO2 and MQ-2 is looking for combustible gas and smoke. Both sensors requires 5 V and create heat. This means that you will get false results if a temperature sensor, like the BMP280 or the BME280, is placed in the same case. The examples shows how to use the analog and the digital output of the sensors. The threshold of the digital output can be adjusted with a little potentiometer.

