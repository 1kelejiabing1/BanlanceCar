/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
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
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f1xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define LED_Pin GPIO_PIN_13
#define LED_GPIO_Port GPIOC
#define LEFT_MOTOR_PWM_Pin GPIO_PIN_0
#define LEFT_MOTOR_PWM_GPIO_Port GPIOA
#define RIGHT_MOTOR_PWM_Pin GPIO_PIN_1
#define RIGHT_MOTOR_PWM_GPIO_Port GPIOA
#define BLE_TX_Pin GPIO_PIN_2
#define BLE_TX_GPIO_Port GPIOA
#define BLE_RX_Pin GPIO_PIN_3
#define BLE_RX_GPIO_Port GPIOA
#define K4_Pin GPIO_PIN_4
#define K4_GPIO_Port GPIOA
#define K3_Pin GPIO_PIN_5
#define K3_GPIO_Port GPIOA
#define LEFT_ENCODER_B_Pin GPIO_PIN_6
#define LEFT_ENCODER_B_GPIO_Port GPIOA
#define LEFT_ENCODER_A_Pin GPIO_PIN_7
#define LEFT_ENCODER_A_GPIO_Port GPIOA
#define K2_Pin GPIO_PIN_0
#define K2_GPIO_Port GPIOB
#define K1_Pin GPIO_PIN_1
#define K1_GPIO_Port GPIOB
#define MPU6500_SCL_Pin GPIO_PIN_10
#define MPU6500_SCL_GPIO_Port GPIOB
#define MPU6500_SDA_Pin GPIO_PIN_11
#define MPU6500_SDA_GPIO_Port GPIOB
#define LEFT_MOTOR_AIN_1_Pin GPIO_PIN_12
#define LEFT_MOTOR_AIN_1_GPIO_Port GPIOB
#define LEFT_MOTOR_AIN_2_Pin GPIO_PIN_13
#define LEFT_MOTOR_AIN_2_GPIO_Port GPIOB
#define RIGHT_MOTOR_BIN_1_Pin GPIO_PIN_14
#define RIGHT_MOTOR_BIN_1_GPIO_Port GPIOB
#define RIGHT_MOTOR_BIN_2_Pin GPIO_PIN_15
#define RIGHT_MOTOR_BIN_2_GPIO_Port GPIOB
#define NRF24L01_CE_Pin GPIO_PIN_8
#define NRF24L01_CE_GPIO_Port GPIOA
#define DEBUG_TX_Pin GPIO_PIN_9
#define DEBUG_TX_GPIO_Port GPIOA
#define DEBUG_RX_Pin GPIO_PIN_10
#define DEBUG_RX_GPIO_Port GPIOA
#define NRF24L01_CSN_Pin GPIO_PIN_15
#define NRF24L01_CSN_GPIO_Port GPIOA
#define NRF24L01_SCK_Pin GPIO_PIN_3
#define NRF24L01_SCK_GPIO_Port GPIOB
#define NRF24L01_MISO_Pin GPIO_PIN_4
#define NRF24L01_MISO_GPIO_Port GPIOB
#define NRF24L01_MOSI_Pin GPIO_PIN_5
#define NRF24L01_MOSI_GPIO_Port GPIOB
#define RIGHT_ENCODER_A_Pin GPIO_PIN_6
#define RIGHT_ENCODER_A_GPIO_Port GPIOB
#define RIGHT_ENCODER_B_Pin GPIO_PIN_7
#define RIGHT_ENCODER_B_GPIO_Port GPIOB
#define OLED_SCL_Pin GPIO_PIN_8
#define OLED_SCL_GPIO_Port GPIOB
#define OLED_SDA_Pin GPIO_PIN_9
#define OLED_SDA_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
