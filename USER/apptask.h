#ifndef __APPTASK_H__
#define __APPTASK_H__


#include "includes.h"
#include "bsp.h"
#include "memblock.h"
#include "globalvar.h"
/************************   �����Լ��������ź��������ȼ�����   *************/


#define CDMA_SEND_PRIO        8     //CDMA ���ͻ����ź������ȼ�

#define START_TASK_PRIO      10     //��ʼ�������ȼ�

#define CDMA_TASK_PRIO       11     //����ͨ��GPRS�������ȼ�
#define CDMARevc_TASK_PRIO   12     //����������·����ݵ���������ȼ�
#define GPS_TASK_PRIO        14     //������λGPS�������ȼ�
#define OBD_TASK_PRIO        15     //�������OBD�������ȼ�

#define CDMA_LED_PRIO        20     //����ͨ��GPRS-LED�������ȼ�
#define GPS_LED_PRIO         21     //������λGPS-LED�������ȼ�
#define OBD_LED_PRIO         22     //�������OBD-LED�������ȼ�
#define BEEP_TASK_PRIO       23     //����������


/************************   �����ջ��С���嶨��   ************************/


#define START_STK_SIZE       128    //��ʼ�����ջ��С
#define CDMA_STK_SIZE        128    //����ͨ��CDMA�����ջ��С
#define CDMARecv_STK_SIZE    128    //�������·�����
#define GPS_STK_SIZE         128    //������λGPS�����ջ��С
#define OBD_STK_SIZE         128    //�����������OBD�����ջ��С

#define LED_STK_SIZE         80     //LED�����ջ��С
#define BEEP_STK_SIZE        80
/************************   ��������            ************************/


void CDMATask(void *pdata);
void GPSTask(void *pdata);
void OBDTask(void *pdata);
void CDMARecvTask(void *pdata);




#endif


