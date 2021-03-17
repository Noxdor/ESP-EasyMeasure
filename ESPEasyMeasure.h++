#include <WiFi.h>
#include <esp_wifi.h>
#include <ESPAsyncWebServer.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <SPIFFS.h>
#include <ErriezDS1302.h>

//DS1302 RTC Module
#ifdef(ARDUINO_ARCH_ESP32)
#define DS1302_CLK_PIN 16
#define DS1302_IO_PIN 17
#define DS1302_CE_PIN 5
#endif
//RTC (DS1302)
DS1302 rtc = DS1302(DS1302_CLK_PIN, DS1302_IO_PIN, DS1302_CE_PIN);
DS1302_DateTime dt;
static void printDateTime(File log);

//Dual Core
TaskHandle_t Blink;

//Error Codes
#define RTC_ERROR 3
#define FS_ERROR 7

//Sensor
#define LED_PIN 2
#define SENSOR_PIN 4 // D4
DHT *sensor_dht;
float last_temp = 0.00;
float last_hum = 0.00;