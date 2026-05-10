#include "telegram_manager.h"
#include "rtos_queues.h"

WiFiClientSecure client;
UniversalTelegramBot bot(BOT_TOKEN, client);

// Cooldown timers to prevent alert flooding
unsigned long lastTempAlert = 0;
unsigned long lastHumAlert = 0;
unsigned long lastFlameAlert = 0;

void initTelegram() {
    // Bypass SSL certificate validation for simplicity in this prototype
    client.setInsecure();
}

void sendTelegramMessage(String message) {
    bot.sendMessage(CHAT_ID, message, "");
}

void checkSystemConditions(float temp, float hum, bool flame) {
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
        
        // 1. ننتظر ثانية واحدة فقط (1000 ملي ثانية) لرسائل الكيوز بدلاً من portMAX_DELAY
        if (xQueueReceive(telegramQueue, receivedMessage, pdMS_TO_TICKS(1000)) == pdTRUE) {
            // إذا كانت هناك رسالة تحذيرية من الحساسات، نرسلها
            sendTelegramMessage(String(receivedMessage));
        }

        // 2. فحص الرسائل الواردة من البوت
        // الدالة getUpdates تجلب الرسائل الجديدة فقط بناءً على آخر رسالة تمت قراءتها
        int numNewMessages = bot.getUpdates(bot.last_message_received + 1);

        for (int i = 0; i < numNewMessages; i++) {
            String text = bot.messages[i].text;
            
            // التحقق من محتوى الرسالة
            if (text == "reboot") {
                // إرسال تأكيد للمستخدم قبل إعادة التشغيل
                bot.sendMessage(bot.messages[i].chat_id, "System is rebooting now...", "");
                delay(500); // تأخير بسيط لضمان وصول رسالة التأكيد
                
                // أمر إعادة تشغيل شريحة ESP32
                ESP.restart(); 
            }
        }
    }
}