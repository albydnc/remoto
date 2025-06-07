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
    "deviceId": "OPTA_WIFI",
    "mqttConnected": False,
    "NTP": 1749162624,
    "lastPublish": 67,
    "inputs": {
        "I1": {"value": 0, "type": True},
        "I2": {"value": 0, "type": True},
        "I3": {"value": 0, "type": True},
        "I4": {"value": 0, "type": True},
        "I5": {"value": 0, "type": True},
        "I6": {"value": 0, "type": True},
        "I7": {"value": 0.005882519, "type": False},
        "I8": {"value": 0, "type": False},
    },
    "outputs": {"O1": 0, "O2": 0, "O3": 0, "O4": 0},
    "expansions": {
        "E1": {
            "type": "D1608E",
            "inputs": {
                "I1": {"value": False, "volt": 0},
                "I2": {"value": False, "volt": 0},
                "I3": {"value": False, "volt": 0},
                "I4": {"value": False, "volt": 0},
                "I5": {"value": False, "volt": 0},
                "I6": {"value": False, "volt": 0},
                "I7": {"value": False, "volt": 0},
                "I8": {"value": False, "volt": 0},
                "I9": {"value": False, "volt": 0},
                "I10": {"value": False, "volt": 0},
                "I11": {"value": False, "volt": 0},
                "I12": {"value": False, "volt": 0},
                "I13": {"value": False, "volt": 0},
                "I14": {"value": False, "volt": 0},
                "I15": {"value": False, "volt": 0},
                "I16": {"value": False, "volt": 0},
            },
            "outputs": {
                "O1": 0,
                "O2": 0,
                "O3": 0,
                "O4": 0,
                "O5": 0,
                "O6": 0,
                "O7": 0,
                "O8": 0,
            },
        },
        "E2": {
            "type": "D1608S",
            "inputs": {
                "I1": {"value": False, "volt": 0},
                "I2": {"value": False, "volt": 0},
                "I3": {"value": False, "volt": 0},
                "I4": {"value": False, "volt": 0},
                "I5": {"value": False, "volt": 0},
                "I6": {"value": False, "volt": 0},
                "I7": {"value": False, "volt": 0},
                "I8": {"value": False, "volt": 0},
                "I9": {"value": False, "volt": 0},
                "I10": {"value": False, "volt": 0},
                "I11": {"value": False, "volt": 0},
                "I12": {"value": False, "volt": 0},
                "I13": {"value": False, "volt": 0},
                "I14": {"value": False, "volt": 0},
                "I15": {"value": False, "volt": 0},
                "I16": {"value": False, "volt": 0},
            },
            "outputs": {
                "O1": 0,
                "O2": 0,
                "O3": 0,
                "O4": 0,
                "O5": 0,
                "O6": 0,
                "O7": 0,
                "O8": 0,
            },
        },
    },
}


# Default configuration for the IoT device
config = {
    "deviceId": "OPTA_WIFI",  # Device ID
    "deviceIpAddress": "192.168.1.231",  # IP Address
    "dhcp": True,  # Indicates if DHCP is enabled
    "preferWifi": True,  # Indicates if the WiFi connection is prefered
    "ssid": "MYSSID",  # Example SSID
    "wifiPass": "SSID Password",  # Example SSID Password
    "timeServer": "TimeServer",
    "mqtt": {  # MQTT broker configuration
        "server": "public.cloud.shiftr.io",
        "port": 1883,
        "user": "public",
        "password": "public",
        "updateInterval": 300,  # Telemetry update interval in seconds
    },
    "inputs": {  # Pin configurations for the inputs
        "I1": 1,
        "I2": 1,
        "I3": 1,
        "I4": 1,
        "I5": 1,
        "I6": 1,
        "I7": 0,
        "I8": 0,
    },
}

# Flask app initialization with a static folder for serving web pages
api = Flask(__name__, static_folder="web/")


# Serve the root HTML page
@api.route("/", methods=["GET"])
def get_root():
    return api.send_static_file("root.html")


# Serve the device configuration HTML page
@api.route("/device", methods=["GET"])
def get_device():
    return api.send_static_file("config.html")


# Endpoint for real-time data retrieval
@api.route("/data", methods=["GET"])
def get_realtime():
    return jsonify(data)


# Endpoint for retrieving or updating device configuration
@api.route("/config", methods=["GET", "POST"])
def config_endpoint():
    global config
    if request.method == "GET":  # Return the current configuration
        return jsonify(config)
    elif request.method == "POST":  # Update the configuration with the provided JSON
        print(request.json)  # Log the received configuration
        config = request.json
        return jsonify({"message": "Configuration updated successfully!"})


# Endpoint for retrieving or updating device configuration
@api.route("/output", methods=["POST"])
def out_endpoint():
    if request.method == "POST":  #
        print(request.json)  #
        return jsonify({"status": "success"})


# Simulated endpoint for MQTT publishing
@api.route("/send", methods=["GET"])
def get_send():
    return jsonify({"message": "MQTT published successfully!"})


# Start the Flask server
if __name__ == "__main__":
    api.run()
