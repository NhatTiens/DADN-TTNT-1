#include <Arduino.h>
#include <Wire.h>
#include <LittleFS.h>
#include "global.h"

#include "led_blinky.h"
#include "neo_blinky.h"
#include "temp_humi_monitor.h"
#include "lcd_display.h"
#include "tinyml.h"

#include "task_check_info.h"
#include "task_toogle_boot.h"
#include "task_wifi.h"
#include "task_webserver.h"
#include "task_core_iot.h"

SystemState* appState = nullptr;
extern void webserver_push_task(void *pvParameters);

void setup() {
    Serial.begin(115200);
    delay(3000); 
    Serial.println("\n--- Thiết bị đang khởi động ---");

    Wire.begin(11, 12); 

    if (!LittleFS.begin(true)) {
        Serial.println("Lỗi khởi động LittleFS!");
    } else {
        Serial.println("LittleFS đã sẵn sàng.");
    }
    
    appState = new SystemState();
    appState->temperature = 0.0; appState->humidity = 0.0;
    appState->displayState = 0; appState->tinyML_Anomaly = false;
    appState->isAutoMode = true; appState->ledState = false; appState->neoState = false; appState->fanState = false;
    appState->wifiSSID = ""; appState->wifiPass = "";         
    appState->coreIotToken = ""; appState->coreIotServer = "app.coreiot.io"; 
    appState->coreIotPort = "1883"; appState->hasInternet = false;
    appState->needsRestart = false; 

    appState->dataMutex = xSemaphoreCreateMutex();
    appState->sensorReadySync = xSemaphoreCreateBinary();
    appState->humiReadySync = xSemaphoreCreateBinary();
    appState->lcdReadySync = xSemaphoreCreateBinary();
    appState->mlReadySync = xSemaphoreCreateBinary();
    appState->internetReadySync = xSemaphoreCreateBinary();

    Load_info_File(); 

    Serial.printf("SSID đã lưu: [%s]\n", appState->wifiSSID.c_str());
    Serial.printf("Token đã lưu: [%s]\n", appState->coreIotToken.c_str());

    WiFi.mode(WIFI_AP_STA);
    startAP();     
    connnectWSV(); 

    xTaskCreate(temp_humi_monitor, "Task DHT", 4096, (void*)appState, 3, NULL);
    xTaskCreate(tiny_ml_task, "Tiny ML Task", 8192, (void*)appState, 2, NULL);
    xTaskCreate(lcd_display_task, "Task LCD", 4096, (void*)appState, 2, NULL);
    
    xTaskCreate(wifi_task, "WiFi Task", 4096, (void*)appState, 3, NULL);
    xTaskCreate(coreiot_task, "CoreIOT Task", 4096, (void*)appState, 2, NULL);
    
    xTaskCreate(led_blinky, "Task LED", 2048, (void*)appState, 1, NULL);
    xTaskCreate(neo_blinky, "Task NEO", 2048, (void*)appState, 1, NULL);
    xTaskCreate(webserver_push_task, "Web Push Task", 4096, (void*)appState, 1, NULL);
    xTaskCreate(Task_Toogle_BOOT, "Task_Toogle_BOOT", 2048, NULL, 1, NULL);
}

void loop() {
    bool restart = false;
    if (xSemaphoreTake(appState->dataMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
        restart = appState->needsRestart;
        xSemaphoreGive(appState->dataMutex);
    }

    if (restart) {
        Serial.println("Chuẩn bị khởi động lại...");
        vTaskDelay(pdMS_TO_TICKS(1000)); 
        ESP.restart();
    }

    ElegantOTA.loop();
    vTaskDelay(pdMS_TO_TICKS(20)); 
}