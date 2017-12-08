#ifndef __APPTASK_H__
#define __APPTASK_H__


#include "includes.h"
#include "bsp.h"
#include "memblock.h"
#include "globalvar.h"
/************************   任务以及互斥型信号量的优先级定义   *************/


#define CDMAPOWER_PRIO        7     //CDMA 电源独占管理
#define CDMA_SEND_PRIO        8     //CDMA 发送互斥信号量优先级

#define USB_TASK_PRIO        9      //USB升级任务

#define START_TASK_PRIO      10     //起始任务优先级

#define CDMA_TASK_PRIO       11     //网络通信GPRS任务优先级
#define CDMARevc_TASK_PRIO   12     //处理服务器下发数据的任务的优先级
#define GPS_TASK_PRIO        14     //车辆定位GPS任务优先级
#define OBD_TASK_PRIO        15     //故障诊断OBD任务优先级
#define J1939_TASK_PRIO      16     //SAE J1939 任务处理

#define POWER_TASK_PRIO      17     //电源管理任务

#define CDMA_LED_PRIO        20     //网络通信GPRS-LED任务优先级
#define GPS_LED_PRIO         21     //车辆定位GPS-LED任务优先级
#define OBD_LED_PRIO         22     //故障诊断OBD-LED任务优先级
#define BEEP_TASK_PRIO       23     //蜂鸣器任务

#define POWER_CONTROL_PRIO   27     //电源管理任务   
#define CAN_BAUD_PRIO        30     //CAN波特率自诊断任务


/************************   任务堆栈大小定义定义   ************************/

#define USB_STK_SIZE         128    //USB升级任务堆栈大小
#define START_STK_SIZE       128    //起始任务堆栈大小
#define CDMA_STK_SIZE        128    //网络通信CDMA任务堆栈大小
#define CDMARecv_STK_SIZE    180    //服务器下发数据
#define GPS_STK_SIZE         128    //车辆定位GPS任务堆栈大小
#define OBD_STK_SIZE         128    //整车故障诊断OBD任务堆栈大小
#define J1939_STK_SIZE       128    //整车故障诊断OBD任务堆栈大小

#define POWER_STK_SIZE       128    //整车故障诊断OBD任务堆栈大小

#define LED_STK_SIZE         80     //LED任务堆栈大小
#define BEEP_STK_SIZE        80
#define CANBUAD_STK_SIZE     80
/************************   任务声明            ************************/


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

void USBUpdataTask (void *pdata);//USB 升级任务声明

/*************************     任务间通信        *************************/
extern OS_EVENT * sendMsg;               //CDMA是否正在发送消息的信号量
extern OS_EVENT * beepSem;               //建立蜂鸣器响信号量
extern OS_EVENT * LoginMes;              //登录报文信号量

extern OS_EVENT * CDMASendMutex;         //建立互斥型信号量，用来独占发向服务器的消息
extern OS_EVENT * CDMAPowerMutex;        //CDMA 电源互斥型信号量

extern OS_EVENT * CDMARecieveQ;          //CDMA 接收消息队列的指针
extern OS_EVENT * CDMASendQ;             //CDMA 发送消息队列的指针
extern OS_EVENT * ZIPRecv_Q;             //指向RECV消息队列的指针

extern OS_EVENT * receGPSQ;              //GPS  接收消息队列指针

extern OS_EVENT * canRecieveQ;           //CAN  总线接收消息队列的指针
extern OS_EVENT * canSendQ;              //CAN  总线发送消息队列的指针
extern OS_EVENT * canJ1939Q;             //SAE-J1939接收消息队列的指针

extern OS_EVENT * USBSendQ;              //USB  发送消息队列的指针
extern OS_EVENT * USBRecieveQ;           //USB  接收消息队列的指针

#endif


