/*
 * IoT Device Firmware for Arduino OPTA with Ethernet and MQTT Support
 * -------------------------------------------------------------------
 * This firmware is designed to run on the Arduino OPTA platform. It provides
 * a robust framework for network connectivity, MQTT communication,
 * and web server functionality. The code allows for dynamic configuration
 * via a built-in web interface and supports telemetry data publishing to
 * an MQTT broker. It includes:
 * - Ethernet-based networking.
 * - WiFi Netowrking
 * - MQTT client for telemetry and control.
 * - JSON-based configuration stored in flash memory.
 * - Web server for monitoring and configuration.
 * - Scheduler for periodic tasks.
 *
 * Author: Alberto Perro
 * Date: 27-12-2024
 * License: CERN-OHL-P
 */

#include <Scheduler.h>
#include <OptaBlue.h>
#include <array>
// Network
#include <ArduinoJson.h>
#include <PortentaEthernet.h>
#include <Ethernet.h>
#include <SPI.h>
#include <MQTT.h>
// Wifi + NTP
#include <WiFi.h>
#include <NTPClient.h>
#include <TimeLib.h>

// flash
#include "KVStore.h"
#include "kvstore_global_api.h"

#include "config.h"
#include "webpage.h"

using namespace remoto;

EthernetClient net;
EthernetServer server(80);
MQTTClient client;
// Wifi
WiFiClient wnet;
WiFiUDP ntpUDP;
WiFiServer wserver(80);
// NTP
NTPClient timeClient(ntpUDP, DEFAULT_TIME_SERVER);
unsigned long timeString = 0;
unsigned long lastBeat = 0;
config conf;
bool mqttConnected = false;
long lastPublish = -1;
bool forceMQTTSend = false;

bool connectMQTT();
void loopHeartbeat();
void loopTele();
void loopExp();
String getStringFromPOST(Client &client);
void handleClient(Client &client);
void mqttReceived(String &topic, String &payload);
IPAddress parseIP(const String &ipaddr);
// Network
int connectWiFi();
int connectEthernet();
void setupNTP();
//
REDIRECT_STDOUT_TO(Serial);
// Expansions
typedef struct
{
  ExpansionType_t type = EXPANSION_DIGITAL_INVALID;
  float volt[16];
  bool in[16];
  bool out[8];
} exp_t;
// expansion array
std::array<exp_t, OPTA_CONTROLLER_MAX_EXPANSION_NUM> exps;

void setup()
{
  // Setup user button early
  pinMode(BTN_USER, INPUT);
  Serial.begin(115200);
  delay(5000);
  Serial.println("Arduino OPTA");
  Serial.println("-----------------------");
  // read config
  Serial.println("Try to read config from flash");
  char readBuffer[1024];
  kv_get("config", readBuffer, 1024, 0);
  Serial.println(readBuffer);
  // init heartbeat led
  pinMode(LED_USER, OUTPUT);

  // if we have a blank flash or the user button is being held then (re)load the config
  Serial.println("Hold the user button for a fresh config write.. waiting 5s..");
  digitalWrite(LED_USER, HIGH);
  delay(5000);

  if (conf.loadFromJson(readBuffer, 1024) != 0 || !digitalRead(BTN_USER))
  {
    kv_reset("/kv/");
    Serial.println("Warning: config not found, writing defaults");
    conf.loadDefaults();
    String def = conf.toJson();
    Serial.println(def);
    Serial.println(def.length());
    kv_set("config", def.c_str(), def.length(), 0);
    Serial.println("read back:");
    kv_get("config", readBuffer, 1024, 0);
    Serial.println(readBuffer);
    conf.loadFromJson(readBuffer, 1024);
  }
  // Turn the user LED back off
  digitalWrite(LED_USER, LOW);

  Serial.println("Configure Pins");
  conf.initializePins();
  Serial.println("Configure Network");
  // init boot led
  pinMode(LEDR, OUTPUT);
  digitalWrite(LEDR, HIGH);

  // Initialize Network
  int ret = 0;
  int wstatus = WL_IDLE_STATUS;
  if (conf.getWiFiPref())
  {
    Serial.println("Config set to prefer WiFi");
    wstatus = connectWiFi();
    if (wstatus != WL_CONNECTED)
    {
      Serial.println("WiFi Failed. Trying Ethernet");
      connectEthernet(); // will get stuck if does not connect
    }
  }
  else
  {
    Serial.println("Config set to prefer Ethernet");
    if (connectEthernet() == 0)
    {
      Serial.println("Ethernet Failed. Trying WiFi");
      wstatus = connectWiFi();
      if (wstatus != WL_CONNECTED)
      {
        Serial.println("ERROR: Cannot connect to WiFi. Halting.");
        while (true)
          ;
      }
    }
  }
  delay(1000);
  Serial.println("Configure MQTT");
  Serial.println("MQTT Server: " + conf.getMqttServer() + " Port: " + String(conf.getMqttPort()));
  if (wstatus == WL_CONNECTED)
  {
    Serial.println("Using WiFi");
    client.begin(conf.getMqttServer().c_str(), conf.getMqttPort(), wnet);
  }
  else
  {
    Serial.println("Using Ethernet");
    client.begin(conf.getMqttServer().c_str(), conf.getMqttPort(), net);
  }
  mqttConnected = connectMQTT();
  client.onMessage(mqttReceived);

  if (wstatus == WL_CONNECTED)
  {
    Serial.print("Start WebServer on Wifi using ");
    Serial.println(WiFi.localIP());
    // Start web server on WiFi
    wserver.begin();
    delay(1000);
  }
  else
  {
    // Start web server on WiFi
    Serial.print("Start WebServer on Ethernet using ");
    server.begin();
  }
  OptaController.begin();
  // Start Scheduler Loops
  Scheduler.startLoop(loopExp);
  Scheduler.startLoop(loopTele);
  Scheduler.startLoop(loopHeartbeat);
  Serial.println("Startup Completed.");
}

void loop()
{
  // Check if we are WiFi or ethernet
  if (WiFi.status() == WL_CONNECTED)
  {
    WiFiClient client = wserver.available();
    if (client)
    {
      handleClient(client); // Handle the client
    }
  }
  else if (Ethernet.linkStatus() == LinkON)
  {
    // Listen for incoming client requests on Ethernet
    EthernetClient client = server.available();
    if (client)
    {
      handleClient(client); // Handle the client
    }
  }
  // reconnect to WiFi if connection is lost
  // and it is the preferred network
  if (WiFi.status() != WL_CONNECTED && conf.getWiFiPref())
  {
    Serial.println("Trying to reconnect to WiFi");
    connectWiFi();
  }
  // NTP
  timeClient.update();
  timeString = timeClient.getEpochTime();
  // For the Scheduler
  yield();
}

// Telemetry Loop
void loopTele()
{
  if ((millis() / 1000) - lastPublish > conf.getMqttUpdateInterval() || lastPublish == -1 || forceMQTTSend == true)
  {
    // update the client state
    forceMQTTSend = false;
    lastPublish = millis() / 1000;
    String rootTopic = conf.getDeviceId() + "/";
    // Device Information
    client.publish(String(rootTopic + "deviceId").c_str(), conf.getDeviceId());
    Serial.println("SendMQTTDevInfo");
    // Inputs
    for (size_t i = 0; i < NUM_INPUTS; i++)
    {
      String inTopic = "I" + String(i + 1) + "/";
      if (conf.getInputType(i) == ANALOG)
      {
        float value = analogRead(conf.getInputPin(i)) * (3.249 / ((1 << ADC_BITS) - 1)) / 0.3034;
        char buffer[10];
        int ret = snprintf(buffer, sizeof(buffer), "%0.2f", value);
        client.publish(String(rootTopic + inTopic + "val").c_str(), buffer);
        client.publish(String(rootTopic + inTopic + "type").c_str(), "0");
      }
      else
      {
        client.publish(String(rootTopic + inTopic + "val").c_str(), String(digitalRead(conf.getInputPin(i))).c_str());
        client.publish(String(rootTopic + inTopic + "type").c_str(), "1");
      }
    }
    // Expansions
    for (size_t i = 0; i < OptaController.getExpansionNum(); i++)
    {
      String expTopic = "E" + String(i + 1) + "/type";
      String type = "";
      switch (exps.at(i).type)
      {
      case EXPANSION_OPTA_DIGITAL_MEC:
        type = "D1608E";
        break;
      case EXPANSION_OPTA_DIGITAL_STS:
        type = "D1608S";
        break;
      default:
        type = "UNSUPPORTED";
        break;
      }
      client.publish(String(rootTopic + expTopic).c_str(), type.c_str());

      for (int k = 0; k < OPTA_DIGITAL_IN_NUM; k++)
      {
        String inTopic = "E" + String(i + 1) + "/I" + String(k + 1) + "/";
        client.publish(String(rootTopic + inTopic + "val").c_str(), String(exps.at(i).in[k]).c_str());
        char buffer[10];
        int ret = snprintf(buffer, sizeof(buffer), "%0.2f", exps.at(i).volt[k]);
        client.publish(String(rootTopic + inTopic + "volt").c_str(), buffer);
      }
    }
    Serial.println("MQTT published successfully. " + String(lastPublish));
  }

  client.loop();
  if (!client.connected())
  {
    mqttConnected = connectMQTT();
  }
  digitalWrite(LEDR, !mqttConnected);
}

// loop expansions
void loopExp()
{
  OptaController.update();
  // read expansions status
  for (int i = 0; i < OPTA_CONTROLLER_MAX_EXPANSION_NUM; i++)
  {
    DigitalMechExpansion mechExp = OptaController.getExpansion(i);
    DigitalStSolidExpansion stsolidExp = OptaController.getExpansion(i);
    if (mechExp)
    {
      // read input data
      mechExp.updateDigitalInputs();
      mechExp.updateAnalogInputs();
      exps.at(i).type = mechExp.getType();
      for (int k = 0; k < OPTA_DIGITAL_IN_NUM; k++)
      {
        exps.at(i).in[k] = mechExp.digitalRead(k, false);
        exps.at(i).volt[k] = mechExp.pinVoltage(k, false);
      }
      // write outputs
      for (int k = 0; k < OPTA_DIGITAL_OUT_NUM; k++)
      {
        PinStatus st = exps.at(i).out[k] ? HIGH : LOW;
        mechExp.digitalWrite(k, st, false);
      }
      mechExp.updateDigitalOutputs();
    }
    else if (stsolidExp)
    {
      // read input data
      stsolidExp.updateDigitalInputs();
      stsolidExp.updateAnalogInputs();
      exps.at(i).type = stsolidExp.getType();
      for (int k = 0; k < OPTA_DIGITAL_IN_NUM; k++)
      {
        exps.at(i).in[k] = stsolidExp.digitalRead(k, false);
        exps.at(i).volt[k] = stsolidExp.pinVoltage(k, false);
      }
      // write outputs
      for (int k = 0; k < OPTA_DIGITAL_OUT_NUM; k++)
      {
        PinStatus st = exps.at(i).out[k] ? HIGH : LOW;
        stsolidExp.digitalWrite(k, st, false);
      }
      stsolidExp.updateDigitalOutputs();
    }
    else
    {
      exps.at(i).type = EXPANSION_NOT_VALID;
    }
  }
  yield();
}

// MQTT Connection Handler
bool connectMQTT()
{
  Serial.print("Connecting to MQTT broker...");
  bool ret = false;
  for (int i = 0; i < 10; i++)
  {
    ret = client.connect(conf.getDeviceId().c_str(), conf.getMqttUser().c_str(), conf.getMqttPassword().c_str());
    if (ret)
      break;
  }
  if (ret)
  {
    Serial.println("\nConnected to MQTT broker!");
    for (size_t i = 0; i < NUM_OUTPUTS; i++)
    {
      String topic = conf.getDeviceId() + "/O" + String(i + 1);
      client.subscribe(topic);
      Serial.println("Subcribed to " + topic);
    }
    // Expansions
    for (size_t i = 0; i < OptaController.getExpansionNum(); i++)
    {
      for (size_t k = 0; k < OPTA_DIGITAL_OUT_NUM; k++)
      {
        String topic = conf.getDeviceId() + "/E" + String(i + 1) + "/O" + String(k + 1);
        client.subscribe(topic);
        Serial.println("Subcribed to " + topic);
      }
    }
  }
  else
  {
    Serial.println("ERR: can't connect to MQTT broker");
  }

  return ret;
}

// mqtt subscribe callback
void mqttReceived(String &topic, String &payload)
{
  Serial.println("Received " + topic + ": " + payload);
  for (size_t i = 0; i < NUM_OUTPUTS; i++)
  {
    String match = conf.getDeviceId() + "/O" + String(i + 1);
    if (topic == match)
    {
      digitalWrite(conf.getOutputPin(i), payload.toInt());
      digitalWrite(conf.getOutputLed(i), payload.toInt());
      Serial.println("Setting output " + String(i + 1));
    }
  }
  // Expansions
  for (size_t i = 0; i < OptaController.getExpansionNum(); i++)
  {
    for (size_t k = 0; k < OPTA_DIGITAL_OUT_NUM; k++)
    {
      String match = conf.getDeviceId() + "/E" + String(i + 1) + "/O" + String(k + 1);
      if (topic == match)
      {
        exps.at(i).out[k] = payload.toInt();
        Serial.println("Setting output E" + String(i + 1) + " O" + String(k + 1));
      }
    }
  }
}
// blink to show it is alive
void loopHeartbeat()
{
  // Non blocking delay
  if (millis() - lastBeat > 4900)
  {
    digitalWrite(LED_USER, HIGH);
    delay(100);
    digitalWrite(LED_USER, LOW);
    lastBeat = millis();
  }
  yield();
}

// handle webserver call
void handleClient(Client &client)
{
  // Read client request
  String request = client.readStringUntil('\r');
  client.flush();

  // Serve JSON data for dynamic updates
  if (request.startsWith("GET /data"))
  {
    String json = getData();
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: application/json");
    client.println("Connection: close");
    client.println();
    client.println(json);
    client.stop();
    Serial.println("HTTP request GET data");
    return;
  }
  else if (request.startsWith("GET /config"))
  {
    String json = conf.toJson();
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: application/json");
    client.println("Connection: close");
    client.println();
    client.println(json);
    client.stop();
    Serial.println("HTTP request GET config");

    return;
  }
  else if (request.startsWith("POST /output"))
  {
    // Retrieve JSON data from the POST request
    String json = getStringFromPOST(client);
    // Respond to the client
    // Parse JSON
    StaticJsonDocument<200> doc;
    DeserializationError error = deserializeJson(doc, json);
    int pin = doc["pin"].as<int>();
    int exp = doc["exp"].as<int>();
    int state = doc["state"].as<int>();
    Serial.println("HTTP request POST output");
    if (error ||
        !doc.containsKey("state") ||
        !doc.containsKey("pin") ||
        !doc.containsKey("exp"))
    {
      client.println("HTTP/1.1 400 Bad Request");
      client.println("Content-Type: application/json");
      client.println("Connection: close");
      client.println();
      client.println("{\"status\":\"error\",\"message\":\"Invalid JSON\"}");
      client.stop();
      return;
    }

    if (exp == 0)
    {
      digitalWrite(conf.getOutputPin(pin - 1), state);
      digitalWrite(conf.getOutputLed(pin - 1), state);
    }
    else
    {
      exps.at(exp - 1).out[pin - 1] = state;
    }

    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: application/json");
    client.println("Connection: close");
    client.println();
    client.println("{\"status\":\"success\"}");
    client.stop();

    return;
  }
  else if (request.startsWith("GET /device"))
  {
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: text/html");
    client.println("Connection: close");
    client.println();

    // Read the HTML from program memory
    client.write(configHtml, strlen_P((const char *)configHtml));
    client.stop();
    Serial.println("HTTP request GET device");

    return;
  }
  else if (request.startsWith("GET /send"))
  {
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: application/json");
    client.println("Connection: close");
    client.println();
    client.println("{\"status\":\"success\",\"message\":\"MQTT forced send received.\"}");
    forceMQTTSend = true;
    client.stop();
    Serial.println("HTTP request GET send");
    return;
  }
  else if (request.startsWith("POST /config"))
  {
    // Retrieve JSON data from the POST request
    String json = getStringFromPOST(client);
    // Respond to the client
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: application/json");
    client.println("Connection: close");
    client.println();
    client.println("{\"status\":\"success\",\"message\":\"Configuration updated\"}");
    client.stop();
    Serial.println("HTTP request POST config");
    Serial.println("New Config Received: " + json);
    if (conf.loadFromJson(json.c_str(), json.length()) == 0)
    {
      kv_set("config", json.c_str(), json.length(), 0);
      Serial.println("Valid Configuration, rebooting.");
      NVIC_SystemReset();
    }
    return;
  }


  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/html");
  client.println("Connection: close");
  client.println();

  // Read the HTML from program memory
  client.write(rootHtml, strlen_P((const char *)rootHtml));
  client.stop();
}

// Create JSON Data
String getData()
{
  StaticJsonDocument<512> doc;
  doc["deviceId"] = conf.getDeviceId();
  // MQTT Connection Status
  doc["mqttConnected"] = mqttConnected;
  // NTP Time
  doc["NTP"] = timeString;
  // Last Publish Time
  if (lastPublish > 0)
  {
    doc["lastPublish"] = (millis() / 1000 - lastPublish);
  }
  else
  {
    doc["lastPublish"] = -1; // Indicate no publish yet
  }

  // Digital Inputs
  JsonObject inputsObject = doc.createNestedObject("inputs");
  for (int i = 0; i < NUM_INPUTS; i++)
  {
    String name = "I" + String(i + 1);
    if (conf.getInputType(i) == DIGITAL)
    {
      JsonObject obj = inputsObject.createNestedObject(name);
      obj["value"] = digitalRead(conf.getInputPin(i));
      obj["type"] = true;
    }
    else
    {
      JsonObject obj = inputsObject.createNestedObject(name);
      obj["value"] = analogRead(conf.getInputPin(i)) * (3.249 / ((1 << ADC_BITS) - 1)) / 0.3034;
      obj["type"] = false;
    }
  }
  JsonObject outputsObj = doc.createNestedObject("outputs");
  for (int i = 0; i < NUM_OUTPUTS; i++)
  {
    String name = "O" + String(i + 1);
    outputsObj[name] = digitalRead(conf.getOutputPin(i));
  }

  JsonObject expsObj = doc.createNestedObject("expansions");
  // Expansions
  for (size_t i = 0; i < OptaController.getExpansionNum(); i++)
  {
    String name = "E" + String(i + 1);
    JsonObject expObj = expsObj.createNestedObject(name);
    String type = "";
    switch (exps.at(i).type)
    {
    case EXPANSION_OPTA_DIGITAL_MEC:
      type = "D1608E";
      break;
    case EXPANSION_OPTA_DIGITAL_STS:
      type = "D1608S";
      break;
    default:
      type = "UNSUPPORTED";
      break;
    }
    expObj["type"] = type;
    JsonObject einObject = expObj.createNestedObject("inputs");
    // expansions inputs
    for (int k = 0; k < OPTA_DIGITAL_IN_NUM; k++)
    {
      String iname = "I" + String(k + 1);
      JsonObject obj = einObject.createNestedObject(iname);
      obj["value"] = exps.at(i).in[k];
      obj["volt"] = exps.at(i).volt[k];
    }
    JsonObject eoutObject = expObj.createNestedObject("outputs");
    // expansions outputs
    for (int k = 0; k < OPTA_DIGITAL_OUT_NUM; k++)
    {
      String oname = "O" + String(k + 1);
      eoutObject[oname] = exps.at(i).out[k];
    }
  }
  String jsonString;
  serializeJson(doc, jsonString);
  return jsonString;
}

String getStringFromPOST(Client &client)
{
  String json = "";
  bool headersEnded = false;

  while (client.available())
  {
    String line = client.readStringUntil('\n'); // Read line-by-line
    // Detect the end of headers (an empty line)
    if (line == "\r")
    {
      headersEnded = true; // Headers end here
      continue;
    }
    // If headers have ended, start collecting the body (JSON)
    if (headersEnded)
    {
      json += line; // Append body content to the json string
    }
  }
  return json; // Return trimmed JSON string
}

IPAddress parseIP(const String &ipaddr)
{
  uint8_t ip[4];
  sscanf(ipaddr.c_str(), "%u.%u.%u.%u", &ip[0], &ip[1], &ip[2], &ip[3]);
  IPAddress ret(ip[0], ip[1], ip[2], ip[3]);
  return ret;
}

int connectWiFi()
{
  int ret = WL_IDLE_STATUS;
  if (conf.getDHCP())
  {
    WiFi.begin(conf.getSSID().c_str(), conf.getWiFiPassword().c_str());
  }
  else
  {
    WiFi.config(parseIP(conf.getDeviceIpAddress()));
  }
  // try 10 times to connect to wifi
  for (int i = 0; i < 10; i++)
  {
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(conf.getSSID());
    // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
    ret = WiFi.begin(conf.getSSID().c_str(), conf.getWiFiPassword().c_str());
    // wait 3 seconds for connection:
    delay(3000);
    if (ret == WL_CONNECTED)
    {
      Serial.println("Connected to wifi");
      digitalWrite(LEDR, LOW);
      return ret;
    }
  }
  return ret;
}

int connectEthernet()
{
  int ret = 0;
  Serial.println("Starting Ethernet");
  // if one is found
  if (conf.getDHCP())
  {
    ret = Ethernet.begin();
  }
  else
  {
    ret = Ethernet.begin(parseIP(conf.getDeviceIpAddress()));
  }

  if (ret == 0)
  {
    Serial.println("Ethernet failed to connect.");

    if (Ethernet.hardwareStatus() == EthernetNoHardware)
    {
      Serial.println("Ethernet shield not found.");
      // if wifi is preferred, this is the last chance to connect
      while (conf.getWiFiPref())
      {
        digitalWrite(LEDR, HIGH);
        delay(1000);
        digitalWrite(LEDR, LOW);
        delay(1000);
      }
    }

    if (Ethernet.linkStatus() == LinkOFF)
    {
      Serial.println("Ethernet cable not connected.");
      // if wifi is preferred, this is the last chance to connect
      while (conf.getWiFiPref())
      {
        digitalWrite(LEDR, HIGH);
        delay(200);
        digitalWrite(LEDR, LOW);
        delay(200);
      }
    }
  }
  return ret;
}