#ifndef OTA_MANAGER_H
#define OTA_MANAGER_H

#include <Arduino.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <Update.h>
#include <ArduinoJson.h> // موجودة مسبقاً في مشروعك
#include "config.h"

// يقوم بالتحقق من وجود نسخة جديدة وتحميلها إذا وجدت
void checkForUpdates();

// دالة داخلية للقيام بعملية التحديث الفعلي
void performUpdate(String binUrl);

#endif