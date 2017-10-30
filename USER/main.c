
#include "bsp.h"
#include "includes.h"
#include "apptask.h"
#include "delay.h"


OS_STK START_TASK_STK[START_STK_SIZE];//��ʼ�����ջ

OS_STK CDMA_TASK_STK[CDMA_STK_SIZE];  //����ͨѶCDMA�����ջ
OS_STK GPS_TASK_STK[GPS_STK_SIZE];    //������λGPS�����ջ
OS_STK OBD_TASK_STK[OBD_STK_SIZE];    //�������OBD�����ջ

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
	MemBuf_Init();                  //����������ƿ�
  	OS_ENTER_CRITICAL();			//�����ٽ���(�޷����жϴ��)    
	
 	OSTaskCreate(CDMATask,(void *)0,(OS_STK*)&CDMA_TASK_STK[CDMA_STK_SIZE-1],CDMA_TASK_PRIO);						   
 	OSTaskCreate(GPSTask,(void *)0,(OS_STK*)&GPS_TASK_STK[GPS_STK_SIZE-1],GPS_TASK_PRIO);		
 	OSTaskCreate(OBDTask,(void *)0,(OS_STK*)&OBD_TASK_STK[OBD_STK_SIZE-1],OBD_TASK_PRIO);						   
	
    OSTaskSuspend(START_TASK_PRIO);	//������ʼ����.
	OS_EXIT_CRITICAL();				//�˳��ٽ���(���Ա��жϴ��)
	while(1)
	{
		OSTimeDlyHMSM(0,0,0,250);
	}
}



