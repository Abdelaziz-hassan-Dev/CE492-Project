#include "telegram_manager.h"
#include "sensor_manager.h"   // لأمر /status
#include "rtos_queues.h"

WiFiClientSecure client;
UniversalTelegramBot bot(BOT_TOKEN, client);

// ── Runtime thresholds (مبدئياً نفس قيم config.h) ────────────────────────────
float g_tempThreshold = TEMP_HIGH_LIMIT;
float g_humThreshold  = HUM_HIGH_LIMIT;

// ── Mutex ─────────────────────────────────────────────────────────────────────
SemaphoreHandle_t g_botMutex = NULL;

// ── Cooldown timers ───────────────────────────────────────────────────────────
static unsigned long lastTempAlert  = 0;
static unsigned long lastHumAlert   = 0;
static unsigned long lastFlameAlert = 0;

// ─────────────────────────────────────────────────────────────────────────────
void initTelegram() {
    client.setInsecure();
    g_botMutex = xSemaphoreCreateMutex(); // أنشئ الـ Mutex هنا
}

// ─────────────────────────────────────────────────────────────────────────────
void sendTelegramMessage(String message) {
    // هذه تُستدعى فقط من داخل telegramTask (بعد أخذ الـ Mutex)
    bot.sendMessage(CHAT_ID, message, "");
}

// ─────────────────────────────────────────────────────────────────────────────
void queueTelegramMessage(String message) {
    if (telegramQueue == NULL) return;
    char buffer[150];
    message.substring(0, 149).toCharArray(buffer, 150);
    if (xQueueSend(telegramQueue, buffer, pdMS_TO_TICKS(0)) != pdTRUE) {
        Serial.println("[RTOS] Telegram queue full, message dropped.");
    }
}

// ─────────────────────────────────────────────────────────────────────────────
void checkSystemConditions(float temp, float hum, bool flame) {
    unsigned long now = millis();

    if (flame) {
        if (now - lastFlameAlert > FLAME_COOLDOWN) {
            queueTelegramMessage("⚠️ Fire Detected! ⚠️");
            lastFlameAlert = now;
        }
    }

    // يستخدم g_tempThreshold بدل TEMP_HIGH_LIMIT الثابت
    if (!isnan(temp) && temp > g_tempThreshold) {
        if (now - lastTempAlert > ALARM_COOLDOWN) {
            String msg = "⚠️ High Temperature Alert!\n";
            msg += "Current: " + String(temp, 1) + "°C\n";
            msg += "Threshold: " + String(g_tempThreshold, 1) + "°C";
            queueTelegramMessage(msg);
            lastTempAlert = now;
        }
    }

    if (!isnan(hum) && hum > g_humThreshold) {
        if (now - lastHumAlert > ALARM_COOLDOWN) {
            String msg = "⚠️ High Humidity Alert!\n";
            msg += "Current: " + String(hum, 1) + "%\n";
            msg += "Threshold: " + String(g_humThreshold, 1) + "%";
            queueTelegramMessage(msg);
            lastHumAlert = now;
        }
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// معالج الأوامر الواردة
// ─────────────────────────────────────────────────────────────────────────────
static void handleCommand(String text, String chat_id) {
    text.trim();

    // ── /reboot ───────────────────────────────────────────────────────────────
    if (text == "/reboot") {
        bot.sendMessage(chat_id, "🔄 Rebooting now...", "");
        delay(500);
        ESP.restart();
        return;
    }

    // ── /version ──────────────────────────────────────────────────────────────
    if (text == "/version") {
        bot.sendMessage(chat_id, "ℹ️ Firmware Version: " + String(CURRENT_VERSION), "");
        return;
    }

    // ── /status ───────────────────────────────────────────────────────────────
    if (text == "/status") {
        float t = getRawTemperature();
        float h = getRawHumidity();
        bool  f = isFlameDetected();

        String msg = "📊 Current Status:\n";
        msg += "🌡 Temp: "    + (isnan(t) ? "Error" : String(t, 1) + " °C") + "\n";
        msg += "💧 Humidity: " + (isnan(h) ? "Error" : String(h, 1) + " %")  + "\n";
        msg += "🔥 Flame: "   + String(f ? "DETECTED ⚠️" : "Safe ✅");
        bot.sendMessage(chat_id, msg, "");
        return;
    }

    // ── /thresholds ───────────────────────────────────────────────────────────
    if (text == "/thresholds") {
        String msg = "⚙️ Current Thresholds:\n";
        msg += "🌡 Temp: "    + String(g_tempThreshold, 1) + " °C\n";
        msg += "💧 Humidity: " + String(g_humThreshold, 1)  + " %\n\n";
        msg += "To change:\n";
        msg += "/settemp <value>  (e.g. /settemp 42.5)\n";
        msg += "/sethum <value>   (e.g. /sethum 75.0)";
        bot.sendMessage(chat_id, msg, "");
        return;
    }

    // ── /settemp <value> ──────────────────────────────────────────────────────
    if (text.startsWith("/settemp ")) {
        String valStr = text.substring(9);
        valStr.trim();
        float newVal = valStr.toFloat();

        // toFloat() ترجع 0.0 إذا الإدخال غلط — نتحقق من نطاق معقول
        if (newVal < 10.0 || newVal > 100.0) {
            bot.sendMessage(chat_id, "❌ Invalid value. Use a number between 10 and 100.", "");
        } else {
            g_tempThreshold = newVal;
            bot.sendMessage(chat_id,
                "✅ Temp threshold updated to: " + String(g_tempThreshold, 1) + " °C", "");
        }
        return;
    }

    // ── /sethum <value> ───────────────────────────────────────────────────────
    if (text.startsWith("/sethum ")) {
        String valStr = text.substring(8);
        valStr.trim();
        float newVal = valStr.toFloat();

        if (newVal < 10.0 || newVal > 100.0) {
            bot.sendMessage(chat_id, "❌ Invalid value. Use a number between 10 and 100.", "");
        } else {
            g_humThreshold = newVal;
            bot.sendMessage(chat_id,
                "✅ Humidity threshold updated to: " + String(g_humThreshold, 1) + " %", "");
        }
        return;
    }

    // ── /help ─────────────────────────────────────────────────────────────────
    if (text == "/help" || text == "/start") {
        String msg = "🤖 Available Commands:\n\n";
        msg += "/status       — Show current readings\n";
        msg += "/thresholds   — Show alert thresholds\n";
        msg += "/settemp X    — Set temp threshold (°C)\n";
        msg += "/sethum X     — Set humidity threshold (%)\n";
        msg += "/version      — Show firmware version\n";
        msg += "/reboot       — Restart the device";
        bot.sendMessage(chat_id, msg, "");
        return;
    }

    // ── Unknown command ───────────────────────────────────────────────────────
    bot.sendMessage(chat_id, "❓ Unknown command. Send /help for the list.", "");
}

// ─────────────────────────────────────────────────────────────────────────────
// Task إرسال (Core 0) — نفس المنطق السابق + Mutex
// ─────────────────────────────────────────────────────────────────────────────
void telegramTask(void* parameter) {
    char receivedMessage[150];
    for (;;) {
        if (xQueueReceive(telegramQueue, receivedMessage, portMAX_DELAY) == pdTRUE) {
            if (xSemaphoreTake(g_botMutex, pdMS_TO_TICKS(5000)) == pdTRUE) {
                sendTelegramMessage(String(receivedMessage));
                xSemaphoreGive(g_botMutex);
            }
        }
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Task استقبال الأوامر (Core 0) — يسأل تيليقرام كل ثانية
// ─────────────────────────────────────────────────────────────────────────────
void telegramPollingTask(void* parameter) {
    // انتظر شوي حتى يستقر النظام ثم تجاهل الرسائل القديمة
    vTaskDelay(pdMS_TO_TICKS(3000));

    if (xSemaphoreTake(g_botMutex, pdMS_TO_TICKS(5000)) == pdTRUE) {
        bot.last_message_received = bot.getUpdates(bot.last_message_received);
        xSemaphoreGive(g_botMutex);
    }

    for (;;) {
        if (xSemaphoreTake(g_botMutex, pdMS_TO_TICKS(5000)) == pdTRUE) {
            int msgCount = bot.getUpdates(bot.last_message_received);
            xSemaphoreGive(g_botMutex);

            // عالج كل رسالة جديدة
            for (int i = 0; i < msgCount; i++) {
                String text    = bot.messages[i].text;
                String chat_id = bot.messages[i].chat_id;

                Serial.println("[TelegramCmd] Received: " + text);

                // خذ الـ Mutex مجدداً أثناء الرد (handleCommand يستدعي bot.sendMessage)
                if (xSemaphoreTake(g_botMutex, pdMS_TO_TICKS(5000)) == pdTRUE) {
                    handleCommand(text, chat_id);
                    xSemaphoreGive(g_botMutex);
                }
            }
        }

        vTaskDelay(pdMS_TO_TICKS(1000)); // Poll كل ثانية
    }
}