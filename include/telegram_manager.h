#ifndef TELEGRAM_MANAGER_H
#define TELEGRAM_MANAGER_H

#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <ArduinoJson.h>
#include "config.h"

extern float g_tempThreshold;
extern float g_humThreshold;

void initTelegram();
void queueTelegramMessage(String message);
void checkSystemConditions(float temp, float hum, bool flame);
void telegramMainTask(void* parameter); // task واحد بس

#endif