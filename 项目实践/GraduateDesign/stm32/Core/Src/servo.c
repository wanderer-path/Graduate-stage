#include "servo.h"
#include "tim.h"
#include "debug.h"

/* 当前舵机状态 */
ServoState_t servo_state = SERVO_STATE_CLOSE;

/* 当前角度缓存（用于 GetAngle 查询） */
static uint8_t current_angle = 0;


/**
 * @brief  舵机初始化
 *         启动 TIM3 PWM 输出，舵机归位到 0°（关闭位置）
 */
void Servo_Init(void)
{
    /* 启动 TIM3 Channel1 PWM 输出 */
    if (HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1) != HAL_OK)
    {
        DEBUG_ERR("Servo: TIM4 PWM start failed!");
        return;
    }

    /* 初始归位到 0° 关闭位置 */
    Servo_Close();
    DEBUG_INFO("Servo initialized, position: CLOSE (0 deg)");
}


/**
 * @brief  通过直接设置 CCR 值控制舵机位置
 * @param  ccr  CCR 寄存器值，范围 500~2500
 *
 * @note   此函数为底层接口，通常使用 Servo_SetAngle() 更直观
 */
void Servo_SetCCR(uint16_t ccr)
{
    /* 限幅保护，防止超出舵机物理范围损坏齿轮 */
    if (ccr < SERVO_CCR_MIN) ccr = SERVO_CCR_MIN;
    if (ccr > SERVO_CCR_MAX) ccr = SERVO_CCR_MAX;

    __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, ccr);
}


/**
 * @brief  按角度控制舵机位置
 * @param  angle  目标角度，范围 0~180°
 *
 * @note   角度到 CCR 线性映射：
 *         CCR = SERVO_CCR_MIN + angle * (SERVO_CCR_MAX - SERVO_CCR_MIN) / 180
 *             = 500 + angle * 2000 / 180
 *             = 500 + angle * 11.11
 */
void Servo_SetAngle(uint8_t angle)
{
    /* 角度限幅 */
    if (angle > 180) angle = 180;

    /* 线性映射计算 CCR */
    uint16_t ccr = SERVO_CCR_MIN
                 + (uint16_t)((uint32_t)angle
                 * (SERVO_CCR_MAX - SERVO_CCR_MIN) / 180);

    Servo_SetCCR(ccr);
    current_angle = angle;

    /* 更新状态枚举 */
    if (angle == SERVO_ANGLE_CLOSE)
        servo_state = SERVO_STATE_CLOSE;
    else if (angle == SERVO_ANGLE_OPEN)
        servo_state = SERVO_STATE_OPEN;
    else
        servo_state = SERVO_STATE_HALF;

    DEBUG_LOG("Servo -> %d deg (CCR: %d)", angle, ccr);
}


/**
 * @brief  舵机转到全开位置（180°）
 *         对应门窗全开，烟雾报警时调用
 */
void Servo_Open(void)
{
    Servo_SetAngle(SERVO_ANGLE_OPEN);
    servo_state = SERVO_STATE_OPEN;
    DEBUG_INFO("Servo -> OPEN (180 deg)");
}


/**
 * @brief  舵机转到关闭位置（0°）
 *         对应门窗关闭，正常状态时调用
 */
void Servo_Close(void)
{
    Servo_SetAngle(SERVO_ANGLE_CLOSE);
    servo_state = SERVO_STATE_CLOSE;
    DEBUG_INFO("Servo -> CLOSE (0 deg)");
}


/**
 * @brief  舵机转到半开位置（90°）
 *         可用于部分通风场景
 */
void Servo_Half(void)
{
    Servo_SetAngle(SERVO_ANGLE_HALF);
    servo_state = SERVO_STATE_HALF;
    DEBUG_INFO("Servo -> HALF (90 deg)");
}


/**
 * @brief  获取舵机当前角度
 * @return 当前角度值 0~180
 */
uint8_t Servo_GetAngle(void)
{
    return current_angle;
}
