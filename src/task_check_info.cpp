#include <Arduino.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include "global.h"

extern SystemState* appState;

extern void startAP(); 

void Load_info_File()
{
    File file = LittleFS.open("/info.dat", "r");
    if (!file) {
        return;
    }
    
    DynamicJsonDocument doc(1024);
    DeserializationError error = deserializeJson(doc, file);
    
    if (error) {
        Serial.println("❌ Lỗi giải mã file info.dat!");
    } else {
        if (xSemaphoreTake(appState->dataMutex, portMAX_DELAY) == pdTRUE) {
            // Đọc chuỗi từ JSON, nếu không có thì gán chuỗi rỗng ""
            appState->wifiSSID = doc["WIFI_SSID"] | "";
            appState->wifiPass = doc["WIFI_PASS"] | "";
            appState->coreIotToken = doc["CORE_IOT_TOKEN"] | "";
            appState->coreIotServer = doc["CORE_IOT_SERVER"] | "10.235.76.226";
            appState->coreIotPort = doc["CORE_IOT_PORT"] | "1883";
            xSemaphoreGive(appState->dataMutex);
        }
        Serial.println("Đã nạp cấu hình mạng thành công!");
    }
    file.close();
}

void Delete_info_File()
{
    if (LittleFS.exists("/info.dat")) {
        LittleFS.remove("/info.dat");
        Serial.println("Đã xóa cấu hình mạng!");
    }
    ESP.restart(); // Khởi động lại mạch
}

void Save_info_File(String wifi_ssid, String wifi_pass, String core_iot_token, String core_iot_server, String core_iot_port)
{
    DynamicJsonDocument doc(1024);
    doc["WIFI_SSID"] = wifi_ssid;
    doc["WIFI_PASS"] = wifi_pass;
    doc["CORE_IOT_TOKEN"] = core_iot_token;
    doc["CORE_IOT_SERVER"] = core_iot_server;
    doc["CORE_IOT_PORT"] = core_iot_port;

    File configFile = LittleFS.open("/info.dat", "w");
    if (configFile) {
        serializeJson(doc, configFile);
        configFile.close();
        Serial.println("Đã lưu cấu hình mới!");
    } else {
        Serial.println("❌ Lỗi: Không thể lưu cấu hình.");
    }
}

bool check_info_File(bool check)
{
    if (!check) {
        if (!LittleFS.begin(true)) {
            Serial.println("❌ Lỗi khởi động LittleFS!");
            return false;
        }
        Load_info_File();
    }
  
    String ssid = "";
    if (xSemaphoreTake(appState->dataMutex, portMAX_DELAY) == pdTRUE) {
        ssid = appState->wifiSSID;
        xSemaphoreGive(appState->dataMutex);
    }

    if (ssid.isEmpty()) {
        if (!check) {
            startAP(); 
        }
        return false;
    }
    return true;
}