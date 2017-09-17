#include <WiFi.h>
#include <BME280.h>
#include <PubSubClient.h>
#include <U8g2lib.h>
#include <RunningMedian.h>

// Links to external libaries:
// https://github.com/e-radionicacom/BME280-Arduino-Library
// https://github.com/RobTillaart/Arduino/tree/master/libraries/RunningMedian

// Device name
String device_name = "delux-ambient-sensor";

// WLAN settings
const char* ssid = "";
const char* password = "";

// 0: Only Web server
// 1: Thingspeak
// 2: MQTT
// 3: Only display
int output = 0;

// MQTT settings
// If you want to use Thingspeak set mqtt_broker to "mqtt.thingspeak.com"
//const char* mqtt_broker = "";
const char* mqtt_broker = "mqtt.thingspeak.com";
// Not needed for Thingspeak
const char* mqtt_username = "";
const char* mqtt_password = "";
int mqtt_port = 1883; // This is the default port for MQTT

// ThingSpeak settings
// Replace <YOUR-CHANNEL-ID> with your channel ID and <YOUR-CHANNEL-WRITEAPIKEY>
// with your write API key
const char* thingspeak_topic = "channels/<YOUR-CHANNEL-ID>/publish/<YOUR-CHANNEL-WRITEAPIKEY>";


// Pin for an additional LED
int led_pin = 2;

// Interval for sending MQTT messages in milliseconds
int interval = 60000;

// Interval to force the sending of MQTT messages
int force_interval = 300000;

// Interval for refresh display in milliseconds
int display_interval = 30000;


// Default MQTT topics (not needed for Thingspeak)
String humidity_topic = device_name + "/humidity";
String temperature_topic =  device_name + "/temperature";
String hall_topic =  device_name + "/hall";
String pressure_topic =  device_name + "/pressure";
String led_topic =  device_name + "/led";
String led_topic_set = device_name + "/led/set";

uint64_t chipid;

// Variables (no need to change)
float temperature = 0;
float humidity = 0;
float pressure = 0;
float hall = 0.0;
float diff_temperature = 0.5;
float diff_pressure = 1.0;
float diff_humidity = 3.0;
float diff_hall = 2.0;
long lastMsg = 0;
long lastForceMsg = 0;
long lastDisplay = 0;
bool forceMsg = false;
float old_temperature = 0.0;
float old_humidity = 0.0;
float old_pressure = 0.0;
float old_hall = 0.0;

// Settings and variables for touch functionality
int thresholdGlobal = 10;
int thresholdTouch0 = 0;
int thresholdTouch2 = 0;
int thresholdTouch3 = 0;
int thresholdTouch4 = 0;

int navigationDisplay = 1;

// Median for the Touch sensors
RunningMedian medianTouch0 = RunningMedian(5);
RunningMedian medianTouch2 = RunningMedian(5);
RunningMedian medianTouch3 = RunningMedian(5);
RunningMedian medianTouch4 = RunningMedian(5);

// Current WLAN status
int status = WL_IDLE_STATUS;

WiFiClient espClient;
PubSubClient mqtt_client(espClient);

WiFiServer server(80);
BME280 bme;

U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);

// Symbol temperatur for display
#define thermometer_width 26
#define thermometer_height 55
static const unsigned char thermometer_bits[] U8X8_PROGMEM = {
  0x00, 0x7c, 0x00, 0x00, 0x00, 0xff, 0x01, 0x00, 0x80, 0xff, 0x03, 0x00,
  0xc0, 0xff, 0x07, 0x00, 0xe0, 0xff, 0x0f, 0x00, 0xe0, 0x83, 0x0f, 0x00,
  0xf0, 0x01, 0x1f, 0x00, 0xf0, 0x01, 0x1f, 0x00, 0xf0, 0x01, 0x1f, 0x00,
  0xf0, 0x01, 0x1f, 0x00, 0xf0, 0x01, 0x1f, 0x00, 0xf0, 0x01, 0x1f, 0x00,
  0xf0, 0x01, 0x1f, 0x00, 0xf0, 0x01, 0x1f, 0x00, 0xf0, 0x01, 0x1f, 0x00,
  0xf0, 0x01, 0x1f, 0x00, 0xf0, 0x01, 0x1f, 0x00, 0xf0, 0x39, 0x1f, 0x00,
  0xf0, 0x39, 0x1f, 0x00, 0xf0, 0x39, 0x1f, 0x00, 0xf0, 0x39, 0x1f, 0x00,
  0xf0, 0x39, 0x1f, 0x00, 0xf0, 0x39, 0x1f, 0x00, 0xf0, 0x39, 0x1f, 0x00,
  0xf0, 0x39, 0x1f, 0x00, 0xf0, 0x39, 0x1f, 0x00, 0xf0, 0x39, 0x1f, 0x00,
  0xf0, 0x39, 0x1f, 0x00, 0xf0, 0x39, 0x1f, 0x00, 0xf0, 0x39, 0x1f, 0x00,
  0xf0, 0x39, 0x1f, 0x00, 0xf0, 0x39, 0x1f, 0x00, 0xf0, 0x39, 0x1f, 0x00,
  0xf8, 0x39, 0x3f, 0x00, 0xfc, 0x38, 0x7e, 0x00, 0x7e, 0x38, 0xfc, 0x00,
  0x3e, 0x38, 0xf8, 0x00, 0x1f, 0x7c, 0xf0, 0x01, 0x1f, 0xff, 0xf1, 0x01,
  0x0f, 0xff, 0xe1, 0x01, 0x8f, 0xff, 0xe3, 0x01, 0x8f, 0xff, 0xe3, 0x01,
  0x8f, 0xff, 0xe3, 0x01, 0x8f, 0xff, 0xe3, 0x01, 0x0f, 0xff, 0xe1, 0x01,
  0x1f, 0xff, 0xf1, 0x01, 0x1f, 0x7c, 0xf0, 0x01, 0x3e, 0x00, 0xf8, 0x00,
  0x7e, 0x00, 0xfc, 0x00, 0xfc, 0x01, 0x7f, 0x00, 0xf8, 0xff, 0x3f, 0x00,
  0xf0, 0xff, 0x1f, 0x00, 0xe0, 0xff, 0x0f, 0x00, 0x80, 0xff, 0x03, 0x00,
  0x00, 0xfe, 0x00, 0x00
};

// Drop symbol for display
#define humidity_width 35
#define humidity_height 40
static const unsigned char humidity_bits[] U8X8_PROGMEM = {
  0x00, 0x00, 0x00, 0x0c, 0x00, 0x00, 0x00, 0x00, 0x1e, 0x00, 0x00, 0x00,
  0x00, 0x1e, 0x00, 0x38, 0x00, 0x00, 0x1f, 0x00, 0x78, 0x00, 0x00, 0x3f,
  0x00, 0x78, 0x00, 0x00, 0x3f, 0x00, 0xfc, 0x00, 0x80, 0x3f, 0x00, 0xfc,
  0x00, 0x80, 0x7b, 0x00, 0xfc, 0x00, 0x80, 0x73, 0x00, 0xfe, 0x00, 0xc0,
  0x73, 0x00, 0xce, 0x01, 0xc0, 0xf3, 0x00, 0xcf, 0x01, 0xc0, 0xf1, 0x00,
  0xcf, 0x03, 0xe0, 0xe1, 0x00, 0xc7, 0x03, 0xe0, 0xe0, 0x01, 0x87, 0x03,
  0xe0, 0xc0, 0x01, 0xcf, 0x03, 0xf0, 0xc0, 0x03, 0xff, 0x03, 0x70, 0xc0,
  0x03, 0xfe, 0x01, 0x70, 0x80, 0x03, 0xfc, 0x60, 0x70, 0x80, 0x03, 0x30,
  0xf0, 0x70, 0x80, 0x03, 0x00, 0xf0, 0x70, 0xc0, 0x03, 0x00, 0xf0, 0xf1,
  0xc0, 0x03, 0x00, 0xf8, 0xe1, 0xf3, 0x01, 0x00, 0xf8, 0xc1, 0xff, 0x00,
  0x00, 0xf8, 0xc3, 0x7f, 0x00, 0x00, 0xfc, 0x03, 0x1e, 0x00, 0x00, 0xbc,
  0x03, 0x00, 0x00, 0x00, 0x9c, 0x07, 0x00, 0x00, 0x00, 0x1e, 0x07, 0x00,
  0x00, 0x00, 0x0e, 0x07, 0x00, 0x00, 0x00, 0x0f, 0x0f, 0x00, 0x00, 0x00,
  0x0f, 0x0e, 0x00, 0x00, 0x00, 0x07, 0x0e, 0x00, 0x00, 0x00, 0x07, 0x0e,
  0x00, 0x00, 0x00, 0x07, 0x0e, 0x00, 0x00, 0x00, 0x0f, 0x0f, 0x00, 0x00,
  0x00, 0x9f, 0x0f, 0x00, 0x00, 0x00, 0xfe, 0x07, 0x00, 0x00, 0x00, 0xfc,
  0x03, 0x00, 0x00, 0x00, 0xf0, 0x00, 0x00, 0x00
};


// IP to String
String toStringIp(IPAddress ip) {
  String res = "";
  for (int i = 0; i < 3; i++) {
    res += String((ip >> (8 * i)) & 0xFF) + ".";
  }
  res += String(((ip >> 8 * 3)) & 0xFF);
  return res;
}

// Check it a value changed more than the given value
bool checkBound(float newValue, float prevValue, float maxDiff) {
  return newValue < prevValue - maxDiff || newValue > prevValue + maxDiff;
}

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

// Callback for MQTT messages
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  if ((char)payload[0] == '1') {
    digitalWrite(led_pin, LOW);
  } else {
    digitalWrite(led_pin, HIGH);
  }
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

// Show the temperature
void anzeigeTemperatur() {
  navigationDisplay = 1;
  u8g2.clearBuffer();
  u8g2.drawXBM(0 , 4, thermometer_width, thermometer_height,  thermometer_bits);
  u8g2.setFont(u8g2_font_helvB18_tf);
  u8g2.setCursor(45, 44);
  u8g2.print(temperature, 1);
  u8g2.print((char)176);
  u8g2.print("C");
  u8g2.sendBuffer();
}

// Show the humitidy
void anzeigeFeuchte() {
  navigationDisplay = 2;
  u8g2.clearBuffer();
  u8g2.drawXBM(0 , 12, humidity_width, humidity_height,  humidity_bits);
  u8g2.setFont(u8g2_font_helvB18_tf);
  u8g2.setCursor(45, 44);
  u8g2.print(humidity, 1);
  u8g2.print("%");
  u8g2.sendBuffer();

}

// Show the pressure
void anzeigeDruck() {
  navigationDisplay = 3;
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_helvB18_tf);
  u8g2.setCursor(8, 46);
  u8g2.print(pressure, 1);
  u8g2.print(" hPa");
  u8g2.sendBuffer();
}

// Show all
void anzeigeAll() {
  navigationDisplay = 4;
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_helvB18_tf);
  u8g2.setCursor(0, 19);
  u8g2.print(temperature, 1);
  u8g2.print(" ");
  u8g2.print((char)176);
  u8g2.print("C");
  u8g2.setCursor(0, 41);
  u8g2.print(humidity, 1);
  u8g2.print("  %");
  u8g2.setCursor(0, 64);
  u8g2.print(pressure, 1);
  u8g2.print(" hPa");
  u8g2.sendBuffer();
}

void setup() {
  // Set up the Serial Monitor
  Serial.begin(115200);
  pinMode(led_pin, OUTPUT);
  delay(10);

  chipid = ESP.getEfuseMac();

  bme.settings(0, 3, 0, 0, 3, 3, 3);
  while (bme.begin()) {
    Serial.println("Could not find BME280 sensor on the I2C bus");
    delay(1000);
  }

  u8g2.begin();                              // OLED Display Initialisieren
  u8g2.setDrawColor(1);                      // Definition for the color of XBM images
  u8g2.setBitmapMode(1);                     // Mode for XBM images

  // Median
  while (medianTouch0.getCount() < medianTouch0.getSize()) {
    medianTouch0.add(touchRead(T0));
    medianTouch2.add(touchRead(T2));
    medianTouch3.add(touchRead(T3));
    medianTouch4.add(touchRead(T4));
  }

  thresholdTouch0 = medianTouch0.getMedian() - thresholdGlobal;
  thresholdTouch2 = medianTouch2.getMedian() - thresholdGlobal;
  thresholdTouch3 = medianTouch3.getMedian() - thresholdGlobal;
  thresholdTouch4 = medianTouch4.getMedian() - thresholdGlobal;

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

  // Start HTTP server
  server.begin();
  Serial.println("Web server started");

  // Setup the MQTT client
  if (output == 1 || output == 2) {
    mqtt_client.setServer(mqtt_broker, mqtt_port);
  }
  // Callback is not needed for Thingspeak
  if (output == 2) {
    mqtt_client.setCallback(callback);
  }
}

void loop() {
  // Read values from sensor
  temperature = bme.readTemp();
  pressure = bme.readPressure() / 100.0F;
  humidity = bme.readHumidity();
  hall = hallRead();

  medianTouch0.add(touchRead(T0));
  medianTouch2.add(touchRead(T2));
  medianTouch3.add(touchRead(T3));
  medianTouch4.add(touchRead(T4));

  // Touch check
  if (medianTouch0.getMedian() < thresholdTouch0) {
    Serial.println("Touch0");
    if (navigationDisplay != 1) {
      anzeigeTemperatur();
    }
  } else if (medianTouch2.getMedian() < thresholdTouch2) {
    Serial.println("Touch2");
    if (navigationDisplay != 2) {
      anzeigeFeuchte();
    }
  } else if (medianTouch3.getMedian() < thresholdTouch3) {
    Serial.println("Touch3");
    if (navigationDisplay != 3) {
      anzeigeDruck();
    }
  } else if (medianTouch4.getMedian() < thresholdTouch4) {
    Serial.println("Touch4");
    if (navigationDisplay != 4) {
      anzeigeAll();
    }
  }

  if (millis() - lastDisplay > display_interval) {
    lastDisplay = millis();
    switch (navigationDisplay) {
      case 1:
        anzeigeTemperatur();
        break;
      case 2:
        anzeigeFeuchte();
        break;
      case 3:
        anzeigeDruck();
        break;
      case 4:
        anzeigeAll();
        break;
      default:
        break;
    }
  }

  if (output != 3) {
    WiFiClient client = server.available();   // Listen for incoming client requests
    if (client) {                             // If you get a client,
      while (client.connected()) {            // Loop while the client's connected
        if (client.available()) {             // If there's bytes to read from the client,
          String request = client.readString();  // Read client request
          Serial.println("-- Complete HTTP method -------------------------------");
          Serial.println(request);
          Serial.println("-- HTTP method -------------------------------");
          String method = splitString(request, '\r', 0);
          Serial.println(method);
          Serial.println("-- POST data -------------------------------");
          String post_data = splitString(request, '\r\n\n', 7);
          Serial.println(post_data);
          Serial.println();
          client.flush();

          if (method.equals("GET /favicon.ico HTTP/1.1")) {
            Serial.println("Get the favicon.ico");
          }

          else if (method.equals("GET / HTTP/1.1")) {
            Serial.println("Request for /");
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println();
            client.println("<!DOCTYPE HTML><html><head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
            client.println("</head><body style='font-family: Verdana,Geneva,sans-serif;'><center>");
            client.println("<p><b><a href='/'>Ambient Sensor</a></b> | <a href='/api'>RESTful API</a></p>");
            client.println("<h1>Overview</h1><p>");

            // Device details
            client.println("<table><tr><th align='left'>Device</th></tr>");
            client.println(String("<tr><td>Device name:</td><td>") + device_name + "</td></tr>");
            client.print("<tr><td>Chip ID:</td><td>");
            client.printf("%04X", (uint16_t)(chipid >> 32));
            client.printf("%08X\n", (uint32_t)chipid);
            client.println("</td></tr>");
            client.println("<tr><td>Restart:</td><td><a href=\"/api/device/reboot\">yes</a></td></tr>");
            //client.println(String("<tr><td>Uptime</td><td>") + timeClient.getFormattedTime() + "</td></tr>");
            client.println("</table>");
            client.println("<br />");

            // Sensor values
            client.println("<table><tr><th align='left'>Sensors</th></tr>");
            client.println(String("<tr><td>Temperature:</td><td>") + temperature + " C</td></tr>");
            client.println(String("<tr><td>Humidity:</td><td>") + humidity + " % RH</td></tr>");
            client.println(String("<tr><td>Pressure:</td><td>") + pressure + " hPa</td></tr>");
            client.println(String("<tr><td>Hall:</td><td>") + hall + "</td></tr>");
            //client.println(String("<tr><td>xxxx</td><td>xxx</td></tr>");
            client.println("</table>");
            client.println("<br />");

            // the content of the HTTP response follows the header:
            client.print(String("<b>LED</b> (Pin ") + led_pin + "):&nbsp;");
            if (digitalRead(led_pin) == HIGH) {
              client.print("on");
            } else {
              client.print("off");
            }
            client.println("&nbsp;(Turn <a href=\"/api/led/on\">on</a> or <a href=\"/api/led/off\">off</a>)");
            client.println("<br />");

            client.println("<hr>");
            client.println("<p>More sensors, actuators, and kits are available in the online shop of <a href='https://www.bastelgarage.ch'>Bastelgarage.ch</a>.</p>");
            client.println("</center></body></html>");
            // The HTTP response ends with another blank line:
            client.println();
            // break out of the while loop:
            client.stop();
          }
          // RESTful API section
          else if (method.equals("GET /api HTTP/1.1")) {
            Serial.println("Request for /api");
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println();
            client.println("<!DOCTYPE HTML><html><head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
            client.println("</head><body style='font-family: Verdana,Geneva,sans-serif;'><center>");
            client.println("<p><b><a href='/'>Ambient Sensor</a></b> | <a href='/api'>RESTful API</a></p>");
            client.println("<h1>RESTful API</h1><p>");
            client.println("<p>The RESTful API let you integrate your Measuring Box in your own home automation system or into third-party tools.</p>");
            client.println("<br />");

            // API end points
            client.println("<table><tr><th align='left'>End points</th><th align='left'>URL</th></tr>");
            client.println("<tr><td>All values:</td><td><a href=\"api/states\">api/states</a></td></tr>");
            client.println("<tr><td>Temperature:</td><td><a href=\"api/temperature\">api/temperature</a></td></tr>");
            client.println("<tr><td>Humidity:</td><td><a href=\"api/humidity\">api/humidity</a></td></tr>");
            client.println("<tr><td>Pressure:</td><td><a href=\"api/pressure\">api/pressure</a></td></tr>");
            client.println("<tr><td>Hall:</td><td><a href=\"api/hall\">api/hall</a></td></tr>");
            client.println("<tr><td>LED:</td><td><a href=\"api/led\">api/led</a></td></tr>");
            // client.println("<tr><td>xxxx</td><td>xxx</td></tr>");
            client.println("</table>");
            client.println("<br />");
            client.println("<p>The states can be retrieved by HTTP GET requests. For the temperature the command-line tool <code>curl</code> could be used.</p>");
            client.println(String("<p>eg. <code>$ curl ") + toStringIp(WiFi.localIP()) + "/api/temperature</code></p>");
            client.println("<p>To change the state of the LED, issue a HTTP POST requests.");
            client.println("As browsers usualy don't support POST requests, GET requests are available to <code>/api/led/on</code> to switch the LED on and <code>api/led/off</code> to switch it off.</p>");

            client.println(String("<p>eg. <code>$ curl -d \"{'state': 1}\" ") + toStringIp(WiFi.localIP()) + "/api/led</code></p>");
            client.println("</center></body></html>");
          }

          else if (method.equals("GET /api/led HTTP/1.1")) {
            client.println("HTTP/1.1 200 OK");
            client.println("Content-Type: application/json");
            client.println();
            client.print("{\"state\": ");
            if (digitalRead(led_pin) == HIGH) {
              client.print(1);
            } else {
              client.print(0);
            }
            client.println("}");
            client.stop();
          }
          else if (method.equals("POST /api/led HTTP/1.1")) {
            char value = post_data.charAt(10);
            if (value == '1') {
              Serial.println("Switch on LED");
              digitalWrite(led_pin, HIGH);
            } else {
              Serial.println("Switch off LED");
              digitalWrite(led_pin, LOW);
            }
            client.println("HTTP/1.1 200 OK");
            client.println("Content-Type: application/json");
            client.println();
            client.print("{\"state\": ");
            if (digitalRead(led_pin) == HIGH) {
              client.print(1);
            } else {
              client.print(0);
            }
            client.println("}");
            client.stop();
          }
          // Brosers don't do POST requests
          else if (method.equals("GET /api/led/on HTTP/1.1")) {
            Serial.println("Switch on LED");
            digitalWrite(led_pin, HIGH);
            client.println("HTTP/1.1 200 OK");
            client.println("Content-Type: application/json");
            client.println();
            client.println("{\"state\": 1}");
            client.println();
            client.stop();
          }
          else if (method.equals("GET /api/led/off HTTP/1.1")) {
            Serial.println("Switch off LED");
            digitalWrite(led_pin, LOW);
            client.println("HTTP/1.1 200 OK");
            client.println("Content-Type: application/json");
            client.println();
            client.println("{\"state\": 0}");
            client.println();
            client.stop();
          }
          else if (method.equals("GET /api/states HTTP/1.1")) {
            client.println("HTTP/1.1 200 OK");
            client.println("Content-Type: application/json");
            client.println();

            client.print("{");
            client.println(String("\"device\":\"") + device_name + "\",");

            client.println("\"states\": {");
            client.println(String("\"temperature\":") + temperature + ",");
            client.println(String("\"pressure\":") + pressure + ",");
            client.println(String("\"humidity\":") + humidity + ",");
            client.println(String("\"hall\":") + hall + ",");
            client.println(String("\"led\":") + digitalRead(led_pin));
            client.print("}}");
            client.println();
            client.stop();
          }
          else if (method.equals("GET /api/hall HTTP/1.1")) {
            client.println("HTTP/1.1 200 OK");
            client.println("Content-Type: application/json");
            client.println();
            client.println(String("{\"value\":") + hallRead() + "}");
            client.println();
            client.stop();
          }
          else if (method.equals("GET /api/temperature HTTP/1.1")) {
            client.println("HTTP/1.1 200 OK");
            client.println("Content-Type: application/json");
            client.println();
            client.println(String("{\"value\":") + pressure + "}");
            client.println();
            client.stop();
          }
          else if (method.equals("GET /api/humidity HTTP/1.1")) {
            client.println("HTTP/1.1 200 OK");
            client.println("Content-Type: application/json");
            client.println();
            client.println(String("{\"value\":") + humidity + "}");
            client.println();
            client.stop();
          }
          else if (method.equals("GET /api/pressure HTTP/1.1")) {
            client.println("HTTP/1.1 200 OK");
            client.println("Content-Type: application/json");
            client.println();
            client.println(String("{\"value\":") + pressure + "}");
            client.println();
            client.stop();
          }
          // Browser don't do HTTP POST requests
          else if ((method.equals("POST /api/device/reboot HTTP/1.1")) or (method.equals("GET /api/device/reboot HTTP/1.1"))) {
            Serial.println("rebooting...");
            ESP.restart();
            client.println("HTTP/1.1 200 OK");
            client.println("Content-Type: application/json");
            client.println();
            client.println("{\"state\": \"rebooting\"}");
            client.stop();
          }
          else {
            client.println("HTTP/1.1 404 OK");
            client.println("Content-type:text/html");
            client.println();
            client.println("<!DOCTYPE HTML><html><head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
            client.println("</head><body style='font-family: Verdana,Geneva,sans-serif;'><center>");
            client.println("<p><b><a href='/'>Ambient sensor</a></b> | <a href='/api'>RESTful API</a></p>");
            client.println("<h1>Page not available</h1><p>");
            client.println("</center></body></html>");
            client.println();
            client.stop();
          }
          client.stop();
        }
      }
      // Close the connection
      client.stop();
      //Serial.println("Client disonnected");
    }
    // MQTT
    if (output == 1 || output == 2) {
      if (!mqtt_client.connected()) {
        reconnect();
      }
      mqtt_client.loop();
    }
    long now = millis();
    if (now - lastMsg > interval) {
      lastMsg = now;
      // MQTT broker could go away and come back at any timeso doing a forced publish to
      // make sure something shows up within the first 5 minutes after a reset
      if (now - lastForceMsg > force_interval) {
        lastForceMsg = now;
        forceMsg = true;
        Serial.println("Forcing publish every 5 minutes...");
      }

      // Sending data to Thingspeak
      if (output == 1) {
        String data = String("field1=" + String(temperature, 2) + "&field2=" + String(pressure, 2) + "&field3=" + String(humidity, 2));
        // Get the data string length
        int length = data.length();
        char msgBuffer[length + 1];
        // Convert data string to character buffer
        data.toCharArray(msgBuffer, length + 1);
        Serial.println(msgBuffer);
        mqtt_client.publish(thingspeak_topic, msgBuffer);
      }

      // Sending MQTT messages
      if (output == 2) {
        if (checkBound(temperature, old_temperature, diff_temperature) || forceMsg) {
          old_temperature = temperature;
          mqtt_client.publish(temperature_topic.c_str(), String(old_temperature).c_str(), true);
        }
        if (checkBound(humidity, old_humidity, diff_humidity) || forceMsg) {
          old_humidity = humidity;
          mqtt_client.publish(humidity_topic.c_str(), String(old_humidity).c_str(), true);
        }
        if (checkBound(pressure, old_pressure, diff_pressure) || forceMsg) {
          old_pressure = pressure;
          mqtt_client.publish(pressure_topic.c_str(), String(old_pressure).c_str(), true);
        }
      }
      forceMsg = false;
    }
  }
}

