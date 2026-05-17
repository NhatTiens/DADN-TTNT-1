#ifndef __GLOBAL_H__
#define __GLOBAL_H__

#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

extern String WIFI_SSID;
extern String WIFI_PASS;
extern String CORE_IOT_TOKEN;
extern String CORE_IOT_SERVER;
extern String CORE_IOT_PORT;

extern boolean isWifiConnected;
extern SemaphoreHandle_t xBinarySemaphoreInternet;

typedef struct {
    float temp;
    float hum;
} SensorData;
extern QueueHandle_t queueLED;
extern QueueHandle_t queueNeo;
extern QueueHandle_t dataQueue;
extern QueueHandle_t queueCoreIoT;
extern QueueHandle_t queueML;

extern bool tinyML_Anomaly;
#endif
