#ifndef TELEGRAM_MANAGER_H
#define TELEGRAM_MANAGER_H

#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <ArduinoJson.h>
#include <freertos/semphr.h>
#include "config.h"

// ── Runtime Thresholds (قابلة للتعديل عبر تيليقرام) ──────────────────────────
extern float g_tempThreshold;
extern float g_humThreshold;

// ── Mutex لحماية bot object المشترك ──────────────────────────────────────────
extern SemaphoreHandle_t g_botMutex;

void initTelegram();
void checkSystemConditions(float temp, float hum, bool flame);
void sendTelegramMessage(String message);
void queueTelegramMessage(String message);

// RTOS Tasks
void telegramTask(void* parameter);        // إرسال
void telegramPollingTask(void* parameter); // استقبال الأوامر ← جديد

#endif