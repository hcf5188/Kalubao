#ifndef __BSP_H__
#define __BSP_H__

#include "includes.h"
#include "memblock.h"
#include "globalvar.h"

#define GPIO_LED		GPIOB
#define LED_GPIO_OBD 	GPIO_Pin_5
#define LED_GPIO_MOD 	GPIO_Pin_4
#define LED_GPIO_GPS 	GPIO_Pin_3

#define GPIO_Pin_KWP_RX		GPIO_Pin_11
#define GPIO_Pin_KWP_TX		GPIO_Pin_10



#define CDMA_POWER_HIGH  GPIO_SetBits(GPIOB,GPIO_Pin_15)
#define CDMA_POWER_LOW   GPIO_ResetBits(GPIOB,GPIO_Pin_15)

#define CDMA_MOS_HIGH    GPIO_SetBits(GPIOB,GPIO_Pin_12)
#define CDMA_MOS_LOW     GPIO_ResetBits(GPIOB,GPIO_Pin_12)

void SystemBspInit(void );

void SysTickInit(void);    //系统时钟初始化
void BspClockInit(void);   //外设时钟初始化
void GPIOLEDInit(void);    //LED  IO口初始化
void ADC1Init(void);
void GPIO_ALL_IN(void);

void GPSConfigInit(uint16_t baud);//设置GPS通信波特率
	
void CAN1Config(void);     //CAN1 初始化

void CDMAUart2Init(void);  //CDMA 串口初始化
void TIM3ConfigInit(void );//定时器3 用来判断CDMA接收超时
void TIM2ConfigInit(void );//定时器2 用来处理GPRS接收逻辑

void CDMASendByte(uint8_t dat);
uint8_t CDMASendDatas(const uint8_t* s,uint16_t length);//CDMA 发送字符串
uint8_t GPSSendDatas(const uint8_t* s,uint8_t length);  //GPS  发送字符串



#endif




