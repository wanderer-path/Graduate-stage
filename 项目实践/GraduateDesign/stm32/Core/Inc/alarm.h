#ifndef __ALARM_H
#define __ALARM_H

#include "stm32f1xx_hal.h"
#include <stdint.h>

/* -------------------------------------------------------
 * 系统报警状态枚举（按优先级从低到高排列）
 * ------------------------------------------------------- */
typedef enum
{
    ALARM_NORMAL     = 0,   /* 正常状态 */
    ALARM_DARK       = 1,   /* 光照过暗 */
    ALARM_SMOKE      = 2,   /* 烟雾超阈值 */
    ALARM_FALL       = 3,   /* 跌倒确认 */
} AlarmLevel_t;

/* -------------------------------------------------------
 * 系统状态结构体
 * ------------------------------------------------------- */
typedef struct
{
    AlarmLevel_t current_level;     /* 当前最高报警级别 */
    AlarmLevel_t last_level;        /* 上一次报警级别，用于检测变化 */

    uint8_t fall_alarm_active;      /* 跌倒报警激活标志 */
    uint8_t smoke_alarm_active;     /* 烟雾报警激活标志 */
    uint8_t dark_alarm_active;      /* 光照报警激活标志 */

    uint32_t fall_alarm_start_ms;   /* 跌倒报警开始时间（用于超时自动解除） */
    uint32_t smoke_alarm_start_ms;  /* 烟雾报警开始时间 */
} SystemStatus_t;

/* 跌倒报警持续时间（ms），超时后若雷达不再检测到跌倒则自动解除 */
#define FALL_ALARM_TIMEOUT_MS    15000U   /* 15 秒 */

/* 烟雾报警持续时间（ms），超时后若浓度降低则自动解除 */
#define SMOKE_ALARM_TIMEOUT_MS   10000U   /* 10 秒 */

/* 全局系统状态，供其他模块访问 */
extern SystemStatus_t sys_status;

/* 函数声明 */
void Alarm_Init(void);
void Alarm_Update(void);
void Alarm_Print(void);

#endif /* __ALARM_H */
