#include <esp_err.h>
#include <esp_https_server.h>

static esp_err_t tur_ileri_handler(httpd_req_t *req);
static esp_err_t tur_geri_handler(httpd_req_t *req);
