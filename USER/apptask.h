#ifndef __APPTASK_H__
#define __APPTASK_H__


#include "includes.h"
#include "bsp.h"
/************************   �����Լ��������ź��������ȼ�����   *************/

#define CDMA_TASK_PRIO       11     //����ͨ��GPRS�������ȼ�
#define GPS_TASK_PRIO        12     //������λGPS�������ȼ�
#define OBD_TASK_PRIO        13     //�������OBD�������ȼ�

#define START_TASK_PRIO      50     //��ʼ�������ȼ�

/************************   �����ջ��С���嶨��   ************************/


#define START_STK_SIZE       128    //��ʼ�����ջ��С
#define CDMA_STK_SIZE        128    //����ͨ��CDMA�����ջ��С
#define GPS_STK_SIZE         128    //������λGPS�����ջ��С
#define OBD_STK_SIZE         128    //�����������OBD�����ջ��С



/************************   ��������            ************************/


void CDMATask(void *pdata);
void GPSTask(void *pdata);
void OBDTask(void *pdata);





#endif


