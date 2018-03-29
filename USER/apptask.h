#ifndef __APPTASK_H__
#define __APPTASK_H__


#include "includes.h"
#include "bsp.h"
#include "memblock.h"
#include "globalvar.h"
/************************   �����Լ��������ź��������ȼ�����   *************/


#define CDMAPOWER_PRIO        6     //CDMA ��Դ��ռ����
#define CDMA_SEND_PRIO        7     //CDMA ���ͻ����ź������ȼ�
#define CAN_SEND_MUTEX        8     //CAN  �������ݻ������ź���

#define USB_TASK_PRIO         9     //USB  ��������

#define START_TASK_PRIO      10     //��ʼ�������ȼ�

#define CDMA_TASK_PRIO       11     //����ͨ��GPRS�������ȼ�
#define CDMARevc_TASK_PRIO   12     //����������·����ݵ���������ȼ�
#define OBD_ON_OFF_PRIO      13     //ECU�ϵ硢�����������ȼ�
#define GPS_TASK_PRIO        14     //������λGPS�������ȼ�
#define OBD_TASK_PRIO        15     //�������OBD�������ȼ�
#define J1939_TASK_PRIO      16     //SAE J1939 ������
#define SAVE_FUEL_PEIO       17     //��������  ���ȼ�

#define POWER_TASK_PRIO      19     //��Դ��������

#define CDMA_LED_PRIO        25     //����ͨ��GPRS-LED�������ȼ�
#define GPS_LED_PRIO         26     //������λGPS-LED�������ȼ�
#define OBD_LED_PRIO         27     //�������OBD-LED�������ȼ�
#define BEEP_TASK_PRIO       28     //����������

#define POWER_CONTROL_PRIO   30     //��Դ��������   
#define CAN_BAUD_PRIO        31     //CAN���������������


/************************   �����ջ��С���嶨��   ************************/

#define USB_STK_SIZE         128    //USB���������ջ��С
#define START_STK_SIZE       128    //��ʼ�����ջ��С
#define CDMA_STK_SIZE        128    //����ͨ��CDMA�����ջ��С
#define CDMARecv_STK_SIZE    180    //�������·�����
#define OBD_ONOFF_STK_SIZE   128    //ECU�ϵ硢��������
#define GPS_STK_SIZE         128    //������λGPS�����ջ��С
#define OBD_STK_SIZE         128    //�����������OBD�����ջ��С
#define J1939_STK_SIZE       128    //�����������OBD�����ջ��С
#define SAVEFUEL_STK_SIZE    128    //���������ջ��С

#define POWER_STK_SIZE       128    //�����������OBD�����ջ��С

#define LED_STK_SIZE         80     //LED�����ջ��С
#define BEEP_STK_SIZE        80
#define CANBUAD_STK_SIZE     80
/************************   ��������            ************************/


void CDMATask(void *pdata);      //CDMA��������
void CDMARecvTask(void *pdata);  //CDMA����������·����ݵ�����

void GPSTask(void *pdata);       //GPS��λ����

void OBD_ON_OFFDeal(void *pdata);       //ECU�ϵ硢�����������ڽ�������߼�

void OBDTask(void *pdata);       //CANͨѶ����
void DealJ1939Date(void *pdata); //����J1939�����³����ݵ�����
void SaveFuleTask(void *pdata);  //��������

void PowerDeal(void *pdata);

void StartTask  (void *pdata);  //��ʼ����        
void CDMALEDTask(void *pdata);  //CDMA ״̬�Ƶ���˸����
void GPSLEDTask (void *pdata);  //GPS  ״̬����˸����
void OBDLEDTask (void *pdata);  //OBD  ״̬����˸����
void BeepTask   (void *pdata);  //������ ������ʾ����


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


