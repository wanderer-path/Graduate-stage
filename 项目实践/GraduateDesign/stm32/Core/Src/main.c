/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "adc.h"
#include "dma.h"
#include "i2c.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "debug.h"
#include "sensors.h"
#include "servo.h"
#include "actuator.h"
#include "display.h"
#include "radar.h"
#include "alarm.h"
#include "esp8266.h"
#include "mqtt_aliyun.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
/* -----------------------------------------------
 * 各任务计时器
 * ----------------------------------------------- */
static uint32_t last_heartbeat  = 0;   /* 板载 LED 心跳   500ms  */
static uint32_t last_adc        = 0;   /* ADC 采集        1000ms */
static uint32_t last_dht11      = 0;   /* DHT11 采集      2000ms */
static uint32_t last_display    = 0;   /* 屏幕刷新        1000ms */
static uint32_t last_status_log = 0;   /* 系统状态打印    2000ms */
static uint32_t last_mqtt_post  = 0;   /* 阿里云数据上报  5000ms */
static uint32_t last_mqtt_set   = 0;   /* 阿里云数据处理  1000ms */
static uint32_t last_mqtt_check = 0;   /* 阿里云mqtt连接  30000ms */
static uint8_t  led_state       = 0;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_TIM2_Init();
  MX_USART3_UART_Init();
  MX_ADC1_Init();
  MX_TIM3_Init();
  MX_I2C1_Init();
  MX_USART2_UART_Init();
  MX_USART1_UART_Init();
  /* USER CODE BEGIN 2 */

	/* -----------------------------------------------
	 * 初始化顺序：底层 → 传感器 → 执行器 → 显示 → 通信
	 * ----------------------------------------------- */

	/* 1. 微秒延时定时器 */
	HAL_TIM_Base_Start(&htim2);

	/* 2. 调试串口 */
	Debug_Init();
	DEBUG_INFO("========= System Boot =========");

	/* 3. 传感器初始化 */
	DEBUG_INFO("Init: ADC sensors...");
	Sensors_ADC_Init();

	DEBUG_INFO("Init: DHT11...");
	Sensors_DHT11_Init();

	/* 4. 执行器初始化 */
	DEBUG_INFO("Init: Servo...");
	Servo_Init();

	DEBUG_INFO("Init: Actuator (LED + Buzzer)...");
	Actuator_Init();

	/* 5. 屏幕初始化 */
	DEBUG_INFO("Init: Display...");
	Display_Init();

	/* 6. 雷达初始化 */
	DEBUG_INFO("Init: Radar...");
	Radar_Init();

	/* 7. 报警逻辑初始化 */
	DEBUG_INFO("Init: Alarm system...");
	Alarm_Init();
	
	/* 8. ESP8266 + 阿里云（最后初始化，含网络连接耗时操作） */
	DEBUG_INFO("Init: ESP8266...");
	ESP_Init();
	DEBUG_INFO("Init: Aliyun MQTT...");
	MQTT_Init();

	/* 上电提示音 */
	Beep_Times(3, 80, 80);

	DEBUG_INFO("======= System Ready ==========");

	/* 初始化所有计时器为当前时间 */
	last_heartbeat  = HAL_GetTick();
	last_adc        = HAL_GetTick();
	last_dht11      = HAL_GetTick();
	last_display    = HAL_GetTick();
	last_status_log = HAL_GetTick();
	last_mqtt_post       = HAL_GetTick();
	last_mqtt_set       = HAL_GetTick();
	last_mqtt_check = HAL_GetTick();
	
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {

	uint32_t now = HAL_GetTick();

    /* -----------------------------------------------
     * Task 1：心跳 LED（板载 PC13）
     * 周期：500ms
     * ----------------------------------------------- */
    if (now - last_heartbeat >= 500)
    {
        last_heartbeat = now;
        led_state = !led_state;
        HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13,
                          led_state ? GPIO_PIN_RESET : GPIO_PIN_SET);
    }

    /* -----------------------------------------------
     * Task 2：ADC 传感器采集（MQ2 + GL5516）
     * 周期：1000ms
     * ----------------------------------------------- */
    if (now - last_adc >= 1000)
    {
        last_adc = now;
        Sensors_ADC_Update();
    }

    /* -----------------------------------------------
     * Task 3：DHT11 温湿度采集
     * 周期：2000ms（DHT11 采样率上限）
     * ----------------------------------------------- */
    if (now - last_dht11 >= 2000)
    {
        last_dht11 = now;
        Sensors_DHT11_Update();
    }

    /* -----------------------------------------------
     * Task 4：雷达数据解析（每次迭代执行，不设周期）
     * 从环形缓冲区取出所有待处理字节，驱动状态机
     * ----------------------------------------------- */
    Radar_Process();

    /* -----------------------------------------------
     * Task 5：雷达结果同步到 sensor_data
     * 有新数据时立即同步（由 updated 标志控制）
     * ----------------------------------------------- */
    Sensors_Radar_Update();

    /* -----------------------------------------------
     * Task 6：报警逻辑评估与执行器联动
     * 每次迭代执行，响应及时
     * ----------------------------------------------- */
    Alarm_Update();

    /* -----------------------------------------------
     * Task 7：执行器非阻塞轮询驱动
     * 驱动 LED 闪烁和蜂鸣器间歇
     * 每次迭代执行
     * ----------------------------------------------- */
    Actuator_Tick();

    /* -----------------------------------------------
     * Task 8：屏幕刷新
     * 周期：1000ms
     * ----------------------------------------------- */
    if (now - last_display >= 1000)
    {
        last_display = now;
        Display_Update();
    }

    /* -----------------------------------------------
     * Task 9：系统状态汇总打印到调试串口
     * 周期：2000ms
     * ----------------------------------------------- */
    if (now - last_status_log >= 2000)
    {
        last_status_log = now;
        Alarm_Print();
    }

    /* Task 10：MQTT 数据上报（5000ms） */
    if (now - last_mqtt_post >= MQTT_POST_INTERVAL_MS)
    {
        last_mqtt_post = now;
        MQTT_Post_Task();
    }

	/* Task 11：MQTT 下行处理（2000ms） */
    if (now - last_mqtt_set >= MQTT_SET_INTERVAL_MS)
    {
        last_mqtt_set = now;
        MQTT_Set_Task();
    }
	
    /* Task 12：MQTT 连接检查与重连（30000ms） */
    if (now - last_mqtt_check >= 30000)
    {
        last_mqtt_check = now;
        MQTT_CheckConnection();
    }

    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_ADC;
  PeriphClkInit.AdcClockSelection = RCC_ADCPCLK2_DIV6;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
