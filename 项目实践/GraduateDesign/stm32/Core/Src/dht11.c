#include "dht11.h"
#include "debug.h"
#include "tim.h"

/* -------------------------------------------------------
 * 内部静态函数：GPIO 方向切换
 * ------------------------------------------------------- */

/**
 * @brief 将 DHT11 数据引脚设置为推挽输出模式
 */
static void DHT11_SetOutput(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin   = DHT11_GPIO_PIN;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(DHT11_GPIO_PORT, &GPIO_InitStruct);
}

/**
 * @brief 将 DHT11 数据引脚设置为浮空输入模式
 */
static void DHT11_SetInput(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin  = DHT11_GPIO_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;      /* 外部已有上拉电阻 */
    HAL_GPIO_Init(DHT11_GPIO_PORT, &GPIO_InitStruct);
}

/**
 * @brief  读取数据引脚当前电平
 * @return 1:高电平  0:低电平
 */
static inline uint8_t DHT11_ReadPin(void)
{
    return (uint8_t)HAL_GPIO_ReadPin(DHT11_GPIO_PORT, DHT11_GPIO_PIN);
}

/* -------------------------------------------------------
 * 内部静态函数：等待引脚电平变化（含超时保护）
 * ------------------------------------------------------- */

/**
 * @brief  等待引脚变为指定电平
 * @param  level    期望电平 1 或 0
 * @param  timeout  超时时间 us
 * @return 实际等待时间 us，超时返回 0xFFFF
 */
static uint16_t DHT11_WaitLevel(uint8_t level, uint16_t timeout)
{
    uint16_t us = 0;
    while (DHT11_ReadPin() != level)
    {
        delay_us(1);
        us++;
        if (us >= timeout)
        {
            return 0xFFFF;   /* 超时标志 */
        }
    }
    return us;
}

/* -------------------------------------------------------
 * 公开函数实现
 * ------------------------------------------------------- */

/**
 * @brief  DHT11 初始化
 *         确保数据线初始为高电平，等待 DHT11 上电稳定
 */
void DHT11_Init(void)
{
    DHT11_SetOutput();
    HAL_GPIO_WritePin(DHT11_GPIO_PORT, DHT11_GPIO_PIN, GPIO_PIN_SET);
    HAL_Delay(1000);    /* 等待 DHT11 上电稳定至少 1s */
    DEBUG_INFO("DHT11 initialized, waiting for sensor stable...");
}

/**
 * @brief  读取 DHT11 温湿度数据
 * @param  data  指向 DHT11_Data_t 结构体的指针，读取结果存入此处
 * @return DHT11_OK / DHT11_ERR_TIMEOUT / DHT11_ERR_CHECKSUM
 *
 * @note   此函数内部会短暂关闭全局中断以保护时序精度
 *         整个读取过程约 4~5ms
 */
uint8_t DHT11_Read(DHT11_Data_t *data)
{
    uint8_t  raw_data[5] = {0};   /* 存放 40bit 原始数据，每字节一个元素 */
    uint8_t  bit_val     = 0;
    uint16_t wait_time   = 0;

    /* -----------------------------------------------
     * Step 1：主机发送起始信号
     * ----------------------------------------------- */
    DHT11_SetOutput();
    HAL_GPIO_WritePin(DHT11_GPIO_PORT, DHT11_GPIO_PIN, GPIO_PIN_RESET);
    HAL_Delay(20);       /* 低电平保持 20ms（规范要求 ≥ 18ms） */
    HAL_GPIO_WritePin(DHT11_GPIO_PORT, DHT11_GPIO_PIN, GPIO_PIN_SET);
    delay_us(30);        /* 高电平保持 20~40us，取 30us */

    /* 切换为输入，释放总线由 DHT11 控制 */
    DHT11_SetInput();

    /* -----------------------------------------------
     * Step 2：等待 DHT11 应答（关闭中断保护时序）
     * ----------------------------------------------- */
    __disable_irq();

    /* 等待 DHT11 拉低（应答低电平，约 80us） */
    wait_time = DHT11_WaitLevel(0, 100);
    if (wait_time == 0xFFFF)
    {
        __enable_irq();
        data->status = DHT11_ERR_TIMEOUT;
        DEBUG_ERR("DHT11: timeout waiting for response LOW");
        return DHT11_ERR_TIMEOUT;
    }

    /* 等待 DHT11 拉高（应答高电平，约 80us） */
    wait_time = DHT11_WaitLevel(1, 100);
    if (wait_time == 0xFFFF)
    {
        __enable_irq();
        data->status = DHT11_ERR_TIMEOUT;
        DEBUG_ERR("DHT11: timeout waiting for response HIGH");
        return DHT11_ERR_TIMEOUT;
    }

    /* 等待应答高电平结束，进入数据传输阶段 */
    wait_time = DHT11_WaitLevel(0, 100);
    if (wait_time == 0xFFFF)
    {
        __enable_irq();
        data->status = DHT11_ERR_TIMEOUT;
        DEBUG_ERR("DHT11: timeout waiting for data start");
        return DHT11_ERR_TIMEOUT;
    }

    /* -----------------------------------------------
     * Step 3：读取 40bit 数据
     * 每个 bit：先等 50us 低电平结束，再测量高电平时长
     *   高电平 26~28us → 0
     *   高电平 70us    → 1
     *   判断阈值取 40us
     * ----------------------------------------------- */
    for (int byte_idx = 0; byte_idx < 5; byte_idx++)
    {
        for (int bit_idx = 7; bit_idx >= 0; bit_idx--)
        {
            /* 等待低电平结束（每个 bit 的起始低电平约 50us） */
            wait_time = DHT11_WaitLevel(1, 80);
            if (wait_time == 0xFFFF)
            {
                __enable_irq();
                data->status = DHT11_ERR_TIMEOUT;
                DEBUG_ERR("DHT11: timeout during data bit read");
                return DHT11_ERR_TIMEOUT;
            }

            /* 延时 40us 后采样：仍为高电平则为 1，否则为 0 */
            delay_us(40);
            bit_val = DHT11_ReadPin();

            /* 将 bit 写入对应字节 */
            if (bit_val)
            {
                raw_data[byte_idx] |= (1 << bit_idx);
            }

            /* 等待高电平结束，准备下一个 bit */
            wait_time = DHT11_WaitLevel(0, 100);
            if (wait_time == 0xFFFF)
            {
                __enable_irq();
                data->status = DHT11_ERR_TIMEOUT;
                DEBUG_ERR("DHT11: timeout waiting for next bit");
                return DHT11_ERR_TIMEOUT;
            }
        }
    }

    __enable_irq();    /* 恢复全局中断 */

    /* -----------------------------------------------
     * Step 4：校验和验证
     * 校验和 = 前四字节之和的低 8 位
     * ----------------------------------------------- */
    data->humidity_int    = raw_data[0];
    data->humidity_dec    = raw_data[1];
    data->temperature_int = raw_data[2];
    data->temperature_dec = raw_data[3];
    data->checksum        = raw_data[4];

    uint8_t calc_checksum = raw_data[0] + raw_data[1]
                          + raw_data[2] + raw_data[3];

    if (calc_checksum != data->checksum)
    {
        data->status = DHT11_ERR_CHECKSUM;
        DEBUG_ERR("DHT11: checksum error! calc=0x%02X recv=0x%02X",
                  calc_checksum, data->checksum);
        return DHT11_ERR_CHECKSUM;
    }

    data->status = DHT11_OK;
    return DHT11_OK;
}
