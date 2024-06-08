#include "main.h"
#include "limits.h"
#include <ArduinoJson.h>
#include <ModbusMaster.h>
#include <WiFi.h>
#include <esp_http_server.h>
#include <esp_log.h>
#include <esp_spiffs.h>
#include <esp_wifi.h>
#include <max6675.h>
#include <mdns.h>
#include <nvs_flash.h>
#include <soc/rtc_cntl_reg.h>
#include <WebSocketsClient.h>
#include <esp_http_client.h>
#include "esp_vfs.h"
#include <HardwareSerial.h>
#include <iostream>
#include <typeinfo>
#include <Ticker.h>
#include "esp_log.h"
#include <WiFiClient.h>
#include <WiFiAP.h>
#include <vector>
#include <algorithm>
#include <Preferences.h>
int con = 0;
int con1 = 0;
int say = 0;
int tek = 0;
float mesafe;
int fark = 0;
int rap = 0;
int first;
int numClients;

Preferences sakla;
WiFiServer server(81); // Server portu
Ticker timer1;
Ticker timer2;
Ticker checkClientsTicker;
// Bağlı istemcilerin MAC adreslerini saklayan vektör
std::vector<String> connectedClients;

WebSocketsClient socketIO;

// gerilim siniri arayuzde input -> oku ve yaz 0x11B8
// sarj siniri 0x11BA ikisinde de V volt yazacak

long last_fwd_time = 0;
long last_bk_time = 0;
volatile bool readEnabled = true;
int counter = 0;
int r = 0, d;
// volatile bool readenable = false;
volatile bool rightenable = false;

httpd_handle_t gubre_siyirma = NULL;
ModbusMaster node;
uint8_t mac[6];
char softap_mac[18] = {0};
MAX6675 thermocouple(thermoCLK, thermoCS, thermoSO);

uint16_t readHoldingRegisters(uint16_t u16ReadAddress, uint16_t u16ReadQty)
{
#ifndef DEBUG
  return node.getResponseBuffer(
      node.readHoldingRegisters(u16ReadAddress, u16ReadQty));
#else
  return 0;
#endif
}

uint8_t writeSingleRegister(uint16_t u16WriteAddress, uint16_t u16WriteValue)
{
#ifndef DEBUG
  return node.writeSingleRegister(u16WriteAddress, u16WriteValue);
#else
  return 0;
#endif
}

uint8_t writeSingleCoil(uint16_t u16WriteAddress, uint8_t u8State)
{
#ifndef DEBUG
  return node.writeSingleCoil(u16WriteAddress, u8State);
#else
  return 0;
#endif
}
uint8_t readCoils(uint16_t u16WriteAddress, uint8_t u8State)
{
#ifndef DEBUG
  return node.getResponseBuffer(node.readCoils(u16WriteAddress, u8State));
#else
  return 0;
#endif
}
void timer2Handler()
{
  Serial.println("Timer 2 triggered");
  if (con1 < 1)
  {
    first = mesafe;
    Serial.println("firste akatrdi");
  }
  con1++;
  if (con1 >= saved_lokaltim)
  {
    fark = mesafe - first;
    if (fark <= saved_lokalfark)
    {
      rightenable = true;
      node.writeSingleCoil(PATIYA, HIGH);
      vTaskDelay(pdMS_TO_TICKS(500));
      node.writeSingleCoil(PATIYA, LOW);
      rightenable = false;
      con1 = 0;
    }
    else
    {
      con1 = 0;
    }
  }
  // Serial.print("con1");
  // Serial.println(con1);
  // Serial.print("mesafe");
  // Serial.println(mesafe);
  // Serial.print("first:");
  // Serial.println(first);
}

// Hedef MAC adresi (örnek olarak verilmiştir)

void checkConnectedClients()
{
  // Bağlı istemci sayısını al
  int numClients = WiFi.softAPgetStationNum();
  Serial.printf("clients: %d\n", numClients);

  // Bağlı istemcilerin MAC adreslerini al
  wifi_sta_list_t stationList;
  tcpip_adapter_sta_list_t adapterStationList;

  memset(&stationList, 0, sizeof(stationList));
  memset(&adapterStationList, 0, sizeof(adapterStationList));

  // İstemci listesini al
  esp_wifi_ap_get_sta_list(&stationList);
  tcpip_adapter_get_sta_list(&stationList, &adapterStationList);
  // Geçici vektörler
  std::vector<String> currentClients;
  std::vector<String> disconnectedClients;

  // Bağlı tüm istemcilerin MAC adreslerini al ve geçici vektöre ekle
  for (int i = 0; i < stationList.num; i++)
  {
    tcpip_adapter_sta_info_t station = adapterStationList.sta[i];
    char macStr[18];
    sprintf(macStr, "%02X:%02X:%02X:%02X:%02X:%02X",
            station.mac[0], station.mac[1], station.mac[2],
            station.mac[3], station.mac[4], station.mac[5]);
    currentClients.push_back(String(macStr));

    // Hedef MAC adresiyle karşılaştır
    if (String(macStr) == hedefMAC)
    {
      WiFiClient client = server.available();
      if (client)
      {
        // İstemci bağlıysa devam edin
        if (client.connected())
        {
          String request = client.readStringUntil('\r');
          if (request.length() > 0)
          {

            if (request.startsWith("mesafe: "))
            {
              int index = request.indexOf("mesafe: ");
              if (index != -1)
              {
                String mesafeStr = request.substring(index + 8); // "mesafe: " 8 karakter uzunluğunda
                mesafe = mesafeStr.toFloat();
              }
            }
          }
        }
        client.stop();
        // Serial.print("client ok: ");
        rap = 0;
      }
      else
      {
        rap++;
      }
      if (rap > 5)
      {
        mesafe = -1000;
        // Serial.print("11111 ");
      }
    }
  }
  if (stationList.num == 0)
  {
    mesafe = -1000;
    // Serial.print("33333");
  }
  // Bağlantısı kesilmiş istemcileri tespit et
  for (const auto &client : connectedClients)
  {
    if (std::find(currentClients.begin(), currentClients.end(), client) == currentClients.end())
    {
      disconnectedClients.push_back(client);
    }
  }

  // Bağlantısı kesilmiş istemcileri yazdır
  for (const auto &client : disconnectedClients)
  {
    Serial.printf("Client disconnected - MAC: %s\n", client.c_str());
    if (client.c_str() != hedefMAC)
    {
      esp_restart();
    }
  }

  // Güncel bağlı istemciler listesini kaydet
  connectedClients = currentClients;
}

void sendMesaj()
{

  char clockbuffer[6];
  DynamicJsonDocument doc(1024);
  JsonArray array = doc.to<JsonArray>();
  JsonObject response = array.createNestedObject();
  response["status"] = "ok";
  response["token"] = token_num;
  response["machineSubID"] = IDNumber;
  response["machineCity"] = city;

  if (rightenable == false)
  {

    d = r;
    r = readHoldingRegisters(state_code, 1);
    Serial.print("hata durumunu:");
    Serial.println(r);

    // Eğer mesafe geçerli bir sayıysa
    Serial.print("Mesafe: ");
    Serial.println(mesafe);

    response["status_code"] = r;
    if (r == 1 || r == 2 || r == 3 || r == 4 || r == 5 || r == 6 || r == 7 || r == 8 || r == 9)
    {
      response["type"] = TEL;
    }
    else if (r == 103 || r == 104)
    {
      response["type"] = SMS;
    }
    else if (r != 0)
    {
      response["type"] = ALL;
    }

    // response["status_code"] = 2;
  }
  String output;
  serializeJson(doc, output);
  if (r != d || r == 99)
  {
    if (r != 99 && r != 65535)
    {
      Serial.println("mesaj gonderildi");
      socketIO.sendTXT(output);
    }

    // return OP_OK;
    if (r == 1 || r == 99 && tek < 1)
    {

      Serial.println("timer çağırıldı");
      timer2.attach(60.0, timer2Handler); // Her 60 saniyede bir (1 dakika)
      tek++;
    }
    else if (r == 2 || r == 4)
    {

      timer2.detach();
      con1 = 0;
      tek = 0;
    }
  }
  if (mesafe == -1000.00)
  {

    timer2.detach();
    con1 = 0;
    tek = 0;
  }
  // Serial.print("tek:");
  // Serial.println(tek);
}

op_errors parse_ektur(const JsonObject &doc)
{
  rightenable = true;

  int tourdist = doc["ektur"][0]["mesafe"];
  ESP_LOGI(TAG, "Tour Dist:%d", tourdist);
  writeSingleRegister(TOUR_DIST, tourdist);
  writeSingleRegister(EKTUR_ILERI, 1);
  DynamicJsonDocument doc1(1024);
  JsonArray array = doc1.to<JsonArray>();
  JsonObject response = array.createNestedObject();
  response["saved"] = "ektur";
  response["myID"] = IDNumber;
  String output;
  serializeJson(doc1, output);
  ESP_LOGI(TAG, "giden mesdaj değeri:");
  serializeJson(doc1, Serial);
  socketIO.sendTXT(output);
  // vTaskDelay(pdMS_TO_TICKS(100));
  vTaskDelay(pdMS_TO_TICKS(500));
  writeSingleRegister(EKTUR_ILERI, 0);
  rightenable = false;
  return OP_OK;
}

static esp_err_t tur_ileri_handler(httpd_req_t *req)
{
  rightenable = true;
  node.writeSingleCoil(EKTUR_ILERI, HIGH);
  Serial.printf("ileri ektur");
  delay(250);
  ESP_LOGI(TAG, "ileri turn");
  node.writeSingleCoil(EKTUR_ILERI, LOW);
  httpd_resp_set_type(req, "text/html");
  rightenable = false;
  return httpd_resp_send(req, "ileri", 5);
}

static esp_err_t tur_geri_handler(httpd_req_t *req)
{
  rightenable = true;
  httpd_resp_set_type(req, "text/html");
  node.writeSingleCoil(EKTUR_GERI, HIGH);
  Serial.printf("geri ektur");
  delay(250);

  ESP_LOGI(TAG, "geri turn");
  node.writeSingleCoil(EKTUR_GERI, LOW);
  rightenable = false;
  return httpd_resp_send(req, "geri", 4);
}
static esp_err_t restart_handler(httpd_req_t *req)
{

  httpd_resp_set_type(req, "text/html");
  delay(250);
  esp_restart();
}

static const char *get_path_from_uri(char *dest, const char *base_path,
                                     const char *uri, size_t destsize)
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

static esp_err_t set_content_type_from_file(httpd_req_t *req,
                                            const char *filename)
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

  esp_vfs_spiffs_conf_t conf = {.base_path = base_path,
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
    ESP_LOGE(TAG, "Failed to get SPIFFS partition information (%s)",
             esp_err_to_name(ret));
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

  const char *filename = get_path_from_uri(
      filepath, ((struct file_server_data *)req->user_ctx)->base_path, req->uri,
      sizeof(filepath));
  if (strcmp(filename, "/") == 0)
  {
    filename = "/data/index.html";
    strncpy(filepath, "/data/index.html", 17);
  }

  if (!filename)
  {
    ESP_LOGE(TAG, "Filename is too long");
    /* Respond with 500 Internal Server Error */
    httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR,
                        "Filename too long");
    return ESP_FAIL;
  }
  if (strcmp(&filepath[strlen(filepath) - 4], ".map") == 0 ||
      strcmp(&filepath[strlen(filepath) - 7], ".map.js") == 0) // Skip map files
  {
    return ESP_FAIL;
  }
  else
  {
    String fl = String(filepath) + String(".gz");
    fd = fopen(fl.c_str(), "r");
    if (!fd)
    {
      ESP_LOGE(TAG, "Failed to read existing file : %s", filepath);
      httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR,
                          "Failed to read existing file");
      return ESP_FAIL;
    }
  }

  ESP_LOGI(TAG, "Sending file : %s (%ld bytes)...", filename,
           file_stat.st_size);
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
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR,
                            "Failed to send file");
        return ESP_FAIL;
      }
    }

  } while (chunksize != 0);

  fclose(fd);
  ESP_LOGI(TAG, "File sending complete");
  httpd_resp_send_chunk(req, NULL, 0);
  return ESP_OK;
}

static esp_err_t winter_status_handler(httpd_req_t *req)
{
  char jsonbuffer[600];
  StaticJsonDocument<600> jsonresponse;
  jsonresponse["kismodu"] =
      node.getResponseBuffer(node.readHoldingRegisters(KIS_MODU_AKTIF, 1));
  jsonresponse["ileri"] =
      node.getResponseBuffer(node.readHoldingRegisters(ILERI_SURE, 1));
  jsonresponse["geri"] =
      node.getResponseBuffer(node.readHoldingRegisters(GERI_SURE, 1));
  jsonresponse["gecikme"] =
      node.getResponseBuffer(node.readHoldingRegisters(GECIKME, 1));
  jsonresponse["hr"] =
      node.getResponseBuffer(node.readHoldingRegisters(SAAT_READ, 1));
  jsonresponse["min"] =
      node.getResponseBuffer(node.readHoldingRegisters(DAKIKA_READ, 1));
  jsonresponse["clk_active"] =
      node.getResponseBuffer(node.readHoldingRegisters(CLOCKS_ENABLED, 1));
  jsonresponse["cr_fwd"] =
      node.getResponseBuffer(node.readHoldingRegisters(AKIM_ILERI, 1)) * 100;
  jsonresponse["cr_back"] =
      node.getResponseBuffer(node.readHoldingRegisters(AKIM_GERI, 1)) * 100;
  jsonresponse["encoder"] =
      node.getResponseBuffer(node.readHoldingRegisters(ENCODER_EN, 1));
  jsonresponse["gerilimlimit"] =
      node.getResponseBuffer(node.readHoldingRegisters(GERILIM_LIMIT, 1)) * 100;
  jsonresponse["sarjlimlimit"] =
      node.getResponseBuffer(node.readHoldingRegisters(SARJ_LIMIT, 1)) * 100;
  jsonresponse["gerilimkalb"] =
      node.getResponseBuffer(node.readHoldingRegisters(GERILIM_kalb, 1));
  jsonresponse["sarjkalb"] =
      node.getResponseBuffer(node.readHoldingRegisters(SARJ_kalb, 1));
  jsonresponse["ileriKalb"] =
      node.getResponseBuffer(node.readHoldingRegisters(ilerisure_kalb, 1));
  jsonresponse["geriKalb"] =
      node.getResponseBuffer(node.readHoldingRegisters(gerisure_kalb, 1));
  uint16_t birinci_mod =
      node.getResponseBuffer(node.readHoldingRegisters(BIRINCI_MOD, 1));
  float mod1 = (float)*((int16_t *)&birinci_mod);
  uint16_t ikinci_mod =
      node.getResponseBuffer(node.readHoldingRegisters(IKINCI_MOD, 1));
  float mod2 = (float)*((int16_t *)&ikinci_mod);
  jsonresponse["birinci"] = mod1;
  jsonresponse["ikinci"] = mod2;
  jsonresponse["sinyaltime"] = saved_lokaltim;
  jsonresponse["sinyalfark"] = saved_lokalfark;

  serializeJson(jsonresponse, jsonbuffer);
  ESP_LOGI(TAG, "%s mem:%d\n", __func__, jsonresponse.memoryUsage());
  return httpd_resp_send(req, jsonbuffer, measureJson(jsonresponse));
  return ESP_OK;
}

static esp_err_t temp_status_handler(httpd_req_t *req)
{
  char jsonbuffer[100];
  StaticJsonDocument<50> jsonresponse;
  jsonresponse["temp"] = thermocouple.readCelsius();
  serializeJson(jsonresponse, jsonbuffer);
  ESP_LOGI(TAG, "%s mem:%d\n", __func__, jsonresponse.memoryUsage());
  return httpd_resp_send(req, jsonbuffer, measureJson(jsonresponse));
  return ESP_OK;
}

static esp_err_t limits_handler(httpd_req_t *req)
{
  char buffer[64] = {
      0,
  };

  size_t buf_len;
  char *remaining;
  buf_len = httpd_req_get_url_query_len(req) + 1;

  char current_fwd_buf[4] = {0};
  char current_back_buf[4] = {0};

  int current_fwd = 0;
  int current_back = 0;

  if (buf_len > 1)
  {
    if (httpd_req_get_url_query_str(req, buffer, buf_len) == ESP_OK)
    {
      if (httpd_query_key_value(buffer, "voltagelimit", current_fwd_buf,
                                sizeof(buffer)) == ESP_OK)
      {
        current_fwd = float(strtol(current_fwd_buf, &remaining, 10));
        Serial.printf("voltagelimit:%d\n", current_fwd);
        if (voltage_l <= current_fwd && current_fwd <= voltage_h)
        {
          // float x = current_fwd * 10.0;
          rightenable = true;
          node.writeSingleRegister(GERILIM_LIMIT, current_fwd / 100);
        }
        // else // current_fwd limitleri disinda ise 404 dön
        // {
        //   httpd_resp_send_404(req);
        //   return ESP_FAIL;
        // }
      }
      if (httpd_query_key_value(buffer, "chargelimit", current_back_buf,
                                sizeof(buffer)) == ESP_OK)
      {
        current_back = strtol(current_back_buf, &remaining, 10);
        Serial.printf("chargelimit:%d\n", current_back);
        if (voltage_l <= current_back && current_back <= voltage_h)
        {
          // float y = current_fwd * 10.0;
          rightenable = true;
          node.writeSingleRegister(SARJ_LIMIT, current_back / 100);
        }
        // else // current_back limitleri disinda ise
        // {
        //   httpd_resp_send_404(req);
        //   return ESP_FAIL;
        // }
      }
    }
    // httpd_resp_send_404(req);
    // return ESP_FAIL;
  }
  // httpd_resp_send_404(req);
  // return ESP_FAIL;
  rightenable = false;
  httpd_resp_send(req, "OK", HTTPD_RESP_USE_STRLEN);
  return ESP_OK;
}

static esp_err_t ali_handler(httpd_req_t *req)
{
  char buffer[64] = {
      0,
  };

  size_t buf_len;
  char *remaining;
  buf_len = httpd_req_get_url_query_len(req) + 1;

  char current_fwd_buf[4] = {0};
  char current_back_buf[4] = {0};

  int current_fwd = 0;
  int current_back = 0;

  if (buf_len > 1)
  {
    if (httpd_req_get_url_query_str(req, buffer, buf_len) == ESP_OK)
    {
      if (httpd_query_key_value(buffer, "voltagekalb", current_fwd_buf,
                                sizeof(buffer)) == ESP_OK)
      {
        current_fwd = strtol(current_fwd_buf, &remaining, 10);
        Serial.printf("voltagekalb:%d\n", current_fwd);
        if (voltage_l <= current_fwd && current_fwd <= voltage_h)
        {
          rightenable = true;
          node.writeSingleRegister(GERILIM_kalb, current_fwd);
        }
      }
      if (httpd_query_key_value(buffer, "chargekalb", current_back_buf,
                                sizeof(buffer)) == ESP_OK)
      {
        current_back = strtol(current_back_buf, &remaining, 10);
        Serial.printf("chargekalb:%d\n", current_back);
        if (voltage_l <= current_back && current_back <= voltage_h)
        {
          rightenable = true;
          node.writeSingleRegister(SARJ_kalb, current_back);
        }
      }
    }
  }

  rightenable = false;
  httpd_resp_send(req, "OK", HTTPD_RESP_USE_STRLEN);
  return ESP_OK;
}

static esp_err_t timekalb_handler(httpd_req_t *req)
{
  Serial.printf("timekalb_hundler ok");
  char buffer[64] = {
      0,
  };

  size_t buf_len;
  char *remaining;
  buf_len = httpd_req_get_url_query_len(req) + 1;

  char current_fwd_buf[4] = {0};
  char current_back_buf[4] = {0};

  int current_fwd = 0;
  int current_back = 0;

  if (buf_len > 1)
  {
    if (httpd_req_get_url_query_str(req, buffer, buf_len) == ESP_OK)
    {
      if (httpd_query_key_value(buffer, "ilerikalb", current_fwd_buf,
                                sizeof(buffer)) == ESP_OK)
      {
        current_fwd = strtol(current_fwd_buf, &remaining, 10);
        Serial.printf("ilreidly:%d\n", current_fwd);
        if (surekalb_l <= current_fwd && current_fwd <= surekalb_h)
        {
          rightenable = true;
          node.writeSingleRegister(ilerisure_kalb, current_fwd);
        }
      }
      if (httpd_query_key_value(buffer, "gerikalb", current_back_buf,
                                sizeof(buffer)) == ESP_OK)
      {
        current_back = strtol(current_back_buf, &remaining, 10);
        Serial.printf("geridly:%d\n", current_back);
        if (surekalb_l <= current_back && current_back <= surekalb_h)
        {
          rightenable = true;
          node.writeSingleRegister(gerisure_kalb, current_back);
        }
      }
    }
  }

  rightenable = false;
  httpd_resp_send(req, "OK", HTTPD_RESP_USE_STRLEN);
  return ESP_OK;
}

static esp_err_t lockalb_handler(httpd_req_t *req)
{
  Serial.printf("lockalb_hundler ok");
  char buffer[64] = {
      0,
  };

  size_t buf_len;
  char *remaining;
  buf_len = httpd_req_get_url_query_len(req) + 1;

  char current_fwd_buf[4] = {0};
  char current_back_buf[4] = {0};

  int current_fwd = 0;
  int current_back = 0;

  if (buf_len > 1)
  {
    if (httpd_req_get_url_query_str(req, buffer, buf_len) == ESP_OK)
    {
      if (httpd_query_key_value(buffer, "loctime", current_fwd_buf,
                                sizeof(buffer)) == ESP_OK)
      {
        current_fwd = strtol(current_fwd_buf, &remaining, 10);
        Serial.printf("loctime:%d\n", current_fwd);
        if (surekalb_l <= current_fwd && current_fwd <= surekalb_h)
        {
          // rightenable = true;
          // node.writeSingleRegister(ilerisure_kalb, current_fwd);
        }
      }
      if (httpd_query_key_value(buffer, "locfark", current_back_buf,
                                sizeof(buffer)) == ESP_OK)
      {
        current_back = strtol(current_back_buf, &remaining, 10);
        Serial.printf("locfark:%d\n", current_back);
        if (surekalb_l <= current_back && current_back <= surekalb_h)
        {
          // rightenable = true;
          // node.writeSingleRegister(gerisure_kalb, current_back);
        }
      }
    }
  }
  lokaltim = current_fwd;
  lokalfark = current_back;
  // Değerleri kaydet
  sakla.begin("myApp", false);
  sakla.putInt("loctime", lokaltim);
  sakla.putInt("locfark", lokalfark);
  sakla.end();
  httpd_resp_send(req, "OK", HTTPD_RESP_USE_STRLEN);
  return ESP_OK;
}

static esp_err_t encoder_handler(httpd_req_t *req)
{
  char buffer[16] = {
      0,
  };

  size_t buf_len;
  char *remaining;
  buf_len = httpd_req_get_url_query_len(req) + 1;

  char enc_buf[3] = {0};
  int encoder = 0;

  if (buf_len > 1)
  {
    if (httpd_req_get_url_query_str(req, buffer, buf_len) == ESP_OK)
    {
      if (httpd_query_key_value(buffer, "enc", enc_buf, sizeof(buffer)) ==
          ESP_OK)
      {
        encoder = strtol(enc_buf, &remaining, 10);
        rightenable = true;
        node.writeSingleRegister(ENCODER_EN, encoder);
        Serial.printf("ENCODER_EN:%d\n", encoder);
        rightenable = false;
      }
      else
      {
        Serial.printf("key fail");
        httpd_resp_send_404(req);
        rightenable = false;
        return ESP_FAIL;
      }
    }
  }
  else
  {
    Serial.printf("len fail");
    httpd_resp_send_404(req);
    return ESP_FAIL;
  }
  memset(buffer, 0, sizeof(buffer));
  httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
  return httpd_resp_send(req, NULL, 0);
}

static esp_err_t currentctrl_handler(httpd_req_t *req)
{
  char buffer[64] = {
      0,
  };

  size_t buf_len;
  char *remaining;
  buf_len = httpd_req_get_url_query_len(req) + 1;

  char current_fwd_buf[3] = {0};
  char current_back_buf[3] = {0};
  int current_fwd = 0;
  int current_back = 0;

  if (buf_len > 1)
  {
    if (httpd_req_get_url_query_str(req, buffer, buf_len) == ESP_OK)
    {

      if (httpd_query_key_value(buffer, "current_fwd", current_fwd_buf,
                                sizeof(buffer)) == ESP_OK)
      {
        current_fwd = strtol(current_fwd_buf, &remaining, 10);
        Serial.printf("current_fwd:%.d\n", current_fwd);
        if (current_l <= current_fwd && current_fwd <= current_h)
        {
          rightenable = true;
          node.writeSingleRegister(AKIM_ILERI, current_fwd / 100);
        }
        // else // current_fwd limitleri disinda ise 404 dön
        // {
        //   Serial.println("current_fwd limit disi111");
        //   httpd_resp_send_404(req);
        //   return ESP_FAIL;
        // }
      }

      if (httpd_query_key_value(buffer, "current_back", current_back_buf,
                                sizeof(buffer)) == ESP_OK)
      {
        current_back = strtol(current_back_buf, &remaining, 10);
        Serial.printf("current_back:%d\n", current_back);
        if (current_l <= current_back && current_back <= current_h)
        {
          rightenable = true;
          node.writeSingleRegister(AKIM_GERI, current_back / 100);
        }
        // else // current_back limitleri disinda ise
        // {
        //   Serial.println("current_back limit disi");
        //   httpd_resp_send_404(req);
        //   return ESP_FAIL;
        // }
      }
    }
  }
  //  else {
  //   Serial.println("no key");
  //   httpd_resp_send_404(req);
  //   return ESP_FAIL;
  // }
  rightenable = false;
  httpd_resp_send(req, "OK", HTTPD_RESP_USE_STRLEN);
  return ESP_OK;
}

static esp_err_t status_handler(httpd_req_t *req)
{
  char jsonbuffer[250];
  char hr_buffer[3];
  char min_buffer[3];
  char sec_buffer[3];

  StaticJsonDocument<150> jsonresponse;
  jsonresponse["volt"] =
      (float)node.getResponseBuffer(node.readHoldingRegisters(VOLTAGE, 1)) /
      10.0;

  // negatif okuma
  uint16_t amp = node.getResponseBuffer(node.readHoldingRegisters(CURRENT, 1));
  int16_t rvalue = *((int16_t *)&amp);
  float amper = rvalue;
  jsonresponse["amp"] = amper / 10.0;

  // negatif okuma
  uint16_t sigstr = mesafe;
  int16_t rvalue1 = *((int16_t *)&sigstr);
  float amper1 = rvalue1;
  jsonresponse["sigstr"] = amper1;

  // const QString entry_hs_in_temp =
  // QString::number(static_cast<qint16>(unit.value(0))); uint16_t mvalue =
  // ModbusRTUClient.read(); int16_t rvalue = *((in16_t *)&mvalue); float units
  // = 0.001 * rvalue;

  int sec = node.getResponseBuffer(node.readHoldingRegisters(SANIYE_READ, 1));
  int min = node.getResponseBuffer(node.readHoldingRegisters(DAKIKA_READ, 1));
  int hr = node.getResponseBuffer(node.readHoldingRegisters(SAAT_READ, 1));
  sprintf(sec_buffer, "%02d", sec);
  sprintf(min_buffer, "%02d", min);
  sprintf(hr_buffer, "%02d", hr);
  jsonresponse["sec"] = sec_buffer;
  jsonresponse["min"] = min_buffer;
  jsonresponse["hr"] = hr_buffer;
  int d = node.getResponseBuffer(node.readHoldingRegisters(DURUM, 1));
  switch (d)
  {
  case 0:
    // jsonresponse["status"] = "Durum: Robot Duruyor.";
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
  case 11:
    // jsonresponse["status"] = "Durum: Şarj Kontrolu";
    jsonresponse["status_code"] = "11";
    break;
  case 12:
    // jsonresponse["status"] = "Durum: Robot Takıldı";
    jsonresponse["status_code"] = "12";
    break;
  default:
    // jsonresponse["status"] = "HATA";
    jsonresponse["status_code"] = "13";
    break;
  }
  serializeJson(jsonresponse, jsonbuffer);
  ESP_LOGI(TAG, "%s mem:%d\n", __func__, jsonresponse.memoryUsage());
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
      if (httpd_query_key_value(buffer, "go", buffer, sizeof(buffer)) ==
          ESP_OK)
      {
      }
      else
      {
        httpd_resp_send_404(req); // time parametresi yoksa
        return ESP_FAIL;
      }
    }
    else
    {
      httpd_resp_send_404(req); // go parametresi yoksa
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
    int f_diff = millis() - last_bk_time;
    if (f_diff > 5000)
    {
      last_fwd_time = millis();
      digitalWrite(LED_PIN, 1);
      rightenable = true;
      node.writeSingleCoil(FORWARD_ADDRESS, HIGH);
      rightenable = false;
    }
    else
    {
      ESP_LOGE(TAG, "bk <5000 %d", f_diff);
    }
  }
  else if (!strcmp(buffer, "left"))
  {
    // Serial.println("Left");
    rightenable = true;
    node.writeSingleCoil(LEFT_ADDRESS, HIGH);
    digitalWrite(LED_PIN, 1);
    rightenable = false;
  }
  else if (!strcmp(buffer, "right"))
  {
    // Serial.println("Right");
    rightenable = true;
    node.writeSingleCoil(RIGHT_ADDRESS, HIGH);
    digitalWrite(LED_PIN, 1);
    rightenable = false;
  }
  else if (!strcmp(buffer, "backward"))
  {
    // Serial.println("Backward");
    int bk_diff = millis() - last_fwd_time;
    if (bk_diff > 5000)
    {
      last_bk_time = millis();
      rightenable = true;
      node.writeSingleCoil(BACKWARD_ADDRESS, HIGH);
      digitalWrite(LED_PIN, 1);
      rightenable = false;
    }
    else
    {
      ESP_LOGE(TAG, "fwd < 5000 %d", bk_diff);
    }
  }
  else if (!strcmp(buffer, "stop"))
  {
    // Serial.println("Stop");
    rightenable = true;
    node.writeSingleCoil(STOP_ADDRESS, HIGH);
    digitalWrite(LED_PIN, 1);
    rightenable = false;
  }
  else if (!strcmp(buffer, "ektur"))
  {
    // Serial.println("Ektur");
  }

  else if (!strcmp(buffer, "x"))
  {
    rightenable = true;
    node.writeSingleCoil(FORWARD_ADDRESS, LOW);
    node.writeSingleCoil(BACKWARD_ADDRESS, LOW);
    node.writeSingleCoil(RIGHT_ADDRESS, LOW);
    node.writeSingleCoil(LEFT_ADDRESS, LOW);
    node.writeSingleCoil(STOP_ADDRESS, LOW);
    digitalWrite(LED_PIN, 0);
    rightenable = false;
    // Serial.println("x");
    // ESP_LOGE(TAG, "fwd:%d bk:%d", last_fwd_time, last_bk_time);
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

      if (httpd_query_key_value(buffer, "mode1", mode1_buf, sizeof(buffer)) ==
          ESP_OK)
      {
        firstmode = strtol(mode1_buf, &remaining, 10);
        if (firstmode_l <= firstmode && firstmode <= firstmode_h)
        {
          rightenable = true;
          node.writeSingleRegister(BIRINCI_MOD, firstmode);
          Serial.printf("first mode:%d\n", firstmode);
          rightenable = false;
        }
        else // firstmode limitleri disinda ise 404 dön
        {
          httpd_resp_send_404(req);
          return ESP_FAIL;
        }
      }
      if (httpd_query_key_value(buffer, "mode2", mode2_buf, sizeof(buffer)) ==
          ESP_OK)
      {
        secondmode = strtol(mode2_buf, &remaining, 10);
        if (secondmode_l <= secondmode && secondmode <= secondmode_h)
        {
          rightenable = true;
          node.writeSingleRegister(IKINCI_MOD, secondmode);
          Serial.printf("second mode:%d\n", secondmode);
          rightenable = false;
        }

        else // secondmode limitleri disinda ise
        {
          httpd_resp_send_404(req);
          return ESP_FAIL;
        }
      }
      if (httpd_query_key_value(buffer, "winter", wintermode_buf,
                                sizeof(buffer)) == ESP_OK)
      {
        int w = strtol(wintermode_buf, &remaining, 10);
        wintermode = w ? 1 : 0;
        if (wintermode == 0 || wintermode == 1)
        {
          rightenable = true;
          node.writeSingleRegister(KIS_MODU_AKTIF, wintermode);
          Serial.printf("winter mode:%d\n", wintermode);
          rightenable = false;
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

  char clock_active_buf[3] = {0};
  char hour_buf[3] = {0};
  char minute_buf[3] = {0};

  int hour = 0;
  int minute = 0;
  bool clock_active = false;

  if (buf_len > 1)
  {
    if (httpd_req_get_url_query_str(req, buffer, buf_len) == ESP_OK)
    {
      if (httpd_query_key_value(buffer, "hour", hour_buf, sizeof(buffer)) ==
          ESP_OK)
      {
        hour = strtol(hour_buf, &remaining, 10);
        if (0 <= hour && hour <= 23)
        {
          rightenable = true;
          node.writeSingleRegister(SAAT_WRITE, hour);
          Serial.printf("hour_buf:%d\n", hour);
          rightenable = false;
        }
        else // hour limitleri disinda ise 404 dön
        {
          ESP_LOGE(TAG, "hour_buf limitleri disinda");
          httpd_resp_send_404(req);
          return ESP_FAIL;
        }
      }
      if (httpd_query_key_value(buffer, "minute", minute_buf, sizeof(buffer)) ==
          ESP_OK)
      {
        minute = strtol(minute_buf, &remaining, 10);
        if (0 <= minute && minute <= 59)
        {
          rightenable = true;
          node.writeSingleRegister(DAKIKA_WRITE, minute);
          Serial.printf("minute_buf:%d\n", minute);
          rightenable = false;
        }

        else // minute limitleri disinda ise
        {
          ESP_LOGE(TAG, "minute_buf limitleri disinda");
          httpd_resp_send_404(req);
          return ESP_FAIL;
        }
      }
      if (httpd_query_key_value(buffer, "active", clock_active_buf,
                                sizeof(buffer)) == ESP_OK)
      {
        int w = strtol(clock_active_buf, &remaining, 10);
        clock_active = w ? 1 : 0;
        if (clock_active == 0 || clock_active == 1)
        {
          rightenable = true;
          node.writeSingleRegister(CLOCKS_ENABLED, clock_active);
          Serial.printf("active:%d\n", clock_active);
          rightenable = false;
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
    ESP_LOGE(TAG, "URL query key yok.");

    httpd_resp_send_404(req);
    return ESP_FAIL;
  }
  httpd_resp_send_404(req);
  return ESP_FAIL;
}

static esp_err_t manual_mode_handler(httpd_req_t *req)
{
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
      if (httpd_query_key_value(buffer, "sure", sure_buf, sizeof(buffer)) ==
          ESP_OK)
      {
        sure = strtol(sure_buf, &remaining, 10);
      }

      if (httpd_query_key_value(buffer, "ilerisure", ileri_sure_buf,
                                sizeof(buffer)) == ESP_OK)
      {
        ileri_sure = strtol(ileri_sure_buf, &remaining, 10);
      }
      if (httpd_query_key_value(buffer, "gecikme", gecikme_buf,
                                sizeof(buffer)) == ESP_OK)
      {
        gecikme = strtol(gecikme_buf, &remaining, 10);
      }
      Serial.printf("sure:%d ileri_sure:%d gecikme:%d\n", sure, ileri_sure,
                    gecikme);

      if ((sure_l <= sure && sure <= sure_h) &&
          (ileri_sure_l <= ileri_sure && ileri_sure <= ileri_sure_h) &&
          gecikme_l <= gecikme && gecikme <= gecikme_h)
      {
        ESP_LOGI(TAG, "sure:%d ileri_sure:%d gecikme:%d\n", sure, ileri_sure,
                 gecikme);
        rightenable = true;
        node.writeSingleRegister(GERI_SURE, sure);
        node.writeSingleRegister(ILERI_SURE, ileri_sure);
        node.writeSingleRegister(GECIKME, gecikme);
        rightenable = false;
        httpd_resp_send(req, "OK", HTTPD_RESP_USE_STRLEN);

        return ESP_OK;
      }
      else // rakamlar limitler disinda
      {
        ESP_LOGE(TAG, "Rakamlar limitler disinda");
        httpd_resp_send_404(req);
        return ESP_FAIL;
      }
    }
    ESP_LOGE(TAG, "URL query key yok.");
    httpd_resp_send_404(req);
    return ESP_FAIL;
  }
  httpd_resp_send_404(req);

  return ESP_FAIL;
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
      if (httpd_query_key_value(buffer, "alarms", buffer, sizeof(buffer)) ==
          ESP_OK)
      {
        char *token, *subtoken;
        char *saveptr1, *saveptr2;
        int saat, dakika;
        bool checkboxStatus;
        int i = 0;
        int addr = ALARM_ADDR_BEGIN;
        int alarm_status_begin = ALARM_SETTINGS_ADDR_BEGIN;
        for (token = strtok_r(buffer, "[", &saveptr1); token != NULL;
             token = strtok_r(NULL, "[", &saveptr1))
        {
          for (subtoken = strtok_r(token, ",", &saveptr2); subtoken != NULL;
               subtoken = strtok_r(NULL, ",", &saveptr2))
          {
            rightenable = true;
            saat = atoi(subtoken);
            subtoken = strtok_r(NULL, ",", &saveptr2);
            dakika = atoi(subtoken);
            subtoken = strtok_r(NULL, ",", &saveptr2);
            checkboxStatus = atoi(subtoken);
            node.writeSingleRegister(addr, saat);
            node.writeSingleRegister(addr + 1, dakika);
            node.writeSingleRegister(alarm_status_begin + i, checkboxStatus);
            rightenable = false;
            // 180 Serial.printf("i:%d saat: %02d, dakika: %02d on_off:%d\n", i,
            // saat, dakika, checkboxStatus);
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
  ESP_LOGI(TAG, "Alarms saved in %d ms\n", t2 - t1);

  return ESP_OK;
}

static esp_err_t get_times_handler(httpd_req_t *req)
{
  char jsonbuffer[100];
  StaticJsonDocument<280> jsonresponse;
  char buffer[8];
  int result = node.readHoldingRegisters(ALARM_ADDR_BEGIN, 16);
  if (result == node.ku8MBSuccess)
  {
    for (int j = 0; j < 16; j += 2)
    {
      int hour = node.getResponseBuffer(j);
      int minute = node.getResponseBuffer(j + 1);
      bool checkboxStatus = node.getResponseBuffer(
          node.readHoldingRegisters(ALARM_SETTINGS_ADDR_BEGIN + (j / 2), 1));
      snprintf(buffer, sizeof(buffer), "%02d:%02d,%d", hour, minute,
               checkboxStatus);
      jsonresponse.add(buffer);
      // Serial.printf(" %X %d
      // %d\n",ALARM_SETTINGS_ADDR_BEGIN+(j/2),checkboxStatus,j/2);
    }
  }
  else
  {
    ESP_LOGE(TAG, "Alarm read error");
  }
  serializeJson(jsonresponse, jsonbuffer);
  ESP_LOGI(TAG, "%s mem:%d\n", __func__, jsonresponse.memoryUsage());
  return httpd_resp_send(req, jsonbuffer, measureJson(jsonresponse));
}

static esp_err_t winter_mode_read()
{
  int ileri_sure =
      node.getResponseBuffer(node.readHoldingRegisters(ILERI_SURE, 1));
  int geri_sure =
      node.getResponseBuffer(node.readHoldingRegisters(GERI_SURE, 1));
  int gecikme = node.getResponseBuffer(node.readHoldingRegisters(GECIKME, 1));
  int kis_modu_aktif =
      node.getResponseBuffer(node.readHoldingRegisters(KIS_MODU_AKTIF, 1));
  // int birinci_mod =
  // node.getResponseBuffer(node.readHoldingRegisters(BIRINCI_MOD, 1)); int
  // ikinci_mod = node.getResponseBuffer(node.readHoldingRegisters(IKINCI_MOD,
  // 1));

  uint16_t birinci_mod =
      node.getResponseBuffer(node.readHoldingRegisters(BIRINCI_MOD, 1));
  float mod1 = (float)*((int16_t *)&birinci_mod);
  Serial.printf("mod1: %f\n", mod1);

  uint16_t ikinci_mod =
      node.getResponseBuffer(node.readHoldingRegisters(IKINCI_MOD, 1));
  float mod2 = (float)*((int16_t *)&ikinci_mod);
  Serial.printf("mod2: %f\n", mod2);

  ESP_LOGI(TAG, "ileri:%d geri:%d gecikme:%d kis:%d 1.:%d 2.:%d\n", ileri_sure,
           geri_sure, gecikme, kis_modu_aktif, birinci_mod, ikinci_mod);
  return ESP_OK;
}

static esp_err_t sysclock_status_handler(httpd_req_t *req)
{
  char jsonbuffer[250];
  StaticJsonDocument<150> jsonresponse;
  char hr_buffer[3];
  char min_buffer[3];
  char sec_buffer[3];
  int sec = node.getResponseBuffer(node.readHoldingRegisters(SANIYE_READ, 1));
  int min = node.getResponseBuffer(node.readHoldingRegisters(DAKIKA_READ, 1));
  int hr = node.getResponseBuffer(node.readHoldingRegisters(SAAT_READ, 1));
  jsonresponse["sec"] = sprintf(sec_buffer, "%02d", sec);
  jsonresponse["min"] = sprintf(min_buffer, "%02d", min);
  jsonresponse["hr"] = sprintf(hr_buffer, "%02d", hr);
  Serial.printf("%02d-%02d-%02d", hr, sec, min);
  Serial.printf("Clock %02d-%02d-%02d\n", hr, min, sec);
  serializeJson(jsonresponse, jsonbuffer);
  ESP_LOGI(TAG, "%s mem:%d\n", __func__, jsonresponse.memoryUsage());
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
    ESP_LOGE(TAG, "Alarm read error");
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
    ESP_LOGE(TAG, "Alarm status read error");
  }
}

static void readClock()
{
  int hr = node.getResponseBuffer(node.readHoldingRegisters(SAAT_READ, 1));
  int min = node.getResponseBuffer(node.readHoldingRegisters(DAKIKA_READ, 1));
  int sec = node.getResponseBuffer(node.readHoldingRegisters(SANIYE_READ, 1));
  ESP_LOGI(TAG, "Saat %02d-%02d-%02d\n", hr, min, sec);
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
  strlcpy(server_data->base_path, base_path, sizeof(server_data->base_path));
  Serial.println(server_data->base_path);

  config.uri_match_fn = httpd_uri_match_wildcard;
  config.max_uri_handlers = 25;
  config.server_port = 80;

  httpd_uri_t index_uri = {.uri = "/",
                           .method = HTTP_GET,
                           .handler = download_handler,
                           .user_ctx = server_data};

  httpd_uri_t cmd_uri = {.uri = "/action",
                         .method = HTTP_GET,
                         .handler = cmd_handler,
                         .user_ctx = NULL};

  httpd_uri_t status_uri = {.uri = "/status",
                            .method = HTTP_GET,
                            .handler = status_handler,
                            .user_ctx = NULL};

  httpd_uri_t ileri_uri = {.uri = "/turIleri",
                           .method = HTTP_GET,
                           .handler = tur_ileri_handler,
                           .user_ctx = NULL};

  httpd_uri_t geri_uri = {.uri = "/turGeri",
                          .method = HTTP_GET,
                          .handler = tur_geri_handler,
                          .user_ctx = NULL};

  httpd_uri_t restart_uri = {.uri = "/restart",
                             .method = HTTP_GET,
                             .handler = restart_handler,
                             .user_ctx = NULL};

  httpd_uri_t get_times_uri = {.uri = "/gettimes",
                               .method = HTTP_GET,
                               .handler = get_times_handler,
                               .user_ctx = NULL};

  httpd_uri_t manual_mode_uri = {.uri = "/manualmode",
                                 .method = HTTP_POST,
                                 .handler = manual_mode_handler,
                                 .user_ctx = NULL};

  httpd_uri_t save_uri = {.uri = "/save",
                          .method = HTTP_POST,
                          .handler = save_handler,
                          .user_ctx = NULL};

  httpd_uri_t winter_mode_uri = {.uri = "/wintermode",
                                 .method = HTTP_POST,
                                 .handler = winter_mode_handler,
                                 .user_ctx = NULL};

  httpd_uri_t sysclock_uri = {.uri = "/sysclock",
                              .method = HTTP_POST,
                              .handler = sysclock_handler,
                              .user_ctx = NULL};

  httpd_uri_t currents_uri = {.uri = "/currentctrl",
                              .method = HTTP_POST,
                              .handler = currentctrl_handler,
                              .user_ctx = NULL};

  httpd_uri_t encoder_uri = {.uri = "/encoder",
                             .method = HTTP_POST,
                             .handler = encoder_handler,
                             .user_ctx = NULL};

  httpd_uri_t limits_uri = {.uri = "/chargectrl",
                            .method = HTTP_POST,
                            .handler = limits_handler,
                            .user_ctx = NULL};
  httpd_uri_t ali_uri = {.uri = "/chargeali",
                         .method = HTTP_POST,
                         .handler = ali_handler,
                         .user_ctx = NULL};

  httpd_uri_t timesure_uri = {.uri = "/surekalb",
                              .method = HTTP_POST,
                              .handler = timekalb_handler,
                              .user_ctx = NULL};

  httpd_uri_t loc_uri = {.uri = "/lockalb",
                         .method = HTTP_POST,
                         .handler = lockalb_handler,
                         .user_ctx = NULL};

  httpd_uri_t winter_status_uri = {.uri = "/winterstatus",
                                   .method = HTTP_GET,
                                   .handler = winter_status_handler,
                                   .user_ctx = NULL};

  httpd_uri_t temp_status_uri = {.uri = "/temp",
                                 .method = HTTP_GET,
                                 .handler = temp_status_handler,
                                 .user_ctx = NULL};

  httpd_uri_t sysclock_status_uri = {.uri = "/sysclock",
                                     .method = HTTP_GET,
                                     .handler = sysclock_status_handler,
                                     .user_ctx = NULL};

  httpd_uri_t file_serve = {.uri = "/data/*",
                            .method = HTTP_GET,
                            .handler = download_handler,
                            .user_ctx = server_data};

  // httpd_uri_t theme_uri = {
  //     .uri = "/theme",
  //     .method = HTTP_GET,
  //     .handler = theme_handler,
  //     .user_ctx = NULL};

  if (httpd_start(&gubre_siyirma, &config) == ESP_OK)
  {
    httpd_register_uri_handler(gubre_siyirma, &file_serve);
    httpd_register_uri_handler(gubre_siyirma, &index_uri);
    httpd_register_uri_handler(gubre_siyirma, &save_uri);
    httpd_register_uri_handler(gubre_siyirma, &cmd_uri);
    httpd_register_uri_handler(gubre_siyirma, &status_uri);
    httpd_register_uri_handler(gubre_siyirma, &ileri_uri);
    httpd_register_uri_handler(gubre_siyirma, &geri_uri);
    httpd_register_uri_handler(gubre_siyirma, &restart_uri);
    httpd_register_uri_handler(gubre_siyirma, &get_times_uri);
    httpd_register_uri_handler(gubre_siyirma, &manual_mode_uri);
    httpd_register_uri_handler(gubre_siyirma, &winter_mode_uri);
    httpd_register_uri_handler(gubre_siyirma, &winter_status_uri);
    httpd_register_uri_handler(gubre_siyirma, &temp_status_uri);
    // httpd_register_uri_handler(gubre_siyirma, &theme_uri);
    httpd_register_uri_handler(gubre_siyirma, &sysclock_uri);
    httpd_register_uri_handler(gubre_siyirma, &sysclock_status_uri);
    httpd_register_uri_handler(gubre_siyirma, &currents_uri);
    httpd_register_uri_handler(gubre_siyirma, &encoder_uri);
    httpd_register_uri_handler(gubre_siyirma, &limits_uri);
    httpd_register_uri_handler(gubre_siyirma, &ali_uri);
    httpd_register_uri_handler(gubre_siyirma, &timesure_uri);
    httpd_register_uri_handler(gubre_siyirma, &loc_uri);
    httpd_register_err_handler(gubre_siyirma, HTTPD_404_NOT_FOUND,
                               redirect_home);
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
}

void webSocketEvent(WStype_t type, uint8_t *payload, size_t length);

// gelen mesajı işlenmesi
void handleServerMessage(const String &message)
{
  Serial.println("Sunucudan gelen JSON mesajı: " + message);

  // JSON verisini işlemek için ArduinoJson kütüphanesini kullanabilirsiniz

  DynamicJsonDocument doc(1024);
  DeserializationError error = deserializeJson(doc, message);

  // Ayrıştırma başarılı ise devam edin
  if (!error)
  {
    // JsonObject kullanarak tüm alanları kontrol et
    JsonObject obj = doc.as<JsonObject>();
    // serializeJson(obj, Serial);
    // serializeJson(doc, Serial);

    JsonObject::iterator it = doc.as<JsonObject>().begin();

    // Alan adını ve değerini al
    String fieldName = it->key().c_str();
    // Serial.println(fieldName);
    String fieldValue = it->value().as<String>();
    op_errors ret = OP_ERR;
    // Elde edilen JsonObject'ü yazdır

    if (fieldName.equals("status"))
    {
      int myID = obj["myID"];

      if (myID == IDNumber)
      {
        // ret = parse_status();
        ESP_LOGI(TAG, "Event status processed: %s",
                 (ret == OP_OK) ? "OP_OK" : "OP_ERR");
      }
    }

    else if (fieldName.equals("ektur"))
    {
      int myID = obj["myID"];

      if (myID == IDNumber)
      {
        ret = parse_ektur(obj);
        ESP_LOGI(TAG, "Event ektur processed: %s",
                 (ret == OP_OK) ? "OP_OK" : "OP_ERR");
      }
    }
    // JSON verisindeki "key2" değerini alma
  }
  else
  {
    Serial.println("JSON mesajının hatası!");
  }
}

// Senkronize olarak arka planda çalışacak olan fonksiyon
void timerCallback()
{

  sendMesaj();
}
void setup()
{

  // Değerleri tekrar oku ve doğrula
  sakla.begin("myApp", true); // True parametresiyle sadece okuma yapılır
  saved_lokaltim = sakla.getInt("loctime", lokaltim);
  saved_lokalfark = sakla.getInt("locfark", lokalfark);
  sakla.end();
  // Bağlı istemcileri her 10 saniyede bir kontrol et
  checkClientsTicker.attach(1, checkConnectedClients);

  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);

  WiFi.onEvent(Wifi_AP_Disconnected,
               WiFiEvent_t::ARDUINO_EVENT_WIFI_AP_STADISCONNECTED);

  pinMode(LED_PIN, OUTPUT);

  Serial.begin(115200);
  Serial1.begin(115200, SERIAL_8N1, SERIAL1_RX, SERIAL1_TX);
  // ss.begin(115200);

  Serial.setDebugOutput(true);
  esp_log_level_set("*", ESP_LOG_ERROR);
  node.begin(2, Serial1);
  Serial.println("");
  WiFi.mode(WIFI_AP_STA);
  WiFi.setTxPower(WIFI_POWER_19_5dBm);
  esp_wifi_get_mac(WIFI_IF_STA, mac);
  // sprintf(softap_mac, "%s_%02X%02X", mdns_host, mac[4], mac[5]);
  WiFi.softAP(mdns_host_uppercase, ap_pwd);
  server.begin(); // Sunucu başlat
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
  startServer("");

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(1000);
    Serial.println("WiFi bağlanıyor...");
    con = con + 1;
    if (con > 5)
      goto ali;
  }

ali:
  if (WiFi.status() == WL_CONNECTED)
  {
    Serial.println("WiFi bağlantısı başarılı!");
    con = 0;
  }
  socketIO.begin(socketUrl, socketPort);
  socketIO.onEvent(webSocketEvent);
}

void loop()
{
  int bagliIstemciSayisi = WiFi.softAPgetStationNum();
  if (say < 5)
  {
    socketIO.loop();
  }
}

void webSocketEvent(WStype_t type, uint8_t *payload, size_t length)
{
  switch (type)
  {

  case WStype_DISCONNECTED:
  {
    Serial.println("Bağlantı koptu!");
    counter = counter + 1;
    if (counter == 200)
    {
      esp_restart();
      counter = 0;
    }
    Serial.println(counter);
    say = say + 1;

    break;
  }

  case WStype_CONNECTED:
  {
    Serial.println("Bağlantı başarılı!");
    counter = 0;
    timer1.attach(1.0, timerCallback);

    break;
  }
  case WStype_TEXT:
  {
    handleServerMessage(String((char *)payload));

    break;
  }
  }
}
