#include "esp8266.h"
#include "usart.h"
#include "debug.h"
#include "display.h"

/* -------------------------------------------------------
 * 全局变量
 * ------------------------------------------------------- */
uint8_t    esp_rx_buf[ESP_RX_BUF_SIZE];
uint16_t   esp_rx_head = 0;
uint16_t   esp_rx_tail = 0;
uint8_t    esp_rx_byte = 0;
ESP_State_t esp_state  = ESP_STATE_RESET;

/* 下行消息临时缓冲（存储最近一条完整的 MQTT 消息） */
static char mqtt_recv_buf[512];
static volatile uint8_t mqtt_msg_ready = 0;


/* -------------------------------------------------------
 * 环形缓冲区
 * ------------------------------------------------------- */

static void ESP_RxBuf_Write(uint8_t byte)
{
    uint16_t next = (esp_rx_head + 1) % ESP_RX_BUF_SIZE;
    if (next != esp_rx_tail)
    {
        esp_rx_buf[esp_rx_head] = byte;
        esp_rx_head = next;
    }
}

uint8_t ESP_RxBuf_Read(uint8_t *out)
{
    if (esp_rx_head == esp_rx_tail) return 0;
    *out = esp_rx_buf[esp_rx_tail];
    esp_rx_tail = (esp_rx_tail + 1) % ESP_RX_BUF_SIZE;
    return 1;
}

void ESP_RxBuf_Clear(void)
{
    esp_rx_head = 0;
    esp_rx_tail = 0;
    memset(esp_rx_buf, 0, sizeof(esp_rx_buf));
}


/* -------------------------------------------------------
 * 中断回调
 * ------------------------------------------------------- */

void ESP_RxCallback(uint8_t byte)
{
    ESP_RxBuf_Write(byte);
    HAL_UART_Receive_IT(&huart2, &esp_rx_byte, 1);
}


/* -------------------------------------------------------
 * AT 指令发送与应答等待
 * ------------------------------------------------------- */

/**
 * @brief  发送 AT 指令并等待期望应答字符串
 * @param  cmd        AT 指令字符串（不含 \r\n，函数内自动添加）
 * @param  expect     期望应答子串，如 "OK" "CONNECTED" 等
 * @param  timeout_ms 超时时间（ms）
 * @return 1:收到期望应答  0:超时或收到 ERROR
 */
uint8_t ESP_SendAT(const char *cmd,
                   const char *expect,
                   uint32_t timeout_ms)
{
    char tx_buf[256];
    snprintf(tx_buf, sizeof(tx_buf), "%s\r\n", cmd);

    /* 清空接收缓冲区 */
    ESP_RxBuf_Clear();

    /* 发送指令 */
    HAL_UART_Transmit(&huart2, (uint8_t *)tx_buf,
                      strlen(tx_buf), AT_TIMEOUT_SHORT);

    DEBUG_LOG("[AT] Send: %s", cmd);

    /* 等待应答 */
    return ESP_WaitResponse(expect, timeout_ms);
}

/**
 * @brief  等待串口接收到包含 expect 的行
 * @return 1:找到期望字符串  0:超时
 */
uint8_t ESP_WaitResponse(const char *expect, uint32_t timeout_ms)
{
    char     resp_buf[512];
    uint16_t resp_idx  = 0;
    uint32_t start     = HAL_GetTick();

    memset(resp_buf, 0, sizeof(resp_buf));

    while (HAL_GetTick() - start < timeout_ms)
    {
        uint8_t byte;
        if (ESP_RxBuf_Read(&byte))
        {
            /* 超出缓冲区则回卷 */
            if (resp_idx >= sizeof(resp_buf) - 1)
                resp_idx = 0;

            resp_buf[resp_idx++] = (char)byte;
            resp_buf[resp_idx]   = '\0';

            /* 检查是否包含期望字符串 */
            if (strstr(resp_buf, expect) != NULL)
            {
                DEBUG_LOG("[AT] Recv OK: found \"%s\"", expect);
                return 1;
            }

            /* 检查是否收到 ERROR（快速失败） */
            if (strstr(resp_buf, "ERROR") != NULL)
            {
                DEBUG_WARN("[AT] Recv ERROR response. err");
                return 0;
            }
        }
    }

    DEBUG_WARN("[AT] Timeout waiting for \"%s\"", expect);
    return 0;
}


/* -------------------------------------------------------
 * 硬件复位
 * ------------------------------------------------------- */

void ESP_HardReset(void)
{
    DEBUG_INFO("[ESP] Hard reset...");
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_RESET);
    HAL_Delay(200);
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET);
    HAL_Delay(2000);    /* 等待 ESP8266 启动完成 */
    ESP_RxBuf_Clear();
    DEBUG_INFO("[ESP] Reset done.");
}


/* -------------------------------------------------------
 * ESP8266 初始化
 * ------------------------------------------------------- */

void ESP_Init(void)
{
    /* 启动 USART2 单字节接收中断 */
    HAL_UART_Receive_IT(&huart2, &esp_rx_byte, 1);

    /* 硬件复位 */
    ESP_HardReset();

    /* 测试 AT 通信 */
    if (!ESP_SendAT("AT", "OK", AT_TIMEOUT_SHORT))
    {
        DEBUG_ERR("[ESP] AT test failed! Check wiring.");
        esp_state = ESP_STATE_ERROR;
        return;
    }

    /* 关闭回显 */
    ESP_SendAT("ATE0", "OK", AT_TIMEOUT_SHORT);

    /* 设置为 Station 模式 */
    ESP_SendAT("AT+CWMODE=1", "OK", AT_TIMEOUT_SHORT);

    esp_state = ESP_STATE_AT_OK;
    DEBUG_INFO("[ESP] AT communication OK.");
}


/* -------------------------------------------------------
 * WiFi 连接
 * ------------------------------------------------------- */

uint8_t ESP_ConnectWiFi(void)
{
    char cmd[128];
    snprintf(cmd, sizeof(cmd),
             "AT+CWJAP=\"%s\",\"%s\"",
             WIFI_SSID, WIFI_PASSWORD);

    DEBUG_INFO("[ESP] Connecting WiFi: %s ...", WIFI_SSID);
    Display_SetWifiStatus(WIFI_CONNECTING);

    esp_state = ESP_STATE_WIFI_CONNECTING;

    if (!ESP_SendAT(cmd, "WIFI GOT IP", AT_TIMEOUT_WIFI))
    {
        DEBUG_ERR("[ESP] WiFi connect failed!");
        Display_SetWifiStatus(WIFI_DISCONNECTED);
        esp_state = ESP_STATE_ERROR;
        return 0;
    }

    esp_state = ESP_STATE_WIFI_CONNECTED;
    Display_SetWifiStatus(WIFI_CONNECTED);
    DEBUG_INFO("[ESP] WiFi connected, IP obtained.");
    return 1;
}


/* -------------------------------------------------------
 * MQTT 连接阿里云
 * ------------------------------------------------------- */

uint8_t ESP_MqttConnect(void)
{
    char cmd[256];

    DEBUG_INFO("[ESP] Connecting to Aliyun MQTT...");
    esp_state = ESP_STATE_MQTT_CONNECTING;

    /* Step 1：MQTT 用户配置 */
    snprintf(cmd, sizeof(cmd),
             "AT+MQTTUSERCFG=0,1,\"NULL\",\"%s\",\"%s\",0,0,\"\"",
             MQTT_USERNAME,
             MQTT_PASSWORD);

    if (!ESP_SendAT(cmd, "OK", AT_TIMEOUT_SHORT))
    {
        DEBUG_ERR("[ESP] MQTTUSERCFG failed.");
        esp_state = ESP_STATE_ERROR;
        return 0;
    }
	
	/* Step 2：MQTT 客户端ID参数 */
    snprintf(cmd, sizeof(cmd),
             "AT+MQTTCLIENTID=0,\"%s\"",
             MQTT_CLIENT_ID);

    if (!ESP_SendAT(cmd, "OK", AT_TIMEOUT_SHORT))
    {
        DEBUG_ERR("[ESP] MQTTCLIENTID failed.");
        esp_state = ESP_STATE_ERROR;
        return 0;
    }

    /* Step 3：建立 MQTT 连接 */
    snprintf(cmd, sizeof(cmd),
             "AT+MQTTCONN=0,\"%s\",%s,1",
             MQTT_BROKER, MQTT_PORT);

    if (!ESP_SendAT(cmd, "CONNECTED", AT_TIMEOUT_MQTT))
    {
        DEBUG_ERR("[ESP] MQTT connect failed! Check three-tuple.");
        esp_state = ESP_STATE_ERROR;
        return 0;
    }

    /* Step 3：订阅云端下发 Topic */
    if (!ESP_MqttSubscribe(MQTT_TOPIC_SET))
    {
        DEBUG_WARN("[ESP] Subscribe failed, control may not work.");
    }

    esp_state = ESP_STATE_MQTT_CONNECTED;
    DEBUG_INFO("[ESP] Aliyun MQTT connected!");
    return 1;
}


/* -------------------------------------------------------
 * MQTT 发布数据
 * ------------------------------------------------------- */

/**
 * @brief  向指定 Topic 发布消息
 * @param  topic    MQTT Topic 字符串
 * @param  payload  JSON 消息体
 * @return 1:成功  0:失败
 */
uint8_t ESP_MqttPublish(const char *topic, const char *payload)
{
    char cmd[768];
    snprintf(cmd, sizeof(cmd),
             "AT+MQTTPUB=0,\"%s\",\"%s\",1,0",
             topic, payload);

    if (!ESP_SendAT(cmd, "OK", AT_TIMEOUT_SHORT))
    {
        DEBUG_WARN("[ESP] MQTT publish failed, reconnecting...");
        esp_state = ESP_STATE_WIFI_CONNECTED;  /* 触发重连 */
        return 0;
    }

    DEBUG_LOG("[ESP] Published OK.");
    return 1;
}

/**
 * @brief  使用 AT+MQTTPUBRAW 发布 MQTT 消息（推荐，避免转义问题）
 * @param  topic    MQTT Topic
 * @param  payload  原始 JSON 字符串（不需要任何转义）
 * @return 1:成功  0:失败
 */
uint8_t ESP_MqttPublishRaw(const char *topic, const char *payload)
{
    char     cmd[256];
    uint16_t payload_len = (uint16_t)strlen(payload);

    /* Step 1：发送 MQTTPUBRAW 指令，告知 payload 长度 */
    snprintf(cmd, sizeof(cmd),
             "AT+MQTTPUBRAW=0,\"%s\",%d,1,0",
             topic, payload_len);

    /* 清空缓冲区，发送指令 */
    ESP_RxBuf_Clear();
    char tx_cmd[256];
    snprintf(tx_cmd, sizeof(tx_cmd), "%s\r\n", cmd);
    HAL_UART_Transmit(&huart2, (uint8_t *)tx_cmd,
                      strlen(tx_cmd), AT_TIMEOUT_SHORT);

    DEBUG_LOG("[AT] Send: %s", cmd);

    /* Step 2：等待 ESP8266 返回 ">" 提示符，表示可以发送数据 */
    if (!ESP_WaitResponse(">", AT_TIMEOUT_SHORT))
    {
        DEBUG_ERR("[ESP] MQTTPUBRAW: no '>' prompt received.");
        return 0;
    }

    /* Step 3：发送原始 JSON payload（无需任何转义） */
    HAL_UART_Transmit(&huart2, (uint8_t *)payload,
                      payload_len, AT_TIMEOUT_SHORT);

    DEBUG_LOG("[AT] Payload sent: %s", payload);

    /* Step 4：等待发布结果 */
    if (!ESP_WaitResponse("OK", AT_TIMEOUT_SHORT))
    {
        DEBUG_WARN("[ESP] MQTTPUBRAW publish failed.");
        esp_state = ESP_STATE_WIFI_CONNECTED;   /* 触发重连 */
        return 0;
    }

    DEBUG_LOG("[ESP] PublishRaw OK.");
    return 1;
}

/* -------------------------------------------------------
 * MQTT 订阅
 * ------------------------------------------------------- */

uint8_t ESP_MqttSubscribe(const char *topic)
{
    char cmd[256];
    snprintf(cmd, sizeof(cmd),
             "AT+MQTTSUB=0,\"%s\",1", topic);

    if (!ESP_SendAT(cmd, "OK", AT_TIMEOUT_SHORT))
    {
        DEBUG_WARN("[ESP] Subscribe failed: %s", topic);
        return 0;
    }

    DEBUG_INFO("[ESP] Subscribed: %s", topic);
    return 1;
}


/* -------------------------------------------------------
 * 主循环处理：接收下行 MQTT 消息
 * ------------------------------------------------------- */

/**
 * @brief  在主循环中调用，扫描接收缓冲区是否有完整 MQTT 下行消息
 *         ESP8266 收到 MQTT 消息后会主动推送格式：
 *         +MQTTSUBRECV:0,"topic",len,payload\r\n
 */
void ESP_Process(void)
{
    static char  line_buf[512];
    static uint16_t line_idx = 0;
    uint8_t byte;

    while (ESP_RxBuf_Read(&byte))
    {
        if (byte == '\n')
        {
            line_buf[line_idx] = '\0';

            /* 检测是否为 MQTT 下行消息 */
            if (strstr(line_buf, "+MQTTSUBRECV") != NULL)
            {
                /* 提取 payload：最后一个逗号后的内容 */
                char *payload_start = NULL;
                uint8_t comma_count = 0;
                for (uint16_t i = 0; i < line_idx; i++)
                {
                    if (line_buf[i] == ',') comma_count++;
                    if (comma_count == 3)
                    {
                        payload_start = &line_buf[i + 1];
                        break;
                    }
                }

                if (payload_start != NULL)
                {
                    strncpy(mqtt_recv_buf, payload_start,
                            sizeof(mqtt_recv_buf) - 1);
                    mqtt_msg_ready = 1;
                    DEBUG_LOG("[MQTT] Recv: %s", mqtt_recv_buf);
                }
            }

            line_idx = 0;
        }
        else if (byte != '\r')
        {
            if (line_idx < sizeof(line_buf) - 1)
                line_buf[line_idx++] = (char)byte;
        }
    }
}


/* -------------------------------------------------------
 * 获取最新下行消息（供 mqtt_aliyun.c 调用）
 * ------------------------------------------------------- */

uint8_t ESP_GetMqttMessage(char *out, uint16_t max_len)
{
    if (!mqtt_msg_ready) return 0;
    mqtt_msg_ready = 0;
    strncpy(out, mqtt_recv_buf, max_len - 1);
    out[max_len - 1] = '\0';
    return 1;
}
