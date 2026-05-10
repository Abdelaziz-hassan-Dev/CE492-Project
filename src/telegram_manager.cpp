#include "telegram_manager.h"
#include "rtos_queues.h"

WiFiClientSecure client;
UniversalTelegramBot bot(BOT_TOKEN, client);

// Cooldown timers to prevent alert flooding
unsigned long lastTempAlert = 0;
unsigned long lastHumAlert = 0;
unsigned long lastFlameAlert = 0;

// إعطاء قيم ابتدائية من config.h
float currentTempThreshold = TEMP_HIGH_LIMIT;
float currentHumThreshold = HUM_HIGH_LIMIT;

void initTelegram() {
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

    // 2. Temperature Logic (استخدام المتغيرات الجديدة)
    if (!isnan(temp) && temp > currentTempThreshold) {
        if (currentMillis - lastTempAlert > ALARM_COOLDOWN) {
            String msg = "⚠️ High Temperature Alert!⚠️\n";
            msg += "Current Temp: " + String(temp, 1) + "°C\n";
            msg += "Threshold: " + String(currentTempThreshold, 1) + "°C";
            queueTelegramMessage(msg);
            lastTempAlert = currentMillis;
        }
    }

    // 3. Humidity Logic (استخدام المتغيرات الجديدة)
    if (!isnan(hum) && hum > currentHumThreshold) {
        if (currentMillis - lastHumAlert > ALARM_COOLDOWN) {
            String msg = "⚠️ High Humidity Alert! ⚠️\n";
            msg += "Current Hum: " + String(hum, 1) + "%\n";
            msg += "Threshold: " + String(currentHumThreshold, 1) + "%";
            queueTelegramMessage(msg);
            lastHumAlert = currentMillis;
        }
    }
}

void queueTelegramMessage(String message) {
    if (telegramQueue == NULL) return;
    
    char buffer[150];
    message.substring(0, 149).toCharArray(buffer, 150);
    
    if (xQueueSend(telegramQueue, buffer, pdMS_TO_TICKS(0)) != pdTRUE) {
        Serial.println("[RTOS] Telegram queue full, message dropped.");
    }
}

// الدالة الجديدة لمعالجة الأوامر
void handleNewMessages(int numNewMessages) {
    for (int i = 0; i < numNewMessages; i++) {
        String chat_id = String(bot.messages[i].chat_id);
        String text = bot.messages[i].text;

        if (chat_id != String(CHAT_ID)) {
            continue;
        }

        if (text == "/reboot") {
            bot.sendMessage(chat_id, "🔄 Rebooting system...", "");
            delay(1000); 
            ESP.restart();
        } 
        else if (text == "/version") {
            // هنا نستخدم CURRENT_VERSION المعرف في config.h مباشرة
            bot.sendMessage(chat_id, "ℹ️ Firmware Version: " + String(CURRENT_VERSION), "");
        }
        else if (text == "/get_thresholds") {
            String msg = "📊 **Current Thresholds**:\n";
            msg += "🌡️ Temp: " + String(currentTempThreshold) + "°C\n";
            msg += "💧 Hum: " + String(currentHumThreshold) + "%";
            bot.sendMessage(chat_id, msg, "");
        }
        else if (text.startsWith("/set_temp ")) {
            // استخراج الرقم بعد المسافة
            String valStr = text.substring(10);
            currentTempThreshold = valStr.toFloat();
            bot.sendMessage(chat_id, "✅ Temp threshold updated to: " + String(currentTempThreshold) + "°C", "");
        }
        else if (text.startsWith("/set_hum ")) {
            // استخراج الرقم بعد المسافة
            String valStr = text.substring(9);
            currentHumThreshold = valStr.toFloat();
            bot.sendMessage(chat_id, "✅ Humidity threshold updated to: " + String(currentHumThreshold) + "%", "");
        }
        else if (text == "/start" || text == "/help") {
            String msg = "🛠️ **Available Commands**:\n";
            msg += "/reboot - Restart the ESP32\n";
            msg += "/version - Show firmware version\n";
            msg += "/get_thresholds - Show current limits\n";
            msg += "/set_temp <value> - Change temp limit\n";
            msg += "/set_hum <value> - Change humidity limit\n";
            bot.sendMessage(chat_id, msg, "");
        }
    }
}

// تعديل الـ Task لتعمل في الاتجاهين (إرسال واستقبال)
void telegramTask(void* parameter) {
    char receivedMessage[150];
    unsigned long lastUpdateCheck = 0;
    const unsigned long BOT_MTBS = 2000; // التحقق من الرسائل كل ثانيتين
    
    for (;;) { 
        // 1. الاستماع للـ Queue للإرسال. 
        // بدلاً من الانتظار للأبد (portMAX_DELAY)، ننتظر ثانية واحدة كحد أقصى.
        if (xQueueReceive(telegramQueue, receivedMessage, pdMS_TO_TICKS(1000)) == pdTRUE) {
            sendTelegramMessage(String(receivedMessage));
        }

        // 2. التحقق من استقبال أوامر جديدة كل ثانيتين
        if (millis() - lastUpdateCheck > BOT_MTBS) {
            int numNewMessages = bot.getUpdates(bot.last_message_received + 1);
            
            while (numNewMessages) {
                handleNewMessages(numNewMessages);
                numNewMessages = bot.getUpdates(bot.last_message_received + 1);
            }
            lastUpdateCheck = millis();
        }
    }
}