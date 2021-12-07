/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2021 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define RX_BUFF_SIZE 32
#define TX_BUFF_SIZE 32
#define CMD_SIZE 32

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
	static uint32_t x1hzTime = 0;
	static uint32_t x4hzTime = 0;
	static uint8_t x1hz = 0;
	static uint8_t x4hz = 0;
	uint16_t buttonMode = 0;

	/* ===== BUFOR RX ===== */

	uint8_t Rx_buff[RX_BUFF_SIZE];
	uint8_t volatile Rx_empty = 0;
	uint8_t volatile Rx_busy = 0;

	/* ===== BUFOR TX ===== */

	uint8_t Tx_buff[TX_BUFF_SIZE];
	uint8_t volatile Tx_empty = 0;
	uint8_t volatile Tx_busy = 0;

	/* ==== COMMAND === */
	char command[CMD_SIZE];
	uint8_t volatile Cmd_busy = 0;



	/* ==== sterowanie diodą ==== */
	uint8_t volatile blink_mode = 0;
	uint32_t volatile pTimeOn;
	uint32_t volatile pTimeOff;
	uint32_t volatile pCount;

	uint16_t volatile time_on = 500;
	uint16_t volatile time_off = 0;
	uint16_t volatile blink_count = 0;
	extern uint16_t led_delay;

	/* ===== timery dioda ==== */

	uint32_t volatile pBlinkTime;
	uint16_t volatile blink_time;
	uint32_t volatile timer_value = 0;


	/* === delay timer logic ==== */
	uint8_t volatile delayFlag = 0;
	uint16_t volatile delayTime = 0;
	uint32_t volatile delayTimer_value = 0;


/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
void delay_1hz(){
	x1hzTime++;
	if( x1hzTime >= 1000){
		x1hz = 1;
		x1hzTime = 0;
	}
}

void delay_4hz(){
	x4hzTime++;

	if(x4hzTime >= 250){
		x4hz = 1;
		x4hzTime = 0;
	}

}

/* TIMER DELAYS */
void delayUs(uint16_t us){
	__HAL_TIM_SET_COUNTER(&htim7, 0);
	while(__HAL_TIM_GET_COUNTER(&htim6) < us);
}

void delayMs(uint16_t ms){
	for(uint16_t i = 0; i < ms; i++){
		delayUs(1000); //1ms delay
	}
}


/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */


/* ===== COMMAND HANDLERS ===== */

void assignBlinkParamsCommand(){
	time_on = pTimeOn;
	time_off = pTimeOff + pTimeOn;
	blink_count = pCount;
	blink_mode = 1;



}
void handleBlinkCommand(){
	if(led_delay <= time_on){
		HAL_GPIO_WritePin(BRO_GPIO_Port, BRO_Pin, GPIO_PIN_SET);

	} else if(led_delay <= time_off){
		HAL_GPIO_WritePin(BRO_GPIO_Port, BRO_Pin, GPIO_PIN_RESET);

	} else {
		led_delay = 0;
		blink_count -= 1;
		if(blink_count <= 0){
			blink_mode = 0;
		}
	}

}

/* ===== SEND USART ==== */

void Send(char* message, ...){
	char temp[64];

	volatile int idx = Tx_empty;


	va_list arglist;
	va_start(arglist, message);
	vsprintf(temp, message, arglist);
	va_end(arglist);

	for(int i = 0; i < strlen(temp); i++){
		Tx_buff[idx] = temp[i];
		idx++;
		if(idx >= TX_BUFF_SIZE){
			idx = 0;
		}

	}
	__disable_irq();

	if(Tx_empty == Tx_busy){
		Tx_empty = idx;
		uint8_t tmp = Tx_buff[Tx_busy];
		Tx_busy++;
		if(Tx_busy >= TX_BUFF_SIZE){
			Tx_busy = 0;
		}

		HAL_UART_Transmit_IT(&huart2, &tmp, 1);

	} else {
		Tx_empty = idx;
	}
	__enable_irq();
}


/* send usart callback */
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart){
	if(Tx_busy != Tx_empty){

		uint8_t temp = Tx_buff[Tx_busy];
		Tx_busy++;

		if(Tx_busy >= TX_BUFF_SIZE){
			Tx_busy = 0;
		}
		HAL_UART_Transmit_IT(&huart2, &temp, 1);
	}
}



/* ===== receive usart callback ===== */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart){
	if(huart->Instance == USART2){
		Rx_empty++;
		if(Rx_empty >= RX_BUFF_SIZE){
			Rx_empty = 0;
		}
		HAL_UART_Receive_IT(&huart2, &Rx_buff[Rx_empty], 1);


	}
}


/* ===== check if data stopped to be received ===== */

uint8_t isRxBuffEmpty(){
	if(Rx_empty != Rx_busy){
		return 1;
	}else {
		return 0;
	}
}



/* ===== command parser ===== */
void parseCmd(){

		if(strcmp("LED[ON]", command) == 0){
				HAL_GPIO_WritePin(BRO_GPIO_Port, BRO_Pin, GPIO_PIN_SET);

			}
			else if(strcmp("LED[OFF]", command) == 0){
				HAL_GPIO_WritePin(BRO_GPIO_Port, BRO_Pin, GPIO_PIN_RESET);
			} else if(sscanf(command, "LED[BLINK,%d,%d,%d]", &pTimeOn, &pTimeOff, &pCount) == 3){
				assignBlinkParamsCommand();

			} else if(sscanf(command, "LED[BLINK,%d]", &pBlinkTime) == 1){
				blink_mode = 2;
				blink_time = pBlinkTime;

			} else if(sscanf(command, "LED[Delay,%d]", &delayTime) == 1){
				delayFlag = 1;
			}
			else {
				Send("Nieprawidłowa komenda \n\r");

			}







	Cmd_busy = 0;



}



void downloadCmd(){

	char temp = Rx_buff[Rx_busy];

	if(temp == 0x3B /* ; */){
		command[Cmd_busy] = 0x00;
		parseCmd();

	} else {
		command[Cmd_busy] = temp;
	}

	/* check cmd length */
	if(temp != 0x3B){
		Cmd_busy++;
		if(Cmd_busy >= CMD_SIZE){
			Rx_busy = Rx_empty;
			Cmd_busy = 0;

		}

	}

	Rx_busy++;
	if(Rx_busy >= RX_BUFF_SIZE){
		Rx_busy = 0;
	}



}






void buttonHandler() {

	 if(!HAL_GPIO_ReadPin(B1_GPIO_Port, B1_Pin)){

			  buttonMode = !buttonMode;

			  x1hz = 0;
			  x4hz = 0;

			  HAL_Delay(400);


		  }

		  if(buttonMode == 1){
			  if(x1hz == 1){
				  HAL_GPIO_TogglePin(BIA_GPIO_Port, BIA_Pin);
				  x1hz = 0;
			  }


		  }
		  else {
			  if(x4hz == 1){
				  HAL_GPIO_TogglePin(BIA_GPIO_Port, BIA_Pin);
				  x4hz = 0;
			  }
		  }

}

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
  MX_USART2_UART_Init();
  MX_TIM6_Init();
  MX_TIM7_Init();
  /* USER CODE BEGIN 2 */


  HAL_UART_Receive_IT(&huart2, &Rx_buff[Rx_empty], 1);

  Send("Hello, im STM32!\r\n");

  /* === TIMER INIT ===== */
  HAL_TIM_Base_Start(&htim6);

  timer_value = __HAL_TIM_GET_COUNTER(&htim6);


  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {

	 buttonHandler();


	 while(isRxBuffEmpty()){
		 downloadCmd();
	 }

	 /* ==== LED BLINKING WITH TIMER AND NORMAL === */
	 if(blink_mode == 1){
		 handleBlinkCommand();
	 } else if(blink_mode == 2){
		 if(__HAL_TIM_GET_COUNTER(&htim6) - timer_value >= blink_time){
			 HAL_GPIO_TogglePin(BRO_GPIO_Port, BRO_Pin);
			 timer_value = __HAL_TIM_GET_COUNTER(&htim6);
		 }
	 }

	 /* ==== delay settings ==== */

	 if(delayFlag == 1){


		 delayMs(delayTime);


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
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  RCC_OscInitStruct.PLL.PREDIV = RCC_PREDIV_DIV1;
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
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USART2;
  PeriphClkInit.Usart2ClockSelection = RCC_USART2CLKSOURCE_PCLK1;
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

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
