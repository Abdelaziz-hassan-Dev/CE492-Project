#ifndef RTOS_QUEUES_H
#define RTOS_QUEUES_H

#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

// Queue تحمل رسائل التلقرام - الـ loop يكتب فيها، والـ Task يقرأ منها
extern QueueHandle_t telegramQueue;

void initQueues();

#endif