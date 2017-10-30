
#include "bsp.h"
#include "includes.h"
#include "apptask.h"
#include "delay.h"


OS_STK START_TASK_STK[START_STK_SIZE];//起始任务堆栈

OS_STK CDMA_TASK_STK[CDMA_STK_SIZE];  //网络通讯CDMA任务堆栈
OS_STK GPS_TASK_STK[GPS_STK_SIZE];    //车辆定位GPS任务堆栈
OS_STK OBD_TASK_STK[OBD_STK_SIZE];    //汽车诊断OBD任务堆栈

void StartTask(void *pdata);          


int main(void )
{
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
	
	SystemBspInit();
	
	OSInit(); 
	
	OSTaskCreate(StartTask,(void *)0,(OS_STK *)&START_TASK_STK[START_STK_SIZE-1],START_TASK_PRIO );
	
	OSStart();	 
}

void StartTask(void *pdata)
{
	OS_CPU_SR cpu_sr=0;
	pdata = pdata; 
	MemBuf_Init();                  //建立任务控制块
  	OS_ENTER_CRITICAL();			//进入临界区(无法被中断打断)    
	
 	OSTaskCreate(CDMATask,(void *)0,(OS_STK*)&CDMA_TASK_STK[CDMA_STK_SIZE-1],CDMA_TASK_PRIO);						   
 	OSTaskCreate(GPSTask,(void *)0,(OS_STK*)&GPS_TASK_STK[GPS_STK_SIZE-1],GPS_TASK_PRIO);		
 	OSTaskCreate(OBDTask,(void *)0,(OS_STK*)&OBD_TASK_STK[OBD_STK_SIZE-1],OBD_TASK_PRIO);						   
	
    OSTaskSuspend(START_TASK_PRIO);	//挂起起始任务.
	OS_EXIT_CRITICAL();				//退出临界区(可以被中断打断)
	while(1)
	{
		OSTimeDlyHMSM(0,0,0,250);
	}
}



