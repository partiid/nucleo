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
#include "i2c.h"
#include "rtc.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>


/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define RX_BUFF_SIZE 512
#define TX_BUFF_SIZE 512
#define FRAME_SIZE 256


/* ===== frame ===== */
#define start_sign 0x24 /* $ */
#define end_sign  0x23 /* # */
#define command_size 128
#define data_size 125








/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */


	static uint16_t x1hzTime = 0;
	static uint16_t x4hzTime = 0;
	static uint8_t x1hz = 0;
	static uint8_t x4hz = 0;
	uint16_t buttonMode = 0;

	/* ============================ USART ====================== */

	/* ===== BUFOR RX ===== */

	uint8_t Rx_buff[RX_BUFF_SIZE];
	uint16_t volatile Rx_empty = 0; //tail //read
	uint16_t volatile Rx_busy = 0; //head //write

	/* ===== BUFOR TX ===== */

	uint8_t Tx_buff[TX_BUFF_SIZE];
	uint16_t volatile Tx_empty = 0;
	uint16_t volatile Tx_busy = 0;

	/* ============================  END USART  ====================== */


	/* ==== FRAME === */

	char frame[FRAME_SIZE];
	uint16_t volatile Frame_busy = 0;
	uint16_t volatile frameLength = 0;
	uint8_t frame_found = 0;

	/* ===== CORE COMMANDS SYSTEM ==== */

	char command[128]; //command
	uint8_t volatile command_busy = 0;

	char data[125]; //data from command
	uint8_t volatile data_busy = 0;

	/* ================ CLOCK LOGIC ================  */
	uint8_t hour_displayed = 0;

	RTC_HandleTypeDef hrtc;
	RTC_AlarmTypeDef sAlarm;

	static RTC_TimeTypeDef sTime;
	static RTC_DateTypeDef sDate;


	uint8_t volatile clock_mode = 1; //Sets the clock mode  1 -  CLOCK MODE ////// 2  - HOUR DISPLAY MODE
	uint8_t volatile hour_to_show = 0; //hour to show on diodes
	uint8_t volatile minute_to_show = 0;
	uint8_t volatile second_to_show = 0;


	/* ============= alarm logic ========== */
	uint8_t volatile day_to_set = 0; //set day to set the alarm
	uint8_t volatile month_to_set = 0;
	uint16_t volatile year_to_set  = 0;

	//poszczegolne zmienne alarmow
	uint16_t volatile hours[16];
	uint16_t volatile months[16];
	uint16_t volatile days[16];
	uint16_t volatile minutes[16];
	uint16_t volatile seconds[16];

	//set hours to set the alarm

	uint8_t volatile hour_to_set = 0;
	uint8_t volatile minute_to_set = 0;
	uint8_t volatile second_to_set = 0;





	/* ==== sterowanie diodą ==== */
	uint8_t volatile blink_mode = 0;
	uint16_t volatile pTimeOn;
	uint16_t volatile pTimeOff;
	uint16_t volatile pCount;

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


/* ===== blink HANDLERS ===== */

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
/* ===== init usart ==== */
void UART_init(){
	Rx_empty = 0;
	Rx_busy = 0;
	Tx_empty = 0;
	Tx_busy = 0;
	memset(Rx_buff, 0, RX_BUFF_SIZE);
	memset(Tx_buff, 0, TX_BUFF_SIZE);
}


/* ===== SEND USART ==== */

/*send poprawiony - dodano flage */



void Send(char* message, ...){
	char temp[256];

	volatile int idx = Tx_empty;
	int i;

	va_list arglist;
	va_start(arglist, message);

	vsprintf(temp, message, arglist);

	va_end(arglist);

	for(i = 0; i < strlen(temp); i++){
		Tx_buff[idx] = temp[i];
		idx++;
		if(idx >= TX_BUFF_SIZE){
			idx = 0;
		}

	}
	__disable_irq();

	if((Tx_empty == Tx_busy) && (__HAL_UART_GET_FLAG(&huart2, UART_FLAG_TXE == SET))){
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

/* ===== check if data stopped being received ===== */

uint8_t uart_ready(){
	if(Rx_empty == Rx_busy){
		return 0;
	} else {
		return 1;
	}
}

/* ====== END USART ==== */

/* ======== RTC =========== */



void HAL_RTCEx_WakeUpTimerEventCallback(RTC_HandleTypeDef *hrtc){

	/* place for checking if the next alarm is set */


	//przy dodawaniu alarmu i przy przerwaniu alarmu sprawdzac czy jest alarm, który jest świeższy

	// zapisywanie alarmu do flasha

	//2. sprawdzenie czy alarm jest na wczesniejsza date od tego nowego
	//3. jeśli jest wczesniejszy -> ustawienie tego alarmu
	//4. przy callbacku alarmu pobrac alarmy z flasha i sprawdzic kolejny i ustawić go


	HAL_RTC_GetTime(hrtc, &sTime, RTC_FORMAT_BIN);
	HAL_RTC_GetDate(hrtc, &sDate, RTC_FORMAT_BIN);

	//Send("%2.2u:%2.2u:%2.2u\n\r", sTime.Hours, sTime.Minutes, sTime.Seconds);
	//HAL_GPIO_TogglePin(SEC_GPIO_Port, SEC_Pin);

	 uint8_t hours = sTime.Hours;
	 uint8_t minutes = sTime.Minutes;
	 uint8_t seconds = sTime.Seconds;


	 if(clock_mode == 1){
		 displayHour(hours, minutes, seconds);
	 }





}







/* ===== PARSERS ===== */

uint8_t test2 = 0;
void parseCommand(){
	uint8_t picked_command = 0;


	//porównywać znak po znaku każdą komendę?
	//todo

	if(strcmp("setMode", command) == 0){

		handleSetClockMode();


	} else if (strcmp("setTime", command) == 0){
		parseTime();
		resetPins();

		setTime(sTime, hour_to_show, minute_to_show, second_to_show);


	} else if (strcmp("getTime", command) == 0) {
		Send("$Success=%2.2u:%2.2u:%2.2u#\r\n", sTime.Hours, sTime.Minutes, sTime.Seconds);
		Send("$Success=%2.2u/%2.2u/%2.2u#\r\n", sDate.Date, sDate.Month, sDate.Year);


	}else if(strcmp("setAlarm", command) == 0){
//		uint8_t test = 0x5a;
//		HAL_I2C_Mem_Write(&hi2c1, 0xa0, 0x10, 1, (uint8_t*)&test, sizeof(test), HAL_MAX_DELAY);

		parseDateTime();
		handleSetAlarm(sDate, day_to_set, month_to_set, year_to_set, hour_to_set, minute_to_set, second_to_set);


	} else if(strcmp("getAlarms", command) == 0){
		handleGetAlarms();


	} else if (strcmp("getAlarmsCount", command) == 0){



	} else if (strcmp("resetAlarms", command) == 0){
		handleResetAlarms();
		clearAlarms();


	} else if (strcmp("showHour", command) == 0){

		parseTime();

		handleShowHour(hour_to_show, minute_to_show, second_to_show);

	}
	else {
		sendFail(1);
	}

	//handle picked command

	clearCommand();
	clearData();
	Frame_busy = 0;

	/*	if(strcmp("$LED[ON]#", frame) == 0){
				HAL_GPIO_WritePin(BRO_GPIO_Port, BRO_Pin, GPIO_PIN_SET);
			}
			else if(strcmp("$LED[OFF]#", frame) == 0){
				HAL_GPIO_WritePin(BRO_GPIO_Port, BRO_Pin, GPIO_PIN_RESET);
			} else if(sscanf(frame, "$LED[BLINK,%d,%d,%d]#", &pTimeOn, &pTimeOff, &pCount) == 3){
				assignBlinkParamsCommand();

			} else if(sscanf(frame, "$LED[BLINK,%d]#", &pBlinkTime) == 1){
				blink_mode = 2;
				blink_time = pBlinkTime;

			} else if(sscanf(frame, "$LED[Delay,%d]#", &delayTime) == 1){
				delayFlag = 1;
			}
			else {
				Send("Nieprawidłowa komenda \n\r");

			} */


}
//data parser
void parseData(){


}


//parse only time in format HH:MM:SS
void parseTime(){
	hour_to_show = 0;
	minute_to_show = 0;
	second_to_show = 0;

	 if(sscanf(data, "%d:%d:%d", &hour_to_show, &minute_to_show, &second_to_show) == 3){

		 Send("$Success=1#\r\n");

	 } else {

		 sendFail(2);
	 }
}

//parse date and time at the same time

void parseDateTime(){

	if(sscanf(data, "%d/%d/%d/%d:%d:%d", &day_to_set, &month_to_set, &year_to_set, &hour_to_set, &minute_to_set, &second_to_set) == 6){


	} else {
		sendFail(2);
	}
}



int parseIntData(){
	int single_param = 0;


	if(sscanf(data, "%d", &single_param) == 1){
		return single_param;
	}
	else {
		sendFail(2);
	}


}
//function to parse alarms downloaded from flash

uint8_t parseAlarms(uint16_t alarms[]){

//					alarm_to_set[0] = day;
//					alarm_to_set[1] = month;
//					alarm_to_set[2] = year;
//					alarm_to_set[3] = hour;
//					alarm_to_set[4] = minute;
//					alarm_to_set[5] = second;

	clearAlarms();


	//arrays with separate values to find the min value


	uint8_t days_idx = 0;


	uint8_t months_idx = 0;


	uint8_t hours_idx = 0;


	uint8_t minutes_idx = 0;


	uint8_t seconds_idx = 0;


	uint8_t alarms_size = 128;

	uint8_t day_start_idx = 0;
	uint8_t month_start_idx = 0;
	uint8_t year_start_idx = 0;
	uint8_t hour_start_idx = 0;
	uint8_t minute_start_idx = 0;
	uint8_t second_start_idx = 0;



	for(int i = 0; i < alarms_size; i++){
		//
		if(i == 0 ){
			day_start_idx = i;

		}
		if(i == 1){
			month_start_idx = i;
		}
		if(i == 3){
			hour_start_idx = i;
		}
		if(i == 4){
			minute_start_idx = i;
		}
		if(i == 5){
			second_start_idx = i;
		}
	}

	//sepearte each value to different array
	//inc by 5 to check only days
	for(int i = day_start_idx; i < alarms_size; i+=6){
		if(alarms[i] != 255){

		days[days_idx++] = alarms[i];
		}
	}

	//check month
	for(int i = month_start_idx; i < alarms_size; i+=6){
		if(alarms[i] != 255){
			months[months_idx++] = alarms[i];
		}

	}

	for(int i = hour_start_idx; i < alarms_size; i+=6){
			if(alarms[i] != 255){
				hours[hours_idx++] = alarms[i];
			}

		}
	for(int i = minute_start_idx; i < alarms_size; i+=6){
				if(alarms[i] != 255){
					minutes[minutes_idx++] = alarms[i];
				}

			}
	for(int i = second_start_idx; i < alarms_size; i+=6){
			if(alarms[i] != 255){
			seconds[seconds_idx++] = alarms[i];
		}

		}
	//parse alarm to get the earliest alarm to set

	uint8_t location_months = 0;
	uint8_t location_days = 0;
	uint8_t location_hours = 0;
	uint8_t location_minutes = 0;
	uint8_t location_seconds = 0;


	uint8_t checker = 0;


		for(int i = 0; i < 16; i++){
			if(months[i] > 0){
				if(months[i] < months[location_months]){
					location_months = i;
				}
			}
			if(days[i] > 0){
				if(days[i] < days[location_days]){
					location_days = i;
				}
			}
			if(hours[i] > 0){
				if(hours[i] < hours[location_hours]){
					location_hours = i;
				}
			}
			if(minutes[i] > 0){
				if(minutes[i] < minutes[location_minutes]){
					location_minutes = i;
				}

			}
			if(seconds[i] > 0){
				if(seconds[i] < seconds[location_seconds]){
					location_seconds = i;
				}
			}

		}


		//get the minimum month - closest alarm
		uint8_t min_day = days[location_days];
		uint8_t min_month = months[location_months];
		uint8_t min_hour = hours[location_hours];
		uint8_t min_minute = minutes[location_minutes];
		uint8_t min_second = seconds[location_seconds];

		//check for same values to distinc the minimum value
		if(months[location_days] == min_month){
			location_months = location_days;
		}

		if(hours[location_minutes] == min_hour){
			location_hours = location_minutes;
		}

		if(minutes[location_seconds] == min_second){
			location_minutes = min_second;
		}

		Send("Location minuets: %d  ", location_minutes);

		return location_minutes;


	//in case


	Send("Minimum month found: %d at location %d", months[location_months], location_months);

	Send("Minimum day found: %d at location %d", days[location_days], location_days);
	//check day and month if they do not overlap


	Send("MInimum alarm: %d %d", min_day, min_month);





}



/* ==== clear after command is executed to receive next command " ==== */

void clearCommand(){
	command_busy = 0;

	memset(command, 0, command_size);

}

void clearData(){
	data_busy = 0;
	memset(data, 0, data_size);
}

void clearAlarms(){
		memset(days, 0, sizeof(days));
		memset(minutes, 0, sizeof(minutes));
		memset(months, 0, sizeof(months));
		memset(seconds, 0, sizeof(seconds));
		memset(hours, 0, sizeof(hours));
}









/* ====FRAME LOGIC ====*/

//decode frame and split dat and command
void decodeFrame() {

		uint8_t data_idx = 0;
		uint8_t command_idx = 0;
		uint8_t command_end_idx = 0;
		uint8_t required_pass = 0; //check if all the required signs are in the frame



	//check if begining exists
	if(frame[0] == start_sign){
		required_pass++;
		frame[0] = 0x00;
		//if char was received, consider it as first sign so the length should be + 1

	}

	//check if end exists
	if(frame[frameLength - 1] == end_sign){
		required_pass++;
		frame[frameLength - 1] = 0x00;
	}

	for(int i = 0; i < frameLength; i++){
		if(frame[i] == '='){
			required_pass++;
			data_idx = i + 1;
			command_end_idx = i - 1;


		}
	}

   //if all required signs are in place, check if command exists
	//===== COMMAND ===== //

	if(required_pass == 3 && (command_end_idx != command_idx)){


		//rewrite command to the command table
		for(int i = 1; i <= command_end_idx; i++){

			//prevent memory leaks
			if(command_busy >= command_size){
				command_busy = 0;
				memset(command, 0, command_size);
				i = 1;
			 }

			 command[command_busy++] = frame[i];

		}

	}

	// ===== DATA ==== //
	//if all required signs are in place check if data exists {

	if(required_pass == 3 && (data_idx != frameLength - 1)){
		//Send("Data exists!\r\n");
		for(int i = data_idx; i <= frameLength - 1; i++){
			//prevent memory leaks
			if(data_busy >= data_size){
				data_busy = 0;
				memset(data, 0, data_size);
				i = data_idx;
			}
			//copy data to the data table
			data[data_busy++] = frame[i];

		}

	}


}

//download frame from data sent
void downloadFrame(){

	char byte = Rx_buff[Rx_busy]; //single frame char

	//control ringbuffer
	Rx_busy++;


	if(Rx_busy >= RX_BUFF_SIZE){
		Rx_busy = 0;
	}
		//if found start of frame char
		if(byte == 0x24 /* $ */ ){
			memset(frame, 0x00, FRAME_SIZE); //reset frame #
			frame_found = 1; //set the flag to continue downloading chars

			Frame_busy = 0;
			frameLength = 1; //set frame length to one cos $ is already in the frame

		} else if(frame_found == 1){ //frame length if more than one start sign is found

			frameLength++;

		}

		//if frame found start downloading frame
		//start downloading the frame
		if(frame_found == 1){

				//copy a frame to analyze it
					frame[Frame_busy++] = byte; //download chars

			}

		//check if frame is not too long
		if(frameLength >= FRAME_SIZE){
			memset(frame, 0x00, FRAME_SIZE);
			Frame_busy = 0;
			frameLength = 0;
			frame_found = 0;
			sendFail(4);
		}


		//if end of frame is reached
		if(byte == 0x23 && frame_found == 1 /* # */ ){
			frame_found = 0; //stop downloading chars
			Frame_busy = 0; //reset frame

		  //if frame is received, analyze it
			decodeFrame();
			parseCommand();

			//reset framelength to zero

			frameLength = 0;

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

/* send functions
 *
 *
 */
void sendFail(uint8_t code){

	Send("$Fail=%d#", code);

}



/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */
	UART_init();
	FLASH_init();
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

  MX_I2C1_Init();
  /* USER CODE BEGIN 2 */
  if (HAL_RTCEx_BKUPRead(&hrtc, RTC_BKP_DR1) != 0x32F2){
	  MX_RTC_Init();
  }
  Alarms_init();

  /* ===== RTC set time ==== */
  RTC_TimeTypeDef sTime;
  RTC_DateTypeDef sDate;





  //set time
  HAL_RTCEx_DeactivateWakeUpTimer(&hrtc);

  HAL_RTCEx_SetWakeUpTimer_IT(&hrtc, 2500 - 1, RTC_WAKEUPCLOCK_RTCCLK_DIV16);


  HAL_UART_Receive_IT(&huart2, &Rx_buff[Rx_empty], 1);


  Send("$Success=Hello, im STM32!#\r\n");

  //uint32_t data2 = {0x3, 0x13, 0x12};
  //flash_write(0x08004410, (uint32_t*)data2, 3);




  /* === TIMER INIT ===== */
  HAL_TIM_Base_Start(&htim6);

  timer_value = __HAL_TIM_GET_COUNTER(&htim6);

//  uint8_t test = 0x5a;
//  HAL_I2C_Mem_Write(&hi2c1, 0xa0, 0x10, 1, (uint8_t*)&test, sizeof(test), HAL_MAX_DELAY);
//
//
//  uint8_t data1;
//
//  HAL_I2C_Mem_Read(&hi2c1, 0xa0, 0x00, 1, &data1, 1, HAL_MAX_DELAY);





  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {


	  //send time
	  //HAL_RTC_GetTime(&hrtc, &sTime, RTC_FORMAT_BIN);

	  //Send("%2.2u:%2.2u:%2.2u\n\r", sTime.Hours, sTime.Minutes, sTime.Seconds);


	  //display hour if clock mode is set to display hour

	  if(clock_mode == 2 && hour_displayed == 0 && hour_to_show != 0){
		  displayHour(hour_to_show, minute_to_show, second_to_show);
	   }


	 buttonHandler();


	 while(uart_ready()){

		 downloadFrame();
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
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI|RCC_OSCILLATORTYPE_LSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.LSIState = RCC_LSI_ON;
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
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USART2|RCC_PERIPHCLK_I2C1
                              |RCC_PERIPHCLK_RTC;
  PeriphClkInit.Usart2ClockSelection = RCC_USART2CLKSOURCE_PCLK1;
  PeriphClkInit.I2c1ClockSelection = RCC_I2C1CLKSOURCE_HSI;
  PeriphClkInit.RTCClockSelection = RCC_RTCCLKSOURCE_LSI;
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
