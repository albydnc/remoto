/*
 * IoT Device Firmware for Arduino OPTA with Ethernet and MQTT Support
 * -------------------------------------------------------------------
 * This firmware is designed to run on the Arduino OPTA platform. It provides
 * a robust framework for network connectivity, MQTT communication,
 * and web server functionality. The code allows for dynamic configuration
 * via a built-in web interface and supports telemetry data publishing to
 * an MQTT broker. It includes:
 * - Ethernet-based networking.
 * - WiFi Netowkring.
 * - MQTT client for telemetry and control.
 * - JSON-based configuration stored in flash memory.
 * - Web server for monitoring and configuration.
 * - Scheduler for periodic tasks.
 *
 * Author: Alberto Perro
 * Date: 27-12-2024
 * License: CERN-OHL-P
 */

#include "config.h"
#include <ArduinoJson.h> // Include ArduinoJson library
#include <Arduino.h>

namespace remoto
{
    config::config()
    {
        loadDefaults();
    }

    // Getter for deviceId
    String config::getDeviceId() const
    {
        return _deviceId;
    }

    // Setter for deviceId
    void config::setDeviceId(const String &id)
    {
        _deviceId = id;
    }

    // Getter for IP address
    String config::getDeviceIpAddress() const
    {
        return _ipaddr;
    }

    // Setter for IP address
    void config::setDeviceIpAddress(const String &ip)
    {
        _ipaddr = ip;
    }

    // Getter for dhcp
    bool config::getDHCP() const
    {
        return _dhcp;
    }

    // Setter for dhcp
    void config::setDHCP(const bool val)
    {
        _dhcp = val;
    }

    // Getter for dhcp
    bool config::getWiFiPref() const
    {
        return _preferWifi;
    }

    // Setter for preferwifi
    void config::setWiFiPref(const bool val)
    {
        _preferWifi = val;
    }

    // Getter for MQTT server
    String config::getMqttServer() const
    {
        return _mqtt.server;
    }

    // Setter for MQTT server
    void config::setMqttServer(const String &server)
    {
        _mqtt.server = server;
    }

    // Getter for MQTT Port
    int config::getMqttPort() const
    {
        return _mqtt.port;
    }

    // Setter for MQTT Port
    void config::setMqttPort(const int port)
    {
        _mqtt.port = port;
    }

    // Getter for MQTT user
    String config::getMqttUser() const
    {
        return _mqtt.user;
    }

    // Setter for MQTT user
    void config::setMqttUser(const String &user)
    {
        _mqtt.user = user;
    }

    // Getter for MQTT password
    String config::getMqttPassword() const
    {
        return _mqtt.password;
    }

    // Setter for MQTT password
    void config::setMqttPassword(const String &password)
    {
        _mqtt.password = password;
    }

    // Getter for MQTT update interval
    int config::getMqttUpdateInterval() const
    {
        return _mqtt.updateInterval;
    }

    // Setter for MQTT update interval
    void config::setMqttUpdateInterval(int interval)
    {
        _mqtt.updateInterval = interval;
    }
    
    // Getter for timeserver address
    String config::getTimeServer() const
    {
        return _timeServer;
    }

    // Setter for timeserver address
    void config::setTimeServer(const String &timeserver)
    {
        _timeServer = timeserver;
    }
        
    // Getter for ssid name
    String config::getSSID() const
    {
        return _ssid;
    }

    // Setter for ssid name
    void config::setSSID(const String &ssid)
    {
        _ssid = ssid;
    }

    // Getter for ssid password
    String config::getWiFiPassword() const
    {
        return _wifiPass;
    }

    // Setter for ssid password
    void config::setWiFiPassword(const String &password)
    {
        _wifiPass = password;
    }

    // Getter for input type (DIGITAL or ANALOG)
    int config::getInputType(int index) const
    {
        if (index >= 0 && index < NUM_INPUTS)
        {
            return _inputs[index][1]; // 0 for ANALOG, 1 for DIGITAL
        }
        return -1; // Invalid index
    }

    // Setter for input type (DIGITAL or ANALOG)
    int config::setInputType(int index, int type)
    {
        if (index >= 0 && index < NUM_INPUTS && (type == DIGITAL || type == ANALOG))
        {
            _inputs[index][1] = type; // 0 for ANALOG, 1 for DIGITAL
            return 0;
        }
        return -1; // Invalid index
    }
    int config::getInputPin(int index) const
    {
        if (index >= 0 && index < NUM_INPUTS)
        {
            return _inputs[index][0];
        }
        return -1; // Invalid index
    }

    int config::getOutputPin(int index) const
    {
        if (index >= 0 && index < NUM_OUTPUTS)
        {
            return _outputs[index];
        }
        return -1; // Invalid index
    }

    int config::getOutputLed(int index) const
    {
        if (index >= 0 && index < NUM_OUTPUTS)
        {
            return _outputsLed[index];
        }
        return -1; // Invalid index
    }

    // Initialize input pin modes
    void config::initializePins()
    {
        analogReadResolution(ADC_BITS);

        for (int i = 0; i < NUM_INPUTS; ++i)
        {
            if (_inputs[i][1] == DIGITAL)
            {
                pinMode(_inputs[i][0], INPUT); // Set pin to digital input
            }
        }
        for (int i = 0; i < NUM_OUTPUTS; ++i)
        {
            pinMode(_outputs[i], OUTPUT);
            pinMode(_outputsLed[i], OUTPUT);
            digitalWrite(_outputs[i], LOW);
            digitalWrite(_outputsLed[i], LOW);
        }
    }

    // Function to load configuration from a JSON string
    // Function to load configuration from a JSON buffer
    int config::loadFromJson(const char *buffer, size_t length)
    {
        DynamicJsonDocument doc(1024);
        DeserializationError error = deserializeJson(doc, buffer, length);

        // Check for deserialization errors
        if (error)
        {
            Serial.println("Failed to parse JSON");
            return -1;
        }

        // Check if all necessary keys are present
        if (!doc.containsKey("deviceId") ||
            !doc.containsKey("deviceIpAddress") ||
            !doc.containsKey("dhcp") ||
            !doc.containsKey("preferWifi") ||
            !doc.containsKey("ssid") ||
            !doc.containsKey("wifiPass") ||
            !doc.containsKey("timeServer") ||
            !doc["mqtt"].containsKey("server") ||
            !doc["mqtt"].containsKey("port") ||
            !doc["mqtt"].containsKey("user") ||
            !doc["mqtt"].containsKey("password") ||
            !doc["mqtt"].containsKey("updateInterval") ||
            !doc.containsKey("inputs"))
        {
            Serial.println("Missing required keys in JSON");
            return -1;
        }

        // Set values from JSON if all keys are valid
        _deviceId = doc["deviceId"].as<String>();
        _ipaddr = doc["deviceIpAddress"].as<String>();
        _dhcp = doc["dhcp"].as<bool>();
        _preferWifi = doc["preferWifi"].as<bool>();
        _ssid = doc["ssid"].as<String>();
        _ssid = doc["wifiPass"].as<String>();
        _ssid = doc["timeServer"].as<String>();
        _mqtt.server = doc["mqtt"]["server"].as<String>();
        _mqtt.port = doc["mqtt"]["port"].as<int>();
        _mqtt.user = doc["mqtt"]["user"].as<String>();
        _mqtt.password = doc["mqtt"]["password"].as<String>();
        _mqtt.updateInterval = doc["mqtt"]["updateInterval"].as<int>();

        // Load input pins and types
        for (int i = 0; i < NUM_INPUTS; ++i)
        {
            String pinName = "I" + String(i + 1);
            _inputs[i][1] = doc["inputs"][pinName].as<int>();
        }

        return 0; // Successfully loaded configuration
    }

    // Function to convert configuration to a JSON string
    String config::toJson() const
    {
        DynamicJsonDocument doc(2048);

        doc["deviceId"] = _deviceId;
        doc["deviceIpAddress"] = _ipaddr;
        doc["dhcp"] = _dhcp;
        doc["preferWifi"] = _preferWifi;
        doc["ssid"] = _ssid;
        doc["wifiPass"] = _wifiPass;
        doc["timeServer"] = _timeServer;
        doc["mqtt"]["server"] = _mqtt.server;
        doc["mqtt"]["port"] = _mqtt.port;
        doc["mqtt"]["user"] = _mqtt.user;
        doc["mqtt"]["password"] = _mqtt.password;
        doc["mqtt"]["updateInterval"] = _mqtt.updateInterval;

        for (int i = 0; i < NUM_INPUTS; ++i)
        {
            String pinName = "I" + String(i + 1);
            doc["inputs"][pinName] = _inputs[i][1];
        }

        String jsonString;
        serializeJson(doc, jsonString);
        return jsonString;
    }

    void config::loadDefaults()
    {
        _deviceId = DEFAULT_DEVICE_ID;
        _mqtt.server = DEFAULT_MQTT_BROKER;
        _mqtt.port = DEFAULT_MQTT_PORT;
        _mqtt.user = DEFAULT_MQTT_USER;
        _mqtt.password = DEFAULT_MQTT_PASSWORD;
        _mqtt.updateInterval = DEFAULT_TELEMETRY_INTERVAL;
        _dhcp = DEFAULT_USE_DHCP;
        _preferWifi = DEFAULT_PREFER_WIFI;
        _ipaddr = DEFAULT_IP_ADDR;
        _ssid = DEFAULT_SSID;
        _wifiPass = DEFAULT_SSID_PASS;
        _timeServer = DEFAULT_TIME_SERVER;
        // Default input pin configurations
        _inputs[0][0] = A0;
        _inputs[0][1] = DIGITAL;
        _inputs[1][0] = A1;
        _inputs[1][1] = DIGITAL;
        _inputs[2][0] = A2;
        _inputs[2][1] = DIGITAL;
        _inputs[3][0] = A3;
        _inputs[3][1] = DIGITAL;
        _inputs[4][0] = A4;
        _inputs[4][1] = DIGITAL;
        _inputs[5][0] = A5;
        _inputs[5][1] = DIGITAL;
        _inputs[6][0] = A6;
        _inputs[6][1] = ANALOG;
        _inputs[7][0] = A7;
        _inputs[7][1] = ANALOG;
    }
} // namespace remoto
