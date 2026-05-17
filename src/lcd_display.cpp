#include "lcd_display.h"
#include <LiquidCrystal_I2C.h>

void lcd_display_task(void *pvParameters) {
    SystemState *state = (SystemState *)pvParameters;
    
    LiquidCrystal_I2C lcd(33, 16, 2);
    lcd.begin();
    lcd.backlight();
    lcd.setCursor(0, 0);
    lcd.print("BTL IOT - HCMUT "); 
    lcd.setCursor(0, 1);
    lcd.print("                ");

    float localTemp = 0;
    float localHumi = 0;
    int localDispState = 0;
    bool localAnomaly = false;
    bool toggle = false; 

    while (1) {
        xSemaphoreTake(state->lcdReadySync, pdMS_TO_TICKS(6000));

        if (xSemaphoreTake(state->dataMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
            localTemp = state->temperature;
            localHumi = state->humidity;
            localDispState = state->displayState;
            localAnomaly = state->tinyML_Anomaly;
            xSemaphoreGive(state->dataMutex);
        }

        lcd.setCursor(0, 0);
        
        if (localDispState == 0) {
            lcd.printf("T:%4.1fC H:%2.0f%%   ", localTemp, localHumi);
            lcd.setCursor(0, 1);
            lcd.print("   STATUS:OK    "); 
            lcd.backlight();
        } 
        else if (localDispState == 1) {
            lcd.printf("T:%4.1fC H:%2.0f%%   ", localTemp, localHumi);
            lcd.setCursor(0, 1);
            lcd.printf("%s", localAnomaly ? " AI ANOMALY !!  " : "   WARNING !!   ");
            lcd.backlight();
        } 
        else {
            toggle = !toggle;
            if (toggle) {
                lcd.printf("T:%4.1fC H:%2.0f%%   ", localTemp, localHumi);
                lcd.setCursor(0, 1);
                lcd.print(" !! CRITICAL !! "); 
                lcd.backlight();
            } else {
                lcd.print("                ");
                lcd.setCursor(0, 1);
                lcd.print("                ");
                lcd.noBacklight(); 
            }
        }

        TickType_t renderDelay = (localDispState == 2) ? 250 : 500;
        vTaskDelay(pdMS_TO_TICKS(renderDelay));
    }
}