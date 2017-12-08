#include "bsp.h"
#include "includes.h"
#include "apptask.h"
#include "delay.h"


OS_STK USB_TASK_STK[USB_STK_SIZE];    //USB升级任务堆栈
OS_STK START_TASK_STK[START_STK_SIZE];//起始任务堆栈

OS_STK CDMA_TASK_STK[CDMA_STK_SIZE];  //网络通讯CDMA任务堆栈
OS_STK CDMARecv_TASK_STK[CDMARecv_STK_SIZE];//网络通讯CDMA接收服务器数据任务堆栈

OS_STK GPS_TASK_STK[GPS_STK_SIZE];     //车辆定位GPS任务堆栈

OS_STK OBD_TASK_STK[OBD_STK_SIZE];     //汽车诊断OBD任务堆栈
OS_STK J1939_TASK_STK[J1939_STK_SIZE]; //SAE - J1939任务堆栈

OS_STK POWER_TASK_STK[POWER_STK_SIZE]; //系统电源管理任务堆栈

OS_STK CDMA_LED_STK[LED_STK_SIZE];  //网络通讯CDMA-LED任务堆栈
OS_STK GPS_LED_STK[LED_STK_SIZE];   //车辆定位GPS-LED任务堆栈
OS_STK OBD_LED_STK[LED_STK_SIZE];   //汽车诊断OBD-LED任务堆栈
OS_STK BEEP_STK[BEEP_STK_SIZE];     //蜂鸣器任务堆栈

//  各串口接收、发送缓冲区初始化
pCIR_QUEUE sendCDMA_Q = NULL;     //指向 CDMA 串口发送队列  的指针
pSTORE     receCDMA_S = NULL;     //指向 CDMA 串口接收数据堆的指针

pCIR_QUEUE sendGPS_Q = NULL;      //指向 GPS 串口发送队列  的指针
pSTORE     receGPS_S = NULL;      //指向 GPS 串口接收数据堆的指针

/****************************************************************
*			void	int main(void )
* 描	述 : 入口函数	 		
* 输入参数  : 无
* 返 回 值 : int
****************************************************************/
int main(void )
{
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
	
	OSInit(); 
	
	MemBuf_Init();   //建立内存块
	
	sendCDMA_Q = Cir_Queue_Init(1000);//CDMA 串口发送 循环队列 缓冲区
	receCDMA_S = Store_Init(1020);    //CDMA 串口接收 数据堆   缓冲区
	
	sendGPS_Q = Cir_Queue_Init(230);  //GPS  串口发送 循环队列 缓冲区
	receGPS_S = Store_Init(230);      //GPS  串口接收 数据堆   缓冲区
	
	SystemBspInit();                  //硬件初始化 
	OSTaskCreate(StartTask,(void *)0,(OS_STK *)&START_TASK_STK[START_STK_SIZE-1],START_TASK_PRIO );
	
	OSStart();	 
}

/***********************   信号量、互斥型信号量、消息队列    ***********************/
#define CDMARECBUF_SIZE   10        //CDMA接收消息队列保存消息的最大量
#define CDMASENDBUF_SIZE  5         //CDMA发送消息队列保存消息的最大量
#define ZIPRECVBUF_SIZE   5         //RECV接收消息队列保存消息的最大量
#define GPSRECBUF_SIZE    10        //接收GPS消息队列保存消息的最大量
#define CANRECBUF_SIZE    20        //CAN接收消息队列保存消息的最大量
#define CANSENDBUF_SIZE   80        //CAN发送消息队列保存消息的最大量
#define CANJ1939BUF_SIZE  20        //CAN接收J1939消息队列保存消息的最大量
#define USBRECBUF_SIZE    10        //接收消息队列保存消息的最大量
#define USBSENDBUF_SIZE   5         //发送消息队列保存消息的最大量

void *cdmaRecBuf[CDMARECBUF_SIZE];  //指向 CDMA 接收消息的指针数组
void *cdmaSendBuf[CDMASENDBUF_SIZE];//指向 CDMA 发送消息的指针数组
void *ZIPRecBuf[ZIPRECVBUF_SIZE];   //指向 CDMA 接收到服务器数据的消息指针数组
void *gpsRecBuf[GPSRECBUF_SIZE];    //指向 GPS  接收消息的指针数组
void *canRecBuf[CANRECBUF_SIZE];    //指向 CAN  接收消息的指针数组
void *canSendBuf[CANSENDBUF_SIZE];  //指向 CAN  发送消息的指针数组
void *canJ1939Buf[CANJ1939BUF_SIZE];//指向 CAN SAE-J1939接收消息的指针数组
void *usbRecBuf[USBRECBUF_SIZE];    //用于存放指向邮箱的指针
void *usbSendBuf[USBSENDBUF_SIZE];  //用于存放指向邮箱的指针

OS_EVENT * sendMsg;               //CDMA是否正在发送消息的信号量
OS_EVENT * beepSem;               //建立蜂鸣器响信号量
OS_EVENT * LoginMes;              //登录报文信号量

OS_EVENT * CDMASendMutex;         //建立互斥型信号量，用来独占发向服务器的消息
OS_EVENT * CDMAPowerMutex;        //CDMA 电源互斥型信号量

OS_EVENT * CDMARecieveQ;          //CDMA 接收消息队列的指针
OS_EVENT * CDMASendQ;             //CDMA 发送消息队列的指针
OS_EVENT * ZIPRecv_Q;             //指向RECV消息队列的指针

OS_EVENT * receGPSQ;              //GPS  接收消息队列指针

OS_EVENT * canRecieveQ;           //CAN  总线接收消息队列的指针
OS_EVENT * canSendQ;              //CAN  总线发送消息队列的指针
OS_EVENT * canJ1939Q;             //SAE-J1939接收消息队列的指针

OS_EVENT * USBSendQ;              //USB  发送消息队列的指针
OS_EVENT * USBRecieveQ;           //USB  接收消息队列的指针

/***************************   CAN指令相关     ***********************************/
#define SEND_MAX_TIME  3000              //3000ms计时时间到，则发送数据

_CDMADataToSend* cdmaDataToSend = NULL;  //CDMAl发送的数据中（OBD、GPS），是通过它来作为载体
extern _OBD_PID_Cmd *ptrPIDAllDat;   

void StartTask(void *pdata)
{
	uint8_t err;
	uint8_t i = 0;
	uint8_t *ptrOBDSend;
	uint32_t timeToSendLogin  = 0;
	OS_CPU_SR cpu_sr=0;
	pdata = pdata; 
	
	cdmaDataToSend = CDMNSendDataInit(1000);//初始化获取发向CDMA的消息结构体
	if(varOperation.USB_NormalMode == 1)//USB 升级模式
	{
		USBRecieveQ = OSQCreate(&usbRecBuf[0],USBRECBUF_SIZE);  //建立USB接收 消息队列
		USBSendQ    = OSQCreate(&usbSendBuf[0],USBSENDBUF_SIZE);//建立USB发送 消息队列
		
		OSTaskCreate(USBUpdataTask,(void *)0,(OS_STK*)&USB_TASK_STK[USB_STK_SIZE-1],USB_TASK_PRIO);
		OSTaskSuspend(OS_PRIO_SELF);//挂起起始任务
	}else
	{
		OS_ENTER_CRITICAL();			//进入临界区(无法被中断打断)    
/***********************************  创建任务间通信的消息     ***************************************************/				
		beepSem   = OSSemCreate(1);       //蜂鸣器信号量（目前没啥用，留着以后扩展蜂鸣器响的方式）
		LoginMes  = OSSemCreate(0);       //登录报文信号量
		sendMsg   = OSSemCreate(0);       //创建CDMA是否正在发送消息的信号量
		
		CDMASendMutex  = OSMutexCreate(CDMA_SEND_PRIO,&err);       //向CDMA发送缓冲区发送数据 独占 互斥型信号量
		CDMAPowerMutex = OSMutexCreate(CDMAPOWER_PRIO,&err);       //CDMA电源互斥信号量管理
		
		CDMARecieveQ = OSQCreate(&cdmaRecBuf[0],CDMARECBUF_SIZE);  //建立CDMA接收 消息队列
		CDMASendQ    = OSQCreate(&cdmaSendBuf[0],CDMASENDBUF_SIZE);//建立CDMA发送 消息队列
		ZIPRecv_Q    = OSQCreate(&ZIPRecBuf[0],ZIPRECVBUF_SIZE);   //建立“ZIPRECV”处理消息队列
		
		receGPSQ     = OSQCreate(&gpsRecBuf[0],GPSRECBUF_SIZE);    //建立GPS接收 消息队列
		canSendQ    = OSQCreate(&canSendBuf[0],CANSENDBUF_SIZE);   //卡路宝向ECU发送指令的消息队列
		canRecieveQ = OSQCreate(&canRecBuf[0],CANRECBUF_SIZE);     //卡路宝从ECU接收指令的循环队列
		canJ1939Q   = OSQCreate(&canJ1939Buf[0],CANJ1939BUF_SIZE); //ECU向卡路宝发送J1939消息队列
/*************************************      创建各任务           ********************************************************/		
		OSTaskCreate(PowerDeal,(void *)0,(OS_STK*)&POWER_TASK_STK[POWER_STK_SIZE-1],POWER_TASK_PRIO);
		
		OSTaskCreate(CDMATask,(void *)0,(OS_STK*)&CDMA_TASK_STK[CDMA_STK_SIZE-1],CDMA_TASK_PRIO);
		
		OSTaskCreate(CDMARecvTask,(void *)0,(OS_STK*)&CDMARecv_TASK_STK[CDMARecv_STK_SIZE-1],CDMARevc_TASK_PRIO);	
		
		OSTaskCreate(GPSTask, (void *)0,(OS_STK*)&GPS_TASK_STK[GPS_STK_SIZE-1],GPS_TASK_PRIO);	
		
		OSTaskCreate(OBDTask, (void *)0,(OS_STK*)&OBD_TASK_STK[OBD_STK_SIZE-1],OBD_TASK_PRIO);
		OSTaskCreate(DealJ1939Date, (void *)0,(OS_STK*)&J1939_TASK_STK[J1939_STK_SIZE-1],J1939_TASK_PRIO);//创建J1939处理任务		
		
		OSTaskCreate(CDMALEDTask,(void *)0,(OS_STK*)&CDMA_LED_STK[LED_STK_SIZE-1],CDMA_LED_PRIO);
		OSTaskCreate(GPSLEDTask, (void *)0,(OS_STK*)&GPS_LED_STK[LED_STK_SIZE-1], GPS_LED_PRIO);		
		OSTaskCreate(OBDLEDTask, (void *)0,(OS_STK*)&OBD_LED_STK[LED_STK_SIZE-1], OBD_LED_PRIO);	
		OSTaskCreate(BeepTask,   (void *)0,(OS_STK*)&BEEP_STK[BEEP_STK_SIZE-1],   BEEP_TASK_PRIO);
		
		OS_EXIT_CRITICAL();				//退出临界区(可以被中断打断)
	}
	while(1)
	{
		
		OSTimeDlyHMSM(0,0,0,4);         //4ms扫描一次
		if(varOperation.isDataFlow == 1)
			continue;
		timeToSendLogin++;
		if(timeToSendLogin % 45000 == 0)//定期3分钟发送登录报文
		{
			LoginDataSend(); 
		}
		if(varOperation.canTest == 2)   //CAN的波特率和ID均已确定
		{
			for(i=0;i<varOperation.pidNum;i++)//todo:PID指令的数目 后期需要配置
			{
				(ptrPIDAllDat + i)->timeCount += 4;
				if((ptrPIDAllDat + i)->timeCount >=(ptrPIDAllDat + i)->period)
				{
					(ptrPIDAllDat + i)->timeCount = 0;
					ptrOBDSend = Mem_malloc(9);
					memcpy(ptrOBDSend,(ptrPIDAllDat + i)->data,9);
					err = OSQPost(canSendQ,ptrOBDSend);//向OBD推送要发送的PID指令
					if(err != OS_ERR_NONE)
						Mem_free(ptrOBDSend);          //推送不成功，需要释放内存块
				}
			}
		}
		if(cdmaDataToSend->datLength > 36)//要发送的数据不为空
			cdmaDataToSend->timeCount += 4;
		if((cdmaDataToSend->timeCount >= 3000) || (cdmaDataToSend->datLength >= 850))//发送时间到或者要发送的数组长度超过850个字节
		{
			MemLog(cdmaDataToSend);//todo：调试时用，产品的时候要注释掉
			
			OSMutexPend(CDMASendMutex,0,&err);
			
			CDMASendDataPack(cdmaDataToSend);
			
			err = OSQPost(CDMASendQ,cdmaDataToSend);
			if(err != OS_ERR_NONE)
			{
				cdmaDataToSend->datLength = 27;
				cdmaDataToSend->timeCount = 0;
			}
			else
			{
				cdmaDataToSend = CDMNSendDataInit(1000);
			}	
			
			OSMutexPost(CDMASendMutex);
		}
	}
}




