#include <esp_err.h>
#include <esp_http_server.h>
#include <ArduinoJson.h>
#include <ModbusMaster.h>
#include "main.h"

extern ModbusMaster node;


static esp_err_t status_handler(httpd_req_t *req)
{
    char jsonbuffer[100];
    StaticJsonDocument<50> jsonresponse;
    jsonresponse["amp"] = (float)node.getResponseBuffer(node.readHoldingRegisters(CURRENT, 1)) / 10.0;
    jsonresponse["volt"] = (float)node.getResponseBuffer(node.readHoldingRegisters(VOLTAGE, 1)) / 10.0;
    int d = node.getResponseBuffer(node.readHoldingRegisters(DURUM, 1));

    switch (d)
    {
    case 0:
        jsonresponse["status"] = "Robot Takıldı.";
        break;
    case 1:
        jsonresponse["status"] = "İleri Turda";
        break;
    case 2:
        jsonresponse["status"] = "Şarjda";
        break;
    case 3:
        jsonresponse["status"] = "Geri Turda.";
        break;
    case 4:
        jsonresponse["status"] = "Acil Stop Basılı.";
        break;
    case 5:
        jsonresponse["status"] = "Yön Switch Basılı.";
        break;
    case 6:
        jsonresponse["status"] = "Robot Şarj Etmiyor.";
        break;
    case 7:
        jsonresponse["status"] = "Manuel İleri";
        break;
    case 8:
        jsonresponse["status"] = "Manuel Geri";
        break;
    case 9:
        jsonresponse["status"] = "Hata!";
        break;
    case 10:
        jsonresponse["status"] = "Şarj Switchi Takılı";
        break;
    default:
        // Serial.printf("status: %d\n", d);
        jsonresponse["status"] = "HATA";
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
    // Serial.printf("buf_len:%d\n", buf_len);
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

static esp_err_t manual_mode_handler(httpd_req_t *req)
{
    Serial.println("manual mode");
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
                    Serial.printf("buffer contains:%s", buffer);
                }
            }
        }
    }

    httpd_resp_send(req, "OK", HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
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
    // for (int i = 0; i < 16; i += 2)
    // {
    //   snprintf(buffer, sizeof(buffer), "%02d:%02d,%d", i, i, 1);
    //   jsonresponse.add(buffer);
    //   Serial.println(buffer);
    // }
    serializeJson(jsonresponse, jsonbuffer);
    return httpd_resp_send(req, jsonbuffer, measureJson(jsonresponse));
}