#include "telegram_manager.h"
#include "sensor_manager.h"
#include "rtos_queues.h"

static WiFiClientSecure client;
static UniversalTelegramBot bot(BOT_TOKEN, client);

float g_tempThreshold = TEMP_HIGH_LIMIT;
float g_humThreshold  = HUM_HIGH_LIMIT;

static unsigned long lastTempAlert  = 0;
static unsigned long lastHumAlert   = 0;
static unsigned long lastFlameAlert = 0;

void initTelegram() {
    client.setInsecure();
}

void queueTelegramMessage(String message) {
    if (telegramQueue == NULL) return;
    char buffer[150];
    message.substring(0, 149).toCharArray(buffer, 150);
    xQueueSend(telegramQueue, buffer, pdMS_TO_TICKS(0));
}

void checkSystemConditions(float temp, float hum, bool flame) {
    unsigned long now = millis();

    if (flame && now - lastFlameAlert > FLAME_COOLDOWN) {
        queueTelegramMessage("⚠️ Fire Detected! ⚠️");
        lastFlameAlert = now;
    }
    if (!isnan(temp) && temp > g_tempThreshold && now - lastTempAlert > ALARM_COOLDOWN) {
        queueTelegramMessage("⚠️ High Temp: " + String(temp, 1) + "°C (limit: " + String(g_tempThreshold, 1) + "°C)");
        lastTempAlert = now;
    }
    if (!isnan(hum) && hum > g_humThreshold && now - lastHumAlert > ALARM_COOLDOWN) {
        queueTelegramMessage("⚠️ High Humidity: " + String(hum, 1) + "% (limit: " + String(g_humThreshold, 1) + "%)");
        lastHumAlert = now;
    }
}

static void handleCommand(String text, String chat_id) {
    text.trim();
    Serial.println("[Telegram] Command: " + text);

    if (text == "/reboot") {
        bot.sendMessage(chat_id, "🔄 Rebooting now...", "");
        delay(500);
        ESP.restart();
    }
    else if (text == "/version") {
        bot.sendMessage(chat_id, "ℹ️ Version: " + String(CURRENT_VERSION), "");
    }
    else if (text == "/status") {
        float t = getRawTemperature();
        float h = getRawHumidity();
        bool  f = isFlameDetected();
        String msg = "📊 Status:\n";
        msg += "🌡 Temp: "     + (isnan(t) ? "Error" : String(t, 1) + " °C") + "\n";
        msg += "💧 Humidity: " + (isnan(h) ? "Error" : String(h, 1) + " %")  + "\n";
        msg += "🔥 Flame: "    + String(f ? "DETECTED ⚠️" : "Safe ✅");
        bot.sendMessage(chat_id, msg, "");
    }
    else if (text == "/thresholds") {
        String msg = "⚙️ Thresholds:\n";
        msg += "🌡 Temp: "     + String(g_tempThreshold, 1) + " °C\n";
        msg += "💧 Humidity: " + String(g_humThreshold, 1)  + " %\n\n";
        msg += "/settemp X  — e.g. /settemp 42.5\n";
        msg += "/sethum X   — e.g. /sethum 75.0";
        bot.sendMessage(chat_id, msg, "");
    }
    else if (text.startsWith("/settemp ")) {
        float v = text.substring(9).toFloat();
        if (v < 10 || v > 100)
            bot.sendMessage(chat_id, "❌ Enter a value between 10 and 100.", "");
        else {
            g_tempThreshold = v;
            bot.sendMessage(chat_id, "✅ Temp threshold: " + String(v, 1) + " °C", "");
        }
    }
    else if (text.startsWith("/sethum ")) {
        float v = text.substring(8).toFloat();
        if (v < 10 || v > 100)
            bot.sendMessage(chat_id, "❌ Enter a value between 10 and 100.", "");
        else {
            g_humThreshold = v;
            bot.sendMessage(chat_id, "✅ Humidity threshold: " + String(v, 1) + " %", "");
        }
    }
    else if (text == "/help" || text == "/start") {
        String msg = "🤖 Commands:\n\n";
        msg += "/status\n/thresholds\n/settemp X\n/sethum X\n/version\n/reboot";
        bot.sendMessage(chat_id, msg, "");
    }
    else {
        bot.sendMessage(chat_id, "❓ Unknown command. Try /help", "");
    }
}

// task واحد يرسل ويستقبل — بدون mutex، بدون تعقيد
void telegramMainTask(void* parameter) {
    // تجاهل الرسائل القديمة قبل ما نبدأ
    vTaskDelay(pdMS_TO_TICKS(3000));
    bot.getUpdates(bot.last_message_received);

    char outMsg[150];

    for (;;) {
        // 1. أرسل كل الرسائل المنتظرة في القائمة
        while (xQueueReceive(telegramQueue, outMsg, 0) == pdTRUE) {
            bot.sendMessage(CHAT_ID, String(outMsg), "");
            vTaskDelay(pdMS_TO_TICKS(200)); // فترة صغيرة بين كل إرسال
        }

        // 2. تحقق من الأوامر الواردة
        int n = bot.getUpdates(bot.last_message_received);
        for (int i = 0; i < n; i++) {
            handleCommand(bot.messages[i].text, bot.messages[i].chat_id);
        }

        vTaskDelay(pdMS_TO_TICKS(1000)); // كل ثانية
    }
}