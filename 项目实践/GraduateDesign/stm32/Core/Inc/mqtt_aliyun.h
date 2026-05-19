#ifndef __MQTT_ALIYUN_H
#define __MQTT_ALIYUN_H

#include "stm32f1xx_hal.h"
#include <stdint.h>

/* 上报数据间隔（ms） */
#define MQTT_POST_INTERVAL_MS   5000U   /* 每 5 秒上报一次 */
#define MQTT_SET_INTERVAL_MS    2000U   /* 每 2 秒处理一次 */

/* 函数声明 */
void    MQTT_Init(void);
void    MQTT_Post_Task(void);           /* 主循环中定时调用 */
void    MQTT_Set_Task(void);            /* 主循环中定时调用 */
void    MQTT_HandleCommand(const char *payload);  /* 处理云端下发指令 */
uint8_t MQTT_PostSensorData(void); /* 上报传感器数据 */
void    MQTT_CheckConnection(void);/* 连接状态检查与重连 */
uint8_t ESP_GetMqttMessage(char *out, uint16_t max_len);

#endif /* __MQTT_ALIYUN_H */
