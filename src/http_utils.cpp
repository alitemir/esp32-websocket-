#include <esp_err.h>
#include <esp_http_server.h>
#include "http_utils.h"
#include "main.h"
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