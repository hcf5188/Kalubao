#ifndef __APPTASK_H__
#define __APPTASK_H__


#include "includes.h"
#include "bsp.h"
#include "memblock.h"
#include "globalvar.h"
/************************   任务以及互斥型信号量的优先级定义   *************/


#define CDMA_SEND_PRIO        8     //CDMA 发送互斥信号量优先级

#define START_TASK_PRIO      10     //起始任务优先级

#define CDMA_TASK_PRIO       11     //网络通信GPRS任务优先级
#define CDMARevc_TASK_PRIO   12     //处理服务器下发数据的任务的优先级
#define GPS_TASK_PRIO        14     //车辆定位GPS任务优先级
#define OBD_TASK_PRIO        15     //故障诊断OBD任务优先级

#define CDMA_LED_PRIO        20     //网络通信GPRS-LED任务优先级
#define GPS_LED_PRIO         21     //车辆定位GPS-LED任务优先级
#define OBD_LED_PRIO         22     //故障诊断OBD-LED任务优先级
#define BEEP_TASK_PRIO       23     //蜂鸣器任务


/************************   任务堆栈大小定义定义   ************************/


#define START_STK_SIZE       128    //起始任务堆栈大小
#define CDMA_STK_SIZE        128    //网络通信CDMA任务堆栈大小
#define CDMARecv_STK_SIZE    128    //服务器下发数据
#define GPS_STK_SIZE         128    //车辆定位GPS任务堆栈大小
#define OBD_STK_SIZE         128    //整车故障诊断OBD任务堆栈大小

#define LED_STK_SIZE         80     //LED任务堆栈大小
#define BEEP_STK_SIZE        80
/************************   任务声明            ************************/


void CDMATask(void *pdata);
void GPSTask(void *pdata);
void OBDTask(void *pdata);
void CDMARecvTask(void *pdata);




#endif


