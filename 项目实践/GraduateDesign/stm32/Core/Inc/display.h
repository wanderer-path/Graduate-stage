#ifndef __DISPLAY_H
#define __DISPLAY_H

#include "stm32f1xx_hal.h"
#include <stdint.h>

/*
 * 4行布局定义
 * ┌────────────────┐
 * │ T:26C   H:58%  │  Row 0
 * │ Smoke:856  OK  │  Row 1
 * │ Light:1302 OK  │  Row 2
 * │ Fall:NO NORMAL │  Row 3
 * └────────────────┘
 */
#define DISP_ROW_TEMP_HUMI   0
#define DISP_ROW_SMOKE       1
#define DISP_ROW_LIGHT       2
#define DISP_ROW_STATUS      3

/* WiFi 状态 */
typedef enum
{
    WIFI_DISCONNECTED = 0,
    WIFI_CONNECTING,
    WIFI_CONNECTED
} WifiStatus_t;

extern WifiStatus_t wifi_status;

/* 跌倒状态 */
extern uint8_t fall_detected;

/* 函数声明 */
uint8_t Display_Init(void);
void    Display_Update(void);
void    Display_ShowAlarm(const char *msg);
void    Display_SetWifiStatus(WifiStatus_t status);
void    Display_SetFallStatus(uint8_t fall);

#endif /* __DISPLAY_H */
