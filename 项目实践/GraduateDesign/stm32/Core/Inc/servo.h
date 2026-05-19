#ifndef __SERVO_H
#define __SERVO_H

#include "stm32f1xx_hal.h"
#include <stdint.h>

/* -------------------------------------------------------
 * 舵机 PWM 参数定义
 * ARR = 19999，计数频率 1MHz
 * CCR 值 = 脉宽(us)
 * ------------------------------------------------------- */
#define SERVO_CCR_MIN       500     /* 0°   对应 CCR 值（0.5ms）  */
#define SERVO_CCR_MID       1500    /* 90°  对应 CCR 值（1.5ms）  */
#define SERVO_CCR_MAX       2500    /* 180° 对应 CCR 值（2.5ms）  */

/* 业务角度语义定义（门窗控制场景） */
#define SERVO_ANGLE_CLOSE   0       /* 0°：门窗关闭位置  */
#define SERVO_ANGLE_HALF    90      /* 90°：门窗半开位置 */
#define SERVO_ANGLE_OPEN    180     /* 180°：门窗全开位置 */

/* 舵机状态枚举 */
typedef enum
{
    SERVO_STATE_CLOSE = 0,
    SERVO_STATE_HALF,
    SERVO_STATE_OPEN
} ServoState_t;

/* 全局舵机状态，供其他模块读取 */
extern ServoState_t servo_state;

/* 函数声明 */
void    Servo_Init(void);
void    Servo_SetAngle(uint8_t angle);
void    Servo_SetCCR(uint16_t ccr);
void    Servo_Open(void);
void    Servo_Close(void);
void    Servo_Half(void);
uint8_t Servo_GetAngle(void);

#endif /* __SERVO_H */
