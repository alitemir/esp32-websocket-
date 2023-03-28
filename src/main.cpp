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
#include <DNSServer.h>
#include "main.h"
#include <max6675.h>
#include "limits.h"

const char *ap_pwd = "12345678";
const char *hotspot_ssid = "mustafa_ali_can";
const char *mdns_host = "ican2";
const char *mdns_host_uppercase = "I-CAN_2";
const char *base_path = "/data";
const char *TAG = "MAIN";

httpd_handle_t gubre_siyirma = NULL;
ModbusMaster node;
uint8_t mac[6];
char softap_mac[18] = {0};
DNSServer dnsServer;
MAX6675 thermocouple(thermoCLK, thermoCS, thermoSO);

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

static esp_err_t mount_storage(const char *base_path)
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
        ESP_LOGE(TAG, "File sending failed! - %c", filename);
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
  char jsonbuffer[250];
  StaticJsonDocument<150> jsonresponse;
  jsonresponse["kismodu"] = node.getResponseBuffer(node.readHoldingRegisters(KIS_MODU_AKTIF, 1));
  jsonresponse["birinci"] = node.getResponseBuffer(node.readHoldingRegisters(BIRINCI_MOD, 1));
  jsonresponse["ikinci"] = node.getResponseBuffer(node.readHoldingRegisters(IKINCI_MOD, 1));
  jsonresponse["ileri"] = node.getResponseBuffer(node.readHoldingRegisters(ILERI_SURE, 1));
  jsonresponse["geri"] = node.getResponseBuffer(node.readHoldingRegisters(GERI_SURE, 1));
  jsonresponse["gecikme"] = node.getResponseBuffer(node.readHoldingRegisters(GECIKME, 1));
  jsonresponse["hr"] = node.getResponseBuffer(node.readHoldingRegisters(SAAT, 1));
  jsonresponse["min"] = node.getResponseBuffer(node.readHoldingRegisters(DAKIKA, 1));
  jsonresponse["clk_active"] = node.getResponseBuffer(node.readHoldingRegisters(CLOCKS_ENABLED, 1));

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
  char jsonbuffer[150];
  StaticJsonDocument<100> jsonresponse;
  jsonresponse["amp"] = 1.2;   // (float)node.getResponseBuffer(node.readHoldingRegisters(CURRENT, 1)) / 10.0;
  jsonresponse["volt"] = 25.8; //(float)node.getResponseBuffer(node.readHoldingRegisters(VOLTAGE, 1)) / 10.0;
  jsonresponse["sec"] = 45;    // node.getResponseBuffer(node.readHoldingRegisters(SANIYE, 1));
  jsonresponse["min"] = 36;    // node.getResponseBuffer(node.readHoldingRegisters(DAKIKA, 1));
  jsonresponse["hr"] = 12;     // node.getResponseBuffer(node.readHoldingRegisters(SAAT, 1));
  int d = 5;                   // node.getResponseBuffer(node.readHoldingRegisters(DURUM, 1));
  switch (d)
  {
  case 0:
    // jsonresponse["status"] = "Durum: Robot Takıldı.";
    jsonresponse["status_code"] = "0";
    break;
  case 1:
    // jsonresponse["status"] = "Durum: İleri Turda";
    jsonresponse["status_code"] = "1";
    break;
  case 2:
    // jsonresponse["status"] = "Durum: Şarjda";
    jsonresponse["status_code"] = "2";
    break;
  case 3:
    // jsonresponse["status"] = "Durum: Geri Turda.";
    jsonresponse["status_code"] = "3";
    break;
  case 4:
    // jsonresponse["status"] = "Durum: Acil Stop Basılı.";
    jsonresponse["status_code"] = "4";
    break;
  case 5:
    // jsonresponse["status"] = "Durum: Yön Switch Basılı.";
    jsonresponse["status_code"] = "5";
    break;
  case 6:
    // jsonresponse["status"] = "Durum: Robot Şarj Etmiyor.";
    jsonresponse["status_code"] = "6";
    break;
  case 7:
    // jsonresponse["status"] = "Durum: Manuel İleri";
    jsonresponse["status_code"] = "7";
    break;
  case 8:
    // jsonresponse["status"] = "Durum: Manuel Geri";
    jsonresponse["status_code"] = "8";
    break;
  case 9:
    // jsonresponse["status"] = "Durum: Hata!";
    jsonresponse["status_code"] = "9";
    break;
  case 10:
    // jsonresponse["status"] = "Durum: Şarj Switchi Takılı";
    jsonresponse["status_code"] = "10";
    break;
  default:
    // jsonresponse["status"] = "HATA";
    jsonresponse["status_code"] = "11";
    break;
  }
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
  Serial.println("winter mode");
  char buffer[64] = {
      0,
  };

  size_t buf_len;
  char *remaining;
  buf_len = httpd_req_get_url_query_len(req) + 1;

  char wintermode_buf[3] = {0};
  char mode1_buf[3] = {0};
  char mode2_buf[3] = {0};

  int firstmode = 0;
  int secondmode = 0;
  bool wintermode = false;

  if (buf_len > 1)
  {
    if (httpd_req_get_url_query_str(req, buffer, buf_len) == ESP_OK)
    {

      if (httpd_query_key_value(buffer, "mode1", mode1_buf, sizeof(buffer)) == ESP_OK)
      {
        firstmode = strtol(mode1_buf, &remaining, 10);
        if (firstmode_l <= firstmode && firstmode <= firstmode_h)
        {
          node.writeSingleRegister(BIRINCI_MOD, firstmode);
          Serial.printf("first mode:%d\n", firstmode);
        }
        else // firstmode limitleri disinda ise 404 dön
        {
          httpd_resp_send_404(req);
          return ESP_FAIL;
        }
      }
      if (httpd_query_key_value(buffer, "mode2", mode2_buf, sizeof(buffer)) == ESP_OK)
      {
        secondmode = strtol(mode2_buf, &remaining, 10);
        if (secondmode_l <= secondmode && secondmode <= secondmode_h)
        {
          node.writeSingleRegister(IKINCI_MOD, secondmode);
          Serial.printf("second mode:%d\n", secondmode);
        }

        else // secondmode limitleri disinda ise
        {
          httpd_resp_send_404(req);
          return ESP_FAIL;
        }
      }
      if (httpd_query_key_value(buffer, "winter", wintermode_buf, sizeof(buffer)) == ESP_OK)
      {
        int w = strtol(wintermode_buf, &remaining, 10);
        wintermode = w ? 1 : 0;
        if (wintermode == 0 || wintermode == 1)
        {
          node.writeSingleRegister(KIS_MODU_AKTIF, wintermode);
          Serial.printf("winter mode:%d\n", wintermode);
        }
        else // kış modu 1 veya 0 degilse 404 dön ve kaydetme.
        {
          httpd_resp_send_404(req);
          return ESP_FAIL;
        }
      }
      httpd_resp_send(req, "OK", HTTPD_RESP_USE_STRLEN);
      return ESP_OK;
    }
    httpd_resp_send_404(req);
    return ESP_FAIL;
  }
  httpd_resp_send_404(req);
  return ESP_FAIL;
}

static esp_err_t sysclock_handler(httpd_req_t *req)
{
  char buffer[64] = {
      0,
  };

  size_t buf_len;
  char *remaining;
  buf_len = httpd_req_get_url_query_len(req) + 1;

  char wintermode_buf[3] = {0};
  char mode1_buf[3] = {0};
  char mode2_buf[3] = {0};

  int firstmode = 0;
  int secondmode = 0;
  bool wintermode = false;

  if (buf_len > 1)
  {
    if (httpd_req_get_url_query_str(req, buffer, buf_len) == ESP_OK)
    {

      if (httpd_query_key_value(buffer, "hour", mode1_buf, sizeof(buffer)) == ESP_OK)
      {
        firstmode = strtol(mode1_buf, &remaining, 10);
        if (firstmode_l <= firstmode && firstmode <= firstmode_h)
        {
          node.writeSingleRegister(SAAT, firstmode);
          Serial.printf("hour:%d\n", firstmode);
        }
        else // firstmode limitleri disinda ise 404 dön
        {
          httpd_resp_send_404(req);
          return ESP_FAIL;
        }
      }
      if (httpd_query_key_value(buffer, "minute", mode2_buf, sizeof(buffer)) == ESP_OK)
      {
        secondmode = strtol(mode2_buf, &remaining, 10);
        if (secondmode_l <= secondmode && secondmode <= secondmode_h)
        {
          node.writeSingleRegister(DAKIKA, secondmode);
          Serial.printf("minute:%d\n", secondmode);
        }

        else // secondmode limitleri disinda ise
        {
          httpd_resp_send_404(req);
          return ESP_FAIL;
        }
      }
      if (httpd_query_key_value(buffer, "active", wintermode_buf, sizeof(buffer)) == ESP_OK)
      {
        int w = strtol(wintermode_buf, &remaining, 10);
        wintermode = w ? 1 : 0;
        if (wintermode == 0 || wintermode == 1)
        {
          node.writeSingleRegister(CLOCKS_ENABLED, wintermode);
          Serial.printf("active:%d\n", wintermode);
        }
        else // kış modu 1 veya 0 degilse 404 dön ve kaydetme.
        {
          httpd_resp_send_404(req);
          return ESP_FAIL;
        }
      }
      httpd_resp_send(req, "OK", HTTPD_RESP_USE_STRLEN);
      return ESP_OK;
    }
    httpd_resp_send_404(req);
    return ESP_FAIL;
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
  char ileri_sure_buf[3] = {0};
  char gecikme_buf[3] = {0};
  int sure = 0;
  int ileri_sure = 0;
  int gecikme = 0;

  if (buf_len > 1)
  {
    if (httpd_req_get_url_query_str(req, buffer, buf_len) == ESP_OK)
    {
      if (httpd_query_key_value(buffer, "sure", sure_buf, sizeof(buffer)) == ESP_OK)
      {
        sure = strtol(sure_buf, &remaining, 10);
      }

      if (httpd_query_key_value(buffer, "ilerisure", ileri_sure_buf, sizeof(buffer)) == ESP_OK)
      {
        ileri_sure = strtol(ileri_sure_buf, &remaining, 10);
      }
      if (httpd_query_key_value(buffer, "gecikme", gecikme_buf, sizeof(buffer)) == ESP_OK)
      {
        gecikme = strtol(gecikme_buf, &remaining, 10);
      }
      if ((sure_l <= sure && sure <= sure_h) && (ileri_sure_l <= ileri_sure && ileri_sure <= ileri_sure_h) && gecikme_l <= gecikme && gecikme <= gecikme_h)
      {
        Serial.printf("sure:%d ileri_sure:%d gecikme:%d\n", sure, ileri_sure, gecikme);
        node.writeSingleRegister(GERI_SURE, sure);
        node.writeSingleRegister(ILERI_SURE, ileri_sure);
        node.writeSingleRegister(GECIKME, gecikme);
        httpd_resp_send(req, "OK", HTTPD_RESP_USE_STRLEN);
        return ESP_OK;
      }
      else // rakamlar limitler disinda
      {
        httpd_resp_send_404(req);
        return ESP_FAIL;
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

static esp_err_t winter_mode_read()
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

static esp_err_t theme_handler(httpd_req_t *req)
{
  Serial.println("theme_handler");
  char jsonbuffer[20];
  StaticJsonDocument<100> jsonresponse;
  jsonresponse["color"] = TEMA;
  serializeJson(jsonresponse, jsonbuffer);
  Serial.printf("theme_handler mem :%d", jsonresponse.memoryUsage());
  return httpd_resp_send(req, jsonbuffer, measureJson(jsonresponse));
  return ESP_OK;
}

static esp_err_t sysclock_status_handler(httpd_req_t *req)
{
  Serial.println("sysclock_status_handler");
  char jsonbuffer[200];
  StaticJsonDocument<100> jsonresponse;
  int sec = node.getResponseBuffer(node.readHoldingRegisters(SANIYE, 1));
  int min = node.getResponseBuffer(node.readHoldingRegisters(DAKIKA, 1));
  int hr = node.getResponseBuffer(node.readHoldingRegisters(SAAT, 1));
  jsonresponse["sec"] = sec;
  jsonresponse["min"] = min;
  jsonresponse["hr"] = hr;
  Serial.printf("Clock %d-%d-%d\n", hr, min, sec);
  serializeJson(jsonresponse, jsonbuffer);
  return httpd_resp_send(req, jsonbuffer, measureJson(jsonresponse));
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

static void readClock()
{
  int hr = node.getResponseBuffer(node.readHoldingRegisters(SAAT, 1));
  int min = node.getResponseBuffer(node.readHoldingRegisters(DAKIKA, 1));
  int sec = node.getResponseBuffer(node.readHoldingRegisters(SANIYE, 1));
  Serial.printf("Saat %d-%d-%d\n", hr, min, sec);
}

static esp_err_t redirect_home(httpd_req_t *req, httpd_err_code_t err)
{
  httpd_resp_set_status(req, "302 Temporary Redirect");
  httpd_resp_set_hdr(req, "Location", "/");
  httpd_resp_send(req, "Redirect to the captive portal", HTTPD_RESP_USE_STRLEN);
  ESP_LOGI(TAG, "Redirecting to root");
  return ESP_OK;
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

  httpd_uri_t sysclock_uri = {
      .uri = "/sysclock",
      .method = HTTP_POST,
      .handler = sysclock_handler,
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

  httpd_uri_t sysclock_status_uri = {
      .uri = "/sysclock",
      .method = HTTP_GET,
      .handler = sysclock_status_handler,
      .user_ctx = NULL};

  httpd_uri_t file_serve = {
      .uri = "/data/*",
      .method = HTTP_GET,
      .handler = download_handler,
      .user_ctx = server_data};

  httpd_uri_t theme_uri = {
      .uri = "/theme",
      .method = HTTP_GET,
      .handler = theme_handler,
      .user_ctx = NULL};

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
    httpd_register_uri_handler(gubre_siyirma, &theme_uri);
    httpd_register_uri_handler(gubre_siyirma, &sysclock_uri);
    httpd_register_uri_handler(gubre_siyirma, &sysclock_status_uri);
    httpd_register_err_handler(gubre_siyirma, HTTPD_404_NOT_FOUND, redirect_home);
  }

  return ESP_OK;
}

void WiFiStationConnected(WiFiEvent_t event, WiFiEventInfo_t info)
{
  Serial.println("");
  Serial.println("Connected to AP successfully!");
}

void Wifi_AP_Disconnected(WiFiEvent_t event, WiFiEventInfo_t info)
{
  Serial.println("Disconnected from AP");
  esp_restart();
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

  WiFi.onEvent(Wifi_AP_Disconnected, WiFiEvent_t::ARDUINO_EVENT_WIFI_AP_STADISCONNECTED);

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
  WiFi.setTxPower(WIFI_POWER_19_5dBm);
  esp_wifi_get_mac(WIFI_IF_STA, mac);
  // sprintf(softap_mac, "%s_%02X%02X", mdns_host, mac[4], mac[5]);
  WiFi.softAP(mdns_host_uppercase, ap_pwd);
  // WiFi.begin(ssid, password);
  WiFi.setHostname(mdns_host);
  // delay(100);
  Serial.print("Wifi access point created: http://");
  Serial.println(WiFi.softAPIP());

  mdns_init();
  mdns_hostname_set(mdns_host);
  Serial.printf("mdns hostname is set to: http://%s.local\n", mdns_host);
  mdns_instance_name_set(mdns_host);

  mdns_service_add("GubreSiyirmaWebServer", "_http", "_tcp", 80, NULL, 0);
  readAlarms();
  readAlarmStatus();
  readClock();
  winter_mode_read();
  mount_storage(base_path);
  nvs_flash_init();
  dnsServer.start(53, "*", WiFi.softAPIP());
  startServer("");
}

void loop()
{
  // dnsServer.processNextRequest();
  int temp = (int)(thermocouple.readCelsius() * 10);
  node.writeSingleRegister(TEMPERATURE, temp);
  Serial.printf("Temperature: %d\n", temp);
  delay(500);
}
