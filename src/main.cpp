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
        delay(1000);
        Serial.print(".");
    }
    Serial.println("\nConnected!");

    // Valid system time is critical for historical data logging
    configTime(gmtOffset_sec, daylightOffset_sec, "pool.ntp.org", "time.nist.gov");
    Serial.print("Waiting for NTP time sync: ");
    struct tm timeinfo;
    
    // Blocking wait until time is synced to avoid invalid timestamps in logs
    while(!getLocalTime(&timeinfo)){
        Serial.print(".");
        delay(500);
    }
    Serial.println("\nTime Synced!");

    initQueues();
    initTelegram();
    initCloud();     
    initFirebase();  

    queueTelegramMessage(" duuuuuude, I'm back online! 🚀");
    Serial.println("Checking for updates...");
    checkForUpdates(); // افحص التحديث عند بدء التشغيل
    Serial.println("update done.....");
    queueTelegramMessage("✅ Update Check Completed!");

  xTaskCreatePinnedToCore(
        telegramTask,    // الدالة
        "TelegramTask",  // اسم للـ debug
        8192,            // حجم الـ stack (كافي للـ HTTPS)
        NULL,            // parameters
        1,               // priority
        NULL,            // task handle (مش محتاجينه)
        0                // Core 0
    );
  xTaskCreatePinnedToCore(loggingTask, "LoggingTask", 8192, NULL, 1, NULL, 0); 
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
            // ← بدل استدعاء الدوال مباشرة، حط البيانات في القيو
            SensorLog_t logData = { temp, hum, isFire };

            if (xQueueSend(loggingQueue, &logData, pdMS_TO_TICKS(0)) != pdTRUE) {
                Serial.println("[Loop] Logging queue full, skipped.");
            }

            lastDataLog = currentMillis;
        }
    }
}