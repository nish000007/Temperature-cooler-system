#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include "max6675.h"

// WiFi credentials
const char* ssid = "BMS";
const char* password = "12345678";

// MAX6675 pins
int thermoDO = 12;
int thermoCS = 15;
int thermoCLK = 14;

// Fan control pin
int fanPin = 4;

// Objects
MAX6675 thermocouple(thermoCLK, thermoCS, thermoDO);
ESP8266WebServer server(80);

// Temperature variables
float correctedTemp = 0.0;
const float fanThreshold = 22.0; // Updated threshold
bool fanStatus = false;

// HTML page generator
String getWebPage() {
  String html = R"rawliteral(
    <!DOCTYPE html>
    <html>
    <head>
      <meta charset="utf-8">
      <meta http-equiv="refresh" content="2">
      <title>Battery Temp Monitor</title>
      <style>
        body {
          font-family: Arial, sans-serif;
          background: #f7f7f7;
          text-align: center;
          padding: 50px;
        }
        .card {
          background: white;
          padding: 30px;
          margin: auto;
          width: 300px;
          box-shadow: 0 4px 8px rgba(0,0,0,0.1);
          border-radius: 10px;
        }
        h2 {
          color: #333;
        }
        .value {
          font-size: 2em;
          color: #0077cc;
        }
        .status {
          font-size: 1.5em;
          font-weight: bold;
          color: green;
        }
        .status.off {
          color: red;
        }
      </style>
    </head>
    <body>
      <div class="card">
        <h2>Battery Temperature Monitor</h2>
        <p>Current Temperature: <span class="value">)rawliteral" + String(correctedTemp, 1) + R"rawliteral( &deg;C</span></p>
        <p>Fan Threshold: <span class="value">)rawliteral" + String(fanThreshold, 1) + R"rawliteral( &deg;C</span></p>
        <p>Fan Status: <span class="status )rawliteral" + (fanStatus ? "" : "off") + R"rawliteral(">)rawliteral" + (fanStatus ? "ON" : "OFF") + R"rawliteral(</span></p>
      </div>
    </body>
    </html>
  )rawliteral";

  return html;
}

// Smoothed temperature reading
float getSmoothedTemperature(int samples = 10, int delayBetween = 10) {
  float total = 0.0;
  int validSamples = 0;

  for (int i = 0; i < samples; i++) {
    float temp = thermocouple.readCelsius();

    if (!isnan(temp) && temp > 0.0) { // Accept readings above 0°C
      total += temp;
      validSamples++;
    }

    delay(delayBetween);
  }

  if (validSamples == 0) return 22.0; // Default if invalid readings

  float average = total / validSamples;
  return average - 2.0; // Apply calibration offset
}

// Webpage handler
void handleRoot() {
  server.send(200, "text/html", getWebPage());
}

void setup() {
  Serial.begin(9600);
  delay(500);
  pinMode(fanPin, OUTPUT);
  digitalWrite(fanPin, LOW);

  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi connected!");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  server.on("/", handleRoot);
  server.begin();
  Serial.println("Web server started");
}

void loop() {
  correctedTemp = getSmoothedTemperature(10, 10);

  if (correctedTemp > fanThreshold) {
    digitalWrite(fanPin, HIGH);
    fanStatus = true;
  } else {
    digitalWrite(fanPin, LOW);
    fanStatus = false;
  }

  server.handleClient();
  delay(900);
}
