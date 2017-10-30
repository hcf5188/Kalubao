#ifndef __APPTASK_H__
#define __APPTASK_H__


#include "includes.h"
#include "bsp.h"
/************************   任务以及互斥型信号量的优先级定义   *************/

#define CDMA_TASK_PRIO       11     //网络通信GPRS任务优先级
#define GPS_TASK_PRIO        12     //车辆定位GPS任务优先级
#define OBD_TASK_PRIO        13     //故障诊断OBD任务优先级

#define START_TASK_PRIO      50     //起始任务优先级

/************************   任务堆栈大小定义定义   ************************/


#define START_STK_SIZE       128    //起始任务堆栈大小
#define CDMA_STK_SIZE        128    //网络通信CDMA任务堆栈大小
#define GPS_STK_SIZE         128    //车辆定位GPS任务堆栈大小
#define OBD_STK_SIZE         128    //整车故障诊断OBD任务堆栈大小



/************************   任务声明            ************************/


void CDMATask(void *pdata);
void GPSTask(void *pdata);
void OBDTask(void *pdata);





#endif


