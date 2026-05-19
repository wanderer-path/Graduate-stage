#include "mqtt_aliyun.h"
#include "esp8266.h"
#include "sensors.h"
#include "actuator.h"
#include "servo.h"
#include "display.h"
#include "alarm.h"
#include "debug.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* 上报消息 ID（每次递增，阿里云 Alink 协议要求） */
static uint32_t msg_id = 1;

/* 下行消息缓冲区 */
static char cmd_buf[512];


/* -------------------------------------------------------
 * 初始化：连接 WiFi + MQTT
 * ------------------------------------------------------- */

void MQTT_Init(void)
{
    DEBUG_INFO("[MQTT] Starting Aliyun IoT connection...");

    uint8_t retry = 0;
    while (!ESP_ConnectWiFi() && retry < 3)
    {
        retry++;
        DEBUG_WARN("[MQTT] WiFi retry %d/3...", retry);
        HAL_Delay(3000);
    }

    if (esp_state != ESP_STATE_WIFI_CONNECTED)
    {
        DEBUG_ERR("[MQTT] WiFi failed, running offline.");
        return;
    }

    retry = 0;
    while (!ESP_MqttConnect() && retry < 3)
    {
        retry++;
        DEBUG_WARN("[MQTT] MQTT retry %d/3...", retry);
        HAL_Delay(3000);
    }

    if (esp_state == ESP_STATE_MQTT_CONNECTED)
    {
        DEBUG_INFO("[MQTT] Aliyun IoT ready!");

        /*
         * 不在此处立即上报
         * 等待主循环第一次 last_mqtt 计时触发时再上报
         * 此时传感器已经完成至少一次采集，数据有效
         */
        DEBUG_INFO("[MQTT] First post will occur in %d seconds.",
                   MQTT_POST_INTERVAL_MS / 1000);
    }
    else
    {
        DEBUG_ERR("[MQTT] MQTT failed, running offline.");
    }
}


/* -------------------------------------------------------
 * 上报传感器数据（Alink JSON 格式）
 * ------------------------------------------------------- */

/**
 * @brief  按阿里云 Alink 标准格式构造 JSON 并发布
 *
 * 上报格式：
 * {
 *   "id": "123",
 *   "version": "1.0",
 *   "params": {
 *     "temperature":  26,
 *     "humidity":     57,
 *     "smoke":        865,
 *     "light":        1302,
 *     "fall_status":  0,
 *     "person_exist": 1,
 *     "alarm_level":  0
 *   },
 *   "method": "thing.event.property.post"
 * }
 *
 * @note  AT+MQTTPUB 的 payload 中双引号需转义为 \"
 */
uint8_t MQTT_PostSensorData(void)
{
    if (esp_state != ESP_STATE_MQTT_CONNECTED)
    {
        DEBUG_WARN("[MQTT] Not connected, skip post.");
        return 0;
    }

    /*
     * 直接构造标准 JSON，双引号无需转义
     * 因为使用 MQTTPUBRAW，payload 以原始字节发送
     */
    char payload[512];
    snprintf(payload, sizeof(payload),
        "{"
            "\"id\":\"%lu\","
            "\"version\":\"1.0\","
            "\"params\":{"
                "\"temperature\":%d,"
                "\"humidity\":%d,"
                "\"smoke\":%d,"
                "\"light\":%d,"
	"\"servo_angle\":0,"
	"\"beep_switch\":0,"
	"\"led_switch\":0,"
                "\"fall_status\":%d,"
                "\"person_exist\":%d,"
                "\"alarm_level\":%d"
            "},"
            "\"method\":\"thing.event.property.post\""
        "}",
        msg_id++,
        sensor_data.temperature,
        sensor_data.humidity,
        sensor_data.mq2_raw,
        sensor_data.light_raw,
        sensor_data.fall_status,
        sensor_data.person_exist,
        (int)sys_status.current_level
    );

    DEBUG_LOG("[MQTT] Post id=%lu T=%d H=%d Smoke=%d "
              "Light=%d Fall=%d Exist=%d Alarm=%d",
              msg_id - 1,
              sensor_data.temperature,
              sensor_data.humidity,
              sensor_data.mq2_raw,
              sensor_data.light_raw,
              sensor_data.fall_status,
              sensor_data.person_exist,
              (int)sys_status.current_level);

    /* 使用 MQTTPUBRAW 发送，稳定可靠 */
    return ESP_MqttPublishRaw(MQTT_TOPIC_POST, payload);
}


/* -------------------------------------------------------
 * 处理云端下发指令
 * ------------------------------------------------------- */

/**
 * @brief  解析云端下发的 property set 指令
 *
 * 云端下发格式示例：
 * {"method":"thing.service.property.set","id":"12345",
 *  "params":{"led_switch":1,"beep_switch":0,"servo_angle":90}}
 *
 * @note  此处使用简单字符串查找解析，无需引入 JSON 库
 *        适合 STM32 RAM 受限场景
 */
void MQTT_HandleCommand(const char *payload)
{
    DEBUG_LOG("[MQTT] Handle cmd: %s", payload);

    /* ---- 解析 led_switch ---- */
    char *p = strstr(payload, "\"led_switch\":");
    if (p != NULL)
    {
        int val = atoi(p + strlen("\"led_switch\":"));
        if (val == 1)
        {
            LED_On();
            DEBUG_INFO("[CMD] LED -> ON");
        }
        else
        {
            LED_Off();
            DEBUG_INFO("[CMD] LED -> OFF");
        }
    }

    /* ---- 解析 beep_switch ---- */
    p = strstr(payload, "\"beep_switch\":");
    if (p != NULL)
    {
        int val = atoi(p + strlen("\"beep_switch\":"));
        if (val == 1)
        {
            Beep_On();
            DEBUG_INFO("[CMD] BEEP -> ON");
        }
        else
        {
            Beep_Off();
            DEBUG_INFO("[CMD] BEEP -> OFF");
        }
    }

    /* ---- 解析 servo_angle ---- */
    p = strstr(payload, "\"servo_angle\":");
    if (p != NULL)
    {
        int angle = atoi(p + strlen("\"servo_angle\":"));
        if (angle == 1)
        {
            Servo_SetAngle(90);
            DEBUG_INFO("[CMD] Servo -> ON");
        }
		else
        {
            Servo_SetAngle(0);
            DEBUG_INFO("[CMD] Servo -> OFF");
        }
    }
}

/* -------------------------------------------------------
 * 主任务：在主循环定时调用
 * ------------------------------------------------------- */

/**
 * @brief  MQTT 主任务，包含：
 *         1. 处理 ESP8266 接收缓冲区（下行消息）
 *         2. 定时上报传感器数据
 *         3. 每 30s 检查一次连接状态
 *
 * @note   此函数由 main.c 按 MQTT_POST_INTERVAL_MS 定时调用
 */
void MQTT_Post_Task(void)
{
    /* 上报传感器数据 */
    MQTT_PostSensorData();
}

void MQTT_Set_Task(void)
{
    /* 处理 ESP8266 下行消息 */
    ESP_Process();

    /* 检查是否有新的 MQTT 下行指令 */
    if (ESP_GetMqttMessage(cmd_buf, sizeof(cmd_buf)))
    {
        MQTT_HandleCommand(cmd_buf);
    }
}

/* -------------------------------------------------------
 * 连接状态检查与断线重连
 * ------------------------------------------------------- */

void MQTT_CheckConnection(void)
{
    if (esp_state == ESP_STATE_MQTT_CONNECTED) return;

    DEBUG_WARN("[MQTT] Disconnected, attempting reconnect...");
    Display_SetWifiStatus(WIFI_CONNECTING);

    /* 先尝试重连 MQTT（WiFi 可能仍连接） */
    if (esp_state == ESP_STATE_WIFI_CONNECTED)
    {
        if (ESP_MqttConnect())
        {
            DEBUG_INFO("[MQTT] Reconnected to MQTT.");
            return;
        }
    }

    /* WiFi 也断了，重新连接 WiFi + MQTT */
    ESP_HardReset();
    HAL_Delay(2000);

    if (ESP_ConnectWiFi())
    {
        ESP_MqttConnect();
    }
}
