#ifndef __DHT11_H
#define __DHT11_H

#include "stm32f1xx_hal.h"
#include <stdint.h>

/* -------------------------------------------------------
 * DHT11 数据引脚定义
 * 修改此处即可适配不同引脚，无需改动驱动逻辑
 * ------------------------------------------------------- */
#define DHT11_GPIO_PORT     GPIOB
#define DHT11_GPIO_PIN      GPIO_PIN_0

/* 读取结果状态码 */
#define DHT11_OK            0
#define DHT11_ERR_TIMEOUT   1    /* 等待应答超时 */
#define DHT11_ERR_CHECKSUM  2    /* 校验和错误 */

/* DHT11 数据结构体 */
typedef struct
{
    uint8_t  humidity_int;       /* 湿度整数部分 0~100 */
    uint8_t  humidity_dec;       /* 湿度小数部分（DHT11 恒为 0） */
    uint8_t  temperature_int;    /* 温度整数部分 0~50 */
    uint8_t  temperature_dec;    /* 温度小数部分（DHT11 恒为 0） */
    uint8_t  checksum;           /* 校验和原始值 */
    uint8_t  status;             /* 读取状态码 DHT11_OK / ERR */
} DHT11_Data_t;

/* 函数声明 */
void DHT11_Init(void);
uint8_t DHT11_Read(DHT11_Data_t *data);

#endif /* __DHT11_H */
