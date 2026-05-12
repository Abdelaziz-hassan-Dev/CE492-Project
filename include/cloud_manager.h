#ifndef CLOUD_MANAGER_H
#define CLOUD_MANAGER_H

#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include "config.h"

void initCloud();
void logDataToGoogleSheet(float temp, float hum, bool flameStatus, bool gas);
void loggingTask(void* parameter);

#endif