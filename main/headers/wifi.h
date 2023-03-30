#ifndef WIFI__H
#define WIFI__H

#include "esp_err.h"
#include "esp_wifi.h"
#include "esp_log.h"

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"

#if CONFIG_ESP_WIFI_AUTH_OPEN
    #define WIFI_SECURITY WIFI_AUTH_OPEN
#elif CONFIG_ESP_WIFI_AUTH_WEP
    #define WIFI_SECURITY WIFI_AUTH_WEP
#elif CONFIG_ESP_WIFI_AUTH_WPA_PSK
    #define WIFI_SECURITY WIFI_AUTH_WPA_PSK
#elif CONFIG_ESP_WIFI_AUTH_WPA2_PSK
    #define WIFI_SECURITY WIFI_AUTH_WPA2_PSK
#elif CONFIG_ESP_WIFI_AUTH_WPA_WPA2_PSK
    #define WIFI_SECURITY WIFI_AUTH_WPA_WPA2_PSK
#elif CONFIG_ESP_WIFI_AUTH_WPA3_PSK
    #define WIFI_SECURITY WIFI_AUTH_WPA3_PSK
#elif CONFIG_ESP_WIFI_AUTH_WPA2_WPA3_PSK
    #define WIFI_SECURITY WIFI_AUTH_WPA2_WPA3_PSK
#elif CONFIG_ESP_WIFI_AUTH_WAPI_PSK
    #define WIFI_SECURITY WIFI_AUTH_WAPI_PSK
#endif

esp_err_t init_wifi(const char *ssid, const char *passw);
esp_err_t wifi_start();

#endif