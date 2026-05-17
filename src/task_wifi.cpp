#include "task_wifi.h"
#include "global.h"
#include <WiFi.h>

const char *MY_AP_SSID = "YOLO_UNO_IOT";
const char *MY_AP_PASS = "12345678";

void startAP() {
    WiFi.softAP(MY_AP_SSID, MY_AP_PASS);
    Serial.printf("Phat WiFi (AP Mode). IP: %s\n", WiFi.softAPIP().toString().c_str());
}

void wifi_task(void *pvParameters) {
    SystemState *appState = (SystemState *)pvParameters;
    
    while (1) {
        String ssid = "";
        String pass = "";

        if (xSemaphoreTake(appState->dataMutex, portMAX_DELAY) == pdTRUE) {
            ssid = appState->wifiSSID;
            pass = appState->wifiPass;
            xSemaphoreGive(appState->dataMutex);
        }

        if (ssid.isEmpty() || WiFi.status() == WL_CONNECTED) {
            vTaskDelay(pdMS_TO_TICKS(2000));
            continue;
        }

        Serial.printf("Đang kết nối với Wifi: %s\n", ssid.c_str());
        WiFi.begin(ssid.c_str(), pass.isEmpty() ? NULL : pass.c_str());

        int attempts = 0;
        while (WiFi.status() != WL_CONNECTED && attempts < 20) {
            vTaskDelay(pdMS_TO_TICKS(500));
            Serial.print(".");
            attempts++;
        }

        if (WiFi.status() == WL_CONNECTED) {
            Serial.printf("\nĐã kết nối WiFi thành công!\nĐịa chỉ IP Local: %s\n", WiFi.localIP().toString().c_str());
            
            if (xSemaphoreTake(appState->dataMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
                appState->hasInternet = true;
                xSemaphoreGive(appState->dataMutex);
            }
            
            xSemaphoreGive(appState->internetReadySync);
        } else {
            Serial.println("\nKết nối thất bại! Phát lại AP Mode.");
            startAP();
            vTaskDelay(pdMS_TO_TICKS(10000));
        }
    }
}