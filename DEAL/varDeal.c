#include "bsp.h"
#include "apptask.h"



#define  FRAME_HEAD_LEN    27     //��ָ�������޹ص�����֡ͷ�����ݳ���


_CDMADataToSend* CDMNSendDataInit(uint16_t length)//Ҫ���͵����ݣ����г�ʼ��
{
	_CDMADataToSend* ptr = NULL;
	ptr = Mem_malloc(sizeof(_CDMADataToSend));
	
	ptr->timeCount = 0;
	ptr->datLength = FRAME_HEAD_LEN;
	ptr->data = Mem_malloc(length);
	
	ptr->data[ptr->datLength ++] = 7;     //ECU �����ļ��汾��
	ptr->data[ptr->datLength ++] = 0x60;
	ptr->data[ptr->datLength ++] = 0x00;
	
	ptr->data[ptr->datLength ++] = (sysUpdateVar.pidVersion >> 24) &0x000000FF;
	ptr->data[ptr->datLength ++] = (sysUpdateVar.pidVersion >> 16) &0x000000FF;
	ptr->data[ptr->datLength ++] = (sysUpdateVar.pidVersion >>  8) &0x000000FF;
	ptr->data[ptr->datLength ++] =  sysUpdateVar.pidVersion & 0x000000FF;

	return ptr;
}
_CDMADataToSend* CDMNSendInfoInit(uint16_t length)//Ҫ���͵����ݣ����г�ʼ��
{
	_CDMADataToSend* ptr = NULL;
	ptr = Mem_malloc(sizeof(_CDMADataToSend));
	
	ptr->timeCount = 0;
	ptr->datLength = FRAME_HEAD_LEN;
	ptr->data = Mem_malloc(length);

	return ptr;
}
uint32_t realTime = 0;
uint16_t crc = 0;
void CDMASendDataPack(_CDMADataToSend* ptr)//���ϴ������ݰ�����֡ͷ��װ��CRCУ���
{
	_PROTOCOL_HEAD *pHead = NULL;
	
	realTime = RTC_GetCounter();//�õ�ϵͳ���е�RTCʱ��
	
	pHead = Mem_malloc(sizeof(_PROTOCOL_HEAD));
	
	pHead->magic    = 0x7E;
	pHead->len      = t_htons(ptr->datLength - 3);   //MAP�����ݳ���
	memcpy(pHead->device,varOperation.imei,16);      //�����豸Ψһ��ʶ�� IMEI
	pHead->msgid    = t_htonl(varOperation.sendId);  //����֡��ˮ�� 
	pHead->time_cli = t_htonl(realTime);             //��¼��ǰ��Ҫ���͵�ʱ�� 
	
	varOperation.sendId++;
	
	memcpy(ptr->data,pHead,sizeof(_PROTOCOL_HEAD));
	Mem_free(pHead);                                  //������ڴ�飬����һ��Ҫ�ͷŰ�
	
	crc = CRC_Compute16(&ptr->data[1],ptr->datLength-1);
	
	ptr->data[ptr->datLength++] = (crc>>8)&0xff;
	ptr->data[ptr->datLength++] = crc&0xff;
	ptr->data[ptr->datLength++] = 0x7E;
}
#if 1
const uint8_t ipAddr[] ="116.228.88.101"; //todo: �������������� ������116.228.88.101  29999  ������116.62.195.99
#define IP_Port          29999            //�˿ں�
#else
const uint8_t ipAddr[] ="116.62.195.99"; //todo: �������������� ������116.228.88.101  29999  ������116.62.195.99
#define IP_Port          9998            //�˿ں�
#endif

_OBD_PID_Cmd *ptrPIDAllDat;    //ָ���һ������
VARConfig    *ptrPIDVars;      //ָ��ڶ�������

uint8_t configData[2048] = {0};//�����洢����PID
void GlobalVarInit(void )      //todo��ȫ�ֱ�����ʼ��  ���ϲ��䣬��Flash�ж�ȡ�費��Ҫ���µ� (ECU�汾)
{    
	//��Flash���������ݽ�ȫ�ֱ���
	Flash_ReadDat(SBOOT_UPGREAD_ADDR,(uint8_t *)&sysUpdateVar,sizeof(_SystemInformation));
	//��Flash�ж�ȡPID����
	Flash_ReadDat(PIDConfig_ADDR,configData,2048);
	ptrPIDAllDat = (_OBD_PID_Cmd *)configData;
		
	varOperation.pidNum = sysUpdateVar.pidNum;//�õ�PIDָ��ĸ���
	varOperation.isDataFlow  = 1;             //�豸������ʱ��������δ����
	varOperation.isCDMAStart = CDMA_CLOSE;    //CDMA��ʼ״̬Ϊ�ر�
	varOperation.isEngineRun = ENGINE_RUN;    //��ʼ��Ϊ�������������˵�
	varOperation.sendId      = 0x80000000;    //���͵�֡��ˮ��
	
	varOperation.pidVersion = sysUpdateVar.pidVersion;
	varOperation.busType    = sysUpdateVar.busType;
	varOperation.canIdType  = sysUpdateVar.canIdType;
	varOperation.canTxId    = sysUpdateVar.canTxId;
	varOperation.canRxId    = sysUpdateVar.canRxId;
	varOperation.canBaud    = sysUpdateVar.canBaud;
	
	memset(varOperation.ecuVersion,0,20);
	
	varOperation.pidVarNum  = sysUpdateVar.pidVarNum;
	
	ptrPIDVars              = (VARConfig*)&configData[varOperation.pidNum * 13];//�õ��ڶ������ļ��ĵ�ַ
	
	varOperation.ipPotr = IP_Port;             //todo:��������������  ��ʼ���˿ں�
	memset(varOperation.ipAddr,0,18);
	memcpy(varOperation.ipAddr,ipAddr,sizeof(ipAddr));//todo��IP��ַ��������������flash�е�IP���˿ں�	
}

 uint8_t* RecvDataAnalysis(uint8_t* ptrDataToDeal)//�������յ������ݰ�
{
	uint16_t  i = 0;
	uint16_t datLen   = 0;
	uint16_t crcDat   = 0;
	uint16_t crcCheck = 0;
	uint8_t* ptr = NULL;
	while(1)
	{
		if(ptrDataToDeal[i++] == 0x7E)
			break;
		if(i >= 60)
		{
			Mem_free(ptrDataToDeal);
			return NULL;
		}
	}
	datLen   = ptrDataToDeal[i++];
	datLen   = (datLen << 8) + ptrDataToDeal[i--];
	crcCheck = CRC_Compute16(&ptrDataToDeal[i],datLen + 2);
	
	crcDat   = ptrDataToDeal[i + datLen +2];
	crcDat   = (crcDat << 8) + ptrDataToDeal[i + datLen +3];
	if(crcCheck != crcDat)
	{
		Mem_free(ptrDataToDeal);
		return NULL;
	}
	ptr = Mem_malloc(datLen);
	datLen = datLen - 24;
	ptr[0] = (datLen >> 8)  & 0x00FF;
	ptr[1] = datLen & 0x00FF;
	memcpy(&ptr[2],&ptrDataToDeal[i + 26],datLen);
	
	Mem_free(ptrDataToDeal);
	return ptr;
}

extern uint16_t freOBDLed; //��
extern uint16_t freCDMALed;//��
extern uint16_t freGPSLed; //��


void RevolvingSpeedDeal(void)//todo:������ת�ٴ���
{
	static uint8_t openClose = 0;
	static uint8_t loginFlag = 0;
//	uint8_t err;
	uint32_t currentTime = 0;  //��ǰʱ��
	uint32_t currentHour = 0;//��ǰСʱ     0��-1��   ���͵�¼���ģ���������
	
	currentTime = RTC_GetCounter();
	currentHour = (currentTime/3600) %24;  //�õ���ǰ��Сʱ��
	if(currentHour > 0)
		loginFlag = 0;
	
	if(varOperation.isEngineRun == ENGINE_RUN)//����������������
	{
		if(openClose != 1)
		{
			openClose = 1;
			//todo:��CDMA��Դ��GPS��Դ������������־������С��
			freOBDLed  = 100;
			freCDMALed = 100;
			freGPSLed  = 100;
			OSTaskResume(CDMA_LED_PRIO);
			OSTaskResume(GPS_LED_PRIO);
			OSTaskResume(OBD_LED_PRIO);
			
			CDMAPowerOpen_Close(CDMA_OPEN);//��CDMA��Դ
			CDMAConfigInit();              //��ʼ��CDMA
			OSTaskResume(CDMA_TASK_PRIO);  //�ظ�CDMA��������
			
			GPS_POWER_ON;  //��
		}
	}
	else if(varOperation.isEngineRun == ENGINE_STOP)//��������ֹͣ����
	{
		if(currentHour == 0 && loginFlag == 0 && openClose == 0)//��㵽,CDMA�ѹرգ����͵�¼����
		{
			loginFlag = 1;
			openClose = 1;
			CDMAPowerOpen_Close(CDMA_OPEN);//��CDMA��Դ
			CDMAConfigInit();              //��ʼ��CDMA
			OSTaskResume(CDMA_TASK_PRIO);  //�ظ�CDMA��������
			LoginDataSend();               //���͵�¼����
		}
		if(openClose != 0 && varOperation.isLoginDeal == 1)//��Ҫ�رղ��������ڴ���ĵ�¼����
		{
			openClose = 0;
			varOperation.isDataFlow  = 1; //��ֹ������
			GPS_POWER_OFF;                //�ر�GPS��Դ
			
			OSTimeDlyHMSM(0,0,5,0);       //5s��ʱ�䣬Ӧ���ܽ�����Ҫ���͵����ݶ����������
			
			//todo���ر�CDMA��Դ��GPS��Դ������������־������С��������𲢹ر�
		
			OSTaskSuspend(GPS_TASK_PRIO);     //����GPS����
			
			OSTaskSuspend(CDMA_TASK_PRIO);    //����CDMA����
			CDMAPowerOpen_Close(CDMA_CLOSE);  //�ر�CDMA��Դ
			
			//����LED��
			OSTaskSuspend(CDMA_LED_PRIO);
			OSTaskSuspend(GPS_LED_PRIO);
			OSTaskSuspend(OBD_LED_PRIO);
			GPIO_SetBits(GPIOB,GPIO_Pin_0 | GPIO_Pin_3 | GPIO_Pin_5 | GPIO_Pin_4);//�ر�����С��
			
			//�ر�LED�ƹ���ʾ  ���ͷ������ػ���ʾ��  
		}
	}
}

extern _CDMADataToSend* cdmaDataToSend;//CDMA���͵������У�OBD��GPS������ͨ��������Ϊ����
void LogReport(char* fmt,...)          //�ϴ���־�ļ�
{
	u8 datLen,err;
	va_list ap;
	uint8_t * ptrSaveLog;

	if(cdmaDataToSend->datLength > 850)
		return;
	ptrSaveLog = Mem_malloc(125);
	va_start(ap,fmt);
	vsprintf((char*)(&ptrSaveLog[3]),fmt,ap);
	va_end(ap);
	
	datLen=strlen((const char*)(&ptrSaveLog[3]));//�˴η������ݵĳ���

	ptrSaveLog[0] = datLen + 3;
	ptrSaveLog[1] = 0x50;
	ptrSaveLog[2] = 0x03;
	
	OSMutexPend(CDMASendMutex,0,&err);
						
	memcpy(&cdmaDataToSend->data[cdmaDataToSend->datLength],ptrSaveLog,ptrSaveLog[0]);
	cdmaDataToSend->datLength += ptrSaveLog[0];
	
	OSMutexPost(CDMASendMutex);
	Mem_free(ptrSaveLog);
}

extern MEM_Check allMemState;         //�ڴ��ر���
//�ϱ��ڴ�ʹ����־�ļ�  ����ǰ���з��׶ε��ڴ��ʹ���������

void MemLog(_CDMADataToSend* ptr)
{
	LogReport("m1:%d,%d;m2:%d,%d;m3:%d,%d;m4:%d,%d;m5:%d,%d;m6:%d,%d;m7:%d,%d;",\
			allMemState.memUsedNum1,allMemState.memUsedNum1,\
			allMemState.memUsedNum2,allMemState.memUsedMax2,\
			allMemState.memUsedNum3,allMemState.memUsedMax3,\
			allMemState.memUsedNum4,allMemState.memUsedMax4,\
			allMemState.memUsedNum5,allMemState.memUsedMax5,\
			allMemState.memUsedNum6,allMemState.memUsedMax6,\
			allMemState.memUsedNum7,allMemState.memUsedMax7);
}






