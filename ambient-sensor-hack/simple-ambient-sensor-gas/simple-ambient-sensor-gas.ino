#include <WiFi.h>
#include <PubSubClient.h>

// Device name
String device_name = "ambient-sensor-simple-gas";

// WLAN settings
const char* ssid = "wireless";
const char* password = "Netzgesichert123";

// 0: only Web server
// 2: MQTT
// 3: Only serial output
int output = 0;

// MQTT settings
const char* mqtt_broker = "192.168.0.11";
const char* mqtt_username = "";
const char* mqtt_password = "";
int mqtt_port = 1883; // This is the default port for MQTT

// Pin for an additional LED
int led_pin = 2;
int gas_pin = 11;
int co2_pin = 10;

// Interval for sending MQTT messages in milliseconds
int interval = 60000;

// Interval to force the sending of MQTT messages
int force_interval = 300000;

// Default MQTT topics (not needed for Thingspeak)
String co2_topic =  device_name + "/co2";
String co2_alarm_topic =  device_name + "/co2/alarm";
String hall_topic =  device_name + "/hall";
String gas_topic =  device_name + "/gas";
String gas_alarm_topic =  device_name + "/gas/alarm";
String led_topic =  device_name + "/led";
String led_topic_set = device_name + "/led/set";

uint64_t chipid;

// Variables (no need to change)
double gas = 0;
double co2 = 0;
float hall = 0.0;
float diff_gas = 0.5;
float diff_co2 = 1.0;
float diff_hall = 2.0;
long lastMsg = 0;
long lastForceMsg = 0;
bool forceMsg = false;
float old_gas = 0.0;
float old_co2 = 0.0;
float old_hall = 0.0;

// Current WLAN status
int status = WL_IDLE_STATUS;

WiFiClient espClient;
PubSubClient mqtt_client(espClient);

WiFiServer server(80);

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

// Callback for the MQTT messages
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

void setup() {
  // Set up the Serial Monitor
  Serial.begin(115200);
  pinMode(led_pin, OUTPUT);
  delay(10);

  chipid = ESP.getEfuseMac();

  if (output != 3) {
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
  }

  // Set up the MQTT client
  if (output == 1 || output == 2) {
    mqtt_client.setServer(mqtt_broker, mqtt_port);
  }
}

void loop() {
  // Read values from sensor
  gas = analogRead(gas_pin);
  co2 = analogRead(co2_pin);
  hall = hallRead();

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
            client.println("<meta http-equiv=\"refresh\" content=\"10\">");
            client.println("</head><body style='font-family: Verdana,Geneva,sans-serif;'><center>");
            client.println("<p><b><a href='/'>Ambient Gas Sensor</a></b> | <a href='/api'>RESTful API</a></p>");
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
            client.println(String("<tr><td>Gas:</td><td>") + gas + "</td></tr>");
            client.println(String("<tr><td>CO2:</td><td>") + co2 + "</td></tr>");
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
            client.println("<p><b><a href='/'>Ambient Gas Sensor</a></b> | <a href='/api'>RESTful API</a></p>");
            client.println("<h1>RESTful API</h1><p>");
            client.println("<p>The RESTful API let you integrate your Measuring Box in your own home automation system or into third-party tools.</p>");
            client.println("<br />");

            // API end points
            client.println("<table><tr><th align='left'>End points</th><th align='left'>URL</th></tr>");
            client.println("<tr><td>All values:</td><td><a href=\"api/states\">api/states</a></td></tr>");
            client.println("<tr><td>Gas:</td><td><a href=\"api/gas\">api/gas</a></td></tr>");
            client.println("<tr><td>CO2:</td><td><a href=\"api/co2\">api/co2</a></td></tr>");
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
            client.println(String("\"gas\":") + gas + ",");
            client.println(String("\"co2\":") + co2 + ",");
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
          else if (method.equals("GET /api/gas HTTP/1.1")) {
            client.println("HTTP/1.1 200 OK");
            client.println("Content-Type: application/json");
            client.println();
            client.println(String("{\"value\":") + gas + "}");
            client.println();
            client.stop();
          }
          else if (method.equals("GET /api/co2 HTTP/1.1")) {
            client.println("HTTP/1.1 200 OK");
            client.println("Content-Type: application/json");
            client.println();
            client.println(String("{\"value\":") + co2 + "}");
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

    // Sending MQTT messages
    if (output == 2) {
      if (checkBound(gas, old_gas, diff_gas) || forceMsg) {
        old_gas = gas;
        mqtt_client.publish(gas_topic.c_str(), String(old_gas).c_str(), true);
      }
      if (checkBound(co2, old_co2, diff_co2) || forceMsg) {
        old_co2 = co2;
        mqtt_client.publish(co2_topic.c_str(), String(old_co2).c_str(), true);
      }
    }
    forceMsg = false;
  }
}
