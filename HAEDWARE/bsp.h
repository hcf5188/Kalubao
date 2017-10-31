#ifndef __BSP_H__
#define __BSP_H__

#include "includes.h"
#include "memblock.h"

#define GPIO_LED		GPIOB
#define LED_GPIO_OBD 	GPIO_Pin_5
#define LED_GPIO_MOD 	GPIO_Pin_4
#define LED_GPIO_GPS 	GPIO_Pin_3

#define GPIO_Pin_KWP_RX		GPIO_Pin_11
#define GPIO_Pin_KWP_TX		GPIO_Pin_10

void SystemBspInit(void );

void SysTickInit(void);    //ϵͳʱ�ӳ�ʼ��
void BspClockInit(void);   //����ʱ�ӳ�ʼ��
void GPIOLEDInit(void);    //LED  IO�ڳ�ʼ��
void CDMAUart2Init(void);  //CDMA ���ڳ�ʼ��
void CAN1Config(void);     //CAN1 ��ʼ��

#endif




