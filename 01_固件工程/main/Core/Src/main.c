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
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "config.h"
#include "ota_boot.h"
#include "ota_layout.h"
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
	/* C 运行库就绪后立即拉高 PB3 锁存供电，避免后续初始化期间掉电。 */
	RCC->AHBENR |= RCC_AHBENR_GPIOBEN;
	GPIOB->BSRR = GPIO_BSRR_BS_3;
	*(volatile uint32_t *)OTA_BOOT_TRACE_ADDRESS = OTA_BOOT_TRACE_APP_MAIN;

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();
	*(volatile uint32_t *)OTA_BOOT_TRACE_ADDRESS = OTA_BOOT_TRACE_HAL_READY;

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();
	*(volatile uint32_t *)OTA_BOOT_TRACE_ADDRESS = OTA_BOOT_TRACE_CLOCK_READY;

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_ADC_Init();
  MX_USART1_UART_Init();
  MX_USART2_UART_Init();
	*(volatile uint32_t *)OTA_BOOT_TRACE_ADDRESS = OTA_BOOT_TRACE_PERIPH_READY;
  /* USER CODE BEGIN 2 */
  
/* 先恢复业务参数，再初始化采集、显示和串口；试运行镜像主动请求蓝牙配对。 */
	SysDataInit();
	*(volatile uint32_t *)OTA_BOOT_TRACE_ADDRESS = OTA_BOOT_TRACE_DATA_READY;
	if (OtaBoot_IsTrial() != 0u) { UI_RequestBluetoothPairing(); }
	Temp_Get_Init();
	Screen_C8721_Init();
	*(volatile uint32_t *)OTA_BOOT_TRACE_ADDRESS = OTA_BOOT_TRACE_SCREEN_READY;
  com_init();
  Wireless_Init();
	
/* 任务参数基于 1 ms SysTick；周期任务到期置待执行计数，主循环串行运行，详见调度器。 */
 SCH_Add_Task(Key_Scan, 0, 10);
 SCH_Add_Task(DisplayTask, 0 , 10); 
 SCH_Add_Task(TempGetTask, 0 , 500);
 SCH_Add_Task(MainControl, 0 , 100);
 SCH_Add_Task(WirelessTask, 0, 20);
/* 延后约 10 秒单次确认试运行镜像；Period=0 表示执行后删除。 */
 SCH_Add_Task(OtaBoot_ConfirmRunningImage, 10000, 0);
 
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
	*(volatile uint32_t *)OTA_BOOT_TRACE_ADDRESS = OTA_BOOT_TRACE_LOOP_RUNNING;
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
	 SCH_Dispatch_Tasks();
		
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  uint32_t timeout;

  /* Fixed clock tree: HSI/2 * 12 = 48 MHz, AHB/APB1 undivided. Keeping this
     board-specific path local avoids linking the generic RCC configuration
     state machines into every application image. */
  RCC->CR |= RCC_CR_HSION;
  timeout = 0x10000u;
  while (((RCC->CR & RCC_CR_HSIRDY) == 0u) && (--timeout != 0u)) {}
  if (timeout == 0u) { Error_Handler(); }

  /* A bootloader may enter the application while PLL is already SYSCLK. */
  RCC->CFGR &= ~RCC_CFGR_SW;
  timeout = 0x10000u;
  while (((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_HSI) && (--timeout != 0u)) {}
  if (timeout == 0u) { Error_Handler(); }

  RCC->CR2 = (RCC->CR2 & ~RCC_CR2_HSI14TRIM) |
             (16u << RCC_CR2_HSI14TRIM_Pos) | RCC_CR2_HSI14ON;
  timeout = 0x10000u;
  while (((RCC->CR2 & RCC_CR2_HSI14RDY) == 0u) && (--timeout != 0u)) {}
  if (timeout == 0u) { Error_Handler(); }

  RCC->CR &= ~RCC_CR_PLLON;
  timeout = 0x10000u;
  while (((RCC->CR & RCC_CR_PLLRDY) != 0u) && (--timeout != 0u)) {}
  if (timeout == 0u) { Error_Handler(); }

  RCC->CFGR = (RCC->CFGR & ~(RCC_CFGR_SW | RCC_CFGR_HPRE |
                RCC_CFGR_PPRE | RCC_CFGR_PLLSRC | RCC_CFGR_PLLMUL)) |
              RCC_CFGR_PLLSRC_HSI_DIV2 | RCC_CFGR_PLLMUL12;
  RCC->CFGR2 &= ~RCC_CFGR2_PREDIV;
  FLASH->ACR = (FLASH->ACR & ~FLASH_ACR_LATENCY) | FLASH_ACR_LATENCY;

  RCC->CR |= RCC_CR_PLLON;
  timeout = 0x10000u;
  while (((RCC->CR & RCC_CR_PLLRDY) == 0u) && (--timeout != 0u)) {}
  if (timeout == 0u) { Error_Handler(); }

  RCC->CFGR = (RCC->CFGR & ~RCC_CFGR_SW) | RCC_CFGR_SW_PLL;
  timeout = 0x10000u;
  while (((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_PLL) && (--timeout != 0u)) {}
  if (timeout == 0u) { Error_Handler(); }

  RCC->CFGR3 &= ~RCC_CFGR3_USART1SW;
  SystemCoreClock = 48000000u;
  if (HAL_InitTick(TICK_INT_PRIORITY) != HAL_OK) { Error_Handler(); }
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
  *(volatile uint32_t *)OTA_BOOT_TRACE_ADDRESS = OTA_BOOT_TRACE_APP_ERROR;
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
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
