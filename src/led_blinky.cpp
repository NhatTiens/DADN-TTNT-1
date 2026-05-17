#include "led_blinky.h"

void led_blinky(void *pvParameters) {
  SystemState *state = (SystemState *)pvParameters;
  pinMode(LED_GPIO, OUTPUT);

  bool localLedState = false;
  bool isAuto = true;
  int localDispState = 0;
  bool blinkState = false;

  TickType_t blinkDelay = pdMS_TO_TICKS(5000);
  TickType_t lastToggleTime = xTaskGetTickCount();

  while (1) {
    if (xSemaphoreTake(state->dataMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
      isAuto = state->isAutoMode;
      localLedState = state->ledState;
      localDispState = state->displayState;
      xSemaphoreGive(state->dataMutex);
    }

    if (localDispState == 2) {
        blinkDelay = pdMS_TO_TICKS(200); // Critical 0.2s
    } else if (localDispState == 1) {
        blinkDelay = pdMS_TO_TICKS(2000); // Warning 2s
    } else {
        blinkDelay = pdMS_TO_TICKS(5000); // Normal 5s
    }

    // MANUAL: Bật/Tắt theo Web. Nếu đang bật, cho nháy theo trạng thái hiện tại.
    if (!isAuto) {
      if (!localLedState) {
          digitalWrite(LED_GPIO, LOW);
          blinkState = false;
          vTaskDelay(pdMS_TO_TICKS(100));
          lastToggleTime = xTaskGetTickCount();
          continue;
      }
    }

    // AUTO hoặc MANUAL (khi đã BẬT): Nháy đèn
    TickType_t now = xTaskGetTickCount();
    if (now - lastToggleTime >= blinkDelay) {
      blinkState = !blinkState;
      digitalWrite(LED_GPIO, blinkState ? HIGH : LOW);
      lastToggleTime = now;
    }

    vTaskDelay(pdMS_TO_TICKS(50));
  }
}