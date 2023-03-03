#include <WiFi.h>
#include <ModbusMaster.h>
#include <ArduinoJson.h>
#include <mdns.h>
#include <esp_wifi.h>
#include <soc/rtc_cntl_reg.h>
#include <esp_http_server.h>
#include <esp_log.h>
#include <nvs_flash.h>
#include <esp_spiffs.h>
#include "main.h"
#include "max6675.h"

// TODO
// allocation islemlerini calloc ile yap statik bufferlari sil.
// html tarafinda sureleri limitle (3 basamak max)
// dosyalari islere gore ayir

// #include <SoftwareSerial.h>

// STA Modu ayarları
// const char *ssid = "VR_Ozel_Ag";
// const char *password = "12345678";

// AP Modu ayarları
// const char *ap_ssid = "GubreSiyirma";

// sicaklik gonderilecek
// kis modu okunacak
const char *ap_pwd = "12345678";

const char *hotspot_ssid = "mustafa_ali_can";
const char *mdns_host = "ican";
const char *mdns_host_uppercase = "I-CAN";

const char *base_path = "/data";
const char *TAG = "test";
httpd_handle_t gubre_siyirma = NULL;
ModbusMaster node;
// SoftwareSerial ss(SERIAL1_RX, SERIAL1_TX);
uint8_t mac[6];
char softap_mac[18] = {0};

int thermoDO = 19;
int thermoCS = 21;
int thermoCLK = 18;
MAX6675 thermocouple(thermoCLK, thermoCS, thermoDO);

static esp_err_t tur_ileri_handler(httpd_req_t *req)
{
  node.writeSingleCoil(EKTUR_ILERI, HIGH);
  delay(250);
  Serial.println("ileri turn");
  node.writeSingleCoil(EKTUR_ILERI, LOW);
  httpd_resp_set_type(req, "text/html");
  return httpd_resp_send(req, "ileri", 5);
}

static esp_err_t tur_geri_handler(httpd_req_t *req)
{
  httpd_resp_set_type(req, "text/html");
  node.writeSingleCoil(EKTUR_GERI, HIGH);
  delay(250);
  Serial.println("geri turn");
  node.writeSingleCoil(EKTUR_GERI, LOW);
  return httpd_resp_send(req, "geri", 4);
}

static const char *get_path_from_uri(char *dest, const char *base_path, const char *uri, size_t destsize)
{
  const size_t base_pathlen = strlen(base_path);
  size_t pathlen = strlen(uri);

  const char *quest = strchr(uri, '?');
  if (quest)
  {
    pathlen = MIN(pathlen, quest - uri);
  }
  const char *hash = strchr(uri, '#');
  if (hash)
  {
    pathlen = MIN(pathlen, hash - uri);
  }

  if (base_pathlen + pathlen + 1 > destsize)
  {
    return NULL;
  }

  strcpy(dest, base_path);
  strlcpy(dest + base_pathlen, uri, pathlen + 1);
  return dest + base_pathlen;
}

static esp_err_t set_content_type_from_file(httpd_req_t *req, const char *filename)
{
  if (IS_FILE_EXT(filename, ".pdf"))
  {
    return httpd_resp_set_type(req, "application/pdf");
  }
  else if (IS_FILE_EXT(filename, ".html"))
  {
    return httpd_resp_set_type(req, "text/html");
  }
  else if (IS_FILE_EXT(filename, ".jpeg"))
  {
    return httpd_resp_set_type(req, "image/jpeg");
  }
  else if (IS_FILE_EXT(filename, ".ico"))
  {
    return httpd_resp_set_type(req, "image/x-icon");
  }

  else if (IS_FILE_EXT(filename, ".js"))
  {
    return httpd_resp_set_type(req, "application/javascript");
  }

  else if (IS_FILE_EXT(filename, ".css"))
  {
    return httpd_resp_set_type(req, "text/css");
  }

  return httpd_resp_set_type(req, "text/plain");
}

esp_err_t mount_storage(const char *base_path)
{
  ESP_LOGI(TAG, "Initializing SPIFFS");

  esp_vfs_spiffs_conf_t conf = {
      .base_path = base_path,
      .partition_label = NULL,
      .max_files = 10,
      .format_if_mount_failed = true};

  esp_err_t ret = esp_vfs_spiffs_register(&conf);
  if (ret != ESP_OK)
  {
    if (ret == ESP_FAIL)
    {
      ESP_LOGE(TAG, "Failed to mount or format filesystem");
    }
    else if (ret == ESP_ERR_NOT_FOUND)
    {
      ESP_LOGE(TAG, "Failed to find SPIFFS partition");
    }
    else
    {
      ESP_LOGE(TAG, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
    }
    return ret;
  }

  size_t total = 0, used = 0;
  ret = esp_spiffs_info(NULL, &total, &used);
  if (ret != ESP_OK)
  {
    ESP_LOGE(TAG, "Failed to get SPIFFS partition information (%s)", esp_err_to_name(ret));
    return ret;
  }

  ESP_LOGI(TAG, "Partition size: total: %d, used: %d", total, used);
  return ESP_OK;
}

static esp_err_t download_handler(httpd_req_t *req)
{
  char filepath[FILE_PATH_MAX];
  FILE *fd = NULL;
  struct stat file_stat;

  const char *filename = get_path_from_uri(filepath, ((struct file_server_data *)req->user_ctx)->base_path,
                                           req->uri, sizeof(filepath));
  if (strcmp(filename, "/") == 0)
  {
    filename = "/data/index.html";
    strncpy(filepath, "/data/index.html", 17);
  }

  if (!filename)
  {
    ESP_LOGE(TAG, "Filename is too long");
    /* Respond with 500 Internal Server Error */
    httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Filename too long");
    return ESP_FAIL;
  }
  if (strcmp(&filepath[strlen(filepath) - 4], ".map") == 0 || strcmp(&filepath[strlen(filepath) - 7], ".map.js") == 0)
  {
    // Serial.println("Skip map file");
    return ESP_FAIL;
  }
  else
  {
    String fl = String(filepath) + String(".gz");

    // Serial.println(filepath);
    // Serial.println(filename);
    // Serial.println(fl);

    fd = fopen(fl.c_str(), "r");

    if (!fd)
    {
      ESP_LOGE(TAG, "Failed to read existing file : %s", filepath);
      httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to read existing file");
      return ESP_FAIL;
    }
  }

  ESP_LOGI(TAG, "Sending file : %s (%ld bytes)...", filename, file_stat.st_size);
  set_content_type_from_file(req, filename);

  char *chunk = ((struct file_server_data *)req->user_ctx)->scratch;
  size_t chunksize;
  do
  {
    chunksize = fread(chunk, 1, SCRATCH_BUFSIZE, fd);

    if (chunksize > 0)
    {
      httpd_resp_set_hdr(req, "Cache-Control", "max-age=3600");
      httpd_resp_set_hdr(req, "Content-Encoding", "gzip");
      if (httpd_resp_send_chunk(req, chunk, chunksize) != ESP_OK)
      {
        fclose(fd);
        ESP_LOGE(TAG, "File sending failed!");
        httpd_resp_sendstr_chunk(req, NULL);
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to send file");
        return ESP_FAIL;
      }
    }

  } while (chunksize != 0);

  fclose(fd);
  ESP_LOGI(TAG, "File sending complete");

#ifdef CONFIG_EXAMPLE_HTTPD_CONN_CLOSE_HEADER
  httpd_resp_set_hdr(req, "Connection", "close");
#endif
  httpd_resp_send_chunk(req, NULL, 0);
  return ESP_OK;
}

static esp_err_t winter_status_handler(httpd_req_t *req)
{
  char jsonbuffer[100];
  StaticJsonDocument<50> jsonresponse;
  jsonresponse["wintermode"] = 1;
  jsonresponse["firstmode"] = node.getResponseBuffer(node.readHoldingRegisters(BIRINCI_MOD, 1));
  jsonresponse["secondmode"] = node.getResponseBuffer(node.readHoldingRegisters(IKINCI_MOD, 1));
  serializeJson(jsonresponse, jsonbuffer);
  return httpd_resp_send(req, jsonbuffer, measureJson(jsonresponse));
  return ESP_OK;
}

static esp_err_t temp_status_handler(httpd_req_t *req)
{
  char jsonbuffer[100];
  StaticJsonDocument<50> jsonresponse;
  jsonresponse["temp"] = thermocouple.readCelsius();
  serializeJson(jsonresponse, jsonbuffer);
  return httpd_resp_send(req, jsonbuffer, measureJson(jsonresponse));
  return ESP_OK;
}

static esp_err_t status_handler(httpd_req_t *req)
{
  char jsonbuffer[100];
  StaticJsonDocument<50> jsonresponse;
  jsonresponse["amp"] = (float)node.getResponseBuffer(node.readHoldingRegisters(CURRENT, 1)) / 10.0;
  jsonresponse["volt"] = (float)node.getResponseBuffer(node.readHoldingRegisters(VOLTAGE, 1)) / 10.0;
  int d = node.getResponseBuffer(node.readHoldingRegisters(DURUM, 1));
  String status = "";
  switch (d)
  {
  case 0:
    status = "Robot Takıldı.";
    break;
  case 1:
    status = "İleri Turda";
    break;
  case 2:
    status = "Şarjda";
    break;
  case 3:
    status = "Geri Turda.";
    break;
  case 4:
    status = "Acil Stop Basılı.";
    break;
  case 5:
    status = "Yön Switch Basılı.";
    break;
  case 6:
    status = "Robot Şarj Etmiyor.";
    break;
  case 7:
    status = "Manuel İleri";
    break;
  case 8:
    status = "Manuel Geri";
    break;
  case 9:
    status = "Hata!";
    break;
  case 10:
    status = "Şarj Switchi Takılı";
    break;
  default:
    // Serial.printf("status: %d\n", d);
    status = "HATA";
    break;
  }

  jsonresponse["status"] = status;
  serializeJson(jsonresponse, jsonbuffer);
  return httpd_resp_send(req, jsonbuffer, measureJson(jsonresponse));
}

static esp_err_t cmd_handler(httpd_req_t *req)
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
    // Serial.println("Forward");
    node.writeSingleCoil(FORWARD_ADDRESS, HIGH);
    digitalWrite(LED_PIN, 1);
  }
  else if (!strcmp(buffer, "left"))
  {
    // Serial.println("Left");
    node.writeSingleCoil(LEFT_ADDRESS, HIGH);
    digitalWrite(LED_PIN, 1);
  }
  else if (!strcmp(buffer, "right"))
  {
    // Serial.println("Right");
    node.writeSingleCoil(RIGHT_ADDRESS, HIGH);
    digitalWrite(LED_PIN, 1);
  }
  else if (!strcmp(buffer, "backward"))
  {
    // Serial.println("Backward");
    node.writeSingleCoil(BACKWARD_ADDRESS, HIGH);
    digitalWrite(LED_PIN, 1);
  }
  else if (!strcmp(buffer, "stop"))
  {
    // Serial.println("Stop");
    node.writeSingleCoil(STOP_ADDRESS, HIGH);
    digitalWrite(LED_PIN, 1);
  }
  else if (!strcmp(buffer, "ektur"))
  {
    Serial.println("Ektur");
  }

  else if (!strcmp(buffer, "x"))
  {
    node.writeSingleCoil(FORWARD_ADDRESS, LOW);
    node.writeSingleCoil(BACKWARD_ADDRESS, LOW);
    node.writeSingleCoil(RIGHT_ADDRESS, LOW);
    node.writeSingleCoil(LEFT_ADDRESS, LOW);
    node.writeSingleCoil(STOP_ADDRESS, LOW);
    digitalWrite(LED_PIN, 0);
    // Serial.println("x");
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

static esp_err_t winter_mode_handler(httpd_req_t *req)
{
  // Serial.println("winter mode");
  char buffer[64] = {
      0,
  };

  size_t buf_len;
  char *remaining;
  buf_len = httpd_req_get_url_query_len(req) + 1;

  char mode_buf[3] = {0};

  int firstmode = 0;
  int secondmode = 0;

  if (buf_len > 1)
  {
    if (httpd_req_get_url_query_str(req, buffer, buf_len) == ESP_OK)
    {
      if (httpd_query_key_value(buffer, "mode1", mode_buf, sizeof(buffer)) == ESP_OK)
      {
        firstmode = strtol(mode_buf, &remaining, 10);
        node.writeSingleRegister(BIRINCI_MOD, firstmode);
        Serial.printf("first mode:%d\n", firstmode);
        httpd_resp_send(req, "OK", HTTPD_RESP_USE_STRLEN);
        return ESP_OK;
      }
      if (httpd_query_key_value(buffer, "mode2", mode_buf, sizeof(buffer)) == ESP_OK)
      {
        secondmode = strtol(mode_buf, &remaining, 10);
        node.writeSingleRegister(IKINCI_MOD, secondmode);
        Serial.printf("second mode:%d\n", secondmode);
        httpd_resp_send(req, "OK", HTTPD_RESP_USE_STRLEN);
        return ESP_OK;
      }
    }
  }
  httpd_resp_send_404(req);
  return ESP_FAIL;
}

static esp_err_t manual_mode_handler(httpd_req_t *req)
{
  Serial.println("manual mode");
  char buffer[64] = {
      0,
  };

  size_t buf_len;
  char *remaining;
  buf_len = httpd_req_get_url_query_len(req) + 1;

  char sure_buf[3] = {0};
  char ilerisurme_buf[3] = {0};
  char gecikme_buf[3] = {0};
  int sure = 0;
  int ilerisurme = 0;
  int gecikme = 0;

  if (buf_len > 1)
  {
    if (httpd_req_get_url_query_str(req, buffer, buf_len) == ESP_OK)
    {
      if (httpd_query_key_value(buffer, "sure", sure_buf, sizeof(buffer)) == ESP_OK)
      {
        sure = strtol(sure_buf, &remaining, 10);
      }

      if (httpd_query_key_value(buffer, "ilerisurme", ilerisurme_buf, sizeof(buffer)) == ESP_OK)
      {
        ilerisurme = strtol(ilerisurme_buf, &remaining, 10);
      }
      if (httpd_query_key_value(buffer, "gecikme", gecikme_buf, sizeof(buffer)) == ESP_OK)
      {
        gecikme = strtol(gecikme_buf, &remaining, 10);
      }
      if (sure && gecikme && ilerisurme)
      {
        Serial.printf("sure:%d ilerisurme:%d gecikme:%d\n", sure, ilerisurme, gecikme);
        node.writeSingleRegister(GERI_SURE, sure);
        node.writeSingleRegister(ILERI_SURE, ilerisurme);
        node.writeSingleRegister(GECIKME, gecikme);
        httpd_resp_send(req, "OK", HTTPD_RESP_USE_STRLEN);
        return ESP_OK;
      }
    }
    httpd_resp_send_404(req);
    return ESP_FAIL;
  }
  httpd_resp_send_404(req);
  return ESP_FAIL;
}

static esp_err_t save_handler(httpd_req_t *req)
{
  Serial.println("save_handler");

  int t1 = millis();
  char buffer[90];
  size_t buf_len;
  buf_len = httpd_req_get_url_query_len(req) + 1;
  if (buf_len > 1)
  {
    if (httpd_req_get_url_query_str(req, buffer, buf_len) == ESP_OK)
    {
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
            node.writeSingleRegister(addr, saat);
            node.writeSingleRegister(addr + 1, dakika);
            node.writeSingleRegister(alarm_status_begin + i, checkboxStatus);
            // 180 Serial.printf("i:%d saat: %02d, dakika: %02d on_off:%d\n", i, saat, dakika, checkboxStatus);
            i++;
            addr += 2;
          }
        }
        const char resp[] = "OK";
        httpd_resp_send(req, resp, HTTPD_RESP_USE_STRLEN);
      }
    }
  }
  int t2 = millis();
  printf("Alarms saved in %d ms\n", t2 - t1);
  return ESP_OK;
}

static esp_err_t get_times_handler(httpd_req_t *req)
{
  char jsonbuffer[100];
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

static esp_err_t winter_mode_handler()
{
  Serial.println("winter mode handler");
  int ileri_sure = node.getResponseBuffer(node.readHoldingRegisters(ILERI_SURE, 1));
  int geri_sure = node.getResponseBuffer(node.readHoldingRegisters(GERI_SURE, 1));
  int gecikme = node.getResponseBuffer(node.readHoldingRegisters(GECIKME, 1));
  int kis_modu_aktif = node.getResponseBuffer(node.readHoldingRegisters(KIS_MODU_AKTIF, 1));
  int birinci_mod = node.getResponseBuffer(node.readHoldingRegisters(BIRINCI_MOD, 1));
  int ikinci_mod = node.getResponseBuffer(node.readHoldingRegisters(IKINCI_MOD, 1));

  Serial.printf("ileri:%d geri:%d gecikme:%d kis:%d 1.:%d 2.:%d\n", ileri_sure, geri_sure,
                gecikme, kis_modu_aktif, birinci_mod, ikinci_mod);
  return ESP_OK;
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
  config.max_uri_handlers = 16;
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

  httpd_uri_t winter_mode_uri = {
      .uri = "/wintermode",
      .method = HTTP_POST,
      .handler = winter_mode_handler,
      .user_ctx = NULL};

  httpd_uri_t winter_status_uri = {
      .uri = "/winterstatus",
      .method = HTTP_GET,
      .handler = winter_status_handler,
      .user_ctx = NULL};

  httpd_uri_t temp_status_uri = {
      .uri = "/temp",
      .method = HTTP_GET,
      .handler = temp_status_handler,
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
    httpd_register_uri_handler(gubre_siyirma, &winter_mode_uri);
    httpd_register_uri_handler(gubre_siyirma, &winter_status_uri);
    httpd_register_uri_handler(gubre_siyirma, &temp_status_uri);
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
  WiFi.onEvent(Wifi_AP_Disconnected, WiFiEvent_t::ARDUINO_EVENT_WIFI_AP_STADISCONNECTED);
  // #else
  //   WiFi.onEvent(WiFiStationConnected, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_DISCONNECTED);
  //   WiFi.onEvent(WiFiGotIP, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_GOT_IP);
  //   WiFi.onEvent(WiFiStationDisconnected, WiFiEvent_t::ARDUINO_EVENT_WIFI_AP_STADISCONNECTED);
  // #endif

  Serial.println("MAX6675 test");

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
  // Serial.println("");
  WiFi.mode(WIFI_AP_STA);
  esp_wifi_get_mac(WIFI_IF_STA, mac);
  // sprintf(softap_mac, "%s_%02X%02X", mdns_host, mac[4], mac[5]);
  WiFi.softAP(mdns_host_uppercase, ap_pwd);

  // WiFi.begin(ssid, password);
  WiFi.setHostname(mdns_host);
  // delay(100);
  Serial.print("Wifi access point created: http://");
  Serial.println(WiFi.softAPIP());

  // Serial.print("Remote Ready! Go to: http://");
  // Serial.println(WiFi.softAPIP());

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
  winter_mode_handler();
  mount_storage(base_path);
  nvs_flash_init();
  startServer("");
  // wait for MAX chip to stabilize
}

void loop()
{
  int temp = (int)(thermocouple.readCelsius() * 10);
  node.writeSingleRegister(TEMPERATURE, temp);
  Serial.printf("Temperature: %d\n", temp);
  delay(500);
}
