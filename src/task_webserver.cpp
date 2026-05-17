#include "task_webserver.h"
#include "global.h"
#include <ArduinoJson.h>
#include <LittleFS.h>

#include "task_handler.h"
#include "task_check_info.h"

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

bool webserver_isrunning = false;

void webserver_push_task(void *pvParameters) {
  SystemState *state = (SystemState *)pvParameters;
  while (1) {
    if (webserver_isrunning && ws.count() > 0 && state != nullptr) {
      float temp = 0.0f, humi = 0.0f;
      int sysStatus = 0;
      bool aiStatus = false, autoMode = true, led = false, neo = false,
           fan = false;

      if (xSemaphoreTake(state->dataMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        temp = state->temperature;
        humi = state->humidity;
        sysStatus = state->displayState;
        aiStatus = state->tinyML_Anomaly;
        autoMode = state->isAutoMode;
        led = state->ledState;
        neo = state->neoState;
        fan = state->fanState;
        xSemaphoreGive(state->dataMutex);
      }

      StaticJsonDocument<256> doc;
      doc["type"] = "update";
      doc["temp"] = temp;
      doc["humi"] = humi;
      doc["sys"] = sysStatus;
      doc["ai"] = aiStatus;
      doc["auto"] = autoMode;
      doc["led"] = led;
      doc["neo"] = neo;
      doc["fan"] = fan;

      String payload;
      serializeJson(doc, payload);
      ws.textAll(payload);
    }
    vTaskDelay(pdMS_TO_TICKS(2000));
  }
}

void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client,
             AwsEventType type, void *arg, uint8_t *data, size_t len) {
  if (type == WS_EVT_CONNECT)
    Serial.printf("WebSocket client #%u connected\n", client->id());
  else if (type == WS_EVT_DISCONNECT)
    Serial.printf("WebSocket client #%u disconnected\n", client->id());
  else if (type == WS_EVT_DATA) {
    AwsFrameInfo *info = (AwsFrameInfo *)arg;
    if (info->opcode == WS_TEXT) {
      String message = "";
      for (size_t i = 0; i < len; i++)
        message += (char)data[i];
      handleWebSocketMessage(message);
    }
  }
}

void connnectWSV() {
  ws.onEvent(onEvent);
  server.addHandler(&ws);

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(LittleFS, "/index.html", "text/html");
  });
  server.on("/script.js", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(LittleFS, "/script.js", "application/javascript");
  });
  server.on("/styles.css", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(LittleFS, "/styles.css", "text/css");
  });

  server.on("/save", HTTP_POST, [](AsyncWebServerRequest *request) {
    // Cập nhật dữ liệu vào biến toàn cục appState
    if (request->hasParam("ssid", true))
      appState->wifiSSID = request->getParam("ssid", true)->value();
    if (request->hasParam("pass", true))
      appState->wifiPass = request->getParam("pass", true)->value();
    if (request->hasParam("token", true))
      appState->coreIotToken = request->getParam("token", true)->value();
    if (request->hasParam("server", true))
      appState->coreIotServer = request->getParam("server", true)->value();
    if (request->hasParam("port", true))
      appState->coreIotPort = request->getParam("port", true)->value();

    Save_info_File(appState->wifiSSID, appState->wifiPass,
                   appState->coreIotToken, appState->coreIotServer,
                   appState->coreIotPort);

    request->send(200, "text/plain", "OK");

    Serial.println(
        ">>> Đã nhận lệnh lưu. Hệ thống sẽ khởi động lại từ loop()...");
    appState->needsRestart = true;
  });

  server.begin();
  ElegantOTA.begin(&server);
  webserver_isrunning = true;
  Serial.println("AsyncWebServer đã khởi động thành công!");
}

void Webserver_stop() {
  ws.closeAll();
  server.end();
  webserver_isrunning = false;
}

void Webserver_reconnect() {
  if (!webserver_isrunning) {
    connnectWSV();
  }
  ElegantOTA.loop();
}