#include "telegram_manager.h"
#include "rtos_queues.h"

WiFiClientSecure client;
UniversalTelegramBot bot(BOT_TOKEN, client);

// Cooldown timers to prevent alert flooding
unsigned long lastTempAlert = 0;
unsigned long lastHumAlert = 0;
unsigned long lastFlameAlert = 0;
unsigned long lastGasAlert   = 0; 

void initTelegram() {
    // Bypass SSL certificate validation for simplicity in this prototype
    client.setInsecure();
}

void sendTelegramMessage(String message) {
    bot.sendMessage(CHAT_ID, message, "");
}

void checkSystemConditions(float temp, float hum, bool flame, bool gas) {
    unsigned long currentMillis = millis();

    // 1. Fire Logic (High Priority)
    if (flame) {
        if (currentMillis - lastFlameAlert > FLAME_COOLDOWN) {
            String msg = "⚠️ Fire Detected! ⚠️\n";
            queueTelegramMessage(msg);
            lastFlameAlert = currentMillis;
        }
    }

    // 2. Temperature Logic
    if (!isnan(temp) && temp > TEMP_HIGH_LIMIT) {
        if (currentMillis - lastTempAlert > ALARM_COOLDOWN) {
            String msg = "⚠️ High Temperature Alert!⚠️\n";
            msg += "Current Temp: " + String(temp, 1) + "°C\n";
            msg += "Threshold: " + String(TEMP_HIGH_LIMIT, 1) + "°C";
            queueTelegramMessage(msg);
            lastTempAlert = currentMillis;
        }
    }

    // 3. Humidity Logic
    if (!isnan(hum) && hum > HUM_HIGH_LIMIT) {
        if (currentMillis - lastHumAlert > ALARM_COOLDOWN) {
            String msg = "⚠️ High Humidity Alert! ⚠️\n";
            msg += "Current Hum: " + String(hum, 1) + "%\n";
            msg += "Threshold: " + String(HUM_HIGH_LIMIT, 1) + "%";
            queueTelegramMessage(msg);
            lastHumAlert = currentMillis;
        }
    }

    if (gas) {
        if (currentMillis - lastGasAlert > GAS_COOLDOWN) {
            queueTelegramMessage("💨 Gas Leak Detected! ⚠️\n");
            lastGasAlert = currentMillis;
        }
    }
}



// بديل sendTelegramMessage القديمة - تضع الرسالة في الـ Queue فقط
// لا تنتظر الإرسال الفعلي
void queueTelegramMessage(String message) {
    if (telegramQueue == NULL) return;
    
    char buffer[150];
    message.substring(0, 149).toCharArray(buffer, 150);
    
    // xQueueSend لا يوقف الـ loop أبداً
    // pdMS_TO_TICKS(0) = إذا الـ Queue ممتلئة، تجاهل الرسالة ولا تنتظر
    if (xQueueSend(telegramQueue, buffer, pdMS_TO_TICKS(0)) != pdTRUE) {
        Serial.println("[RTOS] Telegram queue full, message dropped.");
    }
}

// هذه الـ Task تشتغل على Core 0 بشكل مستقل
void telegramTask(void* parameter) {
    char receivedMessage[150];
    
    for (;;) { // loop أبدي للـ Task
        // انتظر حتى تجي رسالة في الـ Queue (blocking هنا مقبول لأننا في task منفصل)
        if (xQueueReceive(telegramQueue, receivedMessage, portMAX_DELAY) == pdTRUE) {
            // sendTelegramMessage القديمة لا تزال تشتغل هنا
            sendTelegramMessage(String(receivedMessage));
        }
    }
}