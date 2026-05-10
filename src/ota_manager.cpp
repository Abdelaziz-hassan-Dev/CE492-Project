#include "ota_manager.h"
#include "telegram_manager.h"


void checkForUpdates() {
    if (WiFi.status() != WL_CONNECTED) return;

    WiFiClientSecure client;
    client.setInsecure(); 

    HTTPClient http;
    http.begin(client, VERSION_JSON_URL);
    
    int httpCode = http.GET();
    if (httpCode == HTTP_CODE_OK) {
        String payload = http.getString();
        DynamicJsonDocument doc(256);
        deserializeJson(doc, payload);

        String newVersion = doc["version"];
        String binUrl = doc["url"];

        if (newVersion != CURRENT_VERSION) {
            // 1. أرسل الرسالة أولاً لتكون على علم بالبدء
           queueTelegramMessage(" New update available: " + newVersion + "\nStarting download now...");
            
            // 2. ابدأ التحديث
            performUpdate(binUrl);
     //       queueTelegramMessage(" system updated to: " + newVersion );
        } else {
            queueTelegramMessage(" system is up to data" );
        }
    } else {
        // نصيحة: أرسل رسالة في حال فشل الاتصال بالسيرفر لتتمكن من المتابعة بدون سيريال
        queueTelegramMessage("❌ Failed to check for updates. Error code: " + String(httpCode));
    }
    http.end();
}

void performUpdate(String binUrl) {
    WiFiClientSecure client;
    client.setInsecure();
    HTTPClient http;

    http.begin(client, binUrl);
    // ضروري جداً لأن GitHub يقوم بعمل Redirect لملفات الـ Bin
    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS); 

    int httpCode = http.GET();
    if (httpCode == HTTP_CODE_OK) {
        int contentLength = http.getSize();
        bool canBegin = Update.begin(contentLength);

        if (canBegin) {
            WiFiClient* clientRef = http.getStreamPtr();
            size_t written = Update.writeStream(*clientRef);

            if (Update.end()) {
                if (Update.isFinished()) {
                    ESP.restart();
                }
            } 
        }
    }
    http.end();
}