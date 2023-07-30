#ifndef __HTTPD_TESTS_H__
#define __HTTPD_TESTS_H__

#include <esp_http_server.h>

extern httpd_handle_t   http_server_init(void);
extern void              http_server_stop(httpd_handle_t hd);

#endif // __HTTPD_TESTS_H__
