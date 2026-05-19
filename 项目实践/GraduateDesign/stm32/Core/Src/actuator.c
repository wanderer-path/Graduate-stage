#include "actuator.h"
#include "debug.h"

/* 全局执行器状态实例 */
ActuatorStatus_t actuator_status = {0};


/* -------------------------------------------------------
 * 底层 GPIO 操作（内部使用）
 * ------------------------------------------------------- */

static void LED_SetPin(uint8_t state)
{
    HAL_GPIO_WritePin(LED_GPIO_PORT, LED_GPIO_PIN,
                      state ? GPIO_PIN_SET : GPIO_PIN_RESET);
    actuator_status.led_state = state;
}

static void Beep_SetPin(uint8_t state)
{
    HAL_GPIO_WritePin(BEEP_GPIO_PORT, BEEP_GPIO_PIN,
                      state ? GPIO_PIN_RESET : GPIO_PIN_SET);
    actuator_status.beep_state = state;
}


/* -------------------------------------------------------
 * 初始化
 * ------------------------------------------------------- */

/**
 * @brief  执行器初始化
 *         LED 熄灭，蜂鸣器静音，模式均为 OFF
 */
void Actuator_Init(void)
{
    LED_SetPin(0);
    Beep_SetPin(0);

    actuator_status.led_mode       = LED_MODE_OFF;
    actuator_status.beep_mode      = BEEP_MODE_OFF;
    actuator_status.led_last_tick  = HAL_GetTick();
    actuator_status.beep_last_tick = HAL_GetTick();

    DEBUG_INFO("Actuator initialized: LED=OFF, BEEP=OFF");
}


/* -------------------------------------------------------
 * LED 控制函数
 * ------------------------------------------------------- */

/**
 * @brief  LED 常亮
 */
void LED_On(void)
{
    actuator_status.led_mode = LED_MODE_ON;
    LED_SetPin(1);
    DEBUG_LOG("LED -> ON");
}

/**
 * @brief  LED 熄灭
 */
void LED_Off(void)
{
    actuator_status.led_mode = LED_MODE_OFF;
    LED_SetPin(0);
    DEBUG_LOG("LED -> OFF");
}

/**
 * @brief  LED 翻转一次
 */
void LED_Toggle(void)
{
    uint8_t new_state = !actuator_status.led_state;
    LED_SetPin(new_state);
}

/**
 * @brief  设置 LED 工作模式
 * @param  mode  LedMode_t 枚举值
 *
 * @note   设置后由 Actuator_Tick() 在主循环中非阻塞驱动闪烁
 */
void LED_SetMode(LedMode_t mode)
{
    if (actuator_status.led_mode == mode) return;   /* 相同模式不重复设置 */

    actuator_status.led_mode      = mode;
    actuator_status.led_last_tick = HAL_GetTick();

    switch (mode)
    {
        case LED_MODE_OFF:
            LED_SetPin(0);
            DEBUG_LOG("LED mode -> OFF");
            break;
        case LED_MODE_ON:
            LED_SetPin(1);
            DEBUG_LOG("LED mode -> ON (always)");
            break;
        case LED_MODE_BLINK_SLOW:
            DEBUG_LOG("LED mode -> BLINK SLOW (1000ms)");
            break;
        case LED_MODE_BLINK_FAST:
            DEBUG_LOG("LED mode -> BLINK FAST (200ms)");
            break;
        default:
            break;
    }
}


/* -------------------------------------------------------
 * 蜂鸣器控制函数
 * ------------------------------------------------------- */

/**
 * @brief  蜂鸣器持续开启
 */
void Beep_On(void)
{
    actuator_status.beep_mode = BEEP_MODE_ON;
    Beep_SetPin(1);
    DEBUG_LOG("BEEP -> ON");
}

/**
 * @brief  蜂鸣器关闭
 */
void Beep_Off(void)
{
    actuator_status.beep_mode = BEEP_MODE_OFF;
    Beep_SetPin(0);
    DEBUG_LOG("BEEP -> OFF");
}

/**
 * @brief  蜂鸣器鸣叫一次（阻塞方式）
 * @param  duration_ms  鸣叫持续时间（毫秒）
 *
 * @note   此函数为阻塞式，仅用于初始化提示音等短促场景
 *         持续报警请使用 Beep_SetMode(BEEP_MODE_ON)
 */
void Beep_Once(uint32_t duration_ms)
{
    Beep_SetPin(1);
    HAL_Delay(duration_ms);
    Beep_SetPin(0);
    DEBUG_LOG("BEEP -> once (%lu ms)", duration_ms);
}

/**
 * @brief  蜂鸣器鸣叫 n 次（阻塞方式）
 * @param  n        鸣叫次数
 * @param  on_ms    每次鸣叫持续时间（毫秒）
 * @param  off_ms   每次间隔时间（毫秒）
 *
 * @note   阻塞式，用于上电提示音等场景
 *         总阻塞时间 = n * (on_ms + off_ms)
 */
void Beep_Times(uint8_t n, uint32_t on_ms, uint32_t off_ms)
{
    DEBUG_LOG("BEEP -> %d times (on:%lu ms off:%lu ms)", n, on_ms, off_ms);
    for (uint8_t i = 0; i < n; i++)
    {
        Beep_SetPin(1);
        HAL_Delay(on_ms);
        Beep_SetPin(0);
        if (i < n - 1)          /* 最后一次不需要等待间隔 */
        {
            HAL_Delay(off_ms);
        }
    }
}

/**
 * @brief  设置蜂鸣器工作模式
 * @param  mode  BeepMode_t 枚举值
 *
 * @note   设置后由 Actuator_Tick() 在主循环中非阻塞驱动
 */
void Beep_SetMode(BeepMode_t mode)
{
    if (actuator_status.beep_mode == mode) return;

    actuator_status.beep_mode      = mode;
    actuator_status.beep_last_tick = HAL_GetTick();

    switch (mode)
    {
        case BEEP_MODE_OFF:
            Beep_SetPin(0);
            DEBUG_LOG("BEEP mode -> OFF");
            break;
        case BEEP_MODE_ON:
            Beep_SetPin(1);
            DEBUG_LOG("BEEP mode -> ON (continuous)");
            break;
        case BEEP_MODE_INTERVAL:
            DEBUG_LOG("BEEP mode -> INTERVAL (500ms on/off)");
            break;
        default:
            break;
    }
}


/* -------------------------------------------------------
 * 主循环轮询驱动（非阻塞）
 * ------------------------------------------------------- */

/**
 * @brief  执行器轮询驱动，在主循环中每次迭代调用
 *         负责驱动 LED 闪烁模式与蜂鸣器间歇模式
 *         使用 HAL_GetTick() 实现非阻塞定时，不影响其他任务
 */
void Actuator_Tick(void)
{
    uint32_t now = HAL_GetTick();

    /* -----------------------------------------------
     * LED 闪烁驱动
     * ----------------------------------------------- */
    switch (actuator_status.led_mode)
    {
        case LED_MODE_BLINK_SLOW:
            /* 1000ms 周期翻转 */
            if (now - actuator_status.led_last_tick >= 1000)
            {
                actuator_status.led_last_tick = now;
                LED_Toggle();
            }
            break;

        case LED_MODE_BLINK_FAST:
            /* 200ms 周期翻转 */
            if (now - actuator_status.led_last_tick >= 200)
            {
                actuator_status.led_last_tick = now;
                LED_Toggle();
            }
            break;

        case LED_MODE_OFF:
        case LED_MODE_ON:
        default:
            /* 常亮/熄灭不需要轮询处理 */
            break;
    }

    /* -----------------------------------------------
     * 蜂鸣器间歇驱动
     * ----------------------------------------------- */
    switch (actuator_status.beep_mode)
    {
        case BEEP_MODE_INTERVAL:
            /* 500ms 周期翻转 */
            if (now - actuator_status.beep_last_tick >= 500)
            {
                actuator_status.beep_last_tick = now;
                uint8_t new_state = !actuator_status.beep_state;
                Beep_SetPin(new_state);
            }
            break;

        case BEEP_MODE_OFF:
        case BEEP_MODE_ON:
        default:
            break;
    }
}


/* -------------------------------------------------------
 * 调试打印
 * ------------------------------------------------------- */

/**
 * @brief  串口打印执行器当前状态
 */
void Actuator_Print(void)
{
    const char *led_mode_str[] = {"OFF", "ON", "BLINK_SLOW", "BLINK_FAST"};
    const char *beep_mode_str[] = {"OFF", "ON", "INTERVAL"};

    DEBUG_LOG("Actuator | LED: %-10s | BEEP: %-8s",
              led_mode_str[actuator_status.led_mode],
              beep_mode_str[actuator_status.beep_mode]);
}
