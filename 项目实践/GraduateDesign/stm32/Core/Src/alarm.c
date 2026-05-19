#include "alarm.h"
#include "sensors.h"
#include "actuator.h"
#include "servo.h"
#include "display.h"
#include "debug.h"

/* 全局系统状态实例 */
SystemStatus_t sys_status = {0};


/* -------------------------------------------------------
 * 内部函数：各级别报警处理
 * ------------------------------------------------------- */

/**
 * @brief  进入正常状态：关闭所有报警，舵机归位
 */
static void Alarm_EnterNormal(void)
{
    LED_SetMode(LED_MODE_OFF);
    Beep_SetMode(BEEP_MODE_OFF);
    Servo_Close();

    sys_status.fall_alarm_active  = 0;
    sys_status.smoke_alarm_active = 0;
    sys_status.dark_alarm_active  = 0;

    DEBUG_INFO("[Alarm] -> NORMAL: all actuators OFF, servo CLOSE.");
}

/**
 * @brief  进入光照过暗状态：LED 常亮补光
 */
static void Alarm_EnterDark(void)
{
    LED_SetMode(LED_MODE_ON);
    Beep_SetMode(BEEP_MODE_OFF);
    /* 舵机不动作，保持当前位置 */

    sys_status.dark_alarm_active = 1;

    DEBUG_INFO("[Alarm] -> DARK: LED ON for lighting.");
}

/**
 * @brief  进入烟雾报警状态：LED 慢闪 + 蜂鸣器间歇 + 舵机开窗
 */
static void Alarm_EnterSmoke(void)
{
    LED_SetMode(LED_MODE_BLINK_SLOW);
    Beep_SetMode(BEEP_MODE_INTERVAL);
    Servo_Open();

    sys_status.smoke_alarm_active  = 1;
    sys_status.smoke_alarm_start_ms = HAL_GetTick();

    DEBUG_WARN("[Alarm] -> SMOKE: LED blink + BEEP interval + Servo OPEN.");
}

/**
 * @brief  进入跌倒报警状态：LED 快闪 + 蜂鸣器持续
 */
static void Alarm_EnterFall(void)
{
    LED_SetMode(LED_MODE_BLINK_FAST);
    Beep_SetMode(BEEP_MODE_ON);
    /* 跌倒时不操作舵机 */

    sys_status.fall_alarm_active  = 1;
    sys_status.fall_alarm_start_ms = HAL_GetTick();

    DEBUG_WARN("[Alarm] -> FALL: LED fast blink + BEEP continuous.");
}


/* -------------------------------------------------------
 * 对外接口
 * ------------------------------------------------------- */

/**
 * @brief  报警模块初始化，系统上电为正常状态
 */
void Alarm_Init(void)
{
    sys_status.current_level = ALARM_NORMAL;
    sys_status.last_level    = ALARM_NORMAL;
    Alarm_EnterNormal();
    DEBUG_INFO("Alarm system initialized, state: NORMAL.");
}

/**
 * @brief  报警逻辑主更新函数，在主循环中每次迭代调用
 *
 * 执行流程：
 * 1. 根据传感器数据评估当前最高报警级别
 * 2. 与上次级别比较，若发生变化则触发对应报警动作
 * 3. 检查超时自动解除逻辑
 */
void Alarm_Update(void)
{
    uint32_t now = HAL_GetTick();

    /* -----------------------------------------------
     * Step 1：评估当前报警级别（取最高优先级）
     * ----------------------------------------------- */
    AlarmLevel_t new_level = ALARM_NORMAL;

    /* 检查光照（最低优先级） */
    if (sensor_data.light_dark)
    {
        new_level = ALARM_DARK;
    }

    /* 检查烟雾（覆盖光照） */
    if (sensor_data.mq2_alarm)
    {
        new_level = ALARM_SMOKE;
    }

    /* 检查跌倒（最高优先级） */
    if (sensor_data.fall_alarm)
    {
        new_level = ALARM_FALL;
    }

    /* -----------------------------------------------
     * Step 2：超时自动解除判断
     * 若传感器已恢复正常但上次报警仍在持续，
     * 超过超时时间后强制解除
     * ----------------------------------------------- */

    /* 跌倒超时解除：雷达不再检测到跌倒且超过 30s */
    if (sys_status.fall_alarm_active &&
        sensor_data.fall_alarm == 0 &&
        (now - sys_status.fall_alarm_start_ms) > FALL_ALARM_TIMEOUT_MS)
    {
        sys_status.fall_alarm_active = 0;
        DEBUG_INFO("[Alarm] Fall alarm timeout, auto released.");
    }

    /* 烟雾超时解除：烟雾浓度恢复正常且超过 60s */
    if (sys_status.smoke_alarm_active &&
        sensor_data.mq2_alarm == 0 &&
        (now - sys_status.smoke_alarm_start_ms) > SMOKE_ALARM_TIMEOUT_MS)
    {
        sys_status.smoke_alarm_active = 0;
        DEBUG_INFO("[Alarm] Smoke alarm timeout, auto released.");
    }

    /* -----------------------------------------------
     * Step 3：若历史报警标志仍激活，维持对应级别不降低
     * 防止传感器瞬时抖动导致报警级别误降
     * ----------------------------------------------- */
    if (sys_status.fall_alarm_active  && new_level < ALARM_FALL)
        new_level = ALARM_FALL;

    if (sys_status.smoke_alarm_active && new_level < ALARM_SMOKE)
        new_level = ALARM_SMOKE;

    /* -----------------------------------------------
     * Step 4：与上次级别比较，发生变化则切换状态
     * ----------------------------------------------- */
    if (new_level != sys_status.current_level)
    {
        sys_status.last_level    = sys_status.current_level;
        sys_status.current_level = new_level;

        switch (new_level)
        {
            case ALARM_NORMAL: Alarm_EnterNormal(); break;
            case ALARM_DARK:   Alarm_EnterDark();   break;
            case ALARM_SMOKE:  Alarm_EnterSmoke();  break;
            case ALARM_FALL:   Alarm_EnterFall();   break;
            default:           Alarm_EnterNormal(); break;
        }
    }

    /* -----------------------------------------------
     * Step 5：同步跌倒状态到屏幕显示模块
     * ----------------------------------------------- */
    Display_SetFallStatus(sensor_data.fall_alarm);
}

/**
 * @brief  串口打印当前系统报警状态
 */
void Alarm_Print(void)
{
    const char *level_str[] = {"NORMAL", "DARK", "SMOKE", "FALL"};

    DEBUG_LOG("[SYS] T:%dC H:%d%% Smoke:%d(%s) Light:%d(%s) "
              "Exist:%d Fall:%d(%s) | Alarm->%s",
              sensor_data.temperature,
              sensor_data.humidity,
              sensor_data.mq2_raw,
              sensor_data.mq2_alarm   ? "!!" : "OK",
              sensor_data.light_raw,
              sensor_data.light_dark  ? "DK" : "OK",
              sensor_data.person_exist,
              sensor_data.fall_status,
              sensor_data.fall_alarm  ? "!!" : "OK",
              level_str[sys_status.current_level]);
}
