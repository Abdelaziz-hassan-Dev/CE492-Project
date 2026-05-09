#ifndef RTOS_QUEUES_H
#define RTOS_QUEUES_H

#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

// Struct تحمل بيانات الحساسات للـ logging
typedef struct {
    float temp;
    float hum;
    bool flame;
} SensorLog_t;

extern QueueHandle_t telegramQueue;
extern QueueHandle_t loggingQueue;  // ← جديد

void initQueues();

#endif