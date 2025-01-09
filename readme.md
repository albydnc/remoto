# Remoto: Arduino OPTA IoT Firmware with MQTT and Web Server

This repository contains firmware for an Arduino OPTA IoT device equipped with Ethernet and MQTT support. The firmware facilitates dynamic configuration, telemetry reporting, and control through MQTT endpoints and a built-in web server.

---

## Features

- **Ethernet networking** with support for DHCP or static IP configuration.
- **WiFi networking** with support for DHCP or static IP configuration.
- **MQTT integration** for publishing telemetry data and executing control commands.
- **REST API** to get telemetry data and configure the device (and soon executing control commands).
- **Web server interface** for real-time monitoring and configuration management.
- **Persistent configuration storage** using JSON saved to flash memory.
- **Task scheduling** for periodic operations such as telemetry updates and heartbeats.

---

## MQTT Endpoints

The firmware employs structured MQTT topics to handle telemetry data publishing and device control. Below are the details of the supported MQTT endpoints.

### 1. **Telemetry Publishing**

Telemetry data is periodically published to the MQTT broker using topics formatted as:  
**`<deviceId>/<type>/<attribute>`**

#### Example:
For a device with ID `Device123`, telemetry for the first analog input is published under:
- `Device123/I1/val` for the input value.
- `Device123/I1/type` for the input type (0 = analog, 1 = digital).

| **Topic**              | **Description**                                   | **Data Type**                                                |
| ---------------------- | ------------------------------------------------- | ------------------------------------------------------------ |
| `<deviceId>/deviceId`  | The unique identifier of the device.              | String                                                       |
| `<deviceId>/I<n>/val`  | Value of input pin `<n>` (analog or digital).     | Float with 2 decimals (analog, in volts) / Integer (digital) |
| `<deviceId>/I<n>/type` | Type of input pin `<n>`: 0 = analog, 1 = digital. | Integer                                                      |
| `<deviceId>/O<n>`      | State of output pin `<n>`.                        | Integer (0 or 1)                                             |

### 2. **Control Commands**

Control commands are subscribed dynamically at startup, allowing control of output pins via MQTT.

#### Command Topic:  
**`<deviceId>/O<n>`**

| **Topic**         | **Description**                     | **Payload**         |
| ----------------- | ----------------------------------- | ------------------- |
| `<deviceId>/O<n>` | Sets the state of output pin `<n>`. | `0` = OFF, `1` = ON |

#### Example:
For a device with ID `Device123`:
- To turn ON the first output pin, publish `1` to `Device123/O1`.
- To turn OFF the first output pin, publish `0` to `Device123/O1`.

---

## REST Endpoints

The firmware offers HTTP endpoints handle telemetry data publishing and device configuration. Below are the details of the supported REST endpoints.

### 1. **Telemetry Data**

Telemetry data can be retrieved using an HTTP GET request at the URL :  
**`http://<deviceAddress>/data`**

The device will respond with a JSON formatted using this scheme:
```python
{
    "deviceId": "OPTA_WIFI",  # Unique ID for the device
    "mqttConnected": "true",  # Indicates MQTT connection status
    "lastPublish": 125,  # Time in seconds since the last telemetry publish
    "inputs": {  # Inputs with their current values and types (true is digital, false is analog)
        "I1": {"value": True, "type": True},
        "I2": {"value": True, "type": True},
        "I3": {"value": True, "type": True},
        "I4": {"value": True, "type": True},
        "I5": {"value": True, "type": True},
        "I6": {"value": True, "type": True},
        "I7": {"value": 0.05, "type": False}, 
        "I8": {"value": 6.5, "type": False},
    },
    "outputs": {  # Outputs with their current states
        "O1": True,
        "O2": True,
        "O3": False,
        "O4": True,
    }
}
```

### 2. **Configuration**

The device's configuration can be retrieved with an HTTP GET request at the URL:
**`http://<deviceAddress>/config`**

The device will rspond with a JSON formatted using this scheme:
```python
{
    "deviceId": "OPTA_WIFI",  # Device ID
    "deviceIpAddress": "192.168.1.231",  # IP Address
    "dhcp": True,  # Indicates if DHCP is enabled
    "mqtt": {  # MQTT broker configuration
        "server": "public.cloud.shiftr.io",
        "port": 1883,
        "user": "public",
        "password": "public",
        "updateInterval": 300  # Telemetry update interval in seconds
    },
    "inputs": {  # Pin configurations for the inputs (1 is digital, 0 is analog)
        "I1": 1,
        "I2": 1,
        "I3": 1,
        "I4": 1,
        "I5": 1,
        "I6": 1,
        "I7": 0,
        "I8": 0
    }
}
```
This configuration can be updated and sent to the device using a HTTP POST request to the same endpoint.
The device will respond with a JSON formatted string:
```json
{"status":"success","message":"Configuration updated"}
```

### 3. **Control Commands**

Control commands are not yet implemented for the REST API.

### 4. **MQTT control**
MQTT publishing can be forced by making an HTTP GET request to this endpoint:
**`http://<deviceAddress>/send`**

The device will respond with a JSON formatted string:
```json
{"status":"success","message":"MQTT forced send received."}
```

---

## Configuration

Device configuration is stored as a JSON object in flash memory and can be updated dynamically via the web server or MQTT.

Key configuration parameters:
- **Device ID**: Identifier for MQTT topics.
- **Network Settings**: DHCP or static IP configuration.
- **MQTT Settings**: Server address, port, username, and password.
- **Pins**: Type and mappings for input and output pins.

---

## Getting Started

1. **Flash the Firmware**  
   Upload the firmware to your Arduino OPTA device using the Arduino IDE or another compatible tool.

2. **Configure the Device**  
   Open the built-in web server at `http://<device-ip>` to set network and MQTT configurations.

3. **Connect to MQTT Broker**  
   Ensure the device connects to the specified MQTT broker.

4. **Monitor and Control**  
   Use an MQTT client (e.g., MQTT Explorer or Mosquitto) to subscribe to telemetry topics and send control commands.

---

## Troubleshooting

Check the Serial port output for detailed logs.

- **Red LED Behavior**: 
  - **Slow Blink**: No Ethernet hardware detected.
  - **Fast Blink**: No link detected.
  - **Static ON**: Network connection is down.
- **Blue LED Behavior**: 
  - **5s Blink**: Normal heartbeat.  
- **Ethernet Issues**: Verify the cable and network configuration (DHCP or static IP).
- **MQTT Connection Issues**: Confirm the broker address, port, and credentials.
- **No Telemetry Data**: Ensure proper input configuration and verify the device is active.

---

## License

**CERN-OHL-P**  
Copyright © Alberto Perro, 2025

For questions or contributions, please open an issue or submit a pull request.

## Contributors
[@Jersh](https://github.com/Jersh)