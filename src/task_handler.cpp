#include "task_handler.h"
#include "global.h"
#include <ArduinoJson.h>

extern SystemState* appState;
extern void Save_info_File(String wifi_ssid, String wifi_pass, String CORE_IOT_TOKEN, String CORE_IOT_SERVER, String CORE_IOT_PORT);

#define FAN_PIN 6

void handleWebSocketMessage(String msg) {
    DynamicJsonDocument doc(1024);
    DeserializationError error = deserializeJson(doc, msg);
    if (error) {
        Serial.printf("JSON Parse Error: %s\n", error.c_str());
        return;
    }

    String page = doc["page"].as<String>();

    if (page == "device") {
        String dev = doc["value"]["device"].as<String>();
        bool status = doc["value"]["status"].as<bool>();

        if (xSemaphoreTake(appState->dataMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
            if (appState->isAutoMode == false) { 
                if (dev == "fan") {
                    appState->fanState = status;
                    pinMode(FAN_PIN, OUTPUT);
                    digitalWrite(FAN_PIN, status ? HIGH : LOW);
                    Serial.println(status ? "Web: Bật Quạt" : "Web: Tắt Quạt");
                } 
                else if (dev == "led") {
                    appState->ledState = status;
                    Serial.println(status ? "Web: Bật LED" : "Web: Tắt LED");
                }
                else if (dev == "neo") {
                    appState->neoState = status;
                    Serial.println(status ? "Web: Bật NEO" : "Web: Tắt NEO");
                }
            } else {
                Serial.println("Hệ thống đang ở chế độ AUTO!");
            }
            xSemaphoreGive(appState->dataMutex);
        }
    } 
    else if (page == "auto") {
        if (xSemaphoreTake(appState->dataMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
            appState->isAutoMode = doc["value"]["mode"].as<bool>();
            Serial.println(appState->isAutoMode ? "Chuyen sang AUTO" : "Chuyen sang MANUAL");
            xSemaphoreGive(appState->dataMutex);
        }
    } 
    else if (page == "setting") {
        String ssid = doc["value"]["ssid"].as<String>();
        String pass = doc["value"]["password"].as<String>();
        String token = doc["value"]["token"].as<String>();
        String server = doc["value"]["server"].as<String>();
        String port = doc["value"]["port"].as<String>();

        if (xSemaphoreTake(appState->dataMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
            appState->wifiSSID = ssid; appState->wifiPass = pass;
            appState->coreIotToken = token; appState->coreIotServer = server;
            appState->coreIotPort = port;
            
            appState->needsRestart = true;
            xSemaphoreGive(appState->dataMutex);
        }
        Save_info_File(ssid, pass, token, server, port);
    }
}