#include "display.h"
#include "ssd1306.h"
#include "sensors.h"
#include "debug.h"
#include <stdio.h>
#include <string.h>

WifiStatus_t wifi_status   = WIFI_DISCONNECTED;
uint8_t      fall_detected = 0;

/* 行缓冲：8×16 每行最多 16 字符 + '\0' */
static char row_buf[17];


/* -------------------------------------------------------
 * 开机画面
 * ------------------------------------------------------- */

static void Display_BootScreen(void)
{
    SSD1306_Clear();
    SSD1306_PrintRow(0, " Gradate Design ");
    SSD1306_PrintRow(1, " STM32F103C8T6 ");
    SSD1306_PrintRow(3, " Initializing.. ");
    SSD1306_UpdateScreen();
    HAL_Delay(1500);
    SSD1306_Clear();
    SSD1306_UpdateScreen();
}


/* -------------------------------------------------------
 * 初始化
 * ------------------------------------------------------- */

/**
 * @brief  显示模块初始化
 * @return 1:成功  0:失败
 */
uint8_t Display_Init(void)
{
    if (!SSD1306_Init())
    {
        DEBUG_ERR("Display init failed!");
        return 0;
    }
    Display_BootScreen();
    DEBUG_INFO("Display ready: 4 rows x 16 cols (8x16 font).");
    return 1;
}


/* -------------------------------------------------------
 * 主刷新函数
 * ------------------------------------------------------- */

/**
 * @brief  刷新全部 4 行数据到屏幕
 *         布局（每行 16 字符）：
 *
 *   Row 0: "T:26C   H: 58% "   温度 + 湿度
 *   Row 1: "Smoke: 856  OK "   烟雾原始值 + 状态
 *   Row 2: "Light:1302  OK "   光照原始值 + 状态
 *   Row 3: "Fall:NO  NORMAL"   跌倒状态 + 综合状态
 *          或反色高亮：
 *          "!!!FALL ALARM!!"
 *          "!!SMOKE ALARM! "
 */
void Display_Update(void)
{
    /* ---- Row 0：温度 + 湿度 ---- */
    if (sensor_data.dht11_status == 0)
    {
        snprintf(row_buf, sizeof(row_buf),
                 "T:%2dC   H:%2d%% ",
                 sensor_data.temperature,
                 sensor_data.humidity);
    }
    else
    {
        snprintf(row_buf, sizeof(row_buf), "T: --C  H: --%% ");
    }
    SSD1306_PrintRow(DISP_ROW_TEMP_HUMI, row_buf);

    /* ---- Row 1：烟雾 ---- */
    snprintf(row_buf, sizeof(row_buf),
             "Smoke:%4d %s",
             sensor_data.mq2_raw,
             sensor_data.mq2_alarm ? " !!" : " OK");
    SSD1306_PrintRow(DISP_ROW_SMOKE, row_buf);

    /* ---- Row 2：光照 ---- */
    snprintf(row_buf, sizeof(row_buf),
             "Light:%4d %s",
             sensor_data.light_raw,
             sensor_data.light_dark ? " DK" : " OK");
    SSD1306_PrintRow(DISP_ROW_LIGHT, row_buf);

    /* ---- Row 3：跌倒 + 综合状态（优先级：跌倒 > 烟雾 > 暗 > 正常）---- */
	if (fall_detected)
	{
		SSD1306_PrintRowInvert(DISP_ROW_STATUS, "!!!FALL ALARM!!");
	}
	else if (sensor_data.mq2_alarm)
	{
		SSD1306_PrintRowInvert(DISP_ROW_STATUS, "!!SMOKE ALARM! ");
	}
	else if (sensor_data.light_dark)
	{
        snprintf(row_buf, sizeof(row_buf),
                 "%s   DARK",
                 sensor_data.person_exist ? "Exist" : "Empty");
        SSD1306_PrintRow(DISP_ROW_STATUS, row_buf);
	}
	else
	{
        snprintf(row_buf, sizeof(row_buf),
                 "%s NORMAL",
                 sensor_data.person_exist ? "Exist" : "Empty");
        SSD1306_PrintRow(DISP_ROW_STATUS, row_buf);
	}

    SSD1306_UpdateScreen();
}


/* -------------------------------------------------------
 * 辅助接口
 * ------------------------------------------------------- */

/**
 * @brief  在 Row 3 显示自定义报警消息（反色高亮）
 *         下次 Display_Update() 调用后自动恢复
 */
void Display_ShowAlarm(const char *msg)
{
    SSD1306_PrintRowInvert(DISP_ROW_STATUS, msg);
    SSD1306_UpdateScreen();
    DEBUG_WARN("Display alarm: %s", msg);
}

/**
 * @brief  设置 WiFi 连接状态（阶段九调用）
 */
void Display_SetWifiStatus(WifiStatus_t status)
{
    wifi_status = status;
    DEBUG_INFO("Display WiFi status -> %d", status);
}

/**
 * @brief  设置跌倒检测状态（阶段七调用）
 * @param  fall  1:检测到跌倒  0:正常
 */
void Display_SetFallStatus(uint8_t fall)
{
    fall_detected = fall;
}
