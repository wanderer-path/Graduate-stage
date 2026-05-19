#ifndef __DEBUG_H
#define __DEBUG_H

#include "stm32f1xx_hal.h"
#include <stdio.h>
#include <stdint.h>

/* -------------------------------------------------------
 * 调试开关：发布版本将此宏置 0 即可关闭所有 log 输出
 * ------------------------------------------------------- */
#define DEBUG_EN  1

#if DEBUG_EN
    #define DEBUG_LOG(fmt, ...)   printf("[LOG] " fmt "\r\n", ##__VA_ARGS__)
    #define DEBUG_INFO(fmt, ...)  printf("[INFO] " fmt "\r\n", ##__VA_ARGS__)
    #define DEBUG_WARN(fmt, ...)  printf("[WARN] " fmt "\r\n", ##__VA_ARGS__)
    #define DEBUG_ERR(fmt, ...)   printf("[ERR] " fmt "\r\n", ##__VA_ARGS__)
#else
    #define DEBUG_LOG(fmt, ...)
    #define DEBUG_INFO(fmt, ...)
    #define DEBUG_WARN(fmt, ...)
    #define DEBUG_ERR(fmt, ...)
#endif

/* 函数声明 */
void Debug_Init(void);
void delay_us(uint16_t us);

#endif /* __DEBUG_H */
