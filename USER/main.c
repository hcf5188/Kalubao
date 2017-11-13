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

pCIR_QUEUE sendGPS_Q = NULL;     //指向 GPS 串口发送队列  的指针
pSTORE     receGPS_S = NULL;     //指向 GPS 串口接收数据堆的指针

_SystemInformation sysAllData;

int main(void )
{
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
	
	OSInit(); 
	
	MemBuf_Init();   //建立内存块
	
	sendCDMA_Q = Cir_Queue_Init(1000);//CDMA 串口发送 循环队列
	receCDMA_S = Store_Init(1024);   //CDMA 串口接收 数据堆
	
	sendGPS_Q = Cir_Queue_Init(230); //GPS  串口发送 循环队列
	receGPS_S = Store_Init(230);     //GPS  串口接收 数据堆
	
	SystemBspInit();
	 
	OSTaskCreate(StartTask,(void *)0,(OS_STK *)&START_TASK_STK[START_STK_SIZE-1],START_TASK_PRIO );
	
	OSStart();	 
}
#define SEND_MAX_TIME  3000     //3000ms计时时间到，则发送数据
_CDMADataToSend* cdmaDataToSend = NULL;//CDMA发送的数据中（OBD、GPS），是通过它来作为载体

OS_EVENT * CDMASendMutex;//建立互斥型信号量，用来独占处理 发向服务器的消息
extern OS_EVENT *canSendQ;        //向OBD发送PID指令
extern OS_EVENT *CDMASendQ;       //通过CDMA向服务器发送采集到的OBD、GPS数据
extern _OBD_PID_Cmd  obdPIDAll[];
void StartTask(void *pdata)
{
	uint8_t err;
	uint8_t i = 0;
	uint8_t *ptrOBDSend;
	
	OS_CPU_SR cpu_sr=0;
	pdata = pdata; 
	sysAllData.sendId = 0x80000000;     //全局发送指令流水号
	
	cdmaDataToSend = CDMNSendDataInit();//初始化获取发向CDMA的消息结构体
	
  	OS_ENTER_CRITICAL();			//进入临界区(无法被中断打断)    
	
 	OSTaskCreate(CDMATask,(void *)0,(OS_STK*)&CDMA_TASK_STK[CDMA_STK_SIZE-1],CDMA_TASK_PRIO);						   
 	OSTaskCreate(GPSTask, (void *)0,(OS_STK*)&GPS_TASK_STK[GPS_STK_SIZE-1],GPS_TASK_PRIO);		
 	OSTaskCreate(OBDTask, (void *)0,(OS_STK*)&OBD_TASK_STK[OBD_STK_SIZE-1],OBD_TASK_PRIO);	

	OSTaskCreate(CDMALEDTask,(void *)0,(OS_STK*)&CDMA_LED_STK[LED_STK_SIZE-1],CDMA_LED_PRIO);						   
 	OSTaskCreate(GPSLEDTask,(void *)0,(OS_STK*)&GPS_LED_STK[LED_STK_SIZE-1],GPS_LED_PRIO);		
 	OSTaskCreate(OBDLEDTask,(void *)0,(OS_STK*)&OBD_LED_STK[LED_STK_SIZE-1],OBD_LED_PRIO);		
	
	CDMASendMutex = OSMutexCreate(CDMA_SEND_PRIO,&err);//向CDMA发送缓冲区发送数据 独占 互斥型信号量
	
	OS_EXIT_CRITICAL();				//退出临界区(可以被中断打断)
	OSTimeDlyHMSM(0,0,15,4);        //todo:等待其他任务完成初始化之后，再进行数据的流动
	while(1)
	{
		OSTimeDlyHMSM(0,0,0,4);
		//todo:遍历OBD的PID指令，如果时间到达，则向OBD发送队列，让其与ECU通信
		for(i=0;i<10;i++)          //todo:PID指令的数目 后期需要配置
		{
			obdPIDAll[i].timeCount += 4;
			if(obdPIDAll[i].timeCount >= obdPIDAll[i].period)
			{
				obdPIDAll[i].timeCount = 0;
				
				ptrOBDSend = Mem_malloc(9);
				memcpy(ptrOBDSend,obdPIDAll[i].data,9);
				err = OSQPost(canSendQ,ptrOBDSend);  //向OBD推送要发送的PID指令
				if(err != OS_ERR_NONE)
					Mem_free(ptrOBDSend);//推送不成功，需要释放内存块
			}
		}
		if(cdmaDataToSend->datLength > 28)
			cdmaDataToSend->timeCount += 4;
		
		if((cdmaDataToSend->timeCount >= 3000) || (cdmaDataToSend->datLength >= 850))
		{
			OSMutexPend(CDMASendMutex,0,&err);
			//todo:将要发送的数据时间增加2，大于3000并且数据区不为0的时候，需要打包发送,申请新数据，并进行初始化
			CDMASendDataPack(cdmaDataToSend);
			err = OSQPost(CDMASendQ,cdmaDataToSend);
			if(err != OS_ERR_NONE)
			{
				cdmaDataToSend->datLength = 27;
				cdmaDataToSend->timeCount = 0;
			}
			else
			{
				cdmaDataToSend = CDMNSendDataInit();
			}	
			OSMutexPost(CDMASendMutex);
		}
	}
}
uint16_t freCDMALed = 100;
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
uint16_t freGPSLed = 100;
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
uint16_t freOBDLed = 100;
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


