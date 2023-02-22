#include <Arduino.h>
#include <esp_err.h>
#include <esp_https_server.h>
#include "main.h"
#include <ModbusMaster.h>

extern ModbusMaster node;

static esp_err_t tur_ileri_handler(httpd_req_t *req)
{
  node.writeSingleCoil(EKTUR_ILERI, HIGH);
  delay(250);
  // Serial.println("ileri turn");
  node.writeSingleCoil(EKTUR_ILERI, LOW);
  httpd_resp_set_type(req, "text/html");
  return httpd_resp_send(req, "ileri", 5);
}

static esp_err_t tur_geri_handler(httpd_req_t *req)
{
  httpd_resp_set_type(req, "text/html");

  node.writeSingleCoil(EKTUR_GERI, HIGH);
  delay(250);
  // Serial.println("geri turn");
  node.writeSingleCoil(EKTUR_GERI, LOW);
  return httpd_resp_send(req, "geri", 4);
}
