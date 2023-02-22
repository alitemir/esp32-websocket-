#include <esp_err.h>
#include <esp_http_server.h>
#include "main.h"
#include "esp_log.h"
#include <Arduino.h>
extern "C" {
#include "http_utils.h"
}

static const char* TAG = "DLHandler";

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