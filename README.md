# Bastelgarage.ch Ambient Sensor Kit

The Bastelgarage.ch Ambient Sensor is a kit which is available in three different flavors.

- Simple
- Outdoor
- Deluxe

All three type are based on a basic set of components: An ESP32 development board, cables, and a half-size breadboard. They are suitable for beginners and advance users. Children may need the assistance of a parent to complete all tasks. All kits contains cables which will allow one to include more sensors without soldering. The only soldering which is required is to mount the pins to the sensor and the ESP32 development board.

The kits have been available at [Maker Faire Zurich](https://www.makerfairezurich.ch/de/) and can be ordered in the online shop of [Bastelgarage.ch](https://bastelgarage.ch/).

## Simple Ambient Sensor Kit
The "Simple Ambient Sensor Kit" contains additionally a BME280 for measuring the temperature, the pressure, and the humidity, and a plywood box. The box can be put on a table or placed on a wall.

## Outdoor Ambient Sensor Kit
The "Outdoor Ambient Sensor Kit" is shipped with a water-proof box, a cable passage, a pressure equalization valve, and a sealed DS18B20 sensor for measuring the temperature. The battery should allow you to run this kit for a couple of months without re-charging. 

## Deluxe Ambient Sensor Kit
The "Deluxe Ambient Sensor Kit" is for indoor use only. The elegant plywood contains an OLED display that is showing the current conditions including temperature, pressure, and humidity with a BME280. The four button on the front allows one to navigate through the menu.

It is possible to attach more sensors to each type kit. The used controller has free analog, digital, and touch inputs pins. It would also be possible to include actuators like LEDs or relays as long they are operating on 3.3 V.

## Further details
The demo code (sketches) in this repository are showing various use cases for the kits.

### Web interface
The web interface shows you the current values of the sensor. This is useful if you don't want to allow communication of your sensor kit with a third-party application.

The "Outdoor Ambient Sensor" doesn't have a web interface.

### RESTful API
The [RESTful API](https://de.wikipedia.org/wiki/Representational_State_Transfer) is exposing the sensor values over HTTP. The following endpoints are available:

- All values: `/api/states`
- Temperature: `/api/temperature`
- Humidity: `/api/humidity`
- Pressure: `/api/pressure`
- Hall: `/api/hall`
- LED: `/api/led`

### ThingSpeak
To use [ThingSpeak](https://thingspeak.com/) you need to create an account. For further information read the [ThingSpeak's MQTT documentation](https://de.mathworks.com/help/thingspeak/channels-and-charts-api.html). Add your details to the sketch before uploading to the ESP32. The comment in the sketch should give you a hint about the creation of the right topic out of channel ID and API key.

### MQTT
Using [MQTT](https://en.wikipedia.org/wiki/MQTT) requires that you have a local, remote, or cloud-based MQTT broker. The MQTT settings need to be updated in the sketch before uploading.

- `ambient-box/humidity`
- `ambient-box/temperature`
- `ambient-box/pressure`

For the "Outdoor Ambient Sensor" only the temperature is supported as the Dallas DS18b20 is used.

