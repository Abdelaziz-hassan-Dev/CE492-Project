#include "rtos_queues.h"

QueueHandle_t telegramQueue = NULL;
QueueHandle_t loggingQueue  = NULL;  // ← جديد

void initQueues() {
    telegramQueue = xQueueCreate(10, 150 * sizeof(char));
    loggingQueue  = xQueueCreate(5, sizeof(SensorLog_t)); // ← جديد
}