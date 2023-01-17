#include <WiFi.h>
#include <ModbusMaster.h>
#include "esp_timer.h"
#include "Arduino.h"
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include "esp_http_server.h"
#include "html.h"

const char *ssid = "VR_Ozel_Ag";
const char *password = "12345678";

#define LED_PIN 2

#define MAX485_DE 22
#define MAX485_RE_NEG 23

#define ILERI_ADDRESS 0x805
#define GERI_ADDRESS 0x806
#define SAG_ADDRESS 0x807
#define SOL_ADDRESS 0x808
#define DUR_ADDRESS 0x809

#define ALARM_ADDR_BEGIN 0x1000

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

char buffer[64] = {
    0,
};

static esp_err_t index_handler(httpd_req_t *req)
{
  httpd_resp_set_type(req, "text/html");
  return httpd_resp_send(req, (const char *)INDEX_HTML, strlen(INDEX_HTML));
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
        // burada time parametresi işlenir
        char *token, *subtoken;
        char *saveptr1, *saveptr2;
        int x, y;
        int i = 0;
        int addr = ALARM_ADDR_BEGIN;
        for (token = strtok_r(buffer, "[", &saveptr1); token != NULL; token = strtok_r(NULL, "[", &saveptr1))
        {
          for (subtoken = strtok_r(token, ",", &saveptr2); subtoken != NULL; subtoken = strtok_r(NULL, ",", &saveptr2))
          {
            x = atoi(subtoken);
            subtoken = strtok_r(NULL, ",", &saveptr2);
            y = atoi(subtoken);
            printf("i:%d saat: %02d, dakika: %02d\n", i, x, y);
            node.writeSingleRegister(addr, x);
            node.writeSingleRegister(addr + 1, y);
            printf("i:%d addr: %X, addr+1: %X\n", i, addr, addr + 1);
            i++;
            addr += 1;
          }
        }
        printf("\n");
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
    node.writeSingleCoil(ILERI_ADDRESS, HIGH);
    delay(200);
    digitalWrite(LED_PIN, 1);
  }
  else if (!strcmp(buffer, "left"))
  {
    Serial.println("Left");
    node.writeSingleCoil(SOL_ADDRESS, HIGH);
    delay(200);

    digitalWrite(LED_PIN, 1);
  }
  else if (!strcmp(buffer, "right"))
  {
    Serial.println("Right");
    node.writeSingleCoil(SAG_ADDRESS, HIGH);
    delay(200);

    digitalWrite(LED_PIN, 1);
  }
  else if (!strcmp(buffer, "backward"))
  {
    Serial.println("Backward");
    node.writeSingleCoil(GERI_ADDRESS, HIGH);
    delay(200);

    digitalWrite(LED_PIN, 1);
  }
  else if (!strcmp(buffer, "stop"))
  {
    Serial.println("Stop");
    node.writeSingleCoil(ILERI_ADDRESS, LOW);
    node.writeSingleCoil(GERI_ADDRESS, LOW);
    node.writeSingleCoil(SAG_ADDRESS, LOW);
    node.writeSingleCoil(SOL_ADDRESS, LOW);
    delay(200);

    digitalWrite(LED_PIN, 0);
  }
  else if (!strcmp(buffer, "ektur"))
  {
    Serial.println("Ektur");
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

  if (httpd_start(&camera_httpd, &config) == ESP_OK)
  {
    httpd_register_uri_handler(camera_httpd, &index_uri);
    httpd_register_uri_handler(camera_httpd, &cmd_uri);
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
  Serial.setDebugOutput(false);
  node.begin(2, Serial);
  node.preTransmission(preTransmission); // Callback for configuring RS-485 Transreceiver correctly
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

  startCameraServer();
}

void loop()
{
}
