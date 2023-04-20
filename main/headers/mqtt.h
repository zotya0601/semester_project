#ifndef MQTT__H
#define MQTT__H

#include "esp_event.h"
#include "esp_err.h"

#define MQTT_BROKER_URI CONFIG_MQTT_URI
#define MQTT_BROKER_HOSTNAME CONFIG_MQTT_HOSTNAME
#define MQTT_BROKER_PORT CONFIG_MQTT_PORT

#if CONFIG_TRANSPORT_TCP
    #define MQTT_TRANSPORT  MQTT_TRANSPORT_OVER_TCP
#elif CONFIG_TRANSPORT_SSL
    #define MQTT_TRANSPORT  MQTT_TRANSPORT_OVER_SSL
#elif CONFIG_TRANSPORT_WS
    #define MQTT_TRANSPORT  MQTT_TRANSPORT_OVER_WS
#elif CONFIG_TRANSPORT_WSS
    #define MQTT_TRANSPORT  MQTT_TRANSPORT_OVER_WSS
#endif

void init_mqtt(const char *uri, const char *topic);
esp_err_t mqtt_register_event_handler(esp_event_handler_t mqtt_event_handler);
esp_err_t mqtt_start();

esp_err_t mqtt_publish(void *data, int len);
esp_err_t mqtt_publish_qos(uint8_t *data, int len, int qos);

esp_err_t mqtt_publish_topic(uint8_t *data, int len, const char *topic);
esp_err_t mqtt_publish_qos_topic(void *data, int len, int qos, const char *topic);

void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data);

#endif