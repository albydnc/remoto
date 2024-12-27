# Remoto: Arduino OPTA IoT Firmware with MQTT and Web Server

This repository contains firmware for an Arduino OPTA IoT device with Ethernet and MQTT support. The firmware enables dynamic configuration, telemetry, and control through MQTT endpoints and a web server.

---

## Features

- **Ethernet-based networking** with DHCP or static IP support.
- **MQTT integration** for telemetry publishing and device control.
- **Web server** for real-time monitoring and configuration updates.
- **JSON configuration management** stored in flash memory.
- **Scheduler** for periodic tasks such as telemetry updates and heartbeat signaling.

---

## MQTT Endpoints

The firmware defines specific MQTT topics for publishing telemetry data and subscribing to control commands. Below is a detailed breakdown of the MQTT endpoints.

### 1. **Telemetry Publishing**

The device periodically publishes telemetry data to the MQTT broker. The telemetry topics follow this structure:  
**`<deviceId>/<type>/<attribute>`**

#### Example:
If the device ID is `Device123`, telemetry for the first analog input would be published under:
- `Device123/I1/val` for the value.
- `Device123/I1/type` for the type (0 for analog, 1 for digital).

| **Topic**                 | **Description**                                   | **Data Type**                                            |
| ------------------------- | ------------------------------------------------- | -------------------------------------------------------- |
| `<deviceId>/deviceId`     | The unique identifier of the device.              | String                                                   |
| `<deviceId>/I<n>/val`     | Value of input pin `<n>` (analog or digital).     | two decimals float in volts (analog) / Integer (digital) |
| `<deviceId>/I<n>/type`    | Type of input pin `<n>`: 0 = analog, 1 = digital. | Integer                                                  |
| `<deviceId>/outputs/O<n>` | State of output pin `<n>`.                        | Integer (0 or 1)                                         |

### 2. **Control Commands**

The firmware allows control of output pins via MQTT commands. These commands are subscribed to dynamically during startup based on the number of output pins configured.

#### Command Topic:  
**`<deviceId>/O<n>`**

| **Topic**         | **Description**                     | **Payload**         |
| ----------------- | ----------------------------------- | ------------------- |
| `<deviceId>/O<n>` | Sets the state of output pin `<n>`. | `0` = OFF, `1` = ON |

#### Example:
For a device with ID `Device123`:
- To turn ON the first output pin, publish `1` to `Device123/O1`.
- To turn OFF the first output pin, publish `0` to `Device123/O1`.

### 3. **System Status**

The device publishes system-level information such as the MQTT connection status and time since the last telemetry publish.

| **Topic**                  | **Description**                             | **Data Type** |
| -------------------------- | ------------------------------------------- | ------------- |
| `<deviceId>/mqttConnected` | Connection status to the MQTT broker.       | Boolean       |
| `<deviceId>/lastPublish`   | Time (in seconds) since the last telemetry. | Integer       |

---

## Configuration

The device's configuration is stored in flash memory as a JSON object. This configuration can be updated dynamically via the web server or MQTT.

Key parameters:
- **Device ID**: Unique identifier for MQTT topics.
- **Network Settings**: DHCP or static IP.
- **MQTT Settings**: Server address, port, username, and password.
- **Pins**: Types and mappings of input pins.

---

## Getting Started

1. **Flash the Firmware**  
   Upload the provided firmware to your Arduino OPTA device.

2. **Configure the Device**  
   Access the built-in web server at `http://<device-ip>` to configure the network and MQTT settings.

3. **Connect to MQTT Broker**  
   Verify that the device connects to the specified MQTT broker.

4. **Subscribe to Topics**  
   Use an MQTT client (e.g., MQTT Explorer or Mosquitto) to monitor telemetry topics and send control commands.

---

## Troubleshooting
Check the verbose output on the Serial port.
- **Red LED**: If blinking slow, no ethernet hardware is detected. If blinking fast, no link is detected. if static, Network connection is down.
- **Ethernet Connection Issues**: Check the cable and ensure the correct network configuration (DHCP/static IP).
- **MQTT Not Connecting**: Verify broker address, port, and credentials.
- **No Telemetry**: Ensure inputs are configured correctly and the device is running.

---

## License

CERN-OHL-P
Copyright Alberto Perro - 2024

For questions or contributions, feel free to create an issue or submit a pull request.
