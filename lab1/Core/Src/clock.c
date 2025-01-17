/*
 * clock.c
 *
 *  Created on: Jan 2, 2022
 *      Author: NASA
 */

#include "clock.h"



#include <stdio.h>
#include <time.h>



extern RTC_HandleTypeDef hrtc;
extern RTC_DateTypeDef sDate;
extern uint8_t hour_displayed;



static GPIO_TypeDef* port_hour[5] = {{CZE_GPIO_Port}, {ZOL_GPIO_Port}, {ZIE_GPIO_Port}, {NIE_GPIO_Port}, {FIO_GPIO_Port} };
static uint16_t  pin_hour[5] = {{CZE_Pin}, {ZOL_Pin}, {ZIE_Pin}, {NIE_Pin}, {FIO_Pin}};

//MINUTES - tens 5 leds row
static GPIO_TypeDef* port_minute_ones[4] = {{ZOLM_GPIO_Port}, {NIEM_GPIO_Port}, {ZIEM_GPIO_Port}, {FIOM_GPIO_Port}, /* {BIAM_GPIO_Port} */ };
static uint16_t pin_minute_ones[4] = {{ZOLM_Pin}, {NIEM_Pin}, {ZIEM_Pin}, {FIOM_Pin}, /* {BIAM_Pin} */ };

//minutes - ones 3 led row

static GPIO_TypeDef* port_minute_tens[3] = {{CZEMO_GPIO_Port}, {POMMO_GPIO_Port}, {BROMO_GPIO_Port}};
static uint16_t pin_minute_tens[3] = {{CZEMO_Pin}, {POMMO_Pin}, {BROMO_Pin} };


//seconds tens - 3 led row

static GPIO_TypeDef* port_second_tens[3] = {{CZASO_GPIO_Port}, {BIASO_GPIO_Port},  {SZASO_GPIO_Port}};
static uint16_t pin_second_tens[3] = { {CZASO_Pin},{BIASO_Pin}, {SZASO_Pin}};

//seconds
static GPIO_TypeDef* port_second_ones[4] = {{ZOLSO_GPIO_Port}, {ZIESO_GPIO_Port}, {NIESO_GPIO_Port}, {FIOSO_GPIO_Port}};
static uint16_t pin_second_ones[4] = {{ZOLSO_Pin}, {ZIESO_Pin}, {NIESO_Pin}, {FIOSO_Pin}, };


//have all the time
struct Time {
	int hour;
	int minutes;
	int seconds;
};



int splitNumber(int num, int return_val){
	      int arr[3];
          int i =0;
         if(num != 0 || num != 00){
             while(num > 0 ){
              arr[i++] = num % 10;
	          num = num / 10;
              if(num == 0){
                  arr[i++] = 0;
              }
	            printf("%d", num);


            }
        return arr[return_val];
         } else {
             return 0;
         }


}

int convertToBinary(int num){
          int count = 0;
          int arr[8];
          int i = 0;
          while(num != 0){
              arr[i] = num % 2;
              num = num / 2;
              i++;
          }
          for(int j = i - 1; j >= 0; j--){
              count++;

          }
          return count;
}



void setTime(RTC_TimeTypeDef sTime, int8_t hours, uint8_t minutes, uint8_t seconds){

	HAL_RTCEx_DeactivateWakeUpTimer(&hrtc);


	sTime.Hours = hours;
	sTime.Minutes = minutes;
	sTime.Seconds = seconds;



	HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BIN);
	if (HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BIN) != HAL_OK)
	{
		Error_Handler();
	}

//	sDate.WeekDay = RTC_WEEKDAY_SATURDAY;
//	sDate.Date = 13;
//	sDate.Month = 2;
//	sDate.Year = 22;

	//HAL_RTC_SetDate(&hrtc, &sDate, RTC_FORMAT_BIN);
	//handle errors


//	  if (HAL_RTC_SetDate(&hrtc, &sDate, RTC_FORMAT_BIN) != HAL_OK)
//	  {
//	    Error_Handler();
//	  }

	HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR1, 0x32F2);

	HAL_RTCEx_SetWakeUpTimer_IT(&hrtc, 2048 - 1, RTC_WAKEUPCLOCK_RTCCLK_DIV16);



}


//display hour if clock is set to second mode

void displayHour(uint8_t hour, uint8_t minute, uint8_t second){
	resetPins();

	int bits_count = convertToBinary(hour);

	if(bits_count > 5 ){
		Send("Fail: {Data Not acceptable}\r\n");
	} else {
		//display hour
		for(int i = 0; i <= 5; i++){

				if(hour & (1 << i)){ //if bit is set

					HAL_GPIO_WritePin(port_hour[i], pin_hour[i], 1);

				}
			};


		//display minutes

			//dziesiętne od minut
				for(int i = 0; i <= 4; i++){
					if(splitNumber(minute, 0) & (1 << i)){
							HAL_GPIO_WritePin(port_minute_ones[i], pin_minute_ones[i], 1);
						}
				}

				for(int i = 0; i <= 3; i++){
					 if(splitNumber(minute, 1) & (1 << i)){


						HAL_GPIO_WritePin(port_minute_tens[i], pin_minute_tens[i], 1);
					}
				}



		//seconds

			//Send("Second: %d\r\n", second);
			for (int i = 0; i <= 4; i++){
					    if (splitNumber(second, 0) & (1 << i)){
					        HAL_GPIO_WritePin(port_second_ones[i], pin_second_ones[i], 1);
					    }
					}

					for (int i = 0; i <= 3; i++){

					    if (splitNumber(second, 1) & (1 << i)){

					        HAL_GPIO_WritePin(port_second_tens[i], pin_second_tens[i], 1);
					    }
					}






		hour_displayed = 1;

	}


}

void resetPins(){
	//reset hours pins

	for(int i = 0; i < 5; i++){
		HAL_GPIO_WritePin(port_hour[i], pin_hour[i], GPIO_PIN_RESET);
	}

	for(int i = 0; i < 3; i++){
		HAL_GPIO_WritePin(port_minute_tens[i], pin_minute_tens[i], GPIO_PIN_RESET);
		HAL_GPIO_WritePin(port_second_tens[i], pin_second_tens[i], GPIO_PIN_RESET);
	}
	for(int i = 0; i < 4; i++){
		HAL_GPIO_WritePin(port_minute_ones[i], pin_minute_ones[i], GPIO_PIN_RESET);
		HAL_GPIO_WritePin(port_second_ones[i], pin_second_ones[i], GPIO_PIN_RESET);
	}



}





uint8_t getNumOfDaysInMonth(uint8_t N){



	    // Check for 31 Days
	    if (N == 1 || N == 3 || N == 5
	        || N == 7 || N == 8 || N == 10
	        || N == 12) {
	        return 31;
	    }

	    // Check for 30 Days
	    else if (N == 4 || N == 6
	             || N == 9 || N == 11) {
	        return 30;
	    }

	    // Check for 28/29 Days
	    else if (N == 2) {
	        return 28;
	    }

	    // Else Invalid Input
	    else {
	        return 0;
	    }

}






