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

void SysTickInit(void);    //ϵͳʱ�ӳ�ʼ��
void BspClockInit(void);   //����ʱ�ӳ�ʼ��
void GPIOLEDInit(void);    //LED  IO�ڳ�ʼ��
void ADC1Init(void);
void GPIO_ALL_IN(void);

void GPSConfigInit(uint16_t baud);//����GPSͨ�Ų�����
	
void CAN1Config(void);     //CAN1 ��ʼ��

void CDMAUart2Init(void);  //CDMA ���ڳ�ʼ��
void TIM3ConfigInit(void );//��ʱ��3 �����ж�CDMA���ճ�ʱ
void TIM2ConfigInit(void );//��ʱ��2 ��������GPRS�����߼�

void CDMASendByte(uint8_t dat);
uint8_t CDMASendDatas(const uint8_t* s,uint16_t length);//CDMA �����ַ���
uint8_t GPSSendDatas(const uint8_t* s,uint8_t length);  //GPS  �����ַ���



#endif




