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

#if !defined(CONFIGS_H)
#define CONFIGS_H
#include "Arduino.h"
//-------------------- DEFAULTS ---------------------
#define DEFAULT_DEVICE_ID "OPTA_WIFI"
#define DEFAULT_MQTT_BROKER "public.cloud.shiftr.io"
#define DEFAULT_MQTT_PORT 1883
#define DEFAULT_MQTT_USER "public"
#define DEFAULT_MQTT_PASSWORD "public"

#define DEFAULT_TELEMETRY_INTERVAL 5 * 60U

#define DEFAULT_USE_DHCP true
#define DEFAULT_IP_ADDR "192.168.1.231"
#define ADC_BITS 16

//Wifi Secrets
#define DEFAULT_SSID "SSID"
#define DEFAULT_SSID_PASS "SSIDPASS"
#define DEFAULT_PREFER_WIFI true

//NTP
#define DEFAULT_TIME_SERVER "pool.ntp.org"

namespace remoto
{

    constexpr int NUM_INPUTS = 8;
    constexpr int NUM_OUTPUTS = 4;
    constexpr int DIGITAL = 1;
    constexpr int ANALOG = 0;
    class config
    {
    private:
        String _deviceId;
        bool _dhcp;
        bool _preferWifi;
        String _ipaddr;
        String _ssid;
        String _wifiPass;
        String _timeServer;

        struct MqttConfig
        {
            String server;
            String user;
            String password;
            unsigned int port;
            int updateInterval;
        } _mqtt;

        int _inputs[NUM_INPUTS][2]; // Array for input pins and types (DIGITAL or ANALOG)
        const int _outputs[NUM_OUTPUTS] = {D0, D1, D2, D3};
        const int _outputsLed[NUM_OUTPUTS] = {LED_D0, LED_D1, LED_D2, LED_D3};

    public:
        config();
        ~config() = default;

        // load default values
        void loadDefaults();
        // Getter and Setter for deviceId
        String getDeviceId() const;
        void setDeviceId(const String &id);

        // Getter and Setter for device Ip address
        String getDeviceIpAddress() const;
        void setDeviceIpAddress(const String &ip);

        // Getter and Setter for DHCP
        bool getDHCP() const;
        void setDHCP(const bool val);

        // Getter and Setter for MQTT server
        String getMqttServer() const;
        void setMqttServer(const String &server);

        // Getter and Setter for MQTT port
        int getMqttPort() const;
        void setMqttPort(const int port);

        // Getter and Setter for MQTT user
        String getMqttUser() const;
        void setMqttUser(const String &user);

        // Getter and Setter for MQTT password
        String getMqttPassword() const;
        void setMqttPassword(const String &password);

        // Getter and Setter for MQTT update interval
        int getMqttUpdateInterval() const;
        void setMqttUpdateInterval(int interval);
        
        // Getter and Setter for WiFi SSID
        String getSSID() const;
        void setSSID(const String &ssid);

        // Getter and Setter for WiFi password
        String getWiFiPassword() const;
        void setWiFiPassword(const String &password);

        // Getter and Setter for TimeServer Address
        String getTimeServer() const;
        void setTimeServer(const String &timeserver);

        // Getter and Setter for WifiPref
        bool getWiFiPref() const;
        void setWiFiPref(const bool val);

        // Getter and setter for input type (DIGITAL or ANALOG)
        int getInputType(int index) const;
        int setInputType(int index, int type);

        int getInputPin(int index) const;
        int getOutputPin(int index) const;
        int getOutputLed(int index) const;

        // Initialize pin modes
        void initializePins();

        // Function to load configuration from a JSON string
        int loadFromJson(const char *buffer, size_t length);

        // Function to convert configuration to a JSON string
        String toJson() const;
    };
} // namespace remoto

#endif // CONFIGS_H
