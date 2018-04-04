#ifndef __BSP_H__
#define __BSP_H__

#include "includes.h"
#include "memblock.h"
#include "globalvar.h"
#include "gps.h"
#include "stm32f10x_wwdg.h"


#define  BEN_S_C    2            //0 - ����     1 - ���Ի���    2 - �������� 


#define  FRAME_HEAD_LEN    27 

#define GPIO_LED		GPIOB
#define LED_GPIO_OBD 	GPIO_Pin_5
#define LED_GPIO_MOD 	GPIO_Pin_4
#define LED_GPIO_GPS 	GPIO_Pin_3

#define GPIO_Pin_KWP_RX		GPIO_Pin_11
#define GPIO_Pin_KWP_TX		GPIO_Pin_10

extern SYS_OperationVar   varOperation;       //�������й����е�ȫ�ֱ�������

extern _SystemInformation sysUpdateVar;       //��������������
extern _CANDataConfig     canDataConfig;      //����CANͨѶ����

extern CARRunRecord       carAllRecord;       //�������й����У�����ȫ����Ϣ
extern nmea_msg           gpsMC; 	          //����GPS��Ϣ
extern _CDMADataToSend*   cdmaDataToSend;     //���͸� CDMA ����Ϣ����
extern pSTORE             cdmaLogData ;       //������־������
extern STRENFUEL_Struct   strengthFuel;       //��ǿ����������
extern STRENFUEL_Struct   strengthFuelFlash;  // Flash �������ǿ����

#define CDMA_POWER_HIGH  GPIO_SetBits(GPIOB  ,GPIO_Pin_15)
#define CDMA_POWER_LOW   GPIO_ResetBits(GPIOB,GPIO_Pin_15)

#define CDMA_MOS_HIGH    GPIO_SetBits(GPIOB  ,GPIO_Pin_12)
#define CDMA_MOS_LOW     GPIO_ResetBits(GPIOB,GPIO_Pin_12)

#define GPS_POWER_ON     GPIO_ResetBits(GPIOC,GPIO_Pin_8);  //�� GPS ��Դ
#define GPS_POWER_OFF    GPIO_SetBits(GPIOC  ,GPIO_Pin_8);  //�ر� GPS ��Դ

void SystemBspInit(void );

void SysTickInit(void);    //ϵͳʱ�ӳ�ʼ��
void BspClockInit(void);   //����ʱ�ӳ�ʼ��
void GPIOLEDInit(void);    //LED IO �ڳ�ʼ��
void GPIO_ALL_IN(void);
void RTCConfigureInit(void);                 // RTC ʵʱʱ�ӳ�ʼ��
void RTC_Time_Adjust(uint32_t value);        // RTC ʵʱʱ��У��
void WatchDogInit(uint8_t prer,uint16_t rlr);// �������Ź���ʼ��

void CARVarInit(void);        //�복����ʻ��ؽṹ��ĳ�ʼ��

void NVIC_AllConfig(void );   //�жϷ�����ض���

void GPSConfigInit(uint16_t baud);  //����GPSͨ�Ų�����

void CAN1Config(void);        //CAN1 ��ʼ��

void CDMAUart2Init(void);     //CDMA ���ڳ�ʼ��
void TIM1ConfigInit(void);    //��ʱ�� 1 ����ʱ��
void TIM4ConfigInit(void );   //��ʱ�� 3 �����ж�CDMA���ճ�ʱ
void TIM2ConfigInit(void );   //��ʱ�� 2 ��������GPRS�����߼�
void TIM5ConfigInit(void );   //��ʱ�� 5 ��������USB����

void ADC1Init(void);       	  // ADC �ɼ���ʼ��
u16  Get_Adc(u8 ch);          //��ȡ ADC ֵ
u16  Get_Adc_Average(u8 ch,u8 times); //�õ� ADC �ɼ�ƽ��ֵ


void PIDPtrInit(void);     //�����һ�����ڴ������ʼ��

void    CDMASendByte(uint8_t dat);
uint8_t CDMASendDatas(const uint8_t* s,uint16_t length);//CDMA �����ַ���
uint8_t GPSSendDatas(const uint8_t* s,uint8_t length);  //GPS  �����ַ���

#endif


