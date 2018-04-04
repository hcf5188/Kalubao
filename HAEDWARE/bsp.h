#ifndef __BSP_H__
#define __BSP_H__

#include "includes.h"
#include "memblock.h"
#include "globalvar.h"
#include "gps.h"
#include "stm32f10x_wwdg.h"


#define  BEN_S_C    2            //0 - 本地     1 - 测试环境    2 - 生产环境 


#define  FRAME_HEAD_LEN    27 

#define GPIO_LED		GPIOB
#define LED_GPIO_OBD 	GPIO_Pin_5
#define LED_GPIO_MOD 	GPIO_Pin_4
#define LED_GPIO_GPS 	GPIO_Pin_3

#define GPIO_Pin_KWP_RX		GPIO_Pin_11
#define GPIO_Pin_KWP_TX		GPIO_Pin_10

extern SYS_OperationVar   varOperation;       //程序运行过程中的全局变量参数

extern _SystemInformation sysUpdateVar;       //用来保存升级用
extern _CANDataConfig     canDataConfig;      //保存CAN通讯参数

extern CARRunRecord       carAllRecord;       //汽车运行过程中，几乎全部信息
extern nmea_msg           gpsMC; 	          //保存GPS信息
extern _CDMADataToSend*   cdmaDataToSend;     //发送给 CDMA 的信息载体
extern pSTORE             cdmaLogData ;       //发送日志缓冲区
extern STRENFUEL_Struct   strengthFuel;       //增强动力、节油
extern STRENFUEL_Struct   strengthFuelFlash;  // Flash 保存的增强动力

#define CDMA_POWER_HIGH  GPIO_SetBits(GPIOB  ,GPIO_Pin_15)
#define CDMA_POWER_LOW   GPIO_ResetBits(GPIOB,GPIO_Pin_15)

#define CDMA_MOS_HIGH    GPIO_SetBits(GPIOB  ,GPIO_Pin_12)
#define CDMA_MOS_LOW     GPIO_ResetBits(GPIOB,GPIO_Pin_12)

#define GPS_POWER_ON     GPIO_ResetBits(GPIOC,GPIO_Pin_8);  //打开 GPS 电源
#define GPS_POWER_OFF    GPIO_SetBits(GPIOC  ,GPIO_Pin_8);  //关闭 GPS 电源

void SystemBspInit(void );

void SysTickInit(void);    //系统时钟初始化
void BspClockInit(void);   //外设时钟初始化
void GPIOLEDInit(void);    //LED IO 口初始化
void GPIO_ALL_IN(void);
void RTCConfigureInit(void);                 // RTC 实时时钟初始化
void RTC_Time_Adjust(uint32_t value);        // RTC 实时时钟校正
void WatchDogInit(uint8_t prer,uint16_t rlr);// 独立看门狗初始化

void CARVarInit(void);        //与车辆行驶相关结构体的初始化

void NVIC_AllConfig(void );   //中断方面的重定向

void GPSConfigInit(uint16_t baud);  //设置GPS通信波特率

void CAN1Config(void);        //CAN1 初始化

void CDMAUart2Init(void);     //CDMA 串口初始化
void TIM1ConfigInit(void);    //定时器 1 矫正时间
void TIM4ConfigInit(void );   //定时器 3 用来判断CDMA接收超时
void TIM2ConfigInit(void );   //定时器 2 用来处理GPRS接收逻辑
void TIM5ConfigInit(void );   //定时器 5 用来处理USB接收

void ADC1Init(void);       	  // ADC 采集初始化
u16  Get_Adc(u8 ch);          //读取 ADC 值
u16  Get_Adc_Average(u8 ch,u8 times); //得到 ADC 采集平均值


void PIDPtrInit(void);     //多包合一包，内存块分配初始化

void    CDMASendByte(uint8_t dat);
uint8_t CDMASendDatas(const uint8_t* s,uint16_t length);//CDMA 发送字符串
uint8_t GPSSendDatas(const uint8_t* s,uint8_t length);  //GPS  发送字符串

#endif


