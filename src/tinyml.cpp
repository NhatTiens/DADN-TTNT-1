#include "tinyml.h"
#include "global.h" 

const float TEMP_MEAN = 26.30f; 
const float TEMP_STD = 5.95f;
const float HUMI_MEAN = 62.80f; 
const float HUMI_STD = 17.26f;

namespace {
    tflite::ErrorReporter *error_reporter = nullptr;
    const tflite::Model *model = nullptr;
    tflite::MicroInterpreter *interpreter = nullptr;
    TfLiteTensor *input = nullptr;
    TfLiteTensor *output = nullptr;
    constexpr int kTensorArenaSize = 8 * 1024; 
    uint8_t tensor_arena[kTensorArenaSize];
}

void setupTinyML() {
    Serial.println("[TinyML] Dang khoi tao TensorFlow Lite Micro...");
    static tflite::MicroErrorReporter micro_error_reporter;
    error_reporter = &micro_error_reporter;

    model = tflite::GetModel(dht_anomaly_model_tflite); 
    if (model->version() != TFLITE_SCHEMA_VERSION) {
        Serial.println("[TinyML ERROR] Model version mismatch!");
        return;
    }

    static tflite::AllOpsResolver resolver;
    static tflite::MicroInterpreter static_interpreter(
        model, resolver, tensor_arena, kTensorArenaSize, error_reporter);
    interpreter = &static_interpreter;

    if (interpreter->AllocateTensors() != kTfLiteOk) {
        Serial.println("[TinyML ERROR] AllocateTensors() failed!");
        return;
    }

    input = interpreter->input(0);
    output = interpreter->output(0);
}

void tiny_ml_task(void *pvParameters) {
    setupTinyML();

    if (interpreter == nullptr) {
        Serial.println("[TinyML] Setup failed. Task stopping.");
        vTaskDelete(NULL); 
        return;
    }

    vTaskDelay(pdMS_TO_TICKS(2000)); 

    while (1) {
        float currentTemp = glob_temperature;
        float currentHumi = glob_humidity;

        input->data.f[0] = (currentTemp - TEMP_MEAN) / TEMP_STD;
        input->data.f[1] = (currentHumi - HUMI_MEAN) / HUMI_STD;

        unsigned long startTime = millis(); 
        
        if (interpreter->Invoke() == kTfLiteOk) {
            unsigned long inferTime = millis() - startTime; 
            
            float anomaly_score = output->data.f[0];
            bool isAnomaly = (anomaly_score >= 0.7f); 
            
            tinyML_Anomaly = isAnomaly;

            Serial.printf("[TinyML] T:%.1f H:%.1f | Score: %.3f | %s | %lu ms\n", 
                            currentTemp, currentHumi, anomaly_score, 
                            isAnomaly ? "WARNING (ANOMALY)" : "NORMAL", inferTime);
        } else {
            Serial.println("[TinyML ERROR] Invoke failed");
        }

        vTaskDelay(pdMS_TO_TICKS(2000)); 
    }
}