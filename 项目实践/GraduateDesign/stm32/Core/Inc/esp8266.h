#ifndef __ESP8266_H
#define __ESP8266_H

#include "stm32f1xx_hal.h"
#include <stdint.h>
#include <string.h>
#include <stdio.h>

/* -------------------------------------------------------
 * 配置区：WiFi 与阿里云三元组
 * 修改以下宏为实际参数
 * ------------------------------------------------------- */
#define WIFI_SSID           "mingzhe"
#define WIFI_PASSWORD       "ljh123456"

/* 阿里云三元组（从平台设备详情页获取） */
#define ALI_PRODUCT_KEY     "k0shgVICsVV"
#define ALI_DEVICE_NAME     "stm32f103c8t6"
#define ALI_DEVICE_SECRET   "1d12817f9c10d35598dbe27af26f0fb5"

/* 由三元组派生的 MQTT 连接参数（通过阿里云工具生成后填入） */
#define MQTT_BROKER         "iot-06z00i8refyslm1.mqtt.iothub.aliyuncs.com"
#define MQTT_PORT           "1883"
#define MQTT_CLIENT_ID      "k0shgVICsVV.stm32f103c8t6|securemode=2\\,signmethod=hmacsha256\\,timestamp=1776911032558|"
#define MQTT_USERNAME       ALI_DEVICE_NAME "&" ALI_PRODUCT_KEY
#define MQTT_PASSWORD       "c82731b73598e0208981197598e896ad1c682067d74d52b44c5b8a65adca3d8b"

/* MQTT Topic */
#define MQTT_TOPIC_POST     "/sys/" ALI_PRODUCT_KEY "/" ALI_DEVICE_NAME \
                            "/thing/event/property/post"
#define MQTT_TOPIC_SET      "/sys/" ALI_PRODUCT_KEY "/" ALI_DEVICE_NAME \
                            "/thing/service/property/set"

/* -------------------------------------------------------
 * AT 指令超时定义（ms）
 * ------------------------------------------------------- */
#define AT_TIMEOUT_SHORT    2000    /* 普通指令超时 */
#define AT_TIMEOUT_WIFI     15000   /* WiFi 连接超时 */
#define AT_TIMEOUT_MQTT     10000   /* MQTT 连接超时 */

/* -------------------------------------------------------
 * 接收缓冲区
 * ------------------------------------------------------- */
#define ESP_RX_BUF_SIZE     1024

extern uint8_t  esp_rx_buf[ESP_RX_BUF_SIZE];
extern uint16_t esp_rx_head;
extern uint16_t esp_rx_tail;
extern uint8_t  esp_rx_byte;

/* -------------------------------------------------------
 * ESP8266 状态枚举
 * ------------------------------------------------------- */
typedef enum
{
    ESP_STATE_RESET = 0,
    ESP_STATE_AT_OK,
    ESP_STATE_WIFI_CONNECTING,
    ESP_STATE_WIFI_CONNECTED,
    ESP_STATE_MQTT_CONNECTING,
    ESP_STATE_MQTT_CONNECTED,
    ESP_STATE_ERROR
} ESP_State_t;

extern ESP_State_t esp_state;

/* -------------------------------------------------------
 * 函数声明
 * ------------------------------------------------------- */
void     ESP_Init(void);
void     ESP_RxCallback(uint8_t byte);
uint8_t  ESP_SendAT(const char *cmd,
                    const char *expect,
                    uint32_t timeout_ms);
uint8_t  ESP_ConnectWiFi(void);
uint8_t  ESP_MqttConnect(void);
uint8_t  ESP_MqttPublish(const char *topic, const char *payload);
uint8_t ESP_MqttPublishRaw(const char *topic, const char *payload);
uint8_t  ESP_MqttSubscribe(const char *topic);
void     ESP_Process(void);          /* 主循环调用，处理下行消息 */
void     ESP_HardReset(void);

/* 接收缓冲区操作 */
uint8_t  ESP_RxBuf_Read(uint8_t *out);
void     ESP_RxBuf_Clear(void);
uint8_t  ESP_WaitResponse(const char *expect, uint32_t timeout_ms);
uint8_t ESP_GetMqttMessage(char *out, uint16_t max_len);
					
#endif /* __ESP8266_H */
