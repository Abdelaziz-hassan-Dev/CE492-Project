#include "ota_manager.h"

void checkForUpdates() {
    if (WiFi.status() != WL_CONNECTED) return;

    WiFiClientSecure client;
    client.setInsecure(); // لتجاوز فحص شهادة SSL من GitHub للتبسيط

    HTTPClient http;
    http.begin(client, VERSION_JSON_URL);
    
    int httpCode = http.GET();
    if (httpCode == HTTP_CODE_OK) {
        String payload = http.getString();
        //StaticJsonDocument<256> doc;
        JsonDocument doc; // 
        deserializeJson(doc, payload);

        String newVersion = doc["version"];
        String binUrl = doc["url"];

        Serial.print("Current Version: "); Serial.println(CURRENT_VERSION);
        Serial.print("New Version available: "); Serial.println(newVersion);

        if (newVersion != CURRENT_VERSION) {
            Serial.println("Update found! Starting OTA...");
            performUpdate(binUrl);
        } else {
            Serial.println("System is up to date.");
        }
    } else {
        Serial.printf("Failed to check version, error: %s\n", http.errorToString(httpCode).c_str());
    }
    http.end();
}

void performUpdate(String binUrl) {
    WiFiClientSecure client;
    client.setInsecure();
    HTTPClient http;

    Serial.println("Downloading Binary...");
    http.begin(client, binUrl);
    // ضروري جداً لأن GitHub يقوم بعمل Redirect لملفات الـ Bin
    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS); 

    int httpCode = http.GET();
    if (httpCode == HTTP_CODE_OK) {
        int contentLength = http.getSize();
        bool canBegin = Update.begin(contentLength);

        if (canBegin) {
            Serial.println("Begin OTA update...");
            WiFiClient* clientRef = http.getStreamPtr();
            size_t written = Update.writeStream(*clientRef);

            if (written == contentLength) {
                Serial.println("Written : " + String(written) + " successfully");
            }

            if (Update.end()) {
                if (Update.isFinished()) {
                    Serial.println("Update successfully completed. Rebooting...");
                    ESP.restart();
                }
            } else {
                Serial.println("Error Occurred. Error #: " + String(Update.getError()));
            }
        } else {
            Serial.println("Not enough space to begin OTA");
        }
    }
    http.end();
}