#ifndef __APPTASK_H__
#define __APPTASK_H__


#include "includes.h"
#include "bsp.h"
#include "memblock.h"
/************************   �����Լ��������ź��������ȼ�����   *************/

#define CDMA_TASK_PRIO       11     //����ͨ��GPRS�������ȼ�
#define GPS_TASK_PRIO        12     //������λGPS�������ȼ�
#define OBD_TASK_PRIO        13     //�������OBD�������ȼ�

#define CDMA_LED_PRIO        20     //����ͨ��GPRS-LED�������ȼ�
#define GPS_LED_PRIO         21     //������λGPS-LED�������ȼ�
#define OBD_LED_PRIO         22     //�������OBD-LED�������ȼ�

#define START_TASK_PRIO      50     //��ʼ�������ȼ�

/************************   �����ջ��С���嶨��   ************************/


#define START_STK_SIZE       128    //��ʼ�����ջ��С
#define CDMA_STK_SIZE        128    //����ͨ��CDMA�����ջ��С
#define GPS_STK_SIZE         128    //������λGPS�����ջ��С
#define OBD_STK_SIZE         128    //�����������OBD�����ջ��С

#define LED_STK_SIZE         64     //LED�����ջ��С

/************************   ��������            ************************/


void CDMATask(void *pdata);
void GPSTask(void *pdata);
void OBDTask(void *pdata);





#endif


