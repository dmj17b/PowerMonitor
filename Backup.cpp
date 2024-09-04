#include <SPI.h>
#include <Wire.h>
#include <Adafruit_INA260.h>
#include <Adafruit_GFX.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiAP.h>
#include <WiFiServer.h>
#include <BleSerial.h>

Adafruit_INA260 ina260 = Adafruit_INA260();

// Wifi configuration
const char *ssid = "XIAO_ESP32S3";
const char *password = "password";
WiFiServer server(80);

// Variables to control streaming
bool streaming = false;

void setup() {
  Serial.begin(115200);
  ina260.begin();

  ina260.setAveragingCount(INA260_COUNT_4);
  ina260.setVoltageConversionTime(INA260_TIME_140_us);
  ina260.setCurrentConversionTime(INA260_TIME_140_us);

  // Initialize WiFi access point
  Serial.println("Configuring access point...");
  WiFi.softAP(ssid, password);
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
            // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
            // and a content-type so the client knows what's coming, then a blank line:
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println();

            // the content of the HTTP response follows the header:
            client.print("<html><body>");
            client.print("<h1>Power Monitor</h1>");
            client.print("Click <a href=\"/start\">here</a> to START streaming data.<br>");
            client.print("Click <a href=\"/stop\">here</a> to STOP streaming data.<br><br>");
            
            if (streaming) {
              client.print("<h2>Streaming Data...</h2>");
            } else {
              client.print("<h2>Data Stream is Stopped.</h2>");
            }

            // The HTTP response ends with another blank line:
            client.println("</body></html>");
            client.println();
            
            // break out of the while loop:
            break;
          } else {    // if you got a newline, then clear currentLine:
            currentLine = "";
          }
        } else if (c != '\r') {  // if you got anything else but a carriage return character,
          currentLine += c;      // add it to the end of the currentLine
        }

        // Check to see if the client request was "GET /start" or "GET /stop":
        if (currentLine.endsWith("GET /start")) {
          streaming = true;  // Start streaming data
        }
        if (currentLine.endsWith("GET /stop")) {
          streaming = false;  // Stop streaming data
        }
      }
    }

    // If streaming is enabled, send data to the client
    if (streaming) {
      float current = ina260.readCurrent();
      float voltage = ina260.readBusVoltage();
      float power = ina260.readPower();

      // Stream data to the client
      client.print("Current: "); client.print(current); client.println(" mA");
      client.print("Bus Voltage: "); client.print(voltage); client.println(" mV");
      client.print("Power: "); client.print(power); client.println(" mW");
    }

    // close the connection:
    client.stop();
    Serial.println("Client Disconnected.");
  }

  // Optional: Log data to serial monitor for debugging
  if (streaming) {
    Serial.print("Current: "); Serial.print(ina260.readCurrent()); Serial.println(" mA");
    Serial.print("Bus Voltage: "); Serial.print(ina260.readBusVoltage()); Serial.println(" mV");
    Serial.print("Power: "); Serial.print(ina260.readPower()); Serial.println(" mW");
  }
}
