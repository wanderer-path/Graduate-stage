#ifndef __SENSORS_H
#define __SENSORS_H

#include "stm32f1xx_hal.h"
#include <stdint.h>

/* -------------------------------------------------------
 * ADC 原始值缓冲区（DMA 自动填充）
 * 外部声明，在 sensors.c 中定义
 * ------------------------------------------------------- */
extern uint16_t adc_buf[2];

/* ADC 通道索引 */
#define ADC_CH_MQ2    0     /* adc_buf[0] → MQ2 烟雾 */
#define ADC_CH_LIGHT  1     /* adc_buf[1] → GL5516 光照 */

/* ADC 参考电压与分辨率 */
#define ADC_VREF      3.3f
#define ADC_RESOLUTION 4095.0f

/* 报警阈值（根据实际环境标定后修改） */
#define MQ2_ALARM_THRESHOLD    2650    /* ADC 原始值，超过此值触发烟雾报警 */
#define LIGHT_DARK_THRESHOLD   3000    /* ADC 原始值，超过此值判定为光照过暗 */

/* 传感器数据结构体 */
typedef struct
{
    /* ---- ADC：MQ2 烟雾 ---- */
    uint16_t mq2_raw;         /* MQ2 ADC 原始值 0~4095 */
    float    mq2_voltage;     /* MQ2 对应电压值 V */
    uint8_t  mq2_alarm;       /* 烟雾报警标志 1:报警 0:正常 */

	/* ---- ADC：GL5516 光照 ---- */
    uint16_t light_raw;       /* GL5516 ADC 原始值 0~4095 */
    float    light_voltage;   /* GL5516 对应电压值 V */
    uint8_t  light_dark;      /* 光照过暗标志 1:过暗 0:正常 */
	
	/* ---- DHT11：温湿度 ---- */
	uint8_t  temperature;        /* 温度整数值 单位：℃ */
    uint8_t  humidity;           /* 湿度整数值 单位：% */
    uint8_t  dht11_status;       /* DHT11 读取状态 */
	
	/* ---- LD6002C：雷达跌倒---- */
    uint8_t  person_exist;    /* 人体存在 0:无人 1:有人 */
    uint8_t  fall_status;     /* 跌倒状态 0:正常 1:跌倒 */
    uint8_t  fall_alarm;      /* 报警标志，与 fall_status 相同 */
} SensorData_t;

/* 全局传感器数据实例，extern 供其他模块访问 */
extern SensorData_t sensor_data;

/* -------------------------------------------------------
 * 函数声明
 * ------------------------------------------------------- */


/* ADC */
void Sensors_ADC_Init(void);
void Sensors_ADC_Update(void);
void Sensors_Print(void);

/* DHT11 */
void Sensors_DHT11_Init(void);
void Sensors_DHT11_Update(void);
void Sensors_DHT11_Print(void);

/* 雷达（阶段七新增） */
void Sensors_Radar_Update(void);

#endif /* __SENSORS_H */
