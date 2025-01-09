/*
 * Remoto: IoT Device Firmware for Arduino OPTA with Ethernet and MQTT Support
 * -------------------------------------------------------------------
 * This firmware is designed to run on the Arduino OPTA platform. It provides
 * a robust framework for network connectivity, MQTT communication,
 * and web server functionality. The code allows for dynamic configuration
 * via a built-in web interface and supports telemetry data publishing to
 * an MQTT broker. It includes:
 * - Ethernet-based networking.
 * - WiFi Networking.
 * - MQTT client for telemetry and control.
 * - JSON-based configuration stored in flash memory.
 * - Web server for monitoring and configuration.
 * - Scheduler for periodic tasks.
 *
 * Author: Alberto Perro
 * Date: 27-12-2024
 * License: CERN-OHL-P
 */

#if !defined(WEBPAGE_H)
#define WEBPAGE_H
#include <Arduino.h>
#include <Ethernet.h>

namespace remoto {

const char rootHtml[] PROGMEM = R"rawliteral(
   <!DOCTYPE html>
<html>

<head>
  <title>OPTA WiFi Input Status</title>
  <style>
    /* General styling */
    body {
      font-family: Arial, sans-serif;
      margin: 0;
      padding: 0;
      background-color: #f5f5f5;
      color: #333;
    }

    /* Box sizing for all elements to include padding and border in width calculation */
    * {
      box-sizing: border-box;
    }


    /* Header styling */
     h1 {
      display: flex;
      justify-content: center;
      align-items: center;
      position: relative;
      background-color: #6200ea;
      color: #fff;
      margin: 0;
      padding: 20px;
      font-size: 1.8rem;
    }

    /* Status box styling */
    .status,
    ul {
      margin: 20px auto;
      padding: 20px;
      max-width: 600px;
      background: #fff;
      border-radius: 8px;
      box-shadow: 0 2px 4px rgba(0, 0, 0, 0.2);
    }

    .status h2,
    ul h2 {
      margin: 0 0 10px;
      color: #6200ea;
      font-size: 1.4rem;
    }

    /* LED styling with flexbox */
    .status p {
      display: flex;
      align-items: center;
      margin: 5px 0;
    }

    .led {
      width: 20px;
      height: 20px;
      border-radius: 50%;
      margin-left: 10px;
      margin-right: 10px;
      flex-shrink: 0;
    }

    .high {
      background-color: #4caf50;
    }

    .low {
      background-color: #f44336;
    }

    /* List styling */
    ul {
      list-style: none;
      font-weight: bold;
      padding: 5px;
      margin-top: 10px;
      margin-bottom: 10px;
    }

    ul li {
      display: flex;
      align-items: center;
      margin-left: 10px;
      margin-top: 10px;
      margin-bottom: 10px;

    }

    ul li span {
      margin-left: 10px;
      margin-right: 10px;
    }

    .button {
      width: 100%;
      padding: 10px;
      font-size: 16px;
      color: #fff;
      background-color: #007bff;
      border: none;
      border-radius: 4px;
      cursor: pointer;
      text-align: center;
      margin-bottom: 10px;
      /* Ensure space between buttons */
      display: block;
      margin-top: 10px;
      text-decoration: none;
    }

    .button:hover {
      background-color: #0056b3;
    }

    .config {
      max-width: 600px;
      margin: 0 auto;
      padding: 20px;
      background: #fff;
      border-radius: 8px;
      box-shadow: 0 2px 4px rgba(0, 0, 0, 0.2);
    }
  </style>
  <script>
    async function updateStatus() {
      try {
        const response = await fetch('/data');
        const data = await response.json();
        console.log(data.deviceId);
        const epochTime = data.NTP;
        if (epochTime) {
        const date = new Date(epochTime * 1000);
        const formattedTime = date.toLocaleTimeString('en-GB', { hour: '2-digit', minute: '2-digit', second: '2-digit' });
        const formattedDate = date.toLocaleDateString('en-GB');
        document.getElementById('dateTime').innerText = `${formattedTime} ${formattedDate}`;
    }
        console.log(epochTime);
        document.getElementById('deviceId').innerText = data.deviceId;
        document.title = data.deviceId + " Device Status";

        // Update MQTT connection status
        document.getElementById('mqttStatus').className = data.mqttConnected ? 'led high' : 'led low';
        document.getElementById('mqttText').innerText = data.mqttConnected ? 'Connected' : 'Disconnected';

        // Update last publish time
        const lastPublish = data.lastPublish;
        document.getElementById('lastPublish').innerText = lastPublish >= 0 ? `${lastPublish} seconds ago` : 'No publish yet';

        // Update inputs
        const digitalList = document.getElementById('digitalInputs');
        const analogList = document.getElementById('analogInputs');
        digitalList.innerHTML = '';
        analogList.innerHTML = '';

        Object.keys(data.inputs).forEach(pin => {
          if (data.inputs[pin].type) { // is digital
            const li = document.createElement('li');
            const led = document.createElement('span');
            led.className = data.inputs[pin].value ? 'led high' : 'led low';
            li.appendChild(document.createTextNode(`${pin}`));
            li.appendChild(led);
            const val = data.inputs[pin].value ? "ON" : "OFF";
            li.appendChild(document.createTextNode(`${val}`));
            digitalList.appendChild(li);
          } else {
            const li = document.createElement('li');
            li.innerText = `${pin}: ${data.inputs[pin].value.toFixed(2)} V`;
            analogList.appendChild(li);
          }
        });

        if (digitalList.childElementCount === 0) {
        digitalList.parentElement.style.display = 'none';
        } else {
        digitalList.parentElement.style.display = 'block';
        }

        const outputList = document.getElementById('outputs');
        outputList.innerHTML = '';
        Object.keys(data.outputs).forEach(pin => {
            const li = document.createElement('li');
            const led = document.createElement('span');
            led.className = data.outputs[pin] ? 'led high' : 'led low';
            li.appendChild(document.createTextNode(`${pin}`));
            li.appendChild(led);
            const val = data.outputs[pin] ? "ON" : "OFF";
            li.appendChild(document.createTextNode(`${val}`));
            outputList.appendChild(li);
        });
        
        if (analogList.childElementCount === 0) {
        analogList.parentElement.style.display = 'none';
        } else {
        analogList.parentElement.style.display = 'block';
        }

      } catch (error) {
        console.error('Error updating status:', error);
      }
    }
    // Set interval to update the page every second
    setInterval(updateStatus, 1000);
    window.onload = updateStatus;
  </script>
</head>

<body>
  <h1> <span id="title"><span id="deviceId">Opta</span> Device Status</span>
    <span id="dateTime" class="datetime"></span>
  </h1>

  <div class="status">
    <h2>MQTT Connection:</h2>
    <p>
      <span>Status: <span id="mqttText">Disconnected</span></span>
      &nbsp;
      <span id="mqttStatus" class="led low"></span>
    </p>
    <p>Last Publish: &nbsp;<span id="lastPublish">No publish yet</span></p>
    <button class="button" id="publishNow">Publish Now</button>
  </div>
  <div class="status">
    <h2>Digital Inputs:</h2>
    <ul id="digitalInputs"></ul>
  </div>
  <div class="status">
    <h2>Analog Inputs:</h2>
    <ul id="analogInputs"></ul>
  </div>
  <div class="status">
    <h2>Outputs:</h2>
    <ul id="outputs"></ul>
  </div>
  <div class="config">
    <p>
      <a href="device" class="button"> Configure Device</a>
    </p>
  </div>
</body>
<script>
  // react on force publish
  const publishButton = document.getElementById("publishNow");
  publishButton.addEventListener('click', async (e) => {
    e.preventDefault();
    try {
      const response = await fetch('/send', {
        method: 'GET'
      });
      if (!response.ok) throw new Error('Failed to publish MQTT');
      alert('MQTT published successfully!');
      window.location.href = "/";
    } catch (error) {
      alert(`Error: ${error.message}`);
    }
  });

</script>

</html>
    )rawliteral";

const char configHtml[] PROGMEM = R"rawliteral(
    <!DOCTYPE html>
<html lang="en">

<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>Device Configuration</title>
  <style>
    /* Existing styles unchanged */
    /* General styling */
    body {
      font-family: Arial, sans-serif;
      margin: 0;
      padding: 0;
      background-color: #f5f5f5;
      color: #333;
    }

    * {
      box-sizing: border-box;
    }

    h1 {
      background-color: #6200ea;
      color: #fff;
      margin: 0;
      padding: 20px;
      text-align: center;
      font-size: 1.8rem;
    }

    .container {
      max-width: 600px;
      margin: 0 auto;
      padding: 20px;
      background: #fff;
      border-radius: 8px;
      box-shadow: 0 2px 4px rgba(0, 0, 0, 0.2);
    }

    label {
      display: block;
      margin-bottom: 5px;
      font-weight: bold;
    }

    input[type="text"],
    input[type="number"] {
      width: 100%;
      padding: 8px;
      margin-bottom: 15px;
      border: 1px solid #ccc;
      border-radius: 4px;
    }

    .inputs-container {
      margin-bottom: 15px;
    }

    .input-item {
      margin-bottom: 10px;
    }

    .input-item label {
      margin-right: 10px;
    }

    .button {
      width: 100%;
      padding: 10px;
      font-size: 16px;
      color: #fff;
      background-color: #007bff;
      border: none;
      border-radius: 4px;
      cursor: pointer;
      text-align: center;
      margin-bottom: 10px;
    }

    .button:hover {
      background-color: #0056b3;
    }

    .button-back {
      width: 100%;
      text-align: center;
      display: block;
      margin-top: 10px;
      text-decoration: none;
      background-color: #6c757d;
    }

    .button-back:hover {
      background-color: #5a6268;
    }

    .option-buttons {
      display: flex;
      justify-content: space-between;
      align-items: center;
    }

    .option-button {
      padding: 4px 10px;
      font-size: 12px;
      border: 1px solid #ccc;
      border-radius: 4px;
      cursor: pointer;
      background-color: #f9f9f9;
      margin-right: 10px;
    }

    .option-button.selected {
      background-color: #007bff;
      color: white;
    }

    .input-item {
      display: flex;
      align-items: center;
      margin-bottom: 10px;
    }

    .input-item label {
      flex: 1;
    }

    /* New style for the DHCP toggle container */
    .dhcp-toggle {
      margin-bottom: 15px;
    }
  </style>
</head>

<body>
  <h1>Device Configuration</h1>
  <div class="container">
    <form id="configForm">
      <label for="deviceId">Device ID:</label>
      <input type="text" id="deviceId" name="deviceId" required>

      <label for="deviceIpAddress">Device IP Address:</label>
      <input type="text" id="deviceIpAddress" name="deviceIpAddress" required>

      <!-- New DHCP toggle -->
      <div class="dhcp-toggle input-item">
        <label for="dhcp">DHCP:</label>
        <div class="option-buttons">
          <button type="button" class="option-button" data-input="dhcp" data-value="1">Enable</button>
          <button type="button" class="option-button selected" data-input="dhcp" data-value="0">Disable</button>
        </div>
      </div>

      <label for="mqttServer">MQTT Server:</label>
      <input type="text" id="mqttServer" name="mqttServer" required>

      <label for="mqttPort">MQTT Port:</label>
      <input type="text" id="mqttPort" name="mqttPort" required>

      <label for="mqttUser">MQTT User:</label>
      <input type="text" id="mqttUser" name="mqttUser" required>

      <label for="mqttPassword">MQTT Password:</label>
      <input type="text" id="mqttPassword" name="mqttPassword" required>

      <label for="updateInterval">Update Interval (seconds):</label>
      <input type="number" id="updateInterval" name="updateInterval" required>

      <div class="inputs-container">
        <label>Inputs:</label>
        <!-- Dynamically populated clickable labels for inputs -->
      </div>

      <button type="submit" class="button">Set Configuration</button>
    </form>

    <a href="/" class="button button-back">Back to Status</a>
  </div>

  <script>
    async function fetchConfigOnLoad() {
      const inputsContainer = document.querySelector('.inputs-container');
      try {
        const response = await fetch('/config', { method: 'GET' });
        if (!response.ok) throw new Error('Failed to fetch configuration');
        const data = await response.json();

        // Populate the form with fetched data
        document.getElementById('deviceId').value = data.deviceId;
        document.getElementById('deviceIpAddress').value = data.deviceIpAddress;

        // Set DHCP toggle state
        if (data.dhcp !== undefined) {
          const dhcpButtons = document.querySelectorAll('.dhcp-toggle .option-button');
          dhcpButtons.forEach(button => {
            const value = button.getAttribute('data-value');
            if ((value === '1' && data.dhcp) || (value === '0' && !data.dhcp)) {
              button.classList.add('selected');
            } else {
              button.classList.remove('selected');
            }
          });
        }

        document.getElementById('mqttServer').value = data.mqtt.server;
        document.getElementById('mqttPort').value = data.mqtt.port;
        document.getElementById('mqttUser').value = data.mqtt.user;
        document.getElementById('mqttPassword').value = data.mqtt.password;
        document.getElementById('updateInterval').value = data.mqtt.updateInterval;

        // Populate inputs as clickable labels
        inputsContainer.innerHTML = ''; // Clear existing inputs
        for (const input in data.inputs) {
          const isDigital = data.inputs[input];
          const inputItem = document.createElement('div');
          inputItem.className = 'input-item';

          const digitalButtonClass = isDigital ? 'selected' : '';
          const analogButtonClass = !isDigital ? 'selected' : '';

          inputItem.innerHTML = `
                        <label for="${input}">${input}:</label>
                        <div class="option-buttons">
                            <button type="button" class="option-button ${digitalButtonClass}" data-input="${input}" data-value="1">Digital</button>
                            <button type="button" class="option-button ${analogButtonClass}" data-input="${input}" data-value="0">Analog</button>
                        </div>
                    `;
          inputsContainer.appendChild(inputItem);
        }

        // Add click event listeners for all option buttons (including DHCP)
        document.querySelectorAll('.option-button').forEach(button => {
          button.addEventListener('click', function () {
            const input = button.getAttribute('data-input');
            const value = button.getAttribute('data-value');
            const otherButton = button.parentElement.querySelector(`.option-button[data-input="${input}"]:not([data-value="${value}"])`);

            button.classList.add('selected');
            if (otherButton) otherButton.classList.remove('selected');

          });
        });
      } catch (error) {
        console.error(`Error: ${error.message}`);
      }
    }

    const form = document.getElementById('configForm');
    form.addEventListener('submit', async (e) => {
      e.preventDefault();
      const formData = new FormData(form);

      // Prepare configuration object
      const config = {
        deviceId: formData.get('deviceId'),
        deviceIpAddress: formData.get('deviceIpAddress'),
        dhcp: false,
        mqtt: {
          server: formData.get('mqttServer'),
          port: formData.get('mqttPort'),
          user: formData.get('mqttUser'),
          password: formData.get('mqttPassword'),
          updateInterval: parseInt(formData.get('updateInterval'), 10)
        },
        inputs: {},
      };

      // Collect input configurations (selected buttons)
      const inputsButtons = document.querySelectorAll('.inputs-container .option-button.selected');
      inputsButtons.forEach(button => {
        const inputName = button.getAttribute('data-input');
        const inputValue = button.getAttribute('data-value');
        config.inputs[inputName] = inputValue === '1';
      });

      // Get DHCP state
      const dhcpButton = document.querySelector('.dhcp-toggle .option-button.selected');
      if (dhcpButton) {
        config.dhcp = dhcpButton.getAttribute('data-value') === '1';
      }

      try {
        const response = await fetch('/config', {
          method: 'POST',
          headers: { 'Content-Type': 'application/json' },
          body: JSON.stringify(config)
        });
        if (!response.ok) throw new Error('Failed to set configuration');
        alert('Configuration updated successfully! \n Restarting Device!');
        window.location.href = "/";
      } catch (error) {
        alert(`Error: ${error.message}`);
      }
    });

    // Call fetchConfigOnLoad when the page loads
    window.onload = fetchConfigOnLoad;
  </script>
</body>

</html>
)rawliteral";
}
#endif  // WEBPAGE_H
