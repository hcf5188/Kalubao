#include "bsp.h"
#include "apptask.h"

    //��ָ�������޹ص�����֡ͷ�����ݳ���

_CDMADataToSend* CDMNSendDataInit(uint16_t length)//Ҫ���͵����ݣ����г�ʼ��
{
	_CDMADataToSend* ptr = NULL;
	ptr = Mem_malloc(sizeof(_CDMADataToSend));
	
	ptr->timeCount = 0;
	ptr->datLength = FRAME_HEAD_LEN;
	ptr->data = Mem_malloc(length);
	
	ptr->data[ptr->datLength ++] = 7;             //ECU �����ļ��汾��
	ptr->data[ptr->datLength ++] = 0x60;
	ptr->data[ptr->datLength ++] = 0x00;
	
	ptr->data[ptr->datLength ++] = (canDataConfig.pidVersion >> 24) & 0x000000FF;
	ptr->data[ptr->datLength ++] = (canDataConfig.pidVersion >> 16) & 0x000000FF;
	ptr->data[ptr->datLength ++] = (canDataConfig.pidVersion >>  8) & 0x000000FF;
	ptr->data[ptr->datLength ++] =  canDataConfig.pidVersion & 0x000000FF;

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
uint16_t crc      = 0;
void CDMASendDataPack(_CDMADataToSend* ptr)		//���ϴ������ݰ�����֡ͷ��װ��CRCУ���
{
	_PROTOCOL_HEAD *pHead = NULL;
	realTime = RTC_GetCounter();				//�õ�ϵͳ���е�RTCʱ��
	pHead = Mem_malloc(sizeof(_PROTOCOL_HEAD));
	pHead->magic    = 0x7E;
	pHead->len      = t_htons(ptr->datLength - 3);   //MAP�����ݳ���
	memcpy(pHead->device,varOperation.imei,16);      //�����豸Ψһ��ʶ�� IMEI
	pHead->msgid    = t_htonl(varOperation.sendId);  //����֡��ˮ�� 
	pHead->time_cli = t_htonl(realTime);             //��¼��ǰ��Ҫ���͵�ʱ�� 
	
	varOperation.sendId ++;
	memcpy(ptr->data,pHead,sizeof(_PROTOCOL_HEAD));
	Mem_free(pHead);                                 //������ڴ�飬����һ��Ҫ�ͷŰ�
	
	crc = CRC_Compute16(&ptr->data[1],ptr -> datLength-1);
	
	ptr->data[ptr->datLength++] = (crc>>8) & 0xff;
	ptr->data[ptr->datLength++] = crc & 0xff;
	ptr->data[ptr->datLength++] = 0x7E;
}

/***********************************************************************************/
  
#if (BEN_S_C == 0)                
const uint8_t ipAddr[] = "116.228.90.118"; //����IP
#define IP_Port          30020             //���ض˿ں�
#elif (BEN_S_C == 1)
const uint8_t ipAddr[] = "116.62.195.99";  //todo: �������������� ������116.62.195.99    9998
#define IP_Port           9998             //�˿ں� 9998 6556
#elif (BEN_S_C == 2)
#define IP_Port           9998
#endif
const uint8_t proIPAddr[] = "tcp.51gonggui.com";// "47.96.8.114"; //����������tcp.51gonggui.com

#define ProIP_Port        9998             //���������˿�
/***********************************************************************************/


_OBD_PID_Cmd *ptrPIDAllDat;    //ָ���һ������
VARConfig    *ptrPIDVars;      //ָ��ڶ�������

uint8_t configData[6000] = {0};//�����洢����PID
uint8_t strengPower[300] = {0};//�����洢ǿ����ģʽ�µ�����
uint8_t pid2Config[300]  = {0};//��������������PID�ڶ������ļ�����
void GlobalVarInit(void )      //todo��ȫ�ֱ�����ʼ��  ���ϲ��䣬��Flash�ж�ȡ�費��Ҫ���µ� (ECU�汾)
{    
	uint8_t *ptrMode;
	uint8_t offset = 0;
	ptrMode = Mem_malloc(80);
	
	//��Flash���������ݽ�ȫ�ֱ���
	Flash_ReadDat((uint8_t *)&sysUpdateVar,SBOOT_UPGREAD_ADDR,sizeof(_SystemInformation));
	//��Flash�ж�ȡPID����
	Flash_ReadDat((uint8_t *)&canDataConfig,PIDCONFIG,sizeof(_CANDataConfig));
	
	PIDConfig2DataRead(configData,PID1CONFIGADDR,6000);
	ptrPIDAllDat = (_OBD_PID_Cmd *)configData;
	
	Flash_ReadDat(pid2Config,PID2CONFIGADDR,300); //��ȡ�ڶ������ļ�����
	ptrPIDVars   = (VARConfig*)pid2Config;
	
	Flash_ReadDat(strengPower,STRENGE_Q,300);  //������������ԭʼֵ
	if(strengPower[0] != 0xAF)                 //��δ��¼���ó���������
		memset(strengPower,0,300);	
	
	Flash_ReadDat(ptrMode,PROMOTE_ADDR,80);    //��ȡ����������ز���
	memcpy(strengthFuelFlash.ecuVer,&ptrMode[offset],16);
	offset += 16;
	memcpy(strengthFuelFlash.fuelAddr,&ptrMode[offset],5);
	offset += 5;
	memcpy(strengthFuelFlash.mask ,&ptrMode[offset],4);
	offset += 4;
	memcpy(strengthFuelFlash.safe1,&ptrMode[offset],8);
	offset += 8;
	memcpy(strengthFuelFlash.safe2,&ptrMode[offset],8);
	offset += 8;
	memcpy(strengthFuelFlash.mode1,&ptrMode[offset],8);
	offset += 8;
	memcpy(strengthFuelFlash.mode2,&ptrMode[offset],8);
	offset += 8;
	strengthFuelFlash.modeOrder = ptrMode[offset++];
	Mem_free(ptrMode);
	
	varOperation.pidNum      = canDataConfig.pidNum;//�õ�PIDָ��ĸ���
	varOperation.isDataFlow  = 1;              //�豸������ʱ��������δ����
	varOperation.isCDMAStart = CDMA_CLOSE;     //CDMA��ʼ״̬Ϊ�ر�
	varOperation.isEngineRun = ENGINE_RUN;     //��ʼ��Ϊ�������������˵�
	varOperation.sendId      = 0x80000000;     //���͵�֡��ˮ��
	
	varOperation.pidVersion = canDataConfig.pidVersion;
	varOperation.busType    = canDataConfig.busType;
	varOperation.canIdType  = canDataConfig.canIdType;
	varOperation.canTxId    = canDataConfig.canTxId;
	varOperation.canRxId    = canDataConfig.canRxId;
	varOperation.canBaud    = canDataConfig.canBaud;
	varOperation.oilMode    = 0;  //Ĭ������ģʽ
	varOperation.isStrenOilOK = 0;//Ĭ�ϲ����Խ��ж�������
	varOperation.strengthRun = 0;
	varOperation.datOKLeng   = 0;
	varOperation.pidRun      = 1;
	memset(varOperation.ecuVersion,0,20);
	
	varOperation.pidVarNum  = canDataConfig.pidVarNum;
	
	varOperation.ipPotr = IP_Port;             //todo:��������������  ��ʼ���˿ں�
	memset(varOperation.ipAddr,0,18);
#if (BEN_S_C < 2)
	memcpy(varOperation.ipAddr,ipAddr,sizeof(ipAddr));//todo��IP��ַ��������������flash�е�IP���˿ں�	//��������������
#endif
}
 uint8_t* RecvDataAnalysis(uint8_t* ptrDataToDeal)//�������յ������ݰ�
{
	uint16_t i = 0;
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

void LogReport(char* fmt,...)          //�ϴ���־�ļ�
{
	u8 datLen,err;
	va_list ap;
	uint8_t * ptrSaveLog;

	if(cdmaDataToSend->datLength > 750)
		return;
	ptrSaveLog = Mem_malloc(255);
	va_start(ap,fmt);
	vsprintf((char*)ptrSaveLog,fmt,ap);
	va_end(ap);
	
	datLen=strlen((const char*)ptrSaveLog);//�˴η������ݵĳ���
	
	if(varOperation.isDataFlow != 1)
	{
		OSMutexPend(CDMASendMutex,0,&err);
		
		if((datLen + cdmaLogData->top) < 900)				
		{
			memcpy(&cdmaLogData->base[cdmaLogData->top],ptrSaveLog,datLen);
			cdmaLogData->top += datLen;
		}
		OSMutexPost(CDMASendMutex);
	}
	
	Mem_free(ptrSaveLog);
}

extern MEM_Check allMemState;         //�ڴ��ر���
//�ϱ��ڴ�ʹ����־�ļ�  ����ǰ���з��׶ε��ڴ��ʹ���������

void MemLog(_CDMADataToSend* ptr)
{
	LogReport("\r\n03-1:%d;2:%d;3:%d;4:%d;5:%d;6:%d;7:%d;",\
			allMemState.memUsedMax1,\
			allMemState.memUsedMax2,\
			allMemState.memUsedMax3,\
			allMemState.memUsedMax4,\
			allMemState.memUsedMax5,\
			allMemState.memUsedMax6,\
			allMemState.memUsedMax7);
}
//�������е����� ��ʼ��
void CARVarInit(void)
{
	carAllRecord.startTime       = 0;//����������ʱ��
	carAllRecord.stopTime        = 0;//������ֹͣʱ��
	carAllRecord.totalMileage    = 0;//�˴����г�
	carAllRecord.totalFuel       = 0;//���ͺ�
	carAllRecord.startlongitude  = 0;//������ʼ ����
	carAllRecord.startlatitude   = 0;//������ʼ γ��
	carAllRecord.stoplongitude   = 0;//����ֹͣ ����
	carAllRecord.stoplatitude    = 0;//����ֹͣ γ��
	carAllRecord.rapidlyPlusNum  = 0;//�����ٴ���
	carAllRecord.rapidlySubNum   = 0;//�����ٴ���
	carAllRecord.engineSpeedMax  = 0;//���ת��
	carAllRecord.carSpeedMax     = 0;//��߳���
	carAllRecord.messageNum      = 0;//��Ϣ����
	carAllRecord.cdmaReStart     = 0;//CDMA��������
	carAllRecord.netFlow         = 0;//��������
	
	carAllRecord.afterFuel       = 0;//��������
	carAllRecord.afterFuel1      = 0;
	carAllRecord.afterFuel2      = 0;
	carAllRecord.afterFuel3      = 0;
	carAllRecord.afterFuelTemp   = 0;
	carAllRecord.allFuel         = 0; //��������
	carAllRecord.allFuelTemp     = 0;
	carAllRecord.beforeFuel      = 0; //Ԥ������
	carAllRecord.beforeFuel1     = 0;
	carAllRecord.beforeFuel2     = 0;
	carAllRecord.beforeFuel3     = 0;
	carAllRecord.beforeFuelTemp  = 0;
	carAllRecord.carSpeed        = 0; //����
	carAllRecord.carSpeedTemp    = 0;
	carAllRecord.curFuel         = 0; //��ǰ������
	carAllRecord.curFuelTemp     = 0;
	carAllRecord.primaryFuel     = 0; //��������
	carAllRecord.primaryFuelTemp = 0;
	carAllRecord.engineSpeed     = 0; //������ת��
	carAllRecord.engineSpeedTemp = 0; 
	carAllRecord.runLen1         = 0; //��ʻ����Ϊ 0
	carAllRecord.runLen2         = 0; //��������Ϊ 0
	carAllRecord.instantFuel     = 0; //˲ʱ�ͺ�
}





