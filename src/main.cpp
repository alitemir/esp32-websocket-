#include <WiFi.h>
#include <ModbusMaster.h>
#include <ArduinoJson.h>
#include "esp_timer.h"
#include "Arduino.h"
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include "esp_http_server.h"
#include "html.h"
#include <sys/param.h>

const char *ssid = "VR_Ozel_Ag";
const char *password = "12345678";

// TODO:
// akım,voltaj ve durumlar okunacak akım voltaj
// saatler için tik koyulacak kırmızı veya yeşil olacak
// soguk modu kış için, sıcaklık sensörü ile entegre çalışacak, sadece checkbox ekle ,sıcaklık aralığı eklenecek
// ayrı ip adresi

#define LED_PIN 2

#define MAX485_DE 22
#define MAX485_RE_NEG 23

#define FORWARD_ADDRESS 0x8AA
#define BACKWARD_ADDRESS 0x8AB
#define RIGHT_ADDRESS 0x8AC
#define LEFT_ADDRESS 0x8AD
#define STOP_ADDRESS 0x8AE

// saat ve dakikalar için
#define ALARM_ADDR_BEGIN 0x1214

#define CURRENT 0x1012
#define VOLTAGE 0x1011

#define EKTUR_ILERI 0x80A
#define EKTUR_GERI 0x80B

// buradan oku
#define ALARM_SETTINGS_ADDR_BEGIN 0x1017
#define ALARM1_SETTING_REGISTER 0x1017
#define ALARM2_SETTING_REGISTER 0x1018
#define ALARM3_SETTING_REGISTER 0x1019
#define ALARM4_SETTING_REGISTER 0x101A
#define ALARM5_SETTING_REGISTER 0x101B
#define ALARM6_SETTING_REGISTER 0x101C
#define ALARM7_SETTING_REGISTER 0x101D
#define ALARM8_SETTING_REGISTER 0x101E

// Robot Status Coil
#define CHARGING_COIL 0x800
#define ON_TOUR_COIL 0x801

// #define ALARM1_SAAT      0x1000
// #define ALARM1_DAKIKA    0x1001
// #define ALARM2_SAAT      0x1002
// #define ALARM2_DAKIKA    0x1003
// #define ALARM3_SAAT      0x1004
// #define ALARM3_DAKIKA    0x1005
// #define ALARM4_SAAT      0x1006
// #define ALARM4_DAKIKA    0x1007
// #define ALARM5_SAAT      0x1008
// #define ALARM5_DAKIKA    0x1009
// #define ALARM6_SAAT      0x1010
// #define ALARM6_DAKIKA    0x1011
// #define ALARM7_SAAT      0x1012
// #define ALARM7_DAKIKA    0x1013
// #define ALARM8_SAAT      0x1014
// #define ALARM8_DAKIKA    0x1015

httpd_handle_t camera_httpd = NULL;
ModbusMaster node;

char jsonbuffer[100];
char buffer[128] = {
    0,
};

static esp_err_t index_handler(httpd_req_t *req)
{
  httpd_resp_set_type(req, "text/html");
  return httpd_resp_send(req, (const char *)INDEX_HTML, strlen(INDEX_HTML));
}

static esp_err_t tur_ileri_handler(httpd_req_t *req)
{
  httpd_resp_set_type(req, "text/html");
  node.writeSingleCoil(EKTUR_ILERI, HIGH);
  delay(1000);
  node.writeSingleCoil(EKTUR_ILERI, LOW);
  return httpd_resp_send(req, "ileri", 5);
}

static esp_err_t tur_geri_handler(httpd_req_t *req)
{
  httpd_resp_set_type(req, "text/html");

  node.writeSingleCoil(EKTUR_GERI, HIGH);
  delay(1000);
  node.writeSingleCoil(EKTUR_GERI, LOW);
  return httpd_resp_send(req, "geri", 4);
}

// {"volt":17,"amp":4,"status":""Beklemede"}
static esp_err_t status_handler(httpd_req_t *req)
{
  StaticJsonDocument<50> jsonresponse;
  jsonresponse["amp"] = node.getResponseBuffer(node.readHoldingRegisters(CURRENT, 1));
  jsonresponse["volt"] = node.getResponseBuffer(node.readHoldingRegisters(VOLTAGE, 1));

  // 0-> ŞARJ  1-> BEKLEMEDE
  bool c = node.getResponseBuffer(node.readCoils(CHARGING_COIL, 1));
  bool t = node.getResponseBuffer(node.readCoils(ON_TOUR_COIL, 1));
  if (c == HIGH && t == LOW)
  {
    jsonresponse["status"] = "İleri Turda";
  }
  else if (c == LOW && t == HIGH)
  {
    jsonresponse["status"] = "Geri Turda";
  }
  else
  {
    jsonresponse["status"] = "Şarjda";
  }

  serializeJson(jsonresponse, jsonbuffer);
  return httpd_resp_send(req, jsonbuffer, measureJson(jsonresponse));
}

// esp_err_t save_handler(httpd_req_t *req)
// {
//     char buffer[100];
//     size_t recv_size = MIN(req->content_len, sizeof(buffer));
//     int ret = httpd_req_recv(req, buffer, recv_size);
//     if (ret <= 0) {
//         if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
//             httpd_resp_send_408(req);
//         }
//         return ESP_FAIL;
//     }

//     char *token, *subtoken;
//         char *saveptr1, *saveptr2;
//         int saat, dakika;
//         bool checkboxStatus;
//         int i = 0;
//         int addr = ALARM_ADDR_BEGIN;
//         int alarm_status_begin = BURAYA_YAZ;
//         for (token = strtok_r(buffer, "[", &saveptr1); token != NULL; token = strtok_r(NULL, "[", &saveptr1))
//         {
//           for (subtoken = strtok_r(token, ",", &saveptr2); subtoken != NULL; subtoken = strtok_r(NULL, ",", &saveptr2))
//           {
//             saat = atoi(subtoken);
//             subtoken = strtok_r(NULL, ",", &saveptr2);
//             dakika = atoi(subtoken);
//             subtoken = strtok_r(NULL, ",", &saveptr2);
//             checkboxStatus = atoi(subtoken);
//             printf("i:%d saat: %02d, dakika: %02d on_off:%d\n", i, saat, dakika, checkboxStatus);
//             node.writeSingleRegister(addr, saat);
//             node.writeSingleRegister(addr + 1, dakika);
//             node.writeSingleCoil(alarm_status_begin + i, checkboxStatus);
//             // printf("i:%d addr: %X, addr+1: %X alarm_status_begin:%X\n", i, addr, addr + 1,alarm_status_begin+i);
//             i++;
//             addr += 2;
//           }
//         }
//         printf("\n");
//     const char resp[] = "URI POST Response";
//     httpd_resp_send(req, resp, HTTPD_RESP_USE_STRLEN);
//     return ESP_OK;
// }

static esp_err_t get_times_handler(httpd_req_t *req)
{
  Serial.println("get_times_handler");
  StaticJsonDocument<280> jsonresponse;
  char buffer[8];
  int result = node.readHoldingRegisters(ALARM_ADDR_BEGIN, 16);
  if (result == node.ku8MBSuccess)
  {
    for (int j = 0; j < 16; j += 2)
    {
      int hour = node.getResponseBuffer(j);
      int minute = node.getResponseBuffer(j + 1);

      bool checkboxStatus = node.getResponseBuffer(node.readHoldingRegisters(ALARM_SETTINGS_ADDR_BEGIN + (j / 2), 1));

      snprintf(buffer, sizeof(buffer), "%02d:%02d,%d", hour, minute, checkboxStatus);
      jsonresponse.add(buffer);

      // Serial.printf(" %X %d %d\n",ALARM_SETTINGS_ADDR_BEGIN+(j/2),checkboxStatus,j/2);
    }
  }
  else
  {
    Serial.println("Alarm read error");
  }

  serializeJson(jsonresponse, jsonbuffer);
  return httpd_resp_send(req, jsonbuffer, measureJson(jsonresponse));
}

static void readAlarms()
{
  int result = node.readHoldingRegisters(ALARM_ADDR_BEGIN, 16);
  if (result == node.ku8MBSuccess)
  {
    for (int j = 0; j < 16; j += 2)
    {
      int hour = node.getResponseBuffer(j);
      int minute = node.getResponseBuffer(j + 1);

      Serial.printf("%02d:%02d\n", hour, minute);
    }
  }
  else
  {
    Serial.println("Alarm read error");
  }
  Serial.println();
}

static void WriteAlarmStatus()
{ // Buradaki değerler ekrandan plcye gidecek

  // node.writeSingleCoil(ALARM1_STATUS, HIGH);
  // node.writeSingleCoil(ALARM2_STATUS, HIGH);
  // node.writeSingleCoil(ALARM3_STATUS, HIGH);
  // node.writeSingleCoil(ALARM4_STATUS, HIGH);
  // node.writeSingleCoil(ALARM5_STATUS, HIGH);
  // node.writeSingleCoil(ALARM6_STATUS, HIGH);
  // node.writeSingleCoil(ALARM7_STATUS, HIGH);
  // node.writeSingleCoil(ALARM8_STATUS, HIGH);
}

static void readAlarmStatus() // buradaki değerler plcden ekrana
{
  // char buffer[64];

  int result = node.readHoldingRegisters(ALARM_SETTINGS_ADDR_BEGIN, 8);
  if (result == node.ku8MBSuccess)
  {
    for (int j = 0; j < 8; j++)
    {
      int alarmStatus = node.getResponseBuffer(j);
      Serial.printf("%d", alarmStatus);
    }
  }
  else
  {
    Serial.println("Alarm status read error");
  }
  Serial.println();

  // bool status4 = node.getResponseBuffer(3);
  // bool status5 = node.getResponseBuffer(4);
  // bool status6 = node.getResponseBuffer(5);
  // bool status7 = node.getResponseBuffer(6);
  // bool status8 = node.getResponseBuffer(7);
  // snprintf(buffer, sizeof(buffer), "%d:%d:%d:%d", c, d, e,f);

  // snprintf(buffer, sizeof(buffer), "%d:%d:%d:%d:%d:%d:%d:%d", status1, status2, status3, status4, status5, status6, status7, status8);
  // Serial.println(buffer);
}

static esp_err_t cmd_handler(httpd_req_t *req)
{
  size_t buf_len;
  buf_len = httpd_req_get_url_query_len(req) + 1;
  if (buf_len > 1)
  {
    if (httpd_req_get_url_query_str(req, buffer, buf_len) == ESP_OK)
    {
      if (httpd_query_key_value(buffer, "go", buffer, sizeof(buffer)) == ESP_OK)
      {
      }
      else if (httpd_query_key_value(buffer, "time", buffer, sizeof(buffer)) == ESP_OK)
      {
        // burada time parametresi işleniyor
        // {"volt":17,"amp":4,"status":"Beklemede"}
        char *token, *subtoken;
        char *saveptr1, *saveptr2;
        int saat, dakika;
        bool checkboxStatus;
        int i = 0;
        int addr = ALARM_ADDR_BEGIN;
        int alarm_status_begin = ALARM_SETTINGS_ADDR_BEGIN;
        for (token = strtok_r(buffer, "[", &saveptr1); token != NULL; token = strtok_r(NULL, "[", &saveptr1))
        {
          for (subtoken = strtok_r(token, ",", &saveptr2); subtoken != NULL; subtoken = strtok_r(NULL, ",", &saveptr2))
          {
            saat = atoi(subtoken);
            subtoken = strtok_r(NULL, ",", &saveptr2);
            dakika = atoi(subtoken);
            subtoken = strtok_r(NULL, ",", &saveptr2);
            checkboxStatus = atoi(subtoken);
            printf("i:%d saat: %02d, dakika: %02d on_off:%d\n", i, saat, dakika, checkboxStatus);
            node.writeSingleRegister(addr, saat);
            node.writeSingleRegister(addr + 1, dakika);
            node.writeSingleRegister(alarm_status_begin + i, checkboxStatus);
            // printf("i:%d addr: %X, addr+1: %X alarm_status_begin:%X\n", i, addr, addr + 1,alarm_status_begin+i);
            i++;
            addr += 2;
          }
        }
        printf("\n");
        // readAlarms();
        // readAlarmStatus();
      }
      else
      {
        // time parametresi yoksa
        httpd_resp_send_404(req);
        return ESP_FAIL;
      }
    }
    else
    {
      // go parametresi yoksa
      httpd_resp_send_404(req);
      return ESP_FAIL;
    }
    // parametre yoksa
  }
  else
  {
    httpd_resp_send_404(req);
    return ESP_FAIL;
  }

  int res = 0;

  if (!strcmp(buffer, "forward"))
  {
    Serial.println("Forward");
    node.writeSingleCoil(FORWARD_ADDRESS, HIGH);
    digitalWrite(LED_PIN, 1);
  }
  else if (!strcmp(buffer, "left"))
  {
    Serial.println("Left");
    node.writeSingleCoil(LEFT_ADDRESS, HIGH);
    digitalWrite(LED_PIN, 1);
  }
  else if (!strcmp(buffer, "right"))
  {
    Serial.println("Right");
    node.writeSingleCoil(RIGHT_ADDRESS, HIGH);
    digitalWrite(LED_PIN, 1);
  }
  else if (!strcmp(buffer, "backward"))
  {
    Serial.println("Backward");
    node.writeSingleCoil(BACKWARD_ADDRESS, HIGH);
    digitalWrite(LED_PIN, 1);
  }
  else if (!strcmp(buffer, "stop"))
  {
    Serial.println("Stop");
    node.writeSingleCoil(STOP_ADDRESS, HIGH);
    digitalWrite(LED_PIN, 1);
  }
  else if (!strcmp(buffer, "ektur"))
  {
    Serial.println("Ektur");
  }

  else if (!strcmp(buffer, "x"))
  {
    res = -1;
    node.writeSingleCoil(FORWARD_ADDRESS, LOW);
    node.writeSingleCoil(BACKWARD_ADDRESS, LOW);
    node.writeSingleCoil(RIGHT_ADDRESS, LOW);
    node.writeSingleCoil(LEFT_ADDRESS, LOW);
    node.writeSingleCoil(STOP_ADDRESS, LOW);
    digitalWrite(LED_PIN, 0);
    Serial.println("x");
  }
  if (res)
  {
    return httpd_resp_send_500(req);
  }

  // memset(buffer, 0, sizeof(buffer));
  httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
  return httpd_resp_send(req, NULL, 0);
}

void startCameraServer()
{
  httpd_config_t config = HTTPD_DEFAULT_CONFIG();
  config.server_port = 80;
  httpd_uri_t index_uri = {
      .uri = "/",
      .method = HTTP_GET,
      .handler = index_handler,
      .user_ctx = NULL};

  httpd_uri_t cmd_uri = {
      .uri = "/action",
      .method = HTTP_GET,
      .handler = cmd_handler,
      .user_ctx = NULL};

  httpd_uri_t status_uri = {
      .uri = "/status",
      .method = HTTP_GET,
      .handler = status_handler,
      .user_ctx = NULL};

  httpd_uri_t ileri_uri = {
      .uri = "/turIleri",
      .method = HTTP_GET,
      .handler = tur_ileri_handler,
      .user_ctx = NULL};

  httpd_uri_t geri_uri = {
      .uri = "/turGeri",
      .method = HTTP_GET,
      .handler = tur_geri_handler,
      .user_ctx = NULL};

  httpd_uri_t get_times_uri = {
      .uri = "/gettimes",
      .method = HTTP_GET,
      .handler = get_times_handler,
      .user_ctx = NULL};

  // httpd_uri_t save_uri = {
  // .uri = "/save",
  // .method = HTTP_POST,
  // .handler = get_times_handler,
  // .user_ctx = NULL};

  if (httpd_start(&camera_httpd, &config) == ESP_OK)
  {
    httpd_register_uri_handler(camera_httpd, &index_uri);
    httpd_register_uri_handler(camera_httpd, &cmd_uri);
    httpd_register_uri_handler(camera_httpd, &status_uri);
    httpd_register_uri_handler(camera_httpd, &ileri_uri);
    httpd_register_uri_handler(camera_httpd, &geri_uri);
    httpd_register_uri_handler(camera_httpd, &get_times_uri);
    // httpd_register_uri_handler(camera_httpd, &save_uri)
  }
}

void WiFiStationConnected(WiFiEvent_t event, WiFiEventInfo_t info)
{
  Serial.println("Connected to AP successfully!");
}

void WiFiGotIP(WiFiEvent_t event, WiFiEventInfo_t info)
{
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void WiFiStationDisconnected(WiFiEvent_t event, WiFiEventInfo_t info)
{
  Serial.println("Disconnected from WiFi access point");
  Serial.print("WiFi lost connection. Reason: ");
#ifdef ESP32
  Serial.println(info.disconnected.reason);
#else
  Serial.println(info.wifi_sta_disconnected.reason);
#endif
  Serial.println("Trying to Reconnect");
  WiFi.begin(ssid, password);
}

void preTransmission()
{
  digitalWrite(MAX485_RE_NEG, 1);
  digitalWrite(MAX485_DE, 1);
}

void postTransmission()
{
  digitalWrite(MAX485_RE_NEG, 0);
  digitalWrite(MAX485_DE, 0);
}

void setup()
{
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);

#ifdef ESP32
  WiFi.onEvent(WiFiStationConnected, WiFiEvent_t::SYSTEM_EVENT_STA_DISCONNECTED);
  WiFi.onEvent(WiFiGotIP, WiFiEvent_t::SYSTEM_EVENT_GOT_IP6);
  WiFi.onEvent(WiFiStationDisconnected, WiFiEvent_t::SYSTEM_EVENT_STA_DISCONNECTED);
#else
  WiFi.onEvent(WiFiStationConnected, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_DISCONNECTED);
  WiFi.onEvent(WiFiGotIP, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_GOT_IP);
  WiFi.onEvent(WiFiStationDisconnected, WiFiEvent_t::ARDUINO_EVENT_WIFI_AP_STADISCONNECTED);
#endif
  pinMode(MAX485_RE_NEG, OUTPUT);
  pinMode(MAX485_DE, OUTPUT);
  digitalWrite(MAX485_RE_NEG, 0);
  digitalWrite(MAX485_DE, 0);
  pinMode(LED_PIN, OUTPUT);

  Serial.begin(9600);
  Serial2.begin(9600);

  Serial.setDebugOutput(false);
  node.begin(2, Serial2);
  node.preTransmission(preTransmission);
  node.postTransmission(postTransmission);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");

  Serial.print("Camera Stream Ready! Go to: http://");
  Serial.println(WiFi.localIP());
  readAlarms();
  readAlarmStatus();
  WriteAlarmStatus();
  startCameraServer();
}

void loop()
{
}
