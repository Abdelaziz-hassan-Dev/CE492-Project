#include "rtos_queues.h"

QueueHandle_t telegramQueue = NULL;

void initQueues() {
    // Queue تحمل 10 رسائل كحد أقصى، كل رسالة 150 حرف
    telegramQueue = xQueueCreate(10, 150 * sizeof(char));
}