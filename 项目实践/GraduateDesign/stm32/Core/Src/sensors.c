#include "sensors.h"
#include "adc.h"
#include "dht11.h"
#include "tim.h"
#include "debug.h"
#include "radar.h"

/* -------------------------------------------------------
 * DMA 缓冲区：ADC1 双通道结果自动写入此数组
 * [0] = CH0 MQ2 原始值
 * [1] = CH1 GL5516 原始值
 * ------------------------------------------------------- */
uint16_t adc_buf[2] = {0};

/* 全局传感器数据实例 */
SensorData_t sensor_data = {0};


/**
 * @brief  启动 ADC + DMA 循环采集
 *         在 main.c 初始化阶段调用一次
 */
void Sensors_ADC_Init(void)
{
    /* 启动 ADC DMA 循环采集，结果自动存入 adc_buf */
    if (HAL_ADC_Start_DMA(&hadc1, (uint32_t *)adc_buf, 2) != HAL_OK)
    {
        DEBUG_ERR("ADC DMA start failed!");
    }
    else
    {
        DEBUG_INFO("ADC DMA started, dual channel scanning...");
    }
}


/**
 * @brief  读取 DMA 缓冲区数据，换算电压，更新报警标志
 *         在主循环中定时调用（建议每 500ms 调用一次）
 */
void Sensors_ADC_Update(void)
{
    /* 读取 DMA 自动刷新的原始值 */
    sensor_data.mq2_raw   = adc_buf[ADC_CH_MQ2];
    sensor_data.light_raw = adc_buf[ADC_CH_LIGHT];

    /* 换算为电压 */
    sensor_data.mq2_voltage   = (float)sensor_data.mq2_raw
                                 / ADC_RESOLUTION * ADC_VREF;
    sensor_data.light_voltage = (float)sensor_data.light_raw
                                 / ADC_RESOLUTION * ADC_VREF;

    /* 烟雾报警判断：ADC 值越大说明浓度越高 */
    sensor_data.mq2_alarm = (sensor_data.mq2_raw > MQ2_ALARM_THRESHOLD)
                             ? 1 : 0;

    /* 光照过暗判断：分压电路下 ADC 值越大说明光线越暗 */
    sensor_data.light_dark = (sensor_data.light_raw > LIGHT_DARK_THRESHOLD)
                              ? 1 : 0;
}


/**
 * @brief  通过调试串口打印当前传感器数值
 *         调试阶段使用，后续可保留供联调排查
 */
void Sensors_Print(void)
{
    DEBUG_LOG("---- ADC Sensor Data ----");
    DEBUG_LOG("MQ2   | Raw: %4d | Vol: %.2fV | Alarm: %s",
              sensor_data.mq2_raw,
              sensor_data.mq2_voltage,
              sensor_data.mq2_alarm ? "!!! YES !!!" : "NO");
    DEBUG_LOG("Light | Raw: %4d | Vol: %.2fV | Dark : %s",
              sensor_data.light_raw,
              sensor_data.light_voltage,
              sensor_data.light_dark ? "!!! YES !!!" : "NO");
}

/**
 * @brief DHT11 初始化封装
 */
void Sensors_DHT11_Init(void)
{
    DHT11_Init();
}

/**
 * @brief  读取一次 DHT11 数据，结果写入 sensor_data
 *         建议每 2000ms 调用一次（DHT11 采样率上限约 0.5Hz）
 */
void Sensors_DHT11_Update(void)
{
    DHT11_Data_t dht_data = {0};
    uint8_t result = DHT11_Read(&dht_data);

    if (result == DHT11_OK)
    {
        sensor_data.temperature  = dht_data.temperature_int;
        sensor_data.humidity     = dht_data.humidity_int;
        sensor_data.dht11_status = DHT11_OK;
    }
    else
    {
        /* 读取失败时保留上一次有效数据，仅更新状态码 */
        sensor_data.dht11_status = result;
    }
}

/**
 * @brief  串口打印 DHT11 当前数据
 */
void Sensors_DHT11_Print(void)
{
    if (sensor_data.dht11_status == DHT11_OK)
    {
        DEBUG_LOG("DHT11 | Temp: %d C | Humi: %d %%",
                  sensor_data.temperature,
                  sensor_data.humidity);
    }
    else if (sensor_data.dht11_status == DHT11_ERR_TIMEOUT)
    {
        DEBUG_WARN("DHT11 | Read TIMEOUT, check wiring and pull-up resistor");
    }
    else if (sensor_data.dht11_status == DHT11_ERR_CHECKSUM)
    {
        DEBUG_WARN("DHT11 | Checksum ERROR, last valid: Temp=%d C Humi=%d %%",
                   sensor_data.temperature,
                   sensor_data.humidity);
    }
}

/**
 * @brief  将雷达结果同步到 sensor_data
 *         is_fall / is_human 均为简单二值（0或1），无需多级判断
 *		   有新数据时（updated标志）才更新，其余保持上次值	
 */
void Sensors_Radar_Update(void)
{
    /* 同步人体存在状态 */
    if (radar_result.human_updated)
    {
        radar_result.human_updated = 0;
        sensor_data.person_exist   = radar_result.is_human;
    }

    /* 同步跌倒状态 */
    if (radar_result.fall_updated)
    {
        radar_result.fall_updated = 0;
        sensor_data.fall_status   = radar_result.is_fall;
        sensor_data.fall_alarm    = radar_result.is_fall;
    }
}
