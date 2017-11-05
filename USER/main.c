
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


int main(void )
{
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
	
	OSInit(); 
	MemBuf_Init();   //�����ڴ���ƿ�
	
	sendCDMA_Q = Cir_Queue_Init(500);//CDMA ���ڷ��� ѭ������
	receCDMA_S = Store_Init(1024);   //CDMA ���ڽ��� ���ݶ�
	
	SystemBspInit();
	 
	
	OSTaskCreate(StartTask,(void *)0,(OS_STK *)&START_TASK_STK[START_STK_SIZE-1],START_TASK_PRIO );
	
	OSStart();	 
}



void StartTask(void *pdata)
{
	OS_CPU_SR cpu_sr=0;
	pdata = pdata; 

  	OS_ENTER_CRITICAL();			//�����ٽ���(�޷����жϴ��)    
	
 	OSTaskCreate(CDMATask,(void *)0,(OS_STK*)&CDMA_TASK_STK[CDMA_STK_SIZE-1],CDMA_TASK_PRIO);						   
 	OSTaskCreate(GPSTask,(void *)0,(OS_STK*)&GPS_TASK_STK[GPS_STK_SIZE-1],GPS_TASK_PRIO);		
 	OSTaskCreate(OBDTask,(void *)0,(OS_STK*)&OBD_TASK_STK[OBD_STK_SIZE-1],OBD_TASK_PRIO);	

	OSTaskCreate(CDMALEDTask,(void *)0,(OS_STK*)&CDMA_LED_STK[LED_STK_SIZE-1],CDMA_LED_PRIO);						   
 	OSTaskCreate(GPSLEDTask,(void *)0,(OS_STK*)&GPS_LED_STK[LED_STK_SIZE-1],GPS_LED_PRIO);		
 	OSTaskCreate(OBDLEDTask,(void *)0,(OS_STK*)&OBD_LED_STK[LED_STK_SIZE-1],OBD_LED_PRIO);		
	
    OSTaskSuspend(START_TASK_PRIO);	//������ʼ����.
	OS_EXIT_CRITICAL();				//�˳��ٽ���(���Ա��жϴ��)
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


