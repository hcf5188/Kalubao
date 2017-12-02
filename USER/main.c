#include "bsp.h"
#include "includes.h"
#include "apptask.h"
#include "delay.h"




OS_STK USB_TASK_STK[USB_STK_SIZE];    //USB升级任务堆栈
OS_STK START_TASK_STK[START_STK_SIZE];//起始任务堆栈
OS_STK CDMA_TASK_STK[CDMA_STK_SIZE];  //网络通讯CDMA任务堆栈
OS_STK CDMARecv_TASK_STK[CDMARecv_STK_SIZE];  //网络通讯CDMA任务堆栈

OS_STK GPS_TASK_STK[GPS_STK_SIZE];    //车辆定位GPS任务堆栈
OS_STK OBD_TASK_STK[OBD_STK_SIZE];    //汽车诊断OBD任务堆栈

OS_STK CDMA_LED_STK[LED_STK_SIZE];  //网络通讯CDMA-LED任务堆栈
OS_STK GPS_LED_STK[LED_STK_SIZE];   //车辆定位GPS-LED任务堆栈
OS_STK OBD_LED_STK[LED_STK_SIZE];   //汽车诊断OBD-LED任务堆栈
OS_STK BEEP_STK[BEEP_STK_SIZE];     //蜂鸣器任务堆栈


void StartTask  (void *pdata);          
void CDMALEDTask(void *pdata); 
void GPSLEDTask (void *pdata); 
void OBDLEDTask (void *pdata); 
void BeepTask   (void *pdata); 
extern void USBUpdataTask (void *pdata);//USB 升级任务声明


pCIR_QUEUE sendCDMA_Q = NULL;     //指向 CDMA 串口发送队列  的指针
pSTORE     receCDMA_S = NULL;     //指向 CDMA 串口接收数据堆的指针

pCIR_QUEUE sendGPS_Q = NULL;      //指向 GPS 串口发送队列  的指针
pSTORE     receGPS_S = NULL;      //指向 GPS 串口接收数据堆的指针

/****************************************************************
*			void	int main(void )
* 描	述 : 入口函数	 		
* 输入参数 : 无
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

static void UpLog(_CDMADataToSend* ptr);//日志文件
#define SEND_MAX_TIME  3000     //3000ms计时时间到，则发送数据
_CDMADataToSend* cdmaDataToSend = NULL;//CDMA发送的数据中（OBD、GPS），是通过它来作为载体

OS_EVENT * CDMASendMutex;            //建立互斥型信号量，用来独占处理 发向服务器的消息
OS_EVENT * CDMAPowerMutex;           //CDMA 电源互斥型信号量
OS_EVENT * beepSem;                  //建立蜂鸣器响信号量
extern OS_EVENT *canSendQ;           //向OBD发送PID指令
extern OS_EVENT *CDMASendQ;          //通过CDMA向服务器发送采集到的OBD、GPS数据
extern _OBD_PID_Cmd *ptrPIDAllDat;
extern MEM_Check allMemState;        //内存监控变量
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
		OSTaskCreate(USBUpdataTask,(void *)0,(OS_STK*)&USB_TASK_STK[USB_STK_SIZE-1],USB_TASK_PRIO);
		OSTaskSuspend(OS_PRIO_SELF);
	}else
	{
		OS_ENTER_CRITICAL();			//进入临界区(无法被中断打断)    
		
		OSTaskCreate(CDMATask,(void *)0,(OS_STK*)&CDMA_TASK_STK[CDMA_STK_SIZE-1],CDMA_TASK_PRIO);
		
		OSTaskCreate(CDMARecvTask,(void *)0,(OS_STK*)&CDMARecv_TASK_STK[CDMARecv_STK_SIZE-1],CDMARevc_TASK_PRIO);	
		
		OSTaskCreate(GPSTask, (void *)0,(OS_STK*)&GPS_TASK_STK[GPS_STK_SIZE-1],GPS_TASK_PRIO);		
		OSTaskCreate(OBDTask, (void *)0,(OS_STK*)&OBD_TASK_STK[OBD_STK_SIZE-1],OBD_TASK_PRIO);	
		OSTaskCreate(CDMALEDTask,(void *)0,(OS_STK*)&CDMA_LED_STK[LED_STK_SIZE-1],CDMA_LED_PRIO);
		
		OSTaskCreate(GPSLEDTask, (void *)0,(OS_STK*)&GPS_LED_STK[LED_STK_SIZE-1], GPS_LED_PRIO);		
		OSTaskCreate(OBDLEDTask, (void *)0,(OS_STK*)&OBD_LED_STK[LED_STK_SIZE-1], OBD_LED_PRIO);	
		OSTaskCreate(BeepTask,   (void *)0,(OS_STK*)&BEEP_STK[BEEP_STK_SIZE-1],   BEEP_TASK_PRIO);
		
		OSTaskCreate(BeepTask,   (void *)0,(OS_STK*)&BEEP_STK[BEEP_STK_SIZE-1],   BEEP_TASK_PRIO);
			
		CDMASendMutex  = OSMutexCreate(CDMA_SEND_PRIO,&err);//向CDMA发送缓冲区发送数据 独占 互斥型信号量
		CDMAPowerMutex = OSMutexCreate(CDMAPOWER_PRIO,&err);//CDMA电源独占管理
		
		OS_EXIT_CRITICAL();				//退出临界区(可以被中断打断)
		beepSem = OSSemCreate(1);       //蜂鸣器信号量
	}
  	
	while(1)
	{
		
		OSTimeDlyHMSM(0,0,0,4);
		if(varOperation.isDataFlow == 1)
			continue;
		timeToSendLogin++;
		if(timeToSendLogin % 45000 == 0)//定期3分钟发送登录报文
		{
			LoginDataSend(); 
		}

		for(i=0;i<varOperation.pidNum;i++)          //todo:PID指令的数目 后期需要配置
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
		
		if(cdmaDataToSend->datLength > 36)
			cdmaDataToSend->timeCount += 4;
		if((cdmaDataToSend->timeCount >= 3000) || (cdmaDataToSend->datLength >= 850))
		{
			OSMutexPend(CDMASendMutex,0,&err);
			
			UpLog(cdmaDataToSend);
			
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
//上报内存使用日志文件  用于前期研发阶段的内存块使用情况监视
static void UpLog(_CDMADataToSend* ptr)
{
	ptr->data[ptr->datLength++] = 17;
	ptr->data[ptr->datLength++] = 0x50;
	ptr->data[ptr->datLength++] = 0x03;
	ptr->data[ptr->datLength++] = allMemState.memUsedNum1;
	ptr->data[ptr->datLength++] = allMemState.memUsedMax1;
	ptr->data[ptr->datLength++] = allMemState.memUsedNum2;
	ptr->data[ptr->datLength++] = allMemState.memUsedMax2;
	ptr->data[ptr->datLength++] = allMemState.memUsedNum3;
	ptr->data[ptr->datLength++] = allMemState.memUsedMax3;
	ptr->data[ptr->datLength++] = allMemState.memUsedNum4;
	ptr->data[ptr->datLength++] = allMemState.memUsedMax4;
	ptr->data[ptr->datLength++] = allMemState.memUsedNum5;
	ptr->data[ptr->datLength++] = allMemState.memUsedMax5;
	ptr->data[ptr->datLength++] = allMemState.memUsedNum6;
	ptr->data[ptr->datLength++] = allMemState.memUsedMax6;
	ptr->data[ptr->datLength++] = allMemState.memUsedNum7;
	ptr->data[ptr->datLength++] = allMemState.memUsedMax7;	
}


/***********************************************************
*                LED、蜂鸣器控制任务
*
*
*
*
*************************************************************/

uint16_t freCDMALed = 100;//黄灯
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
uint16_t freGPSLed = 100;//绿灯
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
uint16_t freOBDLed = 100;//红灯
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

static void BSP_BeeperTimerInit(uint16_t ck_value)
{
	TIM_TimeBaseInitTypeDef TIM_BaseInitStructure;
    TIM_OCInitTypeDef TIM_OCInitStructure;

	GPIO_InitTypeDef GPIO_InitStructure;

	uint16_t period=(18000000/ck_value);
	uint16_t period_set=period-1;
	uint16_t pluse=period/2-1;
		
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB  | RCC_APB2Periph_AFIO, ENABLE); 
	GPIO_PinRemapConfig(GPIO_PartialRemap_TIM3, ENABLE);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
  	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
  	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  	GPIO_Init(GPIOB,&GPIO_InitStructure);

	//Timer3CH2 Beeper
	TIM_InternalClockConfig(TIM3);


	TIM_BaseInitStructure.TIM_Prescaler=3;//4分频，18M
	TIM_BaseInitStructure.TIM_CounterMode=TIM_CounterMode_Up;
	TIM_BaseInitStructure.TIM_Period=period_set;
	TIM_BaseInitStructure.TIM_ClockDivision=TIM_CKD_DIV1;
	TIM_BaseInitStructure.TIM_RepetitionCounter=0;
	TIM_TimeBaseInit(TIM3,&TIM_BaseInitStructure);
	TIM_ARRPreloadConfig(TIM3, DISABLE);

	TIM_Cmd(TIM3, ENABLE);  //使能TIMx外设
	
	TIM_OCInitStructure.TIM_OCMode=TIM_OCMode_PWM1;
	TIM_OCInitStructure.TIM_OutputState=TIM_OutputState_Enable;
	TIM_OCInitStructure.TIM_Pulse=pluse;
	TIM_OCInitStructure.TIM_OCPolarity=TIM_OCPolarity_High;
	TIM_OC3Init(TIM3,&TIM_OCInitStructure);
	TIM_CtrlPWMOutputs(TIM3,ENABLE);
}
/* 关闭蜂鸣器 */
void BSP_BeeperTimer_Off(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	TIM_CtrlPWMOutputs(TIM3,DISABLE);
	TIM_Cmd(TIM3,DISABLE);  //使能TIMx外设
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, DISABLE);
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
  	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
  	GPIO_Init(GPIOB,&GPIO_InitStructure);
	GPIO_ResetBits(GPIOB, GPIO_Pin_0);
}
void BeepTask(void *pdata)
{
	uint8_t err;
	while(1)
	{
		OSSemPend(beepSem,0,&err);//想让蜂鸣器响，发送个信号量即可

		BSP_BeeperTimerInit(1000);//打开蜂鸣器，频率可以设定
		OSTimeDlyHMSM(0,0,0,300);
		BSP_BeeperTimer_Off();    //关闭蜂鸣器
	}
}


