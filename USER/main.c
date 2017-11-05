
#include "bsp.h"
#include "includes.h"
#include "apptask.h"
#include "delay.h"


OS_STK START_TASK_STK[START_STK_SIZE];//起始任务堆栈

OS_STK CDMA_TASK_STK[CDMA_STK_SIZE];  //网络通讯CDMA任务堆栈
OS_STK GPS_TASK_STK[GPS_STK_SIZE];    //车辆定位GPS任务堆栈
OS_STK OBD_TASK_STK[OBD_STK_SIZE];    //汽车诊断OBD任务堆栈

OS_STK CDMA_LED_STK[LED_STK_SIZE];  //网络通讯CDMA-LED任务堆栈
OS_STK GPS_LED_STK[LED_STK_SIZE];   //车辆定位GPS-LED任务堆栈
OS_STK OBD_LED_STK[LED_STK_SIZE];   //汽车诊断OBD-LED任务堆栈

void StartTask(void *pdata);          
void CDMALEDTask(void *pdata); 
void GPSLEDTask(void *pdata); 
void OBDLEDTask(void *pdata); 

pCIR_QUEUE sendCDMA_Q = NULL;     //指向 CDMA 串口发送队列  的指针
pSTORE     receCDMA_S = NULL;     //指向 CDMA 串口接收数据堆的指针


int main(void )
{
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
	
	OSInit(); 
	MemBuf_Init();   //建立内存控制块
	
	sendCDMA_Q = Cir_Queue_Init(500);//CDMA 串口发送 循环队列
	receCDMA_S = Store_Init(1024);   //CDMA 串口接收 数据堆
	
	SystemBspInit();
	 
	
	OSTaskCreate(StartTask,(void *)0,(OS_STK *)&START_TASK_STK[START_STK_SIZE-1],START_TASK_PRIO );
	
	OSStart();	 
}



void StartTask(void *pdata)
{
	OS_CPU_SR cpu_sr=0;
	pdata = pdata; 

  	OS_ENTER_CRITICAL();			//进入临界区(无法被中断打断)    
	
 	OSTaskCreate(CDMATask,(void *)0,(OS_STK*)&CDMA_TASK_STK[CDMA_STK_SIZE-1],CDMA_TASK_PRIO);						   
 	OSTaskCreate(GPSTask,(void *)0,(OS_STK*)&GPS_TASK_STK[GPS_STK_SIZE-1],GPS_TASK_PRIO);		
 	OSTaskCreate(OBDTask,(void *)0,(OS_STK*)&OBD_TASK_STK[OBD_STK_SIZE-1],OBD_TASK_PRIO);	

	OSTaskCreate(CDMALEDTask,(void *)0,(OS_STK*)&CDMA_LED_STK[LED_STK_SIZE-1],CDMA_LED_PRIO);						   
 	OSTaskCreate(GPSLEDTask,(void *)0,(OS_STK*)&GPS_LED_STK[LED_STK_SIZE-1],GPS_LED_PRIO);		
 	OSTaskCreate(OBDLEDTask,(void *)0,(OS_STK*)&OBD_LED_STK[LED_STK_SIZE-1],OBD_LED_PRIO);		
	
    OSTaskSuspend(START_TASK_PRIO);	//挂起起始任务.
	OS_EXIT_CRITICAL();				//退出临界区(可以被中断打断)
	while(1)
	{
		OSTimeDlyHMSM(0,0,0,250);
	}
}
uint16_t freCDMALed = 300;
void CDMALEDTask(void *pdata)
{
	while(1)
	{
		if(freCDMALed < 50)
			freCDMALed = 50;
		else if(freCDMALed >2000)
			freCDMALed = 2000;
		GPIO_ResetBits(GPIO_LED,LED_GPIO_MOD);
		OSTimeDlyHMSM(0,0,0,freCDMALed);
		GPIO_SetBits(GPIO_LED,LED_GPIO_MOD);
		OSTimeDlyHMSM(0,0,0,freCDMALed);
	}
}
uint16_t freGPSLed = 300;
void GPSLEDTask(void *pdata)
{
	while(1)
	{
		if(freGPSLed < 50)
			freGPSLed = 50;
		else if(freGPSLed >2000)
			freGPSLed = 2000;
		GPIO_ResetBits(GPIO_LED,LED_GPIO_GPS);
		OSTimeDlyHMSM(0,0,0,freGPSLed);
		GPIO_SetBits(GPIO_LED,LED_GPIO_GPS);
		OSTimeDlyHMSM(0,0,0,freGPSLed);
	}
}
uint16_t freOBDLed = 300;
void OBDLEDTask(void *pdata)
{
	while(1)
	{
		if(freOBDLed < 50)
			freOBDLed = 50;
		else if(freOBDLed >2000)
			freOBDLed = 2000;
		GPIO_ResetBits(GPIO_LED,LED_GPIO_OBD);
		OSTimeDlyHMSM(0,0,0,freOBDLed);
		GPIO_SetBits(GPIO_LED,LED_GPIO_OBD);
		OSTimeDlyHMSM(0,0,0,freOBDLed);
	}
}


