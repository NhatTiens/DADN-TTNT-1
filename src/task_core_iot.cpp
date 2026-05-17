#include "task_core_iot.h"
#include "global.h"


constexpr uint32_t MAX_MESSAGE_SIZE = 1024U;
WiFiClient wifiClient;
Arduino_MQTT_Client mqttClient(wifiClient);
ThingsBoard tb(mqttClient, MAX_MESSAGE_SIZE);


RPC_Response setLedSwitchValue(const RPC_Data &data)
{
    Serial.println("Nhận lệnh từ ThingsBoard Dashboard");
    bool newState = data;
    
    // Xin Mutex kiểm tra chế độ Auto, nếu đang Manual thì mới cho phép điều khiển
    if (xSemaphoreTake(appState->dataMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        if (!appState->isAutoMode) {
            appState->ledState = newState;
            Serial.printf("Đã đổi trạng thái LED thành: %s\n", newState ? "ON" : "OFF");
        } else {
            Serial.println("Hệ thống đang AUTO, từ chối lệnh từ ThingsBoard!");
        }
        xSemaphoreGive(appState->dataMutex);
    }
    
    return RPC_Response("setLedSwitchValue", newState);
}

const std::array<RPC_Callback, 1U> callbacks = {
    RPC_Callback{"setLedSwitchValue", setLedSwitchValue}
};

void CORE_IOT_reconnect()
{
    if (!tb.connected())
    {
        String server = "";
        String token = "";
        String port = "";

        // Đọc cấu hình từ SystemState
        if (xSemaphoreTake(appState->dataMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
            server = appState->coreIotServer;
            token = appState->coreIotToken;
            port = appState->coreIotPort;
            xSemaphoreGive(appState->dataMutex);
        }

        if (server.isEmpty() || token.isEmpty()) return;

        Serial.printf("Đang kết nối ThingsBoard: %s\n", server.c_str());

        if (!tb.connect(server.c_str(), token.c_str(), port.toInt())) {
            Serial.println("Kết nối ThingsBoard thất bại!");
            return;
        }

        Serial.println("Đã kết nối ThingsBoard thành công!");
        
        tb.RPC_Subscribe(callbacks.cbegin(), callbacks.cend());
    }
}

void coreiot_task(void *pvParameters)
{
    SystemState *appState = (SystemState *)pvParameters;
    TickType_t lastSendTime = xTaskGetTickCount();

    while (1)
    {
        xSemaphoreTake(appState->internetReadySync, portMAX_DELAY);
        
        while (WiFi.status() == WL_CONNECTED) {
            if (!tb.connected()) {
                CORE_IOT_reconnect();
            } 
            else {
                tb.loop(); 
                if (xTaskGetTickCount() - lastSendTime > pdMS_TO_TICKS(10000)) {
                    float t = 0, h = 0; bool aiAnomaly = false;

                    if (xSemaphoreTake(appState->dataMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
                        t = appState->temperature; h = appState->humidity;
                        aiAnomaly = appState->tinyML_Anomaly;
                        xSemaphoreGive(appState->dataMutex);
                    }

                    tb.sendTelemetryData("temperature", t);
                    tb.sendTelemetryData("humidity", h);
                    tb.sendTelemetryData("anomaly", aiAnomaly ? 1 : 0); 
                    
                    lastSendTime = xTaskGetTickCount();
                }
            }
            vTaskDelay(pdMS_TO_TICKS(100));
        }
    }
}