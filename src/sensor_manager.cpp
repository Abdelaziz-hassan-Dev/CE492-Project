#include "sensor_manager.h"

// Global sensor instances
static DHT dhtSensor(DHTPIN, DHTTYPE);
static bool sensorsInitialized = false;

void initSensors() {
    if (!sensorsInitialized) {
        pinMode(FLAME_PIN, INPUT);
       pinMode(GAS_PIN, INPUT);
        dhtSensor.begin();
        sensorsInitialized = true;
    }
}

float getRawTemperature() {
    return dhtSensor.readTemperature();
}

float getRawHumidity() {
    return dhtSensor.readHumidity();
}

bool isFlameDetected() {
    // Sensor output is Active LOW (LOW means fire detected)
    return (digitalRead(FLAME_PIN) == LOW); 
}

bool isGasDetected() {
    return (digitalRead(GAS_PIN) == LOW);  // Active LOW مثل الشعلة
}