#include <Arduino.h>
#include <LiquidCrystal_I2C.h>
#include <DHTesp.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>

const String ssid = "Wokwi-GUEST";
const String password = "";

#define LED_PIN     32
#define DHT_PIN     15
#define PIR_SENSOR  12

struct SharedData {
    float temperature;
    float humidity;
    bool motionDetected;
};

//Task 1: blink LED
void TaskBlink(void *pvParameters);
//Task 2: Read data from sensors
void TaskSensorRead(void *pvParameters);
//Task 3: Send data to server
void TaskSendingData(void *pvParameters);

LiquidCrystal_I2C lcd(0x27, 16, 2);
DHTesp dhtSensor;

const String serverName = "http://postman-echo.com/post";
// const String serverName = "https://postman-echo.com/post";

SharedData sensorData;
SemaphoreHandle_t dataMutex;

void setup(){
    Serial.begin(115200);
    Serial.println("\nMulti-tasking with FreeRTOS...");

    //create mutex
    dataMutex = xSemaphoreCreateMutex();

    //setup sensors
    pinMode(LED_PIN, OUTPUT);
    pinMode(PIR_SENSOR, INPUT);
    dhtSensor.setup(DHT_PIN, DHTesp::DHT22);
    lcd.init();
    lcd.backlight();
    
    //setup wifi
    WiFi.begin(ssid,password);
    Serial.println("\nConecting to Wifi");
    while(WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
    }
    Serial.println("");
    Serial.print("Connected to WiFi with IP Address: ");
    Serial.println(WiFi.localIP());

    //TODO: Create the task 1 to run TaskBlink
    xTaskCreate(
        TaskBlink,  
        "Blink LED",
        1024,      
        NULL,      
        1,        
        NULL       
    );

    //TODO: Create the task 2 to run TaskSensorRead
    xTaskCreate(
        TaskSensorRead,  
        "Read Sensors",
        4096,      
        NULL,      
        1,        
        NULL       
    );

    //TODO: Create task 3 to run TaskSendingData
    xTaskCreate(
        TaskSendingData,  
        "Send Data",
        16384,      
        NULL,      
        1,        
        NULL       
    );
}

//The function for Task 1
void TaskBlink(void *pvParameters){
    for(;;){
        digitalWrite(LED_PIN, HIGH);
        vTaskDelay(pdMS_TO_TICKS(1000));
        digitalWrite(LED_PIN, LOW);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

//The function for Task 2
void TaskSensorRead(void *pvParameters){
    for(;;) {
        //read sensors
        int current_pir = digitalRead(PIR_SENSOR);
        TempAndHumidity current_dht = dhtSensor.getTempAndHumidity();

        //display on LCD
        lcd.setCursor(0, 0);
        lcd.print("M:" + String(current_pir == HIGH ? "Detected" : "Not Detected"));
        lcd.setCursor(0, 1);
        if (dhtSensor.getStatus() == 0) {
            lcd.print("T:" + String(current_dht.temperature, 1) + "C H:" + String(current_dht.humidity, 1) + "%");
        }
        else {
            lcd.print("Sensor Error");
        }

        //log to Serial
        Serial.println("\nMotion: " + String(current_pir == HIGH ? "Detected" : "Not Detected"));
        if (dhtSensor.getStatus() == 0) {
            Serial.print("Temperature: " + String(current_dht.temperature, 1) + " C");
            Serial.println(" | Humidity: " + String(current_dht.humidity, 1) + " %");
        }
        else {
            Serial.print("DHT22 Sensor Error");
        }

        //update shared data
        if (xSemaphoreTake(dataMutex, portMAX_DELAY) == pdTRUE) {
            
            if (dhtSensor.getStatus() == 0) {
                sensorData.temperature = current_dht.temperature;
                sensorData.humidity = current_dht.humidity;
            }
            sensorData.motionDetected = (current_pir == HIGH);

            xSemaphoreGive(dataMutex);
        }

        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

void TaskSendingData(void* pvParameters){
    for(;;) {
        SharedData dataToSend;

        //get a copy of the shared data
        if (xSemaphoreTake(dataMutex, portMAX_DELAY) == pdTRUE) {
            dataToSend = sensorData;
            xSemaphoreGive(dataMutex);
        }

        //send data to server
        if (WiFi.status() == WL_CONNECTED) {
            WiFiClient client;
            // WiFiClientSecure client;
            // client.setInsecure();
            HTTPClient http;
            
            if(http.begin(client, serverName)) {
                http.addHeader("Content-Type", "application/json");

                //create JSON payload
                DynamicJsonDocument doc(1024);
                doc["temperature"] = dataToSend.temperature;
                doc["humidity"] = dataToSend.humidity;
                doc["motion"] = dataToSend.motionDetected ? "detected" : "not detected";

                String jsonstr;
                serializeJson(doc, jsonstr);
                Serial.println("\nSend Data: " + jsonstr);

                int httpResponseCode = http.POST(jsonstr);
                if (httpResponseCode > 0) {
                    Serial.print("Response Code: ");
                    Serial.println(httpResponseCode);
                    
                    String payload = http.getString();
                    Serial.println("Response Payload:");
                    Serial.println(payload);
                } 
                else {
                    Serial.print("Error code: ");
                    Serial.println(httpResponseCode);
                }

                http.end();
            }
            else {
                Serial.println("Unable to connect to server");
            }
        } 
        else {
            Serial.println("Disconnected to WiFi");
        }
        
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}
void loop() {
    // put your main code here, to run repeatedly
}
