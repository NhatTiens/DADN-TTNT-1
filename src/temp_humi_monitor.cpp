#include "temp_humi_monitor.h"
#include "global.h"
#include <DHT20.h>

// Thu thập dữ liệu cho file csv
// void temp_humi_monitor(void *pvParameters){
//     SystemState *state = (SystemState *)pvParameters;
//     DHT20 dht20;
//     dht20.begin();
//     vTaskDelay(pdMS_TO_TICKS(800));

//     float tempHistory[5] = {0};
//     float humiHistory[5] = {0};
//     uint8_t sampleCount = 0;
//     uint8_t historyIdx = 0;

//     Serial.println("\n# BẮT ĐẦU THU THẬP DỮ LIỆU TỰ ĐỘNG");
//     Serial.println("avg_temp,avg_humi,anomaly"); // In sẵn Header của CSV để

//     while (1) {
//         // ĐÃ XÓA BỎ PHẦN NHẬP BÀN PHÍM (0, 1) VÌ KHÔNG CẦN THIẾT NỮA

//         if (dht20.read() == 0) {
//             float rawT = dht20.getTemperature();
//             float rawH = dht20.getHumidity();

//             // Tính trung bình để dữ liệu CSV mượt, không bị nhiễu
//             tempHistory[historyIdx] = rawT;
//             humiHistory[historyIdx] = rawH;
//             historyIdx = (historyIdx + 1) % 5;
//             if (sampleCount < 5) sampleCount++;

//             float avgT = 0, avgH = 0;
//             for (int i = 0; i < sampleCount; i++) {
//                 avgT += tempHistory[i];
//                 avgH += humiHistory[i];
//             }
//             avgT /= sampleCount; avgH /= sampleCount;

//             // Tính toán THI để làm nhãn tự động
//             float THI = 0.8f * avgT + (avgH / 100.0f) * (avgT - 14.4f)
//             + 46.4f;

//             // Nếu THI >= 72 thì nhãn là 1 (Bất thường), ngược lại là 0
//             int auto_label = (THI >= 72.0f) ? 1 : 0;

//             // In ra dòng CSV chuẩn
//             Serial.printf("%.1f,%.1f,%d\n", avgT, avgH, auto_label);

//             if (xSemaphoreTake(state->dataMutex, pdMS_TO_TICKS(200)) ==
//             pdTRUE) {
//                 state->temperature = avgT;
//                 state->humidity    = avgH;

//                 // 2. Dùng THI để chia trạng thái cho LED/LCD ngay lúc thu
//                 if (THI >= 90.0f) {
//                     state->displayState = 2; // Critical
//                 }
//                 else if (THI >= 72.0f) {
//                     state->displayState = 1; // Warning
//                 }
//                 else {
//                     state->displayState = 0; // Normal
//                 }

//                 xSemaphoreGive(state->dataMutex);

//                 xSemaphoreGive(state->sensorReadySync);
//                 xSemaphoreGive(state->humiReadySync);
//                 xSemaphoreGive(state->lcdReadySync);
//                 xSemaphoreGive(state->mlReadySync); // Cấp thêm cho TinyML
//             }
//         }

//         vTaskDelay(pdMS_TO_TICKS(2000)); // Thu thập 2 giây 1 mẫu
//     }
// }

void temp_humi_monitor(void *pvParameters) {
  SystemState *state = (SystemState *)pvParameters;
  DHT20 dht20;
  dht20.begin();
  vTaskDelay(pdMS_TO_TICKS(800));

  float tempHistory[5] = {0};
  float humiHistory[5] = {0};
  uint8_t sampleCount = 0;
  uint8_t historyIdx = 0;
  uint8_t failCount = 0;

  while (1) {
    if (dht20.read() == 0) {
      failCount = 0;
      float rawT = dht20.getTemperature();
      float rawH = dht20.getHumidity();

      // Lọc nhiễu cảm biến
      tempHistory[historyIdx] = rawT;
      humiHistory[historyIdx] = rawH;
      historyIdx = (historyIdx + 1) % 5;
      if (sampleCount < 5)
        sampleCount++;

      float avgT = 0, avgH = 0;
      for (int i = 0; i < sampleCount; i++) {
        avgT += tempHistory[i];
        avgH += humiHistory[i];
      }
      avgT /= sampleCount;
      avgH /= sampleCount;

      if (xSemaphoreTake(state->dataMutex, pdMS_TO_TICKS(200)) == pdTRUE) {
        state->temperature = avgT;
        state->humidity = avgH;

        // Tính chỉ số THI (Thom, 1959) làm thước đo duy nhất cho toàn hệ thống
        float THI = 0.8f * avgT + (avgH / 100.0f) * (avgT - 14.4f) + 46.4f;

        // Xác định trạng thái vật lý theo ngưỡng THI
        int physicalState = 0;
        if (THI >= 90.0f)
          physicalState = 2; // Nguy hiểm
        else if (THI >= 72.0f)
          physicalState = 1; // Cảnh báo

        // Tổng hợp kết quả: Vật lý + AI
        if (physicalState == 2) {
          state->displayState = 2; // Ưu tiên Nguy hiểm
        } else if (physicalState == 1 || state->tinyML_Anomaly == true) {
          state->displayState =
              1; // Cảnh báo nếu THI cao hoặc AI phát hiện bất thường
        } else {
          state->displayState = 0; // Bình thường
        }

        // TỰ ĐỘNG BẬT QUẠT NẾU TRẠNG THÁI LÀ CRITICAL 
        if (state->isAutoMode) {
          bool shouldFanOn = (state->displayState == 2);
          state->fanState = shouldFanOn;
          pinMode(6, OUTPUT);
          digitalWrite(6, shouldFanOn ? HIGH : LOW);
        }

        xSemaphoreGive(state->dataMutex);

        xSemaphoreGive(state->sensorReadySync);
        xSemaphoreGive(state->humiReadySync);
        xSemaphoreGive(state->lcdReadySync);
        xSemaphoreGive(state->mlReadySync);
      }
    } else {
      if (++failCount >= 3) {
        failCount = 0;
        xSemaphoreGive(state->sensorReadySync);
        xSemaphoreGive(state->humiReadySync);
        xSemaphoreGive(state->lcdReadySync);
        xSemaphoreGive(state->mlReadySync);
      }
    }

    vTaskDelay(pdMS_TO_TICKS(5000));
  }
}