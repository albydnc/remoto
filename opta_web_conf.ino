/*
 * IoT Device Firmware for Arduino OPTA with Ethernet and MQTT Support
 * -------------------------------------------------------------------
 * This firmware is designed to run on the Arduino OPTA platform. It provides
 * a robust framework for network connectivity, MQTT communication,
 * and web server functionality. The code allows for dynamic configuration
 * via a built-in web interface and supports telemetry data publishing to
 * an MQTT broker. It includes:
 * - Ethernet-based networking.
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
// Network
#include <ArduinoJson.h>
#include <PortentaEthernet.h>
#include <Ethernet.h>
#include <SPI.h>
#include <MQTT.h>

// flash
#include "KVStore.h"
#include "kvstore_global_api.h"

#include "config.h"
#include "webpage.h"

using namespace remoto;

EthernetClient net;
EthernetServer server(80);
MQTTClient client;

config conf;
bool mqttConnected = false;
long lastPublish = -1;
bool forceMQTTSend = false;

void connectMQTT();
void loopHeartbeat();
void loopTele();
void getStringFromPOST();
void mqttReceived(String &topic, String &payload);
IPAddress parseIP(const String &ipaddr);

void setup()
{
  Serial.begin(115200);

  delay(5000);
  Serial.println("Arduino OPTA");
  Serial.println("-----------------------");
  // read config
  Serial.println("try to read config from flash");
  char readBuffer[1024];
  kv_get("config", readBuffer, 1024, 0);
  Serial.println(readBuffer);
  if (conf.loadFromJson(readBuffer, 1024) != 0)
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
  Serial.println("Configure Pins");
  conf.initializePins();
  Serial.println("Configure Network");
  // init boot led
  pinMode(LEDR, OUTPUT);
  digitalWrite(LEDR, HIGH);
  // init heartbeat led
  pinMode(LED_USER, OUTPUT);
  digitalWrite(LED_USER, LOW);

  // Initialize Ethernet
  int ret = 0;
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
      while (true)
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
      while (true)
      {
        digitalWrite(LEDR, HIGH);
        delay(200);
        digitalWrite(LEDR, LOW);
        delay(200);
      }
    }
  }

  delay(1000);
  Serial.println("Configure MQTT");
  client.begin(conf.getMqttServer().c_str(), conf.getMqttPort(), net);
  client.onMessage(mqttReceived);
  connectMQTT();

  Serial.print("Start WebServer on ");
  Serial.println(Ethernet.localIP());
  // Start web server
  server.begin();
  delay(1000);
  // Start Scheduler Loops
  Scheduler.startLoop(loopTele);
  Scheduler.startLoop(loopHeartbeat);
  Serial.println("Startup Completed.");
}

void loop()
{
  // Listen for incoming client requests
  EthernetClient client = server.available();
  if (client)
  {
    handleClient(client); // Handle the client
  }
}

// Telemetry Loop
void loopTele()
{
  if ((millis() / 1000) - lastPublish > conf.getMqttUpdateInterval() || lastPublish == -1 || forceMQTTSend == true)
  {
    String rootTopic = conf.getDeviceId() + "/";
    // Device Information
    client.publish(String(rootTopic + "deviceId").c_str(), conf.getDeviceId());
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

    Serial.println("MQTT published successfully.");
    forceMQTTSend = false;
    lastPublish = millis() / 1000;
  }

  client.loop();

  if (!client.connected())
  {
    digitalWrite(LEDR, HIGH);
    mqttConnected = false;
    connectMQTT();
  }
  else
  {
    digitalWrite(LEDR, LOW);
    mqttConnected = true;
  }
}

// MQTT Connection Handler
void connectMQTT()
{
  Serial.print("Connecting to MQTT broker...");
  while (!client.connect(conf.getDeviceId().c_str(), conf.getMqttUser().c_str(), conf.getMqttPassword().c_str()))
  {
    Serial.print(".");
  }
  Serial.println("\nConnected to MQTT broker!");
  mqttConnected = true;
  for (size_t i = 0; i < NUM_OUTPUTS; i++)
  {
    String topic = conf.getDeviceId() + "/O" + String(i + 1);
    client.subscribe(topic);
    Serial.println("Subcribed to " + topic);
  }
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
}
// blink to show it is alive
void loopHeartbeat()
{
  digitalWrite(LED_USER, HIGH);
  delay(100);
  digitalWrite(LED_USER, LOW);
  delay(4900);
}

// handle webserver calls
void handleClient(EthernetClient client)
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
    return;
  }
  else if (request.startsWith("GET /device"))
  {
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: text/html");
    client.println("Connection: close");
    client.println();

    // Read the HTML from program memory
    client.write(configHtml, strlen_P(configHtml));
    client.stop();
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
  client.write(rootHtml, strlen_P(rootHtml));
  client.stop();
}

// Create JSON Data
String getData()
{
  StaticJsonDocument<512> doc;
  doc["deviceId"] = conf.getDeviceId();
  // MQTT Connection Status
  doc["mqttConnected"] = mqttConnected;
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
  String jsonString;
  serializeJson(doc, jsonString);
  return jsonString;
}

String getStringFromPOST(EthernetClient client)
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