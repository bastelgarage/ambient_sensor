#include <WiFi.h>
#include <PubSubClient.h>
#include <esp_wifi.h>
#include <DallasTemperature.h>
#include "esp_deep_sleep.h"

// Device name
String device_name = "ambient-sensor-outdoor";

// WLAN settings
const char* ssid = "";
const char* password = "";


// 1: Thingspeak
// 2: MQTT
int output = 1;

// MQTT settings
// If you want to use Thingspeak set mqtt_broker to "mqtt.thingspeak.com"
//const char* mqtt_broker = "185.32.125.24";
const char* mqtt_broker = "mqtt.thingspeak.com";
// Not needed for Thingspeak
const char* mqtt_username = "";
const char* mqtt_password = "";
int mqtt_port = 1883; // This is the default port for MQTT

// ThingSpeak settings
// Replace <YOUR-CHANNEL-ID> with your channel ID and <YOUR-CHANNEL-WRITEAPIKEY>
// with your write API key
const char* thingspeak_topic = "channels/<YOUR-CHANNEL-ID>/publish/<YOUR-CHANNEL-WRITEAPIKEY>/";



// Time ESP32 will go to sleep (in minutes) also Interval for sending MQTT messages
#define TIME_TO_SLEEP  30


// Default MQTT topics (not needed for Thingspeak)
String humidity_topic = device_name + "/humidity";
String temperature_topic =  device_name + "/temperature";
String hall_topic =  device_name + "/hall";
String pressure_topic =  device_name + "/pressure";
String led_topic =  device_name + "/led";
String led_topic_set = device_name + "/led/set";


// Variables (no need to change)
float temperature = 0;

// Conversion factor for micro seconds to minutes
#define uS_TO_S_FACTOR 60000000


// Current WLAN status
int status = WL_IDLE_STATUS;

WiFiClient espClient;
PubSubClient mqtt_client(espClient);

// OneWire DS18B20 settings
#define ONE_WIRE_BUS 5
OneWire oneWire(ONE_WIRE_BUS);

DallasTemperature DS18B20Sensor(&oneWire);



// https://stackoverflow.com/questions/9072320/split-string-into-string-array
String splitString(String data, char separator, int index) {
  int found = 0;
  int strIndex[] = {0, -1};
  int maxIndex = data.length() - 1;

  for (int i = 0; i <= maxIndex && found <= index; i++) {
    if (data.charAt(i) == separator || i == maxIndex) {
      found++;
      strIndex[0] = strIndex[1] + 1;
      strIndex[1] = (i == maxIndex) ? i + 1 : i;
    }
  }
  return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
}

void reconnect() {
  // Loop until the device is reconnected
  while (!mqtt_client.connected()) {
    Serial.print("MQTT broker: ");
    Serial.println(mqtt_broker);
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect with a random Client ID
    if (mqtt_client.connect((device_name + random(1000)).c_str())) {
      Serial.println("connected");
      if (output == 2) {
        // Once connected, publish an announcement...
        mqtt_client.publish(device_name.c_str(), "connected");
        // Subscribe to topic for setting the state of the LED
        mqtt_client.subscribe(led_topic_set.c_str());
      }
    } else {
      Serial.print("failed, rc=");
      Serial.print(mqtt_client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void setup() {
  // Set up the Serial Monitor
  Serial.begin(115200);

  delay(10);

  // Deep sleep Time
  esp_deep_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);

  // Init DS18b20
  DS18B20Sensor.begin();
  DS18B20Sensor.requestTemperatures();

  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  // Setup the MQTT client
  if (output == 1 || output == 2) {
    mqtt_client.setServer(mqtt_broker, mqtt_port);
  }
}

void loop() {
  // Read values from sensor
  temperature = DS18B20Sensor.getTempCByIndex(0);
  if (temperature <= -100 || temperature >= 80) {
    ESP.restart();
  }

  // MQTT
  if (output == 1 || output == 2) {
    if (!mqtt_client.connected()) {
      reconnect();
    }
    mqtt_client.loop();
  }

  // Sending data to Thingspeak
  if (output == 1) {
    String data = String("field1=" + String(temperature, 2));
    // Get the data string length
    int length = data.length();
    char msgBuffer[length + 1];
    // Convert data string to character buffer
    data.toCharArray(msgBuffer, length + 1);
    Serial.println(msgBuffer);
    mqtt_client.publish(thingspeak_topic, msgBuffer);
    esp_wifi_stop();
    esp_deep_sleep_start();
  }

  // Sending MQTT messages
  if (output == 2) {
    mqtt_client.publish(temperature_topic.c_str(), String(temperature).c_str(), true);
    delay(50);
    esp_wifi_stop();
    esp_deep_sleep_start();
  }
}
