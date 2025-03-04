"""
IoT Device API Server with Flask

This script creates a simple API server for managing and interacting with an IoT device
using Flask. The server provides endpoints for real-time data retrieval, device configuration, 
and MQTT publishing simulation. It also serves static web pages for device monitoring and configuration.

Features:
- Real-time telemetry data API.
- Device configuration API with support for GET and POST methods.
- Static web page serving for device information and configuration.
- Simulated MQTT publish endpoint.
"""


from flask import Flask, jsonify, request

# Mock telemetry data for the IoT device
data = {
    "deviceId": "REMOTO052",  # Unique ID for the device
    "mqttConnected": "true",  # Indicates MQTT connection status
    "lastPublish": 125,  # Time in seconds since the last telemetry publish
    "NTP": 1736370059,
    "inputs": {  # Inputs with their current values and types
        "I1": {"value": True, "type": True},
        "I2": {"value": True, "type": True},
        "I3": {"value": True, "type": True},
        "I4": {"value": True, "type": True},
        "I5": {"value": True, "type": True},
        "I6": {"value": True, "type": True},
        "I7": {"value": 0.05, "type": False},  # Analog input example
        "I8": {"value": 6.5, "type": False},   # Analog input example
    },
    "outputs": {  # Outputs with their current states
        "O1": True,
        "O2": True,
        "O3": False,
        "O4": True,
    }
}

# Default configuration for the IoT device
config = {
    "deviceId": "OPTA_WIFI",  # Device ID
    "deviceIpAddress": "192.168.1.231",  # IP Address
    "dhcp": True,  # Indicates if DHCP is enabled
    "preferWifi": True, # Indicates if the WiFi connection is prefered
    "ssid": "MYSSID", # Example SSID
    "wifiPass": "SSID Password", # Example SSID Password
    "timeServer": "TimeServer",
    "mqtt": {  # MQTT broker configuration
        "server": "public.cloud.shiftr.io",
        "port": 1883,
        "user": "public",
        "password": "public",
        "updateInterval": 300  # Telemetry update interval in seconds
    },
    "inputs": {  # Pin configurations for the inputs
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

# Flask app initialization with a static folder for serving web pages
api = Flask(__name__, static_folder="web/")

# Serve the root HTML page
@api.route('/', methods=['GET'])
def get_root():
    return api.send_static_file("root.html")

# Serve the device configuration HTML page
@api.route('/device', methods=['GET'])
def get_device():
    return api.send_static_file("config.html")

# Endpoint for real-time data retrieval
@api.route('/data', methods=['GET'])
def get_realtime():
    return jsonify(data)

# Endpoint for retrieving or updating device configuration
@api.route('/config', methods=['GET', 'POST'])
def config_endpoint():
    global config
    if request.method == 'GET':  # Return the current configuration
        return jsonify(config)
    elif request.method == 'POST':  # Update the configuration with the provided JSON
        print(request.json)  # Log the received configuration
        config = request.json
        return jsonify({"message": "Configuration updated successfully!"})

# Simulated endpoint for MQTT publishing
@api.route('/send', methods=['GET'])
def get_send():
    return jsonify({"message": "MQTT published successfully!"})

# Start the Flask server
if __name__ == '__main__':
    api.run()
