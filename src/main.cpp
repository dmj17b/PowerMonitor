#include <SPI.h>
#include <Wire.h>
#include <Adafruit_INA260.h>
#include <WiFi.h>
#include <WiFiAP.h>
#include <WiFiServer.h>

Adafruit_INA260 ina260 = Adafruit_INA260();

// WiFi configuration
const char *ssid = "XIAO_ESP32S3";
const char *password = "password";
WiFiServer server(80);

// Variables to control streaming and timestamps
bool streaming = false;
unsigned long startTime = 0;  // To keep track of when streaming starts

void setup() {
  Serial.begin(115200);
  ina260.begin();

  ina260.setAveragingCount(INA260_COUNT_4);
  ina260.setVoltageConversionTime(INA260_TIME_140_us);
  ina260.setCurrentConversionTime(INA260_TIME_140_us);

  // Initialize WiFi access point
  Serial.println("Configuring access point...");
  WiFi.softAP(ssid);
  IPAddress myIP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(myIP);
  server.begin();

  Serial.println("Server started");
}

void loop() {
  WiFiClient client = server.available();   // listen for incoming clients

  if (client) {                             // if you get a client,
    Serial.println("New Client.");           // print a message out the serial port
    String currentLine = "";                // make a String to hold incoming data from the client
    while (client.connected()) {            // loop while the client's connected
      if (client.available()) {             // if there's bytes to read from the client,
        char c = client.read();             // read a byte, then
        Serial.write(c);                    // print it out the serial monitor
        if (c == '\n') {                    // if the byte is a newline character

          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          if (currentLine.length() == 0) {
            // Send the HTML page with buttons and data display
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println();

            // HTML for the web page
            client.println("<html>");
            client.println("<head>");
            client.println("<style>");
            client.println("body { font-family: Arial, sans-serif; }");
            client.println(".sidebar { width: 200px; position: fixed; left: 0; top: 0; bottom: 0; background-color: #f0f0f0; padding: 10px; }");
            client.println(".content { margin-left: 220px; padding: 10px; }");
            client.println(".button { display: block; margin: 10px 0; padding: 10px; background-color: #008CBA; color: white; text-align: center; text-decoration: none; }");
            client.println(".button:hover { background-color: #005f5f; }");
            client.println(".data-container { max-height: 400px; overflow-y: scroll; border: 1px solid #ddd; padding: 10px; }");
            client.println("</style>");
            client.println("</head>");
            client.println("<body>");

            client.println("<div class=\"sidebar\">");
            client.println("<a href=\"/start\" class=\"button\">Start Streaming</a>");
            client.println("<a href=\"/stop\" class=\"button\">Stop Streaming</a>");
            client.println("<a href=\"#\" class=\"button\" onclick=\"clearData(); return false;\">Clear Stream</a>");
            client.println("</div>");

            client.println("<div class=\"content\">");
            client.println("<h1>Power Monitor Data</h1>");
            client.println("<div class=\"data-container\"><pre id=\"data\"></pre></div>"); // Placeholder for streaming data with scrollable container
            client.println("</div>");

            client.println("<script>");
            client.println("var streaming = false;");  // Track streaming status on the client side

            client.println("function fetchData() {");
            client.println("  if (streaming) {");  // Only fetch data if streaming
            client.println("    fetch('/data').then(response => response.text()).then(data => {");
            client.println("      if (data.trim() !== 'Streaming is stopped. Click Start Streaming to begin.') {");
            client.println("        document.getElementById('data').innerHTML += data;"); // Append new data
            client.println("        document.querySelector('.data-container').scrollTop = document.querySelector('.data-container').scrollHeight;"); // Auto-scroll to bottom
            client.println("      }");
            client.println("    });");
            client.println("  }");
            client.println("}");
            client.println("setInterval(fetchData, 50);"); // Fetch data every 50ms

            client.println("function startStreaming() { streaming = true; }");
            client.println("function stopStreaming() { streaming = false; }");
            client.println("function clearData() { document.getElementById('data').innerHTML = ''; }"); // Clear data function

            client.println("document.querySelector('a[href=\"/start\"]').onclick = function(e) { e.preventDefault(); startStreaming(); fetch('/start'); };"); // Start button logic
            client.println("document.querySelector('a[href=\"/stop\"]').onclick = function(e) { e.preventDefault(); stopStreaming(); fetch('/stop'); };"); // Stop button logic

            client.println("</script>");

            client.println("</body>");
            client.println("</html>");

            // break out of the while loop:
            break;
          } else {    // if you got a newline, then clear currentLine:
            currentLine = "";
          }
        } else if (c != '\r') {  // if you got anything else but a carriage return character,
          currentLine += c;      // add it to the end of the currentLine
        }

        // Check to see if the client request was "GET /start", "GET /stop", or "GET /data":
        if (currentLine.endsWith("GET /start")) {
          streaming = true;  // Start streaming data
          startTime = millis();  // Reset the start time to current time
        }
        if (currentLine.endsWith("GET /stop")) {
          streaming = false;  // Stop streaming data
        }
        if (currentLine.endsWith("GET /data")) {
          // Serve the current data
          client.println("HTTP/1.1 200 OK");
          client.println("Content-type:text/plain");
          client.println();

          // Stream data
          if (streaming) {
            float current = ina260.readCurrent();
            float voltage = ina260.readBusVoltage();
            float power = ina260.readPower();
            // Calculate elapsed time since start
            unsigned long elapsedTime = millis() - startTime;

            // Send data in CSV format
            client.print(elapsedTime / 1000.0, 2); // Print elapsed time in seconds with two decimal places
            client.print(",");
            client.print(current);
            client.print(",");
            client.print(voltage);
            client.print(",");
            client.println(power);
          } else {
            client.println("Streaming is stopped. Click Start Streaming to begin.");
          }

          break; // Exit the loop after sending data
        }
      }
    }

    // Close the connection after response is complete
    client.stop();
    Serial.println("Client Disconnected.");
  }
}
