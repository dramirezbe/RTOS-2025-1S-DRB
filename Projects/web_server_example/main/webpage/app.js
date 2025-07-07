/**
 * @file app.js
 * @brief This file contains the JavaScript logic for the ESP32 Javastral web interface.
 * It handles fetching NTC sensor data and sending RGB LED values to the ESP32.
 */

/**
 * Global variables (currently not used in this simplified version, but kept for potential future use)
 */
var seconds = null;
var otaTimerVar = null;
var wifiConnectInterval = null;

/**
 * Initializes functions when the DOM is ready.
 */
$(document).ready(function() {
    // getUpdateStatus(); // Commented out: OTA functionality not currently used
    startDHTSensorInterval(); // Start fetching NTC sensor values
    
    // Activate listeners for RGB input fields to restrict values
    activate_listener('red_val');
    activate_listener('green_val');
    activate_listener('blue_val');
    
    // Attach click event handlers for the RGB and LED control buttons
    $("#send_rgb").on("click", function() {
        send_rgb_values();
    });
    $("#toogle_led").on("click", function() {
        toogle_led();
    });
    // $("#apagar_uart").on("click", function(){ // Commented out: UART control not currently used
    //     turn_off_uart();
    // });
    
    // $("#send_temp_threshold").on("click", function(){ // Commented out: Temperature threshold functionality not implemented yet
    //     // add functionality for temp threshold
    // });
});

/**
 * Commented out: Functions related to OTA firmware update, as they are not currently in use.
 *
 * function getFileInfo() {
 * var x = document.getElementById("selected_file");
 * var file = x.files[0];
 * document.getElementById("file_info").innerHTML = "<h4>File: " + file.name + "<br>" + "Size: " + file.size + " bytes</h4>";
 * }
 *
 * function updateFirmware() {
 * var formData = new FormData();
 * var fileSelect = document.getElementById("selected_file");
 * if (fileSelect.files && fileSelect.files.length == 1) {
 * var file = fileSelect.files[0];
 * formData.set("file", file, file.name);
 * document.getElementById("ota_update_status").innerHTML = "Uploading " + file.name + ", Firmware Update in Progress...";
 * var request = new XMLHttpRequest();
 * request.upload.addEventListener("progress", updateProgress);
 * request.open('POST', "/OTAupdate");
 * request.responseType = "blob";
 * request.send(formData);
 * } else {
 * window.alert('Select A File First')
 * }
 * }
 *
 * function updateProgress(oEvent) {
 * if (oEvent.lengthComputable) {
 * getUpdateStatus();
 * } else {
 * window.alert('total size is unknown')
 * }
 * }
 *
 * function getUpdateStatus() {
 * var xhr = new XMLHttpRequest();
 * var requestURL = "/OTAstatus";
 * xhr.open('POST', requestURL, false);
 * xhr.send('ota_update_status');
 * if (xhr.readyState == 4 && xhr.status == 200) {
 * var response = JSON.parse(xhr.responseText);
 * document.getElementById("latest_firmware").innerHTML = response.compile_date + " - " + response.compile_time
 * if (response.ota_update_status == 1) {
 * seconds = 10;
 * otaRebootTimer();
 * } else if (response.ota_update_status == -1) {
 * document.getElementById("ota_update_status").innerHTML = "!!! Upload Error !!!";
 * }
 * }
 * }
 *
 * function otaRebootTimer() {
 * document.getElementById("ota_update_status").innerHTML = "OTA Firmware Update Complete. This page will close shortly, Rebooting in: " + seconds;
 * if (--seconds == 0) {
 * clearTimeout(otaTimerVar);
 * window.location.reload();
 * } else {
 * otaTimerVar = setTimeout(otaRebootTimer, 1000);
 * }
 * }
 */

/**
 * Gets NTC sensor temperature value for display on the web page.
 * The endpoint is `/dhtSensor.json`, but we are assuming it now returns NTC data.
 */
function getDHTSensorValues() {
    $.getJSON('/dhtSensor.json', function(data) {
        // Assuming the JSON response contains a 'temp' field for the temperature reading
        $("#temperature_reading").text(data["temp"] + " °C"); // Added unit for clarity
    });
}

/**
 * Sets the interval for getting the updated NTC sensor values.
 */
function startDHTSensorInterval() {
    setInterval(getDHTSensorValues, 5000); // Fetch temperature every 5 seconds
}

/**
 * Commented out: Functions related to WiFi connection status, as they are not currently in use.
 *
 * function stopWifiConnectStatusInterval() {
 * if (wifiConnectInterval != null) {
 * clearInterval(wifiConnectInterval);
 * wifiConnectInterval = null;
 * }
 * }
 *
 * function getWifiConnectStatus() {
 * var xhr = new XMLHttpRequest();
 * var requestURL = "/wifiConnectStatus";
 * xhr.open('POST', requestURL, false);
 * xhr.send('wifi_connect_status');
 * if (xhr.readyState == 4 && xhr.status == 200) {
 * var response = JSON.parse(xhr.responseText);
 * document.getElementById("wifi_connect_status").innerHTML = "Connecting...";
 * if (response.wifi_connect_status == 2) {
 * document.getElementById("wifi_connect_status").innerHTML = "<h4 class='rd'>Failed to Connect. Please check your AP credentials and compatibility</h4>";
 * stopWifiConnectStatusInterval();
 * } else if (response.wifi_connect_status == 3) {
 * document.getElementById("wifi_connect_status").innerHTML = "<h4 class='gr'>Connection Success!</h4>";
 * stopWifiConnectStatusInterval();
 * }
 * }
 * }
 *
 * function startWifiConnectStatusInterval() {
 * wifiConnectInterval = setInterval(getWifiConnectStatus, 2800);
 * }
 */

/**
 * Sends the RGB values to the ESP32.
 * The values are read from the input fields and sent as a JSON object.
 */
function send_rgb_values() {
    // Get values from the input fields
    var red_val = $("#red_val").val();
    var green_val = $("#green_val").val();
    var blue_val = $("#blue_val").val();

    // Create a data object to be sent as JSON
    var rgb_data = {
        'red_val': parseInt(red_val),   // Ensure values are integers
        'green_val': parseInt(green_val),
        'blue_val': parseInt(blue_val),
        'timestamp': Date.now()         // Add a timestamp
    };

    // Send the RGB data using AJAX POST request
    $.ajax({
        url: '/rgb_values.json',         // Endpoint for RGB values
        contentType: 'application/json', // Specify that you're sending JSON
        method: 'POST',
        cache: false,
        data: JSON.stringify(rgb_data)   // Convert the data object to a JSON string
    }).done(function() {
        console.log("RGB values sent successfully!");
    }).fail(function(jqXHR, textStatus, errorThrown) {
        console.error("Error sending RGB values:", textStatus, errorThrown);
    });
}

/**
 * Commented out: Function to turn off UART, as it is not currently in use.
 *
 * function turn_off_uart() {
 * $.ajax({
 * url: '/uart_off.json',
 * dataType: 'json',
 * method: 'POST',
 * cache: false,
 * });
 * }
 */

/**
 * Toggles the board LED by sending a POST request to the ESP32.
 */
function toogle_led() {
    $.ajax({
        url: '/toogle_led.json', // Endpoint to toggle the LED
        dataType: 'json',
        method: 'POST',
        cache: false,
    }).done(function() {
        console.log("LED toggle request sent successfully!");
    }).fail(function(jqXHR, textStatus, errorThrown) {
        console.error("Error toggling LED:", textStatus, errorThrown);
    });
}

/**
 * Activates an input listener for specified element IDs.
 * Ensures the input value is an integer between 0 and 255.
 * @param {string} used_id The ID of the input element to listen to.
 */
function activate_listener(used_id) {
    const myInput = document.getElementById(used_id);

    myInput.addEventListener('input', () => {
        // Convert the input value to a number
        let value = Number(myInput.value);

        // Ensure the value is an integer
        if (!Number.isInteger(value)) {
            value = Math.floor(value);
        }

        // Restrict the value to be between 0 and 255
        if (value > 255) {
            value = 255;
        } else if (value < 0) { // Added a lower bound check
            value = 0;
        }
        myInput.value = value;
    });
}