#include "bsp.h"
#include "includes.h"
#include "apptask.h"
#include "delay.h"




OS_STK USB_TASK_STK[USB_STK_SIZE];    //USB���������ջ
OS_STK START_TASK_STK[START_STK_SIZE];//��ʼ�����ջ
OS_STK CDMA_TASK_STK[CDMA_STK_SIZE];  //����ͨѶCDMA�����ջ
OS_STK CDMARecv_TASK_STK[CDMARecv_STK_SIZE];  //����ͨѶCDMA�����ջ

OS_STK GPS_TASK_STK[GPS_STK_SIZE];    //������λGPS�����ջ
OS_STK OBD_TASK_STK[OBD_STK_SIZE];    //�������OBD�����ջ

OS_STK CDMA_LED_STK[LED_STK_SIZE];  //����ͨѶCDMA-LED�����ջ
OS_STK GPS_LED_STK[LED_STK_SIZE];   //������λGPS-LED�����ջ
OS_STK OBD_LED_STK[LED_STK_SIZE];   //�������OBD-LED�����ջ
OS_STK BEEP_STK[BEEP_STK_SIZE];     //�����������ջ


void StartTask  (void *pdata);          
void CDMALEDTask(void *pdata); 
void GPSLEDTask (void *pdata); 
void OBDLEDTask (void *pdata); 
void BeepTask   (void *pdata); 
extern void USBUpdataTask (void *pdata);//USB ������������


pCIR_QUEUE sendCDMA_Q = NULL;     //ָ�� CDMA ���ڷ��Ͷ���  ��ָ��
pSTORE     receCDMA_S = NULL;     //ָ�� CDMA ���ڽ������ݶѵ�ָ��

pCIR_QUEUE sendGPS_Q = NULL;      //ָ�� GPS ���ڷ��Ͷ���  ��ָ��
pSTORE     receGPS_S = NULL;      //ָ�� GPS ���ڽ������ݶѵ�ָ��

/****************************************************************
*			void	int main(void )
* ��	�� : ��ں���	 		
* ������� : ��
* �� �� ֵ : int
****************************************************************/
int main(void )
{
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
	
	OSInit(); 
	
	MemBuf_Init();   //�����ڴ��
	
	sendCDMA_Q = Cir_Queue_Init(1000);//CDMA ���ڷ��� ѭ������ ������
	receCDMA_S = Store_Init(1020);    //CDMA ���ڽ��� ���ݶ�   ������
	
	sendGPS_Q = Cir_Queue_Init(230);  //GPS  ���ڷ��� ѭ������ ������
	receGPS_S = Store_Init(230);      //GPS  ���ڽ��� ���ݶ�   ������
	
	
	SystemBspInit();                  //Ӳ����ʼ�� 
	OSTaskCreate(StartTask,(void *)0,(OS_STK *)&START_TASK_STK[START_STK_SIZE-1],START_TASK_PRIO );
	
	OSStart();	 
}

static void UpLog(_CDMADataToSend* ptr);//��־�ļ�
#define SEND_MAX_TIME  3000     //3000ms��ʱʱ�䵽����������
_CDMADataToSend* cdmaDataToSend = NULL;//CDMA���͵������У�OBD��GPS������ͨ��������Ϊ����

OS_EVENT * CDMASendMutex;            //�����������ź�����������ռ���� �������������Ϣ
OS_EVENT * CDMAPowerMutex;           //CDMA ��Դ�������ź���
OS_EVENT * beepSem;                  //�������������ź���
extern OS_EVENT *canSendQ;           //��OBD����PIDָ��
extern OS_EVENT *CDMASendQ;          //ͨ��CDMA����������Ͳɼ�����OBD��GPS����
extern _OBD_PID_Cmd *ptrPIDAllDat;
extern MEM_Check allMemState;        //�ڴ��ر���
void StartTask(void *pdata)
{
	uint8_t err;
	uint8_t i = 0;
	uint8_t *ptrOBDSend;
	uint32_t timeToSendLogin  = 0;
	OS_CPU_SR cpu_sr=0;
	pdata = pdata; 

	
	cdmaDataToSend = CDMNSendDataInit(1000);//��ʼ����ȡ����CDMA����Ϣ�ṹ��
	if(varOperation.USB_NormalMode == 1)//USB ����ģʽ
	{
		OSTaskCreate(USBUpdataTask,(void *)0,(OS_STK*)&USB_TASK_STK[USB_STK_SIZE-1],USB_TASK_PRIO);
		OSTaskSuspend(OS_PRIO_SELF);
	}else
	{
		OS_ENTER_CRITICAL();			//�����ٽ���(�޷����жϴ��)    
		
		OSTaskCreate(CDMATask,(void *)0,(OS_STK*)&CDMA_TASK_STK[CDMA_STK_SIZE-1],CDMA_TASK_PRIO);
		
		OSTaskCreate(CDMARecvTask,(void *)0,(OS_STK*)&CDMARecv_TASK_STK[CDMARecv_STK_SIZE-1],CDMARevc_TASK_PRIO);	
		
		OSTaskCreate(GPSTask, (void *)0,(OS_STK*)&GPS_TASK_STK[GPS_STK_SIZE-1],GPS_TASK_PRIO);		
		OSTaskCreate(OBDTask, (void *)0,(OS_STK*)&OBD_TASK_STK[OBD_STK_SIZE-1],OBD_TASK_PRIO);	
		OSTaskCreate(CDMALEDTask,(void *)0,(OS_STK*)&CDMA_LED_STK[LED_STK_SIZE-1],CDMA_LED_PRIO);
		
		OSTaskCreate(GPSLEDTask, (void *)0,(OS_STK*)&GPS_LED_STK[LED_STK_SIZE-1], GPS_LED_PRIO);		
		OSTaskCreate(OBDLEDTask, (void *)0,(OS_STK*)&OBD_LED_STK[LED_STK_SIZE-1], OBD_LED_PRIO);	
		OSTaskCreate(BeepTask,   (void *)0,(OS_STK*)&BEEP_STK[BEEP_STK_SIZE-1],   BEEP_TASK_PRIO);
		
		OSTaskCreate(BeepTask,   (void *)0,(OS_STK*)&BEEP_STK[BEEP_STK_SIZE-1],   BEEP_TASK_PRIO);
			
		CDMASendMutex  = OSMutexCreate(CDMA_SEND_PRIO,&err);//��CDMA���ͻ������������� ��ռ �������ź���
		CDMAPowerMutex = OSMutexCreate(CDMAPOWER_PRIO,&err);//CDMA��Դ��ռ����
		
		OS_EXIT_CRITICAL();				//�˳��ٽ���(���Ա��жϴ��)
		beepSem = OSSemCreate(1);       //�������ź���
	}
  	
	while(1)
	{
		
		OSTimeDlyHMSM(0,0,0,4);
		if(varOperation.isDataFlow == 1)
			continue;
		timeToSendLogin++;
		if(timeToSendLogin % 45000 == 0)//����3���ӷ��͵�¼����
		{
			LoginDataSend(); 
		}

		for(i=0;i<varOperation.pidNum;i++)          //todo:PIDָ�����Ŀ ������Ҫ����
		{
			(ptrPIDAllDat + i)->timeCount += 4;
			if((ptrPIDAllDat + i)->timeCount >=(ptrPIDAllDat + i)->period)
			{
				(ptrPIDAllDat + i)->timeCount = 0;
				
				ptrOBDSend = Mem_malloc(9);
				memcpy(ptrOBDSend,(ptrPIDAllDat + i)->data,9);
				err = OSQPost(canSendQ,ptrOBDSend);//��OBD����Ҫ���͵�PIDָ��
				if(err != OS_ERR_NONE)
					Mem_free(ptrOBDSend);          //���Ͳ��ɹ�����Ҫ�ͷ��ڴ��
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
//�ϱ��ڴ�ʹ����־�ļ�  ����ǰ���з��׶ε��ڴ��ʹ���������
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
*                LED����������������
*
*
*
*
*************************************************************/

uint16_t freCDMALed = 100;//�Ƶ�
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
uint16_t freGPSLed = 100;//�̵�
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
uint16_t freOBDLed = 100;//���
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


	TIM_BaseInitStructure.TIM_Prescaler=3;//4��Ƶ��18M
	TIM_BaseInitStructure.TIM_CounterMode=TIM_CounterMode_Up;
	TIM_BaseInitStructure.TIM_Period=period_set;
	TIM_BaseInitStructure.TIM_ClockDivision=TIM_CKD_DIV1;
	TIM_BaseInitStructure.TIM_RepetitionCounter=0;
	TIM_TimeBaseInit(TIM3,&TIM_BaseInitStructure);
	TIM_ARRPreloadConfig(TIM3, DISABLE);

	TIM_Cmd(TIM3, ENABLE);  //ʹ��TIMx����
	
	TIM_OCInitStructure.TIM_OCMode=TIM_OCMode_PWM1;
	TIM_OCInitStructure.TIM_OutputState=TIM_OutputState_Enable;
	TIM_OCInitStructure.TIM_Pulse=pluse;
	TIM_OCInitStructure.TIM_OCPolarity=TIM_OCPolarity_High;
	TIM_OC3Init(TIM3,&TIM_OCInitStructure);
	TIM_CtrlPWMOutputs(TIM3,ENABLE);
}
/* �رշ����� */
void BSP_BeeperTimer_Off(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	TIM_CtrlPWMOutputs(TIM3,DISABLE);
	TIM_Cmd(TIM3,DISABLE);  //ʹ��TIMx����
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
		OSSemPend(beepSem,0,&err);//���÷������죬���͸��ź�������

		BSP_BeeperTimerInit(1000);//�򿪷�������Ƶ�ʿ����趨
		OSTimeDlyHMSM(0,0,0,300);
		BSP_BeeperTimer_Off();    //�رշ�����
	}
}


