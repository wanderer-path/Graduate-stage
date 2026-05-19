#ifndef __RADAR_H
#define __RADAR_H

#include "stm32f1xx_hal.h"
#include <stdint.h>
#include <string.h>

/* -------------------------------------------------------
 * TinyFrame 协议常量
 * ------------------------------------------------------- */
#define TF_SOF              0x01

#define TF_TYPE_FALL        0x0E02   /* 跌倒检测结果 TYPE */
#define TF_TYPE_PRESENCE    0x0F09   /* 有无人检测结果 TYPE */

#define TF_MAX_DATA_LEN     128       /* DATA区最大字节数 */

/* 超过此长度则进入跳过模式（不resync） */
#define TF_SKIP_THRESHOLD   128

/* -------------------------------------------------------
 * 接收环形缓冲区大小
 * ------------------------------------------------------- */
#define RADAR_RX_BUF_SIZE   256

/* -------------------------------------------------------
 * TinyFrame 解析状态机枚举
 * ------------------------------------------------------- */
typedef enum
{
    TF_STATE_SOF = 0,
    TF_STATE_ID,
    TF_STATE_LEN,
    TF_STATE_TYPE,
    TF_STATE_HEAD_CKSUM,
    TF_STATE_DATA,
    TF_STATE_DATA_CKSUM,
    TF_STATE_SKIP,          /* 新增：跳过超大帧的数据区 */
} TF_State_t;

/* -------------------------------------------------------
 * 雷达检测结果结构体
 * ------------------------------------------------------- */
typedef struct
{
    uint8_t is_fall;           /* 跌倒：0=正常 1=跌倒 */
    uint8_t is_human;          /* 存在：0=无人 1=有人 */
    uint8_t fall_updated;      /* 跌倒数据刷新标志，读取后清0 */
    uint8_t human_updated;     /* 存在数据刷新标志，读取后清0 */
} RadarResult_t;

extern RadarResult_t radar_result;
extern uint8_t       radar_rx_byte;

/* -------------------------------------------------------
 * 函数声明
 * ------------------------------------------------------- */
void Radar_Init(void);
void Radar_RxCallback(uint8_t byte);
void Radar_Process(void);
void Radar_Print(void);

#endif /* __RADAR_H */
