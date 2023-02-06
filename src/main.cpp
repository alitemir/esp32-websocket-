#include <WiFi.h>
#include <ModbusMaster.h>
#include <ArduinoJson.h>
#include <mdns.h>
#include <esp_wifi.h>
#include <soc/rtc_cntl_reg.h>
#include <esp_http_server.h>
#include "html.h"

// STA Modu ayarları
const char *ssid = "VR_Ozel_Ag";
const char *password = "12345678";

// AP Modu ayarları
// const char *ap_ssid = "GubreSiyirma";
const char *ap_pwd = "gubresiyirma";

const char *hotspot_ssid = "GubreSiyirma";
const char *mdns_host = "gubresiyirma";

// TODO:
// soguk modu kış için, sıcaklık sensörü ile entegre çalışacak, sadece checkbox ekle ,sıcaklık aralığı eklenecek
// [OK] ayrı ip adresi -- bu konu üzerinde çalışılacak
// [OK] manual mod butonu
// [OK] dil değiştirme için bayrak eklenecek

// Kullanilan pinler
#define LED_PIN 2
#define MAX485_DE 22
#define MAX485_RE_NEG 23

// Uzaktan kontrol PLC adresleri
#define FORWARD_ADDRESS 0x8AA
#define BACKWARD_ADDRESS 0x8AB
#define RIGHT_ADDRESS 0x8AC
#define LEFT_ADDRESS 0x8AD
#define STOP_ADDRESS 0x8AE

// saat ve dakikaların adresi
#define ALARM_ADDR_BEGIN 0x1214

// Akım ve gerilim okuma adresleri
#define CURRENT 0x1012
#define VOLTAGE 0x1011

// Ek tur adresleri
#define EKTUR_ILERI 0x80A
#define EKTUR_GERI 0x80B

// Zamanlayıcı adresleri - Tek seferde 8 adres okunur
#define ALARM_SETTINGS_ADDR_BEGIN 0x119A // 0x119A - 0x11A1 arası

// Robotun sarj veya tur durumu adresleri
#define CHARGING_COIL 0x800
#define ON_TOUR_COIL 0x801

httpd_handle_t gubre_siyirma = NULL;
ModbusMaster node;

char jsonbuffer[100];
uint8_t mac[6];
char softap_mac[18] = {0};

static esp_err_t index_handler(httpd_req_t *req)
{
  httpd_resp_set_type(req, "text/html");
  return httpd_resp_send(req, (const char *)INDEX_HTML, strlen(INDEX_HTML));
}

static esp_err_t tur_ileri_handler(httpd_req_t *req)
{
  httpd_resp_set_type(req, "text/html");
  // node.writeSingleCoil(EKTUR_ILERI, HIGH);
  delay(1000);
  // node.writeSingleCoil(EKTUR_ILERI, LOW);
  return httpd_resp_send(req, "ileri", 5);
}

static esp_err_t tur_geri_handler(httpd_req_t *req)
{
  httpd_resp_set_type(req, "text/html");

  // node.writeSingleCoil(EKTUR_GERI, HIGH);
  delay(1000);
  // node.writeSingleCoil(EKTUR_GERI, LOW);
  return httpd_resp_send(req, "geri", 4);
}

// {"volt":17,"amp":4,"status":"Şarjda"}
static esp_err_t status_handler(httpd_req_t *req)
{
  StaticJsonDocument<50> jsonresponse;
  jsonresponse["amp"] = 0;  // node.getResponseBuffer(node.readHoldingRegisters(CURRENT, 1));
  jsonresponse["volt"] = 0; // node.getResponseBuffer(node.readHoldingRegisters(VOLTAGE, 1));
  // 0-> ŞARJ  1-> BEKLEMEDE
  bool c = 0; // node.getResponseBuffer(node.readCoils(CHARGING_COIL, 1));
  bool t = 0; // node.getResponseBuffer(node.readCoils(ON_TOUR_COIL, 1));
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

static esp_err_t save_handler(httpd_req_t *req)
{
  int t1 = millis();
  char buffer[90];
  size_t buf_len;
  buf_len = httpd_req_get_url_query_len(req) + 1;
  if (buf_len > 1)
  {
    if (httpd_req_get_url_query_str(req, buffer, buf_len) == ESP_OK)
    {
      Serial.print("buf_len:");
      Serial.println(buf_len);
      if (httpd_query_key_value(buffer, "alarms", buffer, sizeof(buffer)) == ESP_OK)
      {
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
            // node.writeSingleRegister(addr, saat);
            // node.writeSingleRegister(addr + 1, dakika);
            // node.writeSingleRegister(alarm_status_begin + i, checkboxStatus);
            // 180 Serial.printf("i:%d saat: %02d, dakika: %02d on_off:%d\n", i, saat, dakika, checkboxStatus);
            i++;
            addr += 2;
          }
        }
        printf("Alarms saved\n");
        const char resp[] = "OK";
        httpd_resp_send(req, resp, HTTPD_RESP_USE_STRLEN);
      }
    }
  }
  int t2 = millis();
  Serial.println(t2 - t1);
  return ESP_OK;
}

static esp_err_t manual_mode_handler(httpd_req_t *req)
{
  char buffer[16] = {
      0,
  };
  size_t buf_len;
  buf_len = httpd_req_get_url_query_len(req) + 1;
  if (buf_len > 1)
  {
    if (httpd_req_get_url_query_str(req, buffer, buf_len) == ESP_OK)
    {
      if (httpd_query_key_value(buffer, "manual", buffer, sizeof(buffer)) == ESP_OK)
      {
        if (!strcmp(buffer, "true"))
        {
          Serial.println("is true");
        }
        else if (!strcmp(buffer, "false"))
        {
          Serial.println("buffer is false");
        }
        else
        {
          Serial.print("buffer contains:");
          Serial.println(buffer);
          // Serial.println("buffer not valid");
        }
      }
    }
  }
  httpd_resp_send(req, "OK", HTTPD_RESP_USE_STRLEN);
  return ESP_OK;
}

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

static void readAlarmStatus()
{
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
  Serial.println("");
}

static esp_err_t cmd_handler(httpd_req_t *req)
{
  char buffer[16] = {
      0,
  };
  size_t buf_len;
  buf_len = httpd_req_get_url_query_len(req) + 1;
  Serial.print("buf_len:");
  Serial.println(buf_len);
  if (buf_len > 1)
  {
    if (httpd_req_get_url_query_str(req, buffer, buf_len) == ESP_OK)
    {
      if (httpd_query_key_value(buffer, "go", buffer, sizeof(buffer)) == ESP_OK)
      {
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
    // node.writeSingleCoil(FORWARD_ADDRESS, HIGH);
    digitalWrite(LED_PIN, 1);
  }
  else if (!strcmp(buffer, "left"))
  {
    Serial.println("Left");
    // node.writeSingleCoil(LEFT_ADDRESS, HIGH);
    digitalWrite(LED_PIN, 1);
  }
  else if (!strcmp(buffer, "right"))
  {
    Serial.println("Right");
    // node.writeSingleCoil(RIGHT_ADDRESS, HIGH);
    digitalWrite(LED_PIN, 1);
  }
  else if (!strcmp(buffer, "backward"))
  {
    Serial.println("Backward");
    // node.writeSingleCoil(BACKWARD_ADDRESS, HIGH);
    digitalWrite(LED_PIN, 1);
  }
  else if (!strcmp(buffer, "stop"))
  {
    Serial.println("Stop");
    // node.writeSingleCoil(STOP_ADDRESS, HIGH);
    digitalWrite(LED_PIN, 1);
  }
  else if (!strcmp(buffer, "ektur"))
  {
    Serial.println("Ektur");
  }

  else if (!strcmp(buffer, "x"))
  {
    // node.writeSingleCoil(FORWARD_ADDRESS, LOW);
    // node.writeSingleCoil(BACKWARD_ADDRESS, LOW);
    // node.writeSingleCoil(RIGHT_ADDRESS, LOW);
    // node.writeSingleCoil(LEFT_ADDRESS, LOW);
    // node.writeSingleCoil(STOP_ADDRESS, LOW);
    digitalWrite(LED_PIN, 0);
    Serial.println("x");
  }
  else
  {

    res = -1;
  }
  if (res)
  {
    return httpd_resp_send_500(req);
  }

  memset(buffer, 0, sizeof(buffer));
  httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
  return httpd_resp_send(req, NULL, 0);
}

void startServer()
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

  httpd_uri_t manual_mod_uri = {
      .uri = "/manualmode",
      .method = HTTP_POST,
      .handler = manual_mode_handler,
      .user_ctx = NULL};

  httpd_uri_t save_uri = {
      .uri = "/save",
      .method = HTTP_POST,
      .handler = save_handler,
      .user_ctx = NULL};

  if (httpd_start(&gubre_siyirma, &config) == ESP_OK)
  {
    httpd_register_uri_handler(gubre_siyirma, &index_uri);
    httpd_register_uri_handler(gubre_siyirma, &cmd_uri);
    httpd_register_uri_handler(gubre_siyirma, &status_uri);
    httpd_register_uri_handler(gubre_siyirma, &ileri_uri);
    httpd_register_uri_handler(gubre_siyirma, &geri_uri);
    httpd_register_uri_handler(gubre_siyirma, &get_times_uri);
    httpd_register_uri_handler(gubre_siyirma, &manual_mod_uri);
    httpd_register_uri_handler(gubre_siyirma, &save_uri);
  }
}

void WiFiStationConnected(WiFiEvent_t event, WiFiEventInfo_t info)
{
  Serial.println("");
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
  Serial.println("");
  WiFi.mode(WIFI_AP_STA);
  esp_wifi_get_mac(WIFI_IF_STA, mac);
  sprintf(softap_mac, "%s_%02X%02X", hotspot_ssid, mac[4], mac[5]);
  WiFi.softAP(softap_mac, ap_pwd);

  WiFi.begin(ssid, password);

  WiFi.setHostname(mdns_host);
  Serial.print("Wifi access point created: ");
  Serial.println(WiFi.softAPIP());

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(100);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.print("Remote Ready! Go to: http://");
  Serial.println(WiFi.localIP());
  Serial.println(WiFi.getHostname());

  mdns_init();
  mdns_hostname_set(softap_mac);
  Serial.printf("mdns hostname set to: http://%s.local\n", softap_mac);
  mdns_instance_name_set(softap_mac);

  mdns_service_add("GubreSiyirmaWebServer", "_http", "_tcp", 80, NULL, 0);

  readAlarms();
  readAlarmStatus();
  startServer();
}

void loop()
{
}
