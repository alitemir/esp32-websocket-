#include <esp_err.h>
#include <esp_http_server.h>

// extern "C"{

// }
esp_err_t status_handler(httpd_req_t *req);
esp_err_t cmd_handler(httpd_req_t *req);
esp_err_t manual_mode_handler(httpd_req_t *req);
esp_err_t save_handler(httpd_req_t *req);
esp_err_t get_times_handler(httpd_req_t *req);
