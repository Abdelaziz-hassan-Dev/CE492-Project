#ifndef TELEGRAM_MANAGER_H
#define TELEGRAM_MANAGER_H

#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <ArduinoJson.h>
#include "config.h" // ملف config.h يحتوي بالفعل على CURRENT_VERSION

// متغيرات الـ Thresholds قابلة للتعديل
extern float currentTempThreshold;
extern float currentHumThreshold;

void initTelegram();
void checkSystemConditions(float temp, float hum, bool flame);
void sendTelegramMessage(String message);
void queueTelegramMessage(String message);
void telegramTask(void* parameter); 

void handleNewMessages(int numNewMessages);

#endif