#ifndef __APPTASK_H__
#define __APPTASK_H__


#include "includes.h"
#include "bsp.h"
#include "memblock.h"
#include "globalvar.h"
/************************   �����Լ��������ź��������ȼ�����   *************/


#define CDMAPOWER_PRIO        7     //CDMA ��Դ��ռ����
#define CDMA_SEND_PRIO        8     //CDMA ���ͻ����ź������ȼ�

#define USB_TASK_PRIO        9      //USB��������

#define START_TASK_PRIO      10     //��ʼ�������ȼ�

#define CDMA_TASK_PRIO       11     //����ͨ��GPRS�������ȼ�
#define CDMARevc_TASK_PRIO   12     //����������·����ݵ���������ȼ�
#define GPS_TASK_PRIO        14     //������λGPS�������ȼ�
#define OBD_TASK_PRIO        15     //�������OBD�������ȼ�
#define J1939_TASK_PRIO      16     //SAE J1939 ������

#define POWER_TASK_PRIO      17     //��Դ��������

#define CDMA_LED_PRIO        20     //����ͨ��GPRS-LED�������ȼ�
#define GPS_LED_PRIO         21     //������λGPS-LED�������ȼ�
#define OBD_LED_PRIO         22     //�������OBD-LED�������ȼ�
#define BEEP_TASK_PRIO       23     //����������

#define POWER_CONTROL_PRIO   27     //��Դ��������   
#define CAN_BAUD_PRIO        30     //CAN���������������


/************************   �����ջ��С���嶨��   ************************/

#define USB_STK_SIZE         128    //USB���������ջ��С
#define START_STK_SIZE       128    //��ʼ�����ջ��С
#define CDMA_STK_SIZE        128    //����ͨ��CDMA�����ջ��С
#define CDMARecv_STK_SIZE    180    //�������·�����
#define GPS_STK_SIZE         128    //������λGPS�����ջ��С
#define OBD_STK_SIZE         128    //�����������OBD�����ջ��С
#define J1939_STK_SIZE       128    //�����������OBD�����ջ��С

#define POWER_STK_SIZE       128    //�����������OBD�����ջ��С

#define LED_STK_SIZE         80     //LED�����ջ��С
#define BEEP_STK_SIZE        80
#define CANBUAD_STK_SIZE     80
/************************   ��������            ************************/


void CDMATask(void *pdata);
void CDMARecvTask(void *pdata);

void GPSTask(void *pdata);

void OBDTask(void *pdata);
void DealJ1939Date(void *pdata);

void PowerDeal(void *pdata);

void StartTask  (void *pdata);          
void CDMALEDTask(void *pdata); 
void GPSLEDTask (void *pdata); 
void OBDLEDTask (void *pdata); 
void BeepTask   (void *pdata); 

void USBUpdataTask (void *pdata);//USB ������������

/*************************     �����ͨ��        *************************/
extern OS_EVENT * sendMsg;               //CDMA�Ƿ����ڷ�����Ϣ���ź���
extern OS_EVENT * beepSem;               //�������������ź���
extern OS_EVENT * LoginMes;              //��¼�����ź���

extern OS_EVENT * CDMASendMutex;         //�����������ź�����������ռ�������������Ϣ
extern OS_EVENT * CDMAPowerMutex;        //CDMA ��Դ�������ź���

extern OS_EVENT * CDMARecieveQ;          //CDMA ������Ϣ���е�ָ��
extern OS_EVENT * CDMASendQ;             //CDMA ������Ϣ���е�ָ��
extern OS_EVENT * ZIPRecv_Q;             //ָ��RECV��Ϣ���е�ָ��

extern OS_EVENT * receGPSQ;              //GPS  ������Ϣ����ָ��

extern OS_EVENT * canRecieveQ;           //CAN  ���߽�����Ϣ���е�ָ��
extern OS_EVENT * canSendQ;              //CAN  ���߷�����Ϣ���е�ָ��
extern OS_EVENT * canJ1939Q;             //SAE-J1939������Ϣ���е�ָ��

extern OS_EVENT * USBSendQ;              //USB  ������Ϣ���е�ָ��
extern OS_EVENT * USBRecieveQ;           //USB  ������Ϣ���е�ָ��

#endif


