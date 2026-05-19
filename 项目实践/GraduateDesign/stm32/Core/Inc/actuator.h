#ifndef __ACTUATOR_H
#define __ACTUATOR_H

#include "stm32f1xx_hal.h"
#include <stdint.h>

/* -------------------------------------------------------
 * 引脚定义
 * ------------------------------------------------------- */
#define LED_GPIO_PORT       GPIOB
#define LED_GPIO_PIN        GPIO_PIN_12

#define BEEP_GPIO_PORT      GPIOB
#define BEEP_GPIO_PIN       GPIO_PIN_13

/* -------------------------------------------------------
 * LED 闪烁模式枚举
 * ------------------------------------------------------- */
typedef enum
{
    LED_MODE_OFF    = 0,    /* 熄灭 */
    LED_MODE_ON,            /* 常亮 */
    LED_MODE_BLINK_SLOW,    /* 慢闪：1000ms 周期（烟雾报警） */
    LED_MODE_BLINK_FAST,    /* 快闪：200ms 周期（跌倒报警） */
} LedMode_t;

/* -------------------------------------------------------
 * 蜂鸣器鸣叫模式枚举
 * ------------------------------------------------------- */
typedef enum
{
    BEEP_MODE_OFF = 0,      /* 静音 */
    BEEP_MODE_ON,           /* 持续鸣叫（跌倒报警） */
    BEEP_MODE_INTERVAL,     /* 间歇鸣叫：500ms 通 / 500ms 断（烟雾报警） */
} BeepMode_t;

/* -------------------------------------------------------
 * 执行器状态结构体
 * ------------------------------------------------------- */
typedef struct
{
    LedMode_t  led_mode;        /* 当前 LED 模式 */
    BeepMode_t beep_mode;       /* 当前蜂鸣器模式 */
    uint8_t    led_state;       /* LED 当前电平状态 0/1 */
    uint8_t    beep_state;      /* 蜂鸣器当前电平状态 0/1 */
    uint32_t   led_last_tick;   /* LED 上次翻转时间戳 */
    uint32_t   beep_last_tick;  /* 蜂鸣器上次翻转时间戳 */
} ActuatorStatus_t;

/* 全局执行器状态，供其他模块访问 */
extern ActuatorStatus_t actuator_status;

/* -------------------------------------------------------
 * 函数声明
 * ------------------------------------------------------- */

/* 初始化 */
void Actuator_Init(void);

/* LED 控制 */
void LED_On(void);
void LED_Off(void);
void LED_Toggle(void);
void LED_SetMode(LedMode_t mode);

/* 蜂鸣器控制 */
void Beep_On(void);
void Beep_Off(void);
void Beep_Once(uint32_t duration_ms);
void Beep_Times(uint8_t n, uint32_t on_ms, uint32_t off_ms);
void Beep_SetMode(BeepMode_t mode);

/* 主循环轮询驱动（非阻塞） */
void Actuator_Tick(void);

/* 状态打印 */
void Actuator_Print(void);

#endif /* __ACTUATOR_H */
