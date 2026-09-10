/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    usart.h
  * @brief   This file contains all the function prototypes for
  *          the usart.c file
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
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __USART_H__
#define __USART_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* USER CODE BEGIN Includes */
#include "config.h"
/* USER CODE END Includes */

extern UART_HandleTypeDef huart1;

extern UART_HandleTypeDef huart2;

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

void MX_USART1_UART_Init(void);
void MX_USART2_UART_Init(void);

/* USER CODE BEGIN Prototypes */
#define COM_RXSIZE  256


/* DMA buffer length definitions */
#define U1_DMA_T_LEN      20     /* UART1 DMA transmit buffer length */
#define U1_DMA_R_LEN      20     /* UART1 DMA receive buffer length */
#define COM_RX1_Lenth     12     /* COM1 receive length */
#define TimeOutSet1       10     /* Timeout setting (ms) */
#define COM_RX2_Lenth     20     /* COM2 receive length */

/**
  * @brief  Communication structure for UART DMA reception
  * @details Stores receive buffer, length, and status flag for each UART
  */
typedef struct
{
	uint8_t  dmaBuf[COM_RXSIZE]; /* Buffer currently owned by RX DMA */
	uint8_t  rxBuf[COM_RXSIZE];  /* Stable snapshot consumed by a task */
	volatile uint16_t rxLen;
	volatile uint8_t  rxFlag;
	volatile uint32_t overrunCount;
} TypeDefCOM;

/* External communication structures for each UART */
extern TypeDefCOM COM1, COM2;

/* External single-byte data buffers */
extern uint8_t U1_Data[2];  /* Single byte data buffer for USART1 */
extern uint8_t U2_Data[2];  /* Single byte data buffer for USART5 */
extern volatile uint8_t usart2_tx_busy;
extern DMA_HandleTypeDef hdma_usart1_tx;
extern DMA_HandleTypeDef hdma_usart1_rx;
extern DMA_HandleTypeDef hdma_usart2_rx;
extern DMA_HandleTypeDef hdma_usart2_tx;
void com_init(void);
void _usart_callback(UART_HandleTypeDef *huart, DMA_HandleTypeDef *hdma_uart, TypeDefCOM *com);
void uart_dma_poll_check(UART_HandleTypeDef *huart, DMA_HandleTypeDef *hdma_uart, TypeDefCOM *com);
uint16_t COM_TakeRx(TypeDefCOM *com, uint8_t *dest, uint16_t capacity);
void USART1_SendData(uint8_t *data, uint16_t num);
void USART2_SendData(uint8_t *data, uint16_t num);
HAL_StatusTypeDef USART2_SendData_DMA(uint8_t *data, uint16_t num);

/* USER CODE END Prototypes */

#ifdef __cplusplus
}
#endif

#endif /* __USART_H__ */

