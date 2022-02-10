/*
 * flash.c
 *
 *  Created on: 6 sty 2022
 *      Author: NASA
 */
//read from flash memory
//sectors are
#include "main.h"
#define COUNTER_ADDR 0x01
#define FLASHTX_BUFF_SIZE 128

int counter = 0x00; //counter of the current top address
extern I2C_HandleTypeDef hi2c1;

uint16_t FlashTx_buff[128];


uint8_t FlashTx_empty = 0; //tail //read
uint8_t FlashTx_busy = 0; //head //write


/* ===== FLASH INIT ===== */


void FLASH_init(){


	FlashTx_empty = 0;
	FlashTx_busy = 0;
	memset(FlashTx_buff, 0, FLASHTX_BUFF_SIZE);
}



void HAL_I2C_MemTxCpltCallback(I2C_HandleTypeDef *hi2c) {

//	if(FlashTx_busy != FlashTx_empty){
//		++counter;
//			uint8_t temp = FlashTx_buff[FlashTx_empty];
//
//			FlashTx_empty++;
//			Send("Temp: %d", temp);
//			Send("COunter at: %d\r\n", counter);
//			if(FlashTx_empty >= FLASHTX_BUFF_SIZE){
//				FlashTx_empty = 0;
//			}
// 	HAL_I2C_Mem_Write_IT(&hi2c1, 0xa0, counter, 1, (uint8_t*)&temp, sizeof(temp));
//
//			HAL_Delay(5);
//
//		}

}



void getAddress(){

	HAL_I2C_Mem_Read(&hi2c1, 0xa0, COUNTER_ADDR, 1, (uint8_t*)&counter, sizeof(counter), HAL_MAX_DELAY);


}

//gets the last address of memory written and increments it
void setAddress(){

	HAL_I2C_Mem_Write(&hi2c1, 0xa0, COUNTER_ADDR, 1, (uint8_t*)&counter, sizeof(counter), HAL_MAX_DELAY);




}

void resetAddress(){
	uint8_t zero = 0;
	HAL_I2C_Mem_Write(&hi2c1, 0xa0, COUNTER_ADDR, 1, (uint8_t*)&zero, sizeof(zero), HAL_MAX_DELAY);

}


void Flash_flush(){
	uint8_t zero = 0xff;
	for(int i = 0; i <= 100; i++){
		HAL_I2C_Mem_Write_IT(&hi2c1, 0xa0, i, 1 , (uint8_t*)&zero, sizeof(zero));
		HAL_Delay(5);
	}
}



uint8_t bt = 0;

void Flash_write(int data[], int start_idx){

		counter = start_idx;

		uint8_t arr_idx = 0;
		uint8_t arr_size = 5;


		Send("Counter at: %d\r\n", counter);

		for(int i = 0; i < arr_size; i++){
			HAL_I2C_Mem_Write_IT(&hi2c1, 0xa0, counter, 1 , (uint8_t*)&data[i], sizeof(data[i]));
			//FlashTx_buff[FlashTx_busy++] = data[i];
			counter++;
			HAL_Delay(5);
		}








}


void Flash_read(){
	uint8_t byte = 0x00;
		for(int i = 0; i < 10; i++){
			HAL_I2C_Mem_Read(&hi2c1, 0xa1, i, 1, (uint8_t*)&byte, sizeof(byte), HAL_MAX_DELAY);
			Send("byte: %d\r\n", byte);
		}

	//Flash_getFreeSpace();

	//HAL_I2C_Mem_Read(&hi2c1, 0xa0, 0x01, 1, (uint16_t*)&bt, sizeof(bt), HAL_MAX_DELAY);

	//Send("%x, %d", bt, bt);
	//getAddress();

//	for(int i = 0x00; i < counter; i++){
//		Send("%x", i);
//
//		if(bt != 0){
//				Flash_buff[Flash_rx++] = bt;
//				if(Flash_rx >= 128){
//					Flash_rx = 0;
//				}
//			}
//	}

}
int Flash_getFreeSpace(){
	uint8_t byte = 0x00;
	int i = 0;

	while(byte != 0xff){
		HAL_I2C_Mem_Read(&hi2c1, 0xa1, i, 1, (uint8_t*)&byte, sizeof(byte), 1000);
		i++;

	}
	return i;







	//HAL_I2C_Mem_Write(&hi2c1, 0xa0, 0x01, 1, (uint16_t*)&data, sizeof(data), HAL_MAX_DELAY);

//	for(int i = 0; i < 10; i++){
//		HAL_I2C_Mem_Read(&hi2c1, 0xa0, i, 1, (uint8_t*)&byte, sizeof(byte), HAL_MAX_DELAY);
//		Send("byte: %d\r\n", byte);
//	}

}
