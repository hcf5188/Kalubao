#ifndef __BSP_H__
#define __BSP_H__

#include "includes.h"
#include "memblock.h"
#include "globalvar.h"
#include "gps.h"


#define GPIO_LED		GPIOB
#define LED_GPIO_OBD 	GPIO_Pin_5
#define LED_GPIO_MOD 	GPIO_Pin_4
#define LED_GPIO_GPS 	GPIO_Pin_3

#define GPIO_Pin_KWP_RX		GPIO_Pin_11
#define GPIO_Pin_KWP_TX		GPIO_Pin_10

extern SYS_OperationVar   varOperation; //程序运行过程中的全局变量参数
extern _SystemInformation sysUpdateVar; //用来保存升级用
extern CARRunRecord       carAllRecord; //汽车运行过程中，几乎全部信息
extern nmea_msg           gpsMC; 	    //保存GPS信息
extern _CDMADataToSend* cdmaDataToSend; //发送给 CDMA 的信息载体

#define CDMA_POWER_HIGH  GPIO_SetBits(GPIOB,GPIO_Pin_15)
#define CDMA_POWER_LOW   GPIO_ResetBits(GPIOB,GPIO_Pin_15)

#define CDMA_MOS_HIGH    GPIO_SetBits(GPIOB,GPIO_Pin_12)
#define CDMA_MOS_LOW     GPIO_ResetBits(GPIOB,GPIO_Pin_12)

#define GPS_POWER_ON     GPIO_ResetBits(GPIOC,GPIO_Pin_8);  //打开GPS电源
#define GPS_POWER_OFF    GPIO_SetBits(GPIOC,GPIO_Pin_8);    //关闭GPS电源

void SystemBspInit(void );

void SysTickInit(void);    //系统时钟初始化
void BspClockInit(void);   //外设时钟初始化
void GPIOLEDInit(void);    //LED IO 口初始化
void ADC1Init(void);
void GPIO_ALL_IN(void);
void RTCConfigureInit(void);//RTC实时时钟初始化
void RTC_Time_Adjust(uint32_t value);//RTC实时时钟校正

void CARVarInit(void); //与车辆行驶相关结构体的初始化

void NVIC_AllConfig(void );//中断方面的重定向

void GPSConfigInit(uint16_t baud);//设置GPS通信波特率
	
void CAN1Config(void);     //CAN1 初始化

void CDMAUart2Init(void);  //CDMA 串口初始化
void TIM4ConfigInit(void );//定时器3 用来判断CDMA接收超时
void TIM2ConfigInit(void );//定时器2 用来处理GPRS接收逻辑
void TIM5ConfigInit(void );//定时器5 用来处理USB接收

void CDMASendByte(uint8_t dat);
uint8_t CDMASendDatas(const uint8_t* s,uint16_t length);//CDMA 发送字符串
uint8_t GPSSendDatas(const uint8_t* s,uint8_t length);  //GPS  发送字符串

#endif


