#include <Arduino.h>
#include <LiquidCrystal_I2C.h>
#include <DHTesp.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>

// --- Pin Definitions ---
// Based on the Diagram and Document
#define LED_PIN     32
#define DHT_PIN     15
#define PIR_SENSOR  12

// --- Objects Initialization ---
// LCD at address 0x27, 16 columns, 2 rows
LiquidCrystal_I2C lcd(0x27, 16, 2);
DHTesp dhtSensor;

// --- Timing Variables ---
unsigned long lastDHTReadTime = 0;
const long intervalDHT = 2000; // Read DHT every 2000ms (2 seconds)

// Wokwi provides a virtual WiFi network with these exact credentials
const String ssid = "Wokwi-GUEST";
const String password = "";

// API Endpoint for testing
// const String serverName = "https://postman-echo.com/get";
const String serverName = "https://postman-echo.com/post";

void setup() {
  // 1. Setup Serial Communication
  Serial.begin(115200);
  Serial.println("ESP32 collecting sensors data");

  // 2. Setup LED and PIR Pins
  pinMode(LED_PIN, OUTPUT);
  pinMode(PIR_SENSOR, INPUT);

  // 3. Setup DHT Sensor
  dhtSensor.setup(DHT_PIN, DHTesp::DHT22);

  // 4. Setup LCD
  lcd.init();
  lcd.backlight();

  // Initial Display Message
  lcd.setCursor(0, 0);
  lcd.print("ESP32 Starting...");
  delay(1000);
  lcd.clear();

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
  unsigned long currentMillis = millis();

  // --- TASK A: Motion Detection (Real-time) ---
  // Requirements: Detect motion -> LED ON, Else -> LED OFF
  int pir_value = digitalRead(PIR_SENSOR);

  if (pir_value == HIGH) {
    digitalWrite(LED_PIN, HIGH);
  } else {
    digitalWrite(LED_PIN, LOW);
  }

  // --- TASK B: DHT Sensor Reading (Periodic) ---
  // Requirements: Read every 2s, Display on LCD, Print to Serial
  TempAndHumidity data;
  if (currentMillis - lastDHTReadTime >= intervalDHT) {
    lastDHTReadTime = currentMillis;

    // Read Data
    data = dhtSensor.getTempAndHumidity();

    // Check if read was successful
    if (dhtSensor.getStatus() != 0) {
      Serial.println("DHT Sensor error");
    } else {
      // Log to Serial
      Serial.println("\n--- Sensor Data ---");
      Serial.println("Temp: " + String(data.temperature, 1) + "C");
      Serial.println("Humidity: " + String(data.humidity, 1) + "%");

      // Log Motion Status synchronized with data log
      if(pir_value == HIGH) Serial.println("Status: Motion Detected (LED ON)");
      else Serial.println("Status: No Motion (LED OFF)");

      // Display on LCD
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Temp: " + String(data.temperature, 1) + "C");
      lcd.setCursor(0, 1);
      lcd.print("Humid: " + String(data.humidity, 1) + "%");

      if (WiFi.status() == WL_CONNECTED) {
        // Create the Secure Client
        WiFiClientSecure client;
        // Set it to ignore certificate checks
        client.setInsecure();

        HTTPClient http;

        Serial.println("[HTTP] Beginning Request...");
        
        // // HTTP GET url-encoded
        // String serverPath = serverName + "?temp=" + String(data.temperature, 1) + "&humid=" + String(data.humidity, 1);
        // http.begin(serverPath);
        // int httpResponseCode = http.GET();
        
        // // HTTP POST url-encoded
        // http.begin(serverName);
        // http.addHeader("Content-Type", "application/x-www-form-urlencoded");
        // String httpRequestData = "temp=" + String(data.temperature, 1) + "&humid=" + String(data.humidity, 1);
        // Serial.println(httpRequestData);
        // int httpResponseCode = http.POST(httpRequestData);

        // HTTP POST json
        http.begin(serverName);
        http.addHeader("Content-Type", "application/json");
        DynamicJsonDocument doc(1024);
        String jsonstr;
        JsonObject root = doc.to<JsonObject>();
        root["temperature"] = data.temperature;
        root["humidity"] = data.humidity;
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
    }
  }
}