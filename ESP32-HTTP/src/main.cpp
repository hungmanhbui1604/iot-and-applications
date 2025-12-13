// HTTP post request, json
#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>

// Wokwi provides a virtual WiFi network with these exact credentials
const String ssid = "Wokwi-GUEST";
const String password = "";

// API Endpoint for testing
// const String serverName = "https://postman-echo.com/get";
const String serverName = "https://postman-echo.com/post";

void setup() {
  Serial.begin(115200);
  
  // Connect to WiFi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(100);
    Serial.print(".");
  }
  Serial.println("Connected to the WiFi network");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
}

void loop() {
  if (WiFi.status() == WL_CONNECTED) {
    // Create the Secure Client
    WiFiClientSecure client;
    // Set it to ignore certificate checks
    client.setInsecure();

    HTTPClient http;

    Serial.println("\n[HTTP] Beginning Request...");

    float x = 30.5; //get temperature value
    float y = 78;   //get humidity value

    // HTTP GET url-encoded
    // String serverPath = serverName + "?temp=" + String(x) + "&humid=" + String(y);
    // http.begin(serverPath);
    // int httpResponseCode = http.GET();

    // HTTP POST url-encoded
    // http.begin(serverName);
    // http.addHeader("Content-Type", "application/x-www-form-urlencoded");
    // String httpRequestData = "temp=" + String(x) + "&humid=" + String(y);
    // Serial.println(httpRequestData);
    // int httpResponseCode = http.POST(httpRequestData);

    // HTTP POST json
    http.begin(serverName);
    http.addHeader("Content-Type", "application/json");
    // String httpRequestData = "{\"temp\": " + String(x) +",\"humid\": " + String(y) + "}";
    DynamicJsonDocument doc(1024);
    String jsonstr;
    JsonObject root = doc.to<JsonObject>();
    root["temperature"] = x;
    root["humidity"] = y;
    serializeJson(doc, jsonstr);
    String httpRequestData = jsonstr;
    Serial.println(httpRequestData);
    int httpResponseCode = http.POST(httpRequestData);

    if (httpResponseCode > 0) {
      Serial.print("[HTTP] Response Code: ");
      Serial.println(httpResponseCode);
      
      String payload = http.getString();
      Serial.println("[HTTP] Response Payload:");
      Serial.println(payload);
    } else {
      Serial.print("[HTTP] Request failed. Error code: ");
      Serial.println(httpResponseCode);
    }

    http.end(); // Free resources
  } else {
    Serial.println("WiFi Disconnected");
  }
  
  delay(3000); // Wait 3 seconds before next request
}