#include "bsp.h"
#include "includes.h"
#include "apptask.h"
#include "delay.h"


OS_STK START_TASK_STK[START_STK_SIZE];//��ʼ�����ջ

OS_STK CDMA_TASK_STK[CDMA_STK_SIZE];  //����ͨѶCDMA�����ջ
OS_STK GPS_TASK_STK[GPS_STK_SIZE];    //������λGPS�����ջ
OS_STK OBD_TASK_STK[OBD_STK_SIZE];    //�������OBD�����ջ

OS_STK CDMA_LED_STK[LED_STK_SIZE];  //����ͨѶCDMA-LED�����ջ
OS_STK GPS_LED_STK[LED_STK_SIZE];   //������λGPS-LED�����ջ
OS_STK OBD_LED_STK[LED_STK_SIZE];   //�������OBD-LED�����ջ

void StartTask(void *pdata);          
void CDMALEDTask(void *pdata); 
void GPSLEDTask(void *pdata); 
void OBDLEDTask(void *pdata); 

pCIR_QUEUE sendCDMA_Q = NULL;     //ָ�� CDMA ���ڷ��Ͷ���  ��ָ��
pSTORE     receCDMA_S = NULL;     //ָ�� CDMA ���ڽ������ݶѵ�ָ��

pCIR_QUEUE sendGPS_Q = NULL;     //ָ�� GPS ���ڷ��Ͷ���  ��ָ��
pSTORE     receGPS_S = NULL;     //ָ�� GPS ���ڽ������ݶѵ�ָ��

_SystemInformation sysAllData;

int main(void )
{
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
	
	OSInit(); 
	
	MemBuf_Init();   //�����ڴ��
	
	sendCDMA_Q = Cir_Queue_Init(1000);//CDMA ���ڷ��� ѭ������
	receCDMA_S = Store_Init(1024);   //CDMA ���ڽ��� ���ݶ�
	
	sendGPS_Q = Cir_Queue_Init(230); //GPS  ���ڷ��� ѭ������
	receGPS_S = Store_Init(230);     //GPS  ���ڽ��� ���ݶ�
	
	SystemBspInit();
	 
	OSTaskCreate(StartTask,(void *)0,(OS_STK *)&START_TASK_STK[START_STK_SIZE-1],START_TASK_PRIO );
	
	OSStart();	 
}
#define SEND_MAX_TIME  3000     //3000ms��ʱʱ�䵽����������
_CDMADataToSend* cdmaDataToSend = NULL;//CDMA���͵������У�OBD��GPS������ͨ��������Ϊ����

OS_EVENT * CDMASendMutex;//�����������ź�����������ռ���� �������������Ϣ
extern OS_EVENT *canSendQ;        //��OBD����PIDָ��
extern OS_EVENT *CDMASendQ;       //ͨ��CDMA����������Ͳɼ�����OBD��GPS����
extern _OBD_PID_Cmd  obdPIDAll[];
void StartTask(void *pdata)
{
	uint8_t err;
	uint8_t i = 0;
	uint8_t *ptrOBDSend;
	
	OS_CPU_SR cpu_sr=0;
	pdata = pdata; 
	sysAllData.sendId = 0x80000000;     //ȫ�ַ���ָ����ˮ��
	
	cdmaDataToSend = CDMNSendDataInit();//��ʼ����ȡ����CDMA����Ϣ�ṹ��
	
  	OS_ENTER_CRITICAL();			//�����ٽ���(�޷����жϴ��)    
	
 	OSTaskCreate(CDMATask,(void *)0,(OS_STK*)&CDMA_TASK_STK[CDMA_STK_SIZE-1],CDMA_TASK_PRIO);						   
 	OSTaskCreate(GPSTask, (void *)0,(OS_STK*)&GPS_TASK_STK[GPS_STK_SIZE-1],GPS_TASK_PRIO);		
 	OSTaskCreate(OBDTask, (void *)0,(OS_STK*)&OBD_TASK_STK[OBD_STK_SIZE-1],OBD_TASK_PRIO);	

	OSTaskCreate(CDMALEDTask,(void *)0,(OS_STK*)&CDMA_LED_STK[LED_STK_SIZE-1],CDMA_LED_PRIO);						   
 	OSTaskCreate(GPSLEDTask,(void *)0,(OS_STK*)&GPS_LED_STK[LED_STK_SIZE-1],GPS_LED_PRIO);		
 	OSTaskCreate(OBDLEDTask,(void *)0,(OS_STK*)&OBD_LED_STK[LED_STK_SIZE-1],OBD_LED_PRIO);		
	
	CDMASendMutex = OSMutexCreate(CDMA_SEND_PRIO,&err);//��CDMA���ͻ������������� ��ռ �������ź���
	
	OS_EXIT_CRITICAL();				//�˳��ٽ���(���Ա��жϴ��)
	OSTimeDlyHMSM(0,0,15,4);        //todo:�ȴ�����������ɳ�ʼ��֮���ٽ������ݵ�����
	while(1)
	{
		OSTimeDlyHMSM(0,0,0,4);
		//todo:����OBD��PIDָ����ʱ�䵽�����OBD���Ͷ��У�������ECUͨ��
		for(i=0;i<10;i++)          //todo:PIDָ�����Ŀ ������Ҫ����
		{
			obdPIDAll[i].timeCount += 4;
			if(obdPIDAll[i].timeCount >= obdPIDAll[i].period)
			{
				obdPIDAll[i].timeCount = 0;
				
				ptrOBDSend = Mem_malloc(9);
				memcpy(ptrOBDSend,obdPIDAll[i].data,9);
				err = OSQPost(canSendQ,ptrOBDSend);  //��OBD����Ҫ���͵�PIDָ��
				if(err != OS_ERR_NONE)
					Mem_free(ptrOBDSend);//���Ͳ��ɹ�����Ҫ�ͷ��ڴ��
			}
		}
		if(cdmaDataToSend->datLength > 28)
			cdmaDataToSend->timeCount += 4;
		
		if((cdmaDataToSend->timeCount >= 3000) || (cdmaDataToSend->datLength >= 850))
		{
			OSMutexPend(CDMASendMutex,0,&err);
			//todo:��Ҫ���͵�����ʱ������2������3000������������Ϊ0��ʱ����Ҫ�������,���������ݣ������г�ʼ��
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


