#ifndef __GLOBAL_H__
#define __GLOBAL_H__

#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

struct SystemState {
    float temperature;   // Biến lưu giá trị nhiệt độ
    float humidity;      // Biến lưu giá trị độ ẩm

    int displayState;   // Biến trạng thái hiển thị LCD
                        // 0: Normal, 1: Warning , 2: Critical

    bool tinyML_Anomaly;    // Biến dùng để lưu kết quả suy luận của TinyML

    bool isAutoMode;        // Chế độ Auto (true) hay Manual (false)
    bool ledState;          // Trạng thái Bật/Tắt của LED
    bool neoState;          // Trạng thái Bật/Tắt của NeoPixel
    bool fanState;          // Trạng thái Bật/Tắt của Quạt

    String wifiSSID;        // Tên Wi-Fi
    String wifiPass;        // Mật khẩu Wi-Fi
    String coreIotToken;    // Token của CoreIOT
    String coreIotServer;   // Server CoreIOT
    String coreIotPort;     // Cổng CoreIOT
    bool hasInternet;       // Cờ báo hiệu có Internet hay chưa
    bool needsRestart;

    SemaphoreHandle_t dataMutex;     // Mutex bảo vệ dữ liệu chung
    SemaphoreHandle_t sensorReadySync;
    SemaphoreHandle_t humiReadySync;
    SemaphoreHandle_t lcdReadySync;
    SemaphoreHandle_t mlReadySync;
    SemaphoreHandle_t internetReadySync;
};

extern SystemState* appState;

#endif
