#include "cloud_manager.h"
#include "rtos_queues.h"
#include "firebase_manager.h" 

WiFiClientSecure secureClient;

void initCloud() {
    // Allow insecure connection for Google Scripts (simplifies SSL handling)
    secureClient.setInsecure(); 
}

void logDataToGoogleSheet(float temp, float hum, bool flameStatus) {
    if (WiFi.status() != WL_CONNECTED) {
        return;
    }

    HTTPClient http;
    
    // Construct GET request URL with query parameters
    String url = String(G_SCRIPT_URL) + "?temp=" + String(temp, 1) + 
                 "&hum=" + String(hum, 1) + 
                 "&fire=" + (flameStatus ? "FIRE!" : "Safe");
    
    http.begin(secureClient, url); 
    
    // Essential for Google Apps Script as it redirects requests
    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
    
    int httpCode = http.GET(); 
    
    if (httpCode > 0) {
        String payload = http.getString();
    }
    
    http.end();
}

void loggingTask(void* parameter) {
    SensorLog_t data;

    for (;;) {
        // انتظر حتى تجي بيانات — blocking هنا مقبول لأننا في task منفصل
        if (xQueueReceive(loggingQueue, &data, portMAX_DELAY) == pdTRUE) {
            logDataToGoogleSheet(data.temp, data.hum, data.flame);

            String flameStr = data.flame ? "DETECTED" : "Safe";
            logHistoryToFirebase(data.temp, data.hum, flameStr);

        }
    }
}