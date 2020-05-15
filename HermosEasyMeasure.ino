#if defined(ARDUINO_ARCH_ESP32)
#define DS1302_CLK_PIN 16
#define DS1302_IO_PIN 17
#define DS1302_CE_PIN 5
#endif

#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <SPIFFS.h>
#include <ErriezDS1302.h>

//Access Point and Server
const char *ssid = "HermosLogger";
const char *password = "HERMOSSYSTEMS";
const IPAddress apIP(192, 168, 1, 1);
AsyncWebServer server(80);
String last_adress = "/";

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

//RTC (DS1302)
DS1302 rtc = DS1302(DS1302_CLK_PIN, DS1302_IO_PIN, DS1302_CE_PIN);
DS1302_DateTime dt;
static void printDateTime(File log);

//Maximum byte size of one row of data: 34 Byte
//interval measure
float hours;
unsigned int sec_per_interval;
double next_measurement;
bool on = false;
String logfile_name = "/csv/log.csv";

void statusBlinking(void *parameter)
{
  for (;;)
  {
    digitalWrite(LED_PIN, HIGH);
    delay(1000);
    digitalWrite(LED_PIN, LOW);
    delay(3000);
  }
}

void setup()
{
  delay(5000);

  //Setup SPIFFS FS
  if (!SPIFFS.begin(true))
  {
    for (int i = 0; i < FS_ERROR; i++)
    {
      digitalWrite(LED_PIN, HIGH);
      delay(250);
      digitalWrite(LED_PIN, LOW);
      delay(500);
    }
  }

  //Task pin Blink to Core 0
  xTaskCreatePinnedToCore(
      statusBlinking,
      "Blink",
      500,
      NULL,
      0,
      &Blink,
      0);

  //Setup Serial and Controlling
  pinMode(LED_PIN, OUTPUT);
  Serial.begin(115200);
  while (!Serial)
  {
    ;
  }
  if (!rtc.begin())
  {
    for (int i = 0; i < RTC_ERROR; i++)
    {
      digitalWrite(LED_PIN, HIGH);
      delay(250);
      digitalWrite(LED_PIN, LOW);
      delay(500);
    }
  }

  //Setup Access Point
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid, password);
  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
  Serial.println(WiFi.softAPIP());

  //Setup redirects
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(SPIFFS, "/start.html", "text/html", false, processor);
    last_adress = request->url();
  });

  server.on("/startup", HTTP_GET, [](AsyncWebServerRequest *request) {
    AsyncWebParameter *p = request->getParam("sensor");
    sensor_dht = new DHT(SENSOR_PIN, (p->value()).toInt());

    p = request->getParam("h");
    hours = p->value().toFloat();

    p = request->getParam("filename");
    logfile_name = "/csv/";
    logfile_name.concat(p->value());
    logfile_name.concat(".csv");

    p = request->getParam("min");
    sec_per_interval = (p->value().toInt()) * 60;
    p = request->getParam("sec");
    sec_per_interval += p->value().toInt();
    Serial.print("Messintervall: ");
    Serial.print(sec_per_interval);
    Serial.println(" Sekunden");
    Serial.println(logfile_name);

    File log = SPIFFS.open(logfile_name, FILE_WRITE);
    if(log) {
      log.println("date, time, temperature, humidity");
      log.close();
    } else {
      Serial.println("Konnte keinen Header in die CSV schreiben.");
    }
    
    on = true;
    rtc.getDateTime(&dt);
    next_measurement = (millis()/1000)+sec_per_interval;

    sensor_dht->begin();

    request->redirect("/main");
  });

  server.on("/main", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(SPIFFS, "/main.html", "text/html", false, processor);
    last_adress = request->url();
  });

  server.on("/aktiv", HTTP_GET, [](AsyncWebServerRequest *request) {
    AsyncWebParameter *p = request->getParam("sensor");
    sensor_dht = new DHT(SENSOR_PIN, (p->value()).toInt());

    sensor_dht->begin();

    request->send(SPIFFS, "/aktiv.html", "text/html", false, processor);
  });

  server.on("/tabelle", HTTP_GET, [](AsyncWebServerRequest *request) {
     last_adress = "/tabelle";
    Serial.println("Tabelle geöffnet.");
    request->send(SPIFFS, "/tabelle.html", "text/html", false);
  });

  //Update values on aktiv page
  server.on("/temperature", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/plain", readDHTTemperature().c_str());
  });

  server.on("/humidity", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/plain", readDHTHumidity().c_str());
  });

  //Show Logfiles
  server.on("/files", HTTP_GET, [](AsyncWebServerRequest *request) {
    AsyncResponseStream *response = request->beginResponseStream("text/html");
    Serial.print("Last Page: ");
    Serial.println(last_adress);
    displayFileSystem(response);

    request->send(response);
    last_adress = request->url();
  });

  server.on("/download_file", HTTP_GET, [](AsyncWebServerRequest *request) {
    AsyncWebParameter *p = request->getParam("fname");
    String fname = "/csv/";
    fname.concat(p->value());

    request->send(SPIFFS, fname, "text/csv", true);
    request->redirect("/csv");
  });

  server.on("/display_file", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(SPIFFS, logfile_name, "text/csv", false);
  });

  server.on("/delete_file", HTTP_GET, [](AsyncWebServerRequest *request) {
    AsyncWebParameter *p = request->getParam("fname");
    String fname = "/csv/";
    fname.concat(p->value());

    SPIFFS.remove(fname);

    request->redirect("/csv");
  });

  //Error-Page
  server.onNotFound([](AsyncWebServerRequest *request) {
    request->send(404);
  });

  //images
  server.on("/favicon.ico", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(SPIFFS, "/favicon.ico");
  });
  server.on("/images/sort_both.png", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(SPIFFS, "/images/sort_both.png", "image/png");
  });
  server.on("/images/sort_asc.png", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(SPIFFS, "/images/sort_asc.png", "image/png");
  });
  server.on("/images/sort_desc.png", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(SPIFFS, "/images/sort_desc.png", "image/png");
  });

  //css and js files
  server.on("/css/bootstrap.min.css", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(SPIFFS, "/css/bootstrap.min.css");
  });
  server.on("/css/bootstrap.css.map", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(SPIFFS, "/css/bootstrap.css.map");
  });
  server.on("/css/custom.css", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(SPIFFS, "/css/custom.css");
  });
  server.on("/css/dataTables.bootstrap.css", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(SPIFFS, "/css/dataTables.bootstrap.css");
  });
  server.on("/js/bootstrap.min.js", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(SPIFFS, "/js/bootstrap.min.js");
  });
  server.on("/js/csv_to_html_table.js", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(SPIFFS, "/js/csv_to_html_table.js");
  });
  server.on("/js/dataTables.bootstrap.js", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(SPIFFS, "/js/dataTables.bootstrap.js");
  });
  server.on("/js/jquery.csv.min.js", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(SPIFFS, "/js/jquery.csv.min.js");
  });
  server.on("/js/jquery.dataTables.min.js", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(SPIFFS, "/js/jquery.dataTables.min.js");
  });
  server.on("/js/jquery.min.js", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(SPIFFS, "/js/jquery.min.js");
  });

  //Start Webserver
  server.begin();

  Serial.println("HermosEasyMeasure vollständig gestartet.");
}

void loop()
{
  if (on && checkInterval())
  {
    readDHTTemperature();
    readDHTHumidity();
    File log = SPIFFS.open(logfile_name, FILE_APPEND);
    if (log)
    {
      printData(log);
      next_measurement=(millis()/1000)+sec_per_interval;
      Serial.print("Nächster Messzeitpunkt (epoch): ");
      Serial.println(next_measurement);
      log.close();
    }
    else
    {
      Serial.println("Writing to log file failed.");
      for (int i = 0; i < FS_ERROR; i++)
      {
        digitalWrite(LED_PIN, HIGH);
        delay(250);
        digitalWrite(LED_PIN, LOW);
        delay(500);
      }
    }
  }
}
// Replaces placeholder with DHT values
String processor(const String &var)
{
  //Serial.println(var);
  if (var == "TEMPERATURE")
  {
    return readDHTTemperature();
  }
  else if (var == "HUMIDITY")
  {
    return readDHTHumidity();
  }
  else if (var == "STORAGE_TOTAL")
  {
    return String(SPIFFS.totalBytes() / 1000000.000);
  }
  else if (var == "STORAGE_USED")
  {
    return String(SPIFFS.usedBytes() / 1000000.000);
  }
  else if (var == "STORAGE_FREE")
  {
    return String((SPIFFS.totalBytes() - SPIFFS.usedBytes()) / 1000000.000);
  }
  else if (var == "LOGFILE") {
    return String(logfile_name);
  }
  return String();
}

String readDHTTemperature()
{
  //if (!isnan(sensor_dht)) {
  float t = sensor_dht->readTemperature();
  last_temp = t;

  if (isnan(t))
  {
    Serial.println("Failed to read from DHT sensor!");
    return "--";
  }
  else
  {
    Serial.println(t);
    return String(t);
  }
  // }
}

String readDHTHumidity()
{

  float h = sensor_dht->readHumidity();
  last_hum = h;
  if (isnan(h))
  {
    Serial.println("Failed to read from DHT sensor!");
    return "--";
  }
  else
  {
    Serial.println(h);
    return String(h);
  }
}

static void printData(File& log)
{
  DS1302_DateTime dt;
  char buf[32];

  // Get and print RTC date and time
  if (rtc.getDateTime(&dt))
  {
    snprintf(buf, sizeof(buf), "%d %02d-%02d-%d %d:%02d:%02d",
             dt.dayWeek, dt.dayMonth, dt.month, dt.year, dt.hour, dt.minute, dt.second);
    Serial.println(buf);
    log.print(dt.dayMonth);
    log.print('-');
    log.print(dt.month);
    log.print('-');
    log.print(dt.year);
    log.print(", ");
    if (dt.hour < 10)
    {
      log.print('0');
    }
    log.print(dt.hour);
    log.print(':');
    if (dt.minute < 10)
    {
      log.print('0');
    }
    log.print(dt.minute);
    log.print(':');
    if (dt.second < 10)
    {
      log.print('0');
    }
    log.print(dt.second);
    log.print(", ");
  }
  else
  {
    Serial.println(F("Error: DS1302 read failed"));
    log.print("null, null, ");
  }
  log.print(last_temp, 2);
  log.print(", ");
  log.println(last_hum, 2);

  log.close();
}

void displayFileSystem(AsyncResponseStream *response)
{
  response->print("<!DOCTYPE html> <html lang=\"de\"> <head> <title>Hermos-EasyMeasure</title> <meta charset=\"utf-8\"> <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0, user-scalable=yes\" />");
  response->print("<style>.flex_container {width: 350px;display: flex;flex-wrap: wrap;}.delete, .filename, .size {text-align: center;display: inline-block;flex: 1 0 21%;min-width: 105px;margin: 5px 0;padding: 0 5px;height: 50px;text-align: center;");
  response->print("vertical-align: middle;line-height: 50px;}.delete {background-color: #FF6666;}.size {background-color: #7F7FFF;}.filename {background-color: #7FBF7F;}.size p {margin: 0;}</style>");
  response->print("<h1>Dateien</h1>");
  response->print("<a href=\"");
  response->print(last_adress);
  response->print("\">Zurück</a>");
  response->print("<div class=\"flex_container\">");

  File root = SPIFFS.open("/csv");
  File file = root.openNextFile();
  while (file)
  {
    String fname = file.name();
    fname.remove(0, 5);
    response->print("<div class=\"filename\"><a href=\"/download_file?fname=");
    response->print(fname);
    response->print("\">");
    response->print(fname);
    response->print("</a></div>");
    response->print("<div class=\"size\"><p>");
    response->print(file.size() / 1000.00, 3);
    response->print(" kB</p></div><div class=\"delete\"><a  href=\"/delete_file?fname=");
    response->print(fname);
    response->print("\">Löschen</a></div>");
    file = root.openNextFile();
  }
  response->print("</div></body></html>");
}

bool checkInterval() {
  if(next_measurement<=(millis()/1000)) {
    return true;
  }
  else {
    return false;
  }
}