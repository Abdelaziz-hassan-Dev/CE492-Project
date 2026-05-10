#include <Arduino.h>
#include <WiFi.h>
#include "time.h" 
#include "config.h"
#include "sensor_manager.h"
#include "telegram_manager.h"
#include "cloud_manager.h"    
#include "firebase_manager.h" 
#include "ota_manager.h" 
#include "rtos_queues.h" 


unsigned long lastSystemTick = 0;
const long SYSTEM_TICK_INTERVAL = 2000;

unsigned long lastDataLog = 0;

// NTP Time settings (Adjust gmtOffset_sec for your timezone)
const long  gmtOffset_sec = 3 * 3600; 
const int   daylightOffset_sec = 0;

void setup() {
    Serial.begin(115200);
    initSensors();
    
    // WiFi Connection
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    Serial.print("Connecting to WiFi");
    while (WiFi.status() != WL_CONNECTED) {
        delay(400);
        Serial.print(".");
    }

    // Valid system time is critical for historical data logging
    configTime(gmtOffset_sec, daylightOffset_sec, "pool.ntp.org", "time.nist.gov");
    struct tm timeinfo;
    
    // Blocking wait until time is synced to avoid invalid timestamps in logs
    while(!getLocalTime(&timeinfo)){
        delay(200);
    }

    initQueues();
    initTelegram();
    initCloud();     
    initFirebase();  

    xTaskCreatePinnedToCore(telegramTask, "TelegramTask", 8192, NULL, 1, NULL, 0);
    xTaskCreatePinnedToCore(loggingTask,  "LoggingTask",  8192, NULL, 1, NULL, 0);

    delay(250); // تأخير بسيط قبل بدء التحقق من التحديثات للسماح للمهام الأخرى بالبدء بشكل صحيح
    checkForUpdates(); 

}

void loop() {
    unsigned long currentMillis = millis();

    if (currentMillis - lastSystemTick >= SYSTEM_TICK_INTERVAL) {
        lastSystemTick = currentMillis;

        float temp   = getRawTemperature();
        float hum    = getRawHumidity();
        bool  isFire = isFlameDetected();
        String flameStr = isFire ? "DETECTED" : "Safe";

        sendDataToFirebase(temp, hum, flameStr);
        checkSystemConditions(temp, hum, isFire);

       if (currentMillis - lastDataLog >= LOG_INTERVAL) {
       if (!isnan(temp) && !isnan(hum)) {
        SensorLog_t logData = { temp, hum, isFire };
        if (xQueueSend(loggingQueue, &logData, pdMS_TO_TICKS(0)) != pdTRUE) {
        }
    }
    lastDataLog = currentMillis;
        }
    }
}