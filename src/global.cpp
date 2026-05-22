#include "global.h"

String WIFI_SSID;
String WIFI_PASS;
String CORE_IOT_TOKEN;
String CORE_IOT_SERVER;
String CORE_IOT_PORT;

String ssid = "ESP32-YOUR NETWORK HERE!!!";
String password = "12345678";
String wifi_ssid = "abcde";
String wifi_password = "123456789";
boolean isWifiConnected = false;
SemaphoreHandle_t xBinarySemaphoreInternet = NULL;
QueueHandle_t queueLED = NULL;
QueueHandle_t queueNeo = NULL;
QueueHandle_t dataQueue = NULL;
QueueHandle_t queueCoreIoT = NULL;
QueueHandle_t queueML = NULL;

