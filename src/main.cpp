#include <WiFi.h>
#include <ModbusMaster.h>
#include <ArduinoJson.h>
#include <mdns.h>
#include <esp_wifi.h>
#include <soc/rtc_cntl_reg.h>
#include <esp_http_server.h>
#include "esp_log.h"
#include "nvs_flash.h"

#include "fs_utils.h"
#include "dl_handlers.h"
#include "main.h"
#include "cmd_handlers.h"
extern "C"
{
#include "alarms.h"
#include "http_utils.h"
#include "tur_handlers.h"
}

// #include <SoftwareSerial.h>

// STA Modu ayarları
// const char *ssid = "VR_Ozel_Ag";
// const char *password = "12345678";

// AP Modu ayarları
// const char *ap_ssid = "GubreSiyirma";
const char *ap_pwd = "12345678";

const char *hotspot_ssid = "mustafa_ali_can";
const char *mdns_host = "gubresiyirma";
const char *base_path = "/data";

httpd_handle_t gubre_siyirma = NULL;
ModbusMaster node;
// SoftwareSerial ss(SERIAL1_RX, SERIAL1_TX);
uint8_t mac[6];
char softap_mac[18] = {0};

// esp_err_t error_handler(httpd_req_t *req, httpd_err_code_t err)
// {
//     httpd_resp_set_status(req, "302 Temporary Redirect");
//     httpd_resp_set_hdr(req, "Location", "/");
//     httpd_resp_send(req, "Redirect to the captive portal", HTTPD_RESP_USE_STRLEN);
//     // ESP_LOGI(TAG, "Redirecting to root");
//     return ESP_OK;
// }

esp_err_t startServer(const char *base_path)
{
  httpd_config_t config = HTTPD_DEFAULT_CONFIG();

  static struct file_server_data *server_data = NULL;

  if (server_data)
  {
    Serial.println("File server already started");
    return ESP_ERR_INVALID_STATE;
  }
  server_data = (file_server_data *)calloc(1, sizeof(struct file_server_data));
  if (!server_data)
  {
    Serial.println("Failed to allocate memory for server data");
    return ESP_ERR_NO_MEM;
  }
  strlcpy(server_data->base_path, base_path,
          sizeof(server_data->base_path));
  Serial.println(server_data->base_path);

  config.uri_match_fn = httpd_uri_match_wildcard;
  config.max_uri_handlers = 8;
  config.server_port = 80;

  httpd_uri_t index_uri = {
      .uri = "/",
      .method = HTTP_GET,
      .handler = download_handler,
      .user_ctx = server_data};

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

  httpd_uri_t file_serve = {
      .uri = "/data/*",
      .method = HTTP_GET,
      .handler = download_handler,
      .user_ctx = server_data};

  if (httpd_start(&gubre_siyirma, &config) == ESP_OK)
  {

    httpd_register_uri_handler(gubre_siyirma, &file_serve);
    httpd_register_uri_handler(gubre_siyirma, &index_uri);
    httpd_register_uri_handler(gubre_siyirma, &save_uri);
    httpd_register_uri_handler(gubre_siyirma, &cmd_uri);
    httpd_register_uri_handler(gubre_siyirma, &status_uri);
    httpd_register_uri_handler(gubre_siyirma, &ileri_uri);
    httpd_register_uri_handler(gubre_siyirma, &geri_uri);
    httpd_register_uri_handler(gubre_siyirma, &get_times_uri);
    httpd_register_uri_handler(gubre_siyirma, &manual_mod_uri);
    // httpd_register_err_handler(gubre_siyirma, HTTPD_404_NOT_FOUND, error_handler);
  }
  return ESP_OK;
}

void WiFiStationConnected(WiFiEvent_t event, WiFiEventInfo_t info)
{
  Serial.println("");
  Serial.println("Connected to AP successfully!");
}

// void WiFiGotIP(WiFiEvent_t event, WiFiEventInfo_t info)
// {
//   Serial.println("WiFi connected");
//   Serial.println("IP address: ");
//   Serial.println(WiFi.localIP());
// }

void Wifi_AP_Disconnected(WiFiEvent_t event, WiFiEventInfo_t info)
{
  Serial.println("Disconnected from AP");
  esp_restart();
}

// void WiFiStationDisconnected(WiFiEvent_t event, WiFiEventInfo_t info)
// {
//   Serial.println("Disconnected from WiFi access point");
//   Serial.print("WiFi lost connection. Reason: ");
// #ifdef ESP32
//   Serial.println(info.disconnected.reason);
// #else
//   Serial.println(info.wifi_sta_disconnected.reason);
// #endif
//   Serial.println("Trying to Reconnect");
//   WiFi.begin(ssid, password);
// }

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

  // #ifdef ESP32
  // WiFi.onEvent(WiFiStationConnected, WiFiEvent_t::SYSTEM_EVENT_STA_DISCONNECTED);
  // WiFi.onEvent(WiFiGotIP, WiFiEvent_t::SYSTEM_EVENT_GOT_IP6);
  // WiFi.onEvent(WiFiStationDisconnected, WiFiEvent_t::SYSTEM_EVENT_STA_DISCONNECTED);
  WiFi.onEvent(Wifi_AP_Disconnected, WiFiEvent_t::SYSTEM_EVENT_AP_STADISCONNECTED);
  // #else
  //   WiFi.onEvent(WiFiStationConnected, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_DISCONNECTED);
  //   WiFi.onEvent(WiFiGotIP, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_GOT_IP);
  //   WiFi.onEvent(WiFiStationDisconnected, WiFiEvent_t::ARDUINO_EVENT_WIFI_AP_STADISCONNECTED);
  // #endif

  pinMode(MAX485_RE_NEG, OUTPUT);
  pinMode(MAX485_DE, OUTPUT);
  digitalWrite(MAX485_RE_NEG, 0);
  digitalWrite(MAX485_DE, 0);
  pinMode(LED_PIN, OUTPUT);

  Serial.begin(9600);
  Serial1.begin(115200, SERIAL_8N1, SERIAL1_RX, SERIAL1_TX);
  // ss.begin(115200);

  Serial.setDebugOutput(true);
  node.begin(2, Serial1);
  node.preTransmission(preTransmission);
  node.postTransmission(postTransmission);
  Serial.println("");
  WiFi.mode(WIFI_AP_STA);
  esp_wifi_get_mac(WIFI_IF_STA, mac);
  sprintf(softap_mac, "%s_%02X%02X", mdns_host, mac[4], mac[5]);
  WiFi.softAP(softap_mac, ap_pwd);

  // WiFi.begin(ssid, password);

  WiFi.setHostname(mdns_host);
  Serial.print("Wifi access point created: http://");
  Serial.println(WiFi.softAPIP());
  Serial.print("Remote Ready! Go to: http://");
  Serial.println(WiFi.softAPIP());

  // while (WiFi.status() != WL_CONNECTED)
  // {
  //   delay(100);
  //   Serial.print(".");
  // }
  // Serial.println("");
  // Serial.println("WiFi connected");

  mdns_init();
  mdns_hostname_set(softap_mac);
  Serial.printf("mdns hostname is set to: http://%s.local\n", softap_mac);
  mdns_instance_name_set(softap_mac);

  mdns_service_add("GubreSiyirmaWebServer", "_http", "_tcp", 80, NULL, 0);
  readAlarms();
  readAlarmStatus();

  mount_storage(base_path);
  nvs_flash_init();
  startServer("");
}

void loop()
{
}
