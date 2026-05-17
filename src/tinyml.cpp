#include "tinyml.h"

namespace {
    tflite::ErrorReporter *error_reporter = nullptr;
    const tflite::Model *model = nullptr;
    tflite::MicroInterpreter *interpreter = nullptr;
    TfLiteTensor *input = nullptr;
    TfLiteTensor *output = nullptr;
    constexpr int kTensorArenaSize = 8 * 1024; 
    uint8_t tensor_arena[kTensorArenaSize];
}

// Hàm khởi tạo mô hình AI 
void setupTinyML() {
    Serial.println("[TinyML] Dang khoi tao TensorFlow Lite Micro...");
    static tflite::MicroErrorReporter micro_error_reporter;
    error_reporter = &micro_error_reporter;

    model = tflite::GetModel(dht_anomaly_model_tflite); // Tải mô hình đã train từ mảng byte
    if (model->version() != TFLITE_SCHEMA_VERSION) {
        Serial.println("[TinyML ERROR] Model version mismatch!");
        return;
    }

    static tflite::AllOpsResolver resolver;
    static tflite::MicroInterpreter static_interpreter(
        model, resolver, tensor_arena, kTensorArenaSize, error_reporter);
    interpreter = &static_interpreter;

    // Cấp phát vùng nhớ. 
    if (interpreter->AllocateTensors() != kTfLiteOk) {
        Serial.println("[TinyML ERROR] AllocateTensors() failed!");
        return;
    }

    // Ánh xạ con trỏ tới cổng Đầu vào (Input) và Đầu ra (Output) của mô hình
    input = interpreter->input(0);
    output = interpreter->output(0);
}

// Luồng xử lý chính của AI 
void tiny_ml_task(void *pvParameters) {
    SystemState *state = (SystemState *)pvParameters;
    setupTinyML();

    // Failsafe: Tự hủy Task nếu khởi tạo AI lỗi để tránh làm sập toàn bộ hệ thống 
    if (interpreter == nullptr) {
        Serial.println("[TinyML] Setup failed. Task stopping.");
        vTaskDelete(NULL); 
        return;
    }

    while (1) {
        // Chỉ thức dậy khi nhận được Semaphore từ Task Cảm biến
        if (xSemaphoreTake(state->mlReadySync, portMAX_DELAY) == pdTRUE) {
            
            float currentTemp = 0.0f;
            float currentHumi = 0.0f;
            bool dataOk = false;

            // Xin khóa Mutex để đọc dữ liệu cảm biến an toàn
            if (xSemaphoreTake(state->dataMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
                currentTemp = state->temperature;
                currentHumi = state->humidity;
                xSemaphoreGive(state->dataMutex);
                dataOk = true;
            }

            if (!dataOk) continue; // Bỏ qua nhịp suy luận này nếu không lấy được khóa

            const float TEMP_MEAN = 26.30f; const float TEMP_STD = 5.95f;
            const float HUMI_MEAN = 62.80f; const float HUMI_STD = 17.26f;

            // Đưa vào Input cho AI (chỉ 2 thông số)
            input->data.f[0] = (currentTemp - TEMP_MEAN) / TEMP_STD;
            input->data.f[1] = (currentHumi - HUMI_MEAN) / HUMI_STD;

            unsigned long startTime = millis(); // Bắt đầu tính giờ suy luận
            
            // Gọi AI chạy tính toán 
            if (interpreter->Invoke() == kTfLiteOk) {
                unsigned long inferTime = millis() - startTime; // Tính thời gian hoàn thành
                
                float anomaly_score = output->data.f[0];
                bool isAnomaly = (anomaly_score >= 0.7f); // Ngưỡng quyết định: >= 0.7 là Bất thường

                // Cập nhật kết quả cảnh báo vào Struct an toàn để NeoPixel chớp đỏ
                if (xSemaphoreTake(state->dataMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
                    state->tinyML_Anomaly = isAnomaly; 
                    xSemaphoreGive(state->dataMutex);
                }

                Serial.printf("[TinyML] T:%.1f H:%.1f | Score: %.3f | %s | %lu ms\n", 
                              currentTemp, currentHumi, anomaly_score, 
                              isAnomaly ? "WARNING (ANOMALY)" : "NORMAL", inferTime);
            } else {
                Serial.println("[TinyML ERROR] Invoke failed");
            }
        }
    }
}