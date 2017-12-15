#include "bsp.h"
#include "includes.h"
#include "apptask.h"
#include "delay.h"
#include "obd.h"

/***********************  ȫ�� �ź������������ź�������Ϣ����    ***********************/

#define CDMARECBUF_SIZE   10        //CDMA������Ϣ���б�����Ϣ�������
#define CDMASENDBUF_SIZE  5         //CDMA������Ϣ���б�����Ϣ�������
#define ZIPRECVBUF_SIZE   5         //RECV������Ϣ���б�����Ϣ�������
#define GPSRECBUF_SIZE    10        //����GPS��Ϣ���б�����Ϣ�������
#define CANRECBUF_SIZE    20        //CAN������Ϣ���б�����Ϣ�������
#define CANSENDBUF_SIZE   80        //CAN������Ϣ���б�����Ϣ�������
#define CANJ1939BUF_SIZE  20        //CAN����J1939��Ϣ���б�����Ϣ�������
#define USBRECBUF_SIZE    10        //������Ϣ���б�����Ϣ�������
#define USBSENDBUF_SIZE   5         //������Ϣ���б�����Ϣ�������

void *cdmaRecBuf[CDMARECBUF_SIZE];  //ָ�� CDMA ������Ϣ��ָ������
void *cdmaSendBuf[CDMASENDBUF_SIZE];//ָ�� CDMA ������Ϣ��ָ������
void *ZIPRecBuf[ZIPRECVBUF_SIZE];   //ָ�� CDMA ���յ����������ݵ���Ϣָ������
void *gpsRecBuf[GPSRECBUF_SIZE];    //ָ�� GPS  ������Ϣ��ָ������
void *canRecBuf[CANRECBUF_SIZE];    //ָ�� CAN  ������Ϣ��ָ������
void *canSendBuf[CANSENDBUF_SIZE];  //ָ�� CAN  ������Ϣ��ָ������
void *canJ1939Buf[CANJ1939BUF_SIZE];//ָ�� CAN SAE-J1939������Ϣ��ָ������
void *usbRecBuf[USBRECBUF_SIZE];    //���ڴ��ָ�������ָ��
void *usbSendBuf[USBSENDBUF_SIZE];  //���ڴ��ָ�������ָ��

OS_EVENT * sendMsg;               //CDMA�Ƿ����ڷ�����Ϣ���ź���
OS_EVENT * beepSem;               //�������������ź���
OS_EVENT * LoginMes;              //��¼�����ź���

OS_EVENT * CDMASendMutex;         //�����������ź�����������ռ�������������Ϣ
OS_EVENT * CDMAPowerMutex;        //CDMA ��Դ�������ź���

OS_EVENT * CDMARecieveQ;          //CDMA ������Ϣ���е�ָ��
OS_EVENT * CDMASendQ;             //CDMA ������Ϣ���е�ָ��
OS_EVENT * ZIPRecv_Q;             //ָ��RECV��Ϣ���е�ָ��

OS_EVENT * receGPSQ;              //GPS  ������Ϣ����ָ��

OS_EVENT * canRecieveQ;           //CAN  ���߽�����Ϣ���е�ָ��
OS_EVENT * canSendQ;              //CAN  ���߷�����Ϣ���е�ָ��
OS_EVENT * canJ1939Q;             //SAE-J1939������Ϣ���е�ָ��

OS_EVENT * USBSendQ;              //USB  ������Ϣ���е�ָ��
OS_EVENT * USBRecieveQ;           //USB  ������Ϣ���е�ָ��

/*************************   �����ջ   ******************************/

OS_STK USB_TASK_STK[USB_STK_SIZE];     //USB���������ջ
OS_STK START_TASK_STK[START_STK_SIZE]; //��ʼ�����ջ

OS_STK CDMA_TASK_STK[CDMA_STK_SIZE];   //����ͨѶCDMA�����ջ
OS_STK CDMARecv_TASK_STK[CDMARecv_STK_SIZE];//����ͨѶCDMA���շ��������������ջ

OS_STK GPS_TASK_STK[GPS_STK_SIZE];     //������λGPS�����ջ

OS_STK OBD_TASK_STK[OBD_STK_SIZE];     //�������OBD�����ջ
OS_STK J1939_TASK_STK[J1939_STK_SIZE]; //SAE - J1939�����ջ

OS_STK POWER_TASK_STK[POWER_STK_SIZE]; //ϵͳ��Դ���������ջ

OS_STK CDMA_LED_STK[LED_STK_SIZE];     //����ͨѶCDMA-LED�����ջ
OS_STK GPS_LED_STK[LED_STK_SIZE];      //������λGPS-LED�����ջ
OS_STK OBD_LED_STK[LED_STK_SIZE];      //�������OBD-LED�����ջ
OS_STK BEEP_STK[BEEP_STK_SIZE];        //�����������ջ

/******** �����ڽ��ա����ͻ�������ʼ��   **************/

pCIR_QUEUE sendCDMA_Q = NULL;     //ָ�� CDMA ���ڷ��Ͷ���  ��ָ��
pSTORE     receCDMA_S = NULL;     //ָ�� CDMA ���ڽ������ݶѵ�ָ��

pCIR_QUEUE sendGPS_Q = NULL;      //ָ�� GPS ���ڷ��Ͷ���  ��ָ��
pSTORE     receGPS_S = NULL;      //ָ�� GPS ���ڽ������ݶѵ�ָ��

/***********************  ϵͳȫ�ֱ���  *****************************/

_SystemInformation sysUpdateVar;  //��������������
SYS_OperationVar   varOperation;  //�������е�ȫ�ֱ�������
CARRunRecord       carAllRecord;  //��������״̬��Ϣ
nmea_msg           gpsMC;         //GPS��Ϣ

_CDMADataToSend*   cdmaDataToSend = NULL;  //CDMAl���͵������У�OBD��GPS������ͨ��������Ϊ����

/****************************************************************
*			void	int main(void )
* ��	�� : ������ں���	 		
* �������  : ��
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
	CARVarInit();                     //�복����ʻ��ؽṹ��ĳ�ʼ��
	OSTaskCreate(StartTask,(void *)0,(OS_STK *)&START_TASK_STK[START_STK_SIZE-1],START_TASK_PRIO );
	
	OSStart();	 
}

/****************************************************************
*			void StartTask(void *pdata)
* ��	�� : ��ʼ��ʼ�����񣬸��𴴽����������ͨ�š�CAN PIDɨ�衢��CDMA�ϱ����� 		
* �������  : ��
* �� �� ֵ : int
****************************************************************/
#define SEND_MAX_TIME  3000              //3000ms��ʱʱ�䵽����������
extern _OBD_PID_Cmd *ptrPIDAllDat;   

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
		USBRecieveQ = OSQCreate(&usbRecBuf[0],USBRECBUF_SIZE);  //����USB���� ��Ϣ����
		USBSendQ    = OSQCreate(&usbSendBuf[0],USBSENDBUF_SIZE);//����USB���� ��Ϣ����
		
		OSTaskCreate(USBUpdataTask,(void *)0,(OS_STK*)&USB_TASK_STK[USB_STK_SIZE-1],USB_TASK_PRIO);
		OSTaskSuspend(OS_PRIO_SELF);//������ʼ����
	}else
	{
		OS_ENTER_CRITICAL();			//�����ٽ���(�޷����жϴ��)    
/***********************************  ���������ͨ�ŵ���Ϣ     ***************************************************/				
		beepSem   = OSSemCreate(1);       //�������ź�����Ŀǰûɶ�ã������Ժ���չ��������ķ�ʽ��
		LoginMes  = OSSemCreate(0);       //��¼�����ź���
		sendMsg   = OSSemCreate(0);       //����CDMA�Ƿ����ڷ�����Ϣ���ź���
		
		CDMASendMutex  = OSMutexCreate(CDMA_SEND_PRIO,&err);       //��CDMA���ͻ������������� ��ռ �������ź���
		CDMAPowerMutex = OSMutexCreate(CDMAPOWER_PRIO,&err);       //CDMA��Դ�����ź�������
		
		CDMARecieveQ = OSQCreate(&cdmaRecBuf[0],CDMARECBUF_SIZE);  //����CDMA���� ��Ϣ����
		CDMASendQ    = OSQCreate(&cdmaSendBuf[0],CDMASENDBUF_SIZE);//����CDMA���� ��Ϣ����
		ZIPRecv_Q    = OSQCreate(&ZIPRecBuf[0],ZIPRECVBUF_SIZE);   //������ZIPRECV��������Ϣ����
		
		receGPSQ     = OSQCreate(&gpsRecBuf[0],GPSRECBUF_SIZE);    //����GPS���� ��Ϣ����
		canSendQ    = OSQCreate(&canSendBuf[0],CANSENDBUF_SIZE);   //��·����ECU����ָ�����Ϣ����
		canRecieveQ = OSQCreate(&canRecBuf[0],CANRECBUF_SIZE);     //��·����ECU����ָ���ѭ������
		canJ1939Q   = OSQCreate(&canJ1939Buf[0],CANJ1939BUF_SIZE); //ECU��·������J1939��Ϣ����
/*************************************      ����������           ********************************************************/		
		OSTaskCreate(PowerDeal,(void *)0,(OS_STK*)&POWER_TASK_STK[POWER_STK_SIZE-1],POWER_TASK_PRIO);
		
		OSTaskCreate(CDMATask,(void *)0,(OS_STK*)&CDMA_TASK_STK[CDMA_STK_SIZE-1],CDMA_TASK_PRIO);
		
		OSTaskCreate(CDMARecvTask,(void *)0,(OS_STK*)&CDMARecv_TASK_STK[CDMARecv_STK_SIZE-1],CDMARevc_TASK_PRIO);	
		
		OSTaskCreate(GPSTask, (void *)0,(OS_STK*)&GPS_TASK_STK[GPS_STK_SIZE-1],GPS_TASK_PRIO);	
		
		OSTaskCreate(OBDTask, (void *)0,(OS_STK*)&OBD_TASK_STK[OBD_STK_SIZE-1],OBD_TASK_PRIO);
		OSTaskCreate(DealJ1939Date, (void *)0,(OS_STK*)&J1939_TASK_STK[J1939_STK_SIZE-1],J1939_TASK_PRIO);//����J1939��������		
		
		OSTaskCreate(CDMALEDTask,(void *)0,(OS_STK*)&CDMA_LED_STK[LED_STK_SIZE-1],CDMA_LED_PRIO);
		OSTaskCreate(GPSLEDTask, (void *)0,(OS_STK*)&GPS_LED_STK[LED_STK_SIZE-1], GPS_LED_PRIO);		
		OSTaskCreate(OBDLEDTask, (void *)0,(OS_STK*)&OBD_LED_STK[LED_STK_SIZE-1], OBD_LED_PRIO);	
		OSTaskCreate(BeepTask,   (void *)0,(OS_STK*)&BEEP_STK[BEEP_STK_SIZE-1],   BEEP_TASK_PRIO);
		
		OS_EXIT_CRITICAL();				//�˳��ٽ���(���Ա��жϴ��)
	}
	while(1)
	{
		OSTimeDlyHMSM(0,0,0,4);         //4msɨ��һ��
		if(varOperation.isDataFlow == 1)
			continue;
		timeToSendLogin++;
		if(timeToSendLogin % 45000 == 0)//����3���ӷ��͵�¼����
		{
			LoginDataSend(); 
		}
		if(varOperation.canTest == 2)   //CAN�Ĳ����ʺ�ID����ȷ��
		{
			for(i=0;i<varOperation.pidNum;i++)//todo:PIDָ�����Ŀ ������Ҫ����
			{
				(ptrPIDAllDat + i)->timeCount += 4;
				if((ptrPIDAllDat + i)->timeCount < (ptrPIDAllDat + i)->period)
					continue;
				(ptrPIDAllDat + i)->timeCount = 0;
				ptrOBDSend = Mem_malloc(9);
				memcpy(ptrOBDSend,(ptrPIDAllDat + i)->data,9);
				err = OSQPost(canSendQ,ptrOBDSend);//��OBD����Ҫ���͵�PIDָ��
				if(err != OS_ERR_NONE)
					Mem_free(ptrOBDSend);          //���Ͳ��ɹ�����Ҫ�ͷ��ڴ��
			}
		}
		if(cdmaDataToSend->datLength > 36)         //Ҫ���͵����ݲ�Ϊ��
			cdmaDataToSend->timeCount += 4;
		if((cdmaDataToSend->timeCount >= 3000) || (cdmaDataToSend->datLength >= 850))//����ʱ�䵽����Ҫ���͵����鳤�ȳ���850���ֽ�
		{
			MemLog(cdmaDataToSend);                //todo������ʱ�ã���Ʒ��ʱ��Ҫע�͵�
			J1939DataLog();                        
			
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





