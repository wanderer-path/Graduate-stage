#include "debug.h"
#include "usart.h"
#include "tim.h"

/**
 * @brief  printf 重定向到 USART3
 *         Keil 需在 Options → Target 中勾选 Use MicroLIB
 */
int fputc(int ch, FILE *f)
{
    HAL_UART_Transmit(&huart3, (uint8_t *)&ch, 1, HAL_MAX_DELAY);
    return ch;
}

/**
 * @brief  调试串口初始化提示
 *         在 main.c 中所有外设初始化完成后调用一次
 */
void Debug_Init(void)
{
    printf("\r\n");
    printf("========================================\r\n");
    printf("  Smart Elder Monitor - Debug Console   \r\n");
    printf("  Build: %s %s                \r\n", __DATE__, __TIME__);
    printf("  MCU  : STM32F103C8T6 @ 72MHz          \r\n");
    printf("========================================\r\n");
    printf("\r\n");
}

/**
 * @brief  微秒级延时函数（基于 TIM2，1MHz 计数）
 *         阶段三 DHT11 时序使用，此处提前实现
 * @param  us : 延时微秒数，最大 65535us
 */
void delay_us(uint16_t us)
{
    __HAL_TIM_SET_COUNTER(&htim2, 0);          /* 计数器清零 */
    HAL_TIM_Base_Start(&htim2);                /* 启动定时器 */
    while (__HAL_TIM_GET_COUNTER(&htim2) < us) /* 等待计数到目标值 */
    {
        /* 空等待 */
    }
    HAL_TIM_Base_Stop(&htim2);                 /* 停止定时器 */
}
