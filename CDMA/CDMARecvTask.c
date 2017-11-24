#include "includes.h"
#include "bsp.h"
#include "apptask.h"

/*************************     ����������          **********************/

static void RecvDatDeal(uint8_t* ptr);
static void SendFrameNum(uint16_t frameNum);
static void OTA_Updata(void );
static void ConfigUpdata(void );
static void GetConfigInfo(void);
extern void CDMASendCmd(const uint8_t sendDat[],char* compString,uint16_t sendLength);



/*************************     ��.c�ļ��õ��ı���         **********************/
#define ZIPRECVBUF_SIZE  5       //RECV������Ϣ���б�����Ϣ�������
OS_EVENT *ZIPRecv_Q;             //ָ��RECV��Ϣ���е�ָ��
void *ZIPRecBuf[ZIPRECVBUF_SIZE];

extern _SystemInformation sysUpdateVar; //�����ñ���
extern SYS_OperationVar  varOperation;  //ϵͳ�����е�ȫ�ֱ���
extern OS_EVENT *CDMASendQ;      //ͨ��CDMA����������Ͳɼ�����OBD��GPS����¼���ĵ�����

extern OS_EVENT *sendMsg;        //���ϵͳ����ͨ��CDMA�������ݣ���ʱ�����ԶϿ�TCP��Ҫ�ȴ�������Ϻ��ٶϿ�TCP�����������µ�IP

OS_EVENT * loginSend;
//�����������Ϸ���¼���ġ�����OTA�����������ļ�������ģʽ�л���ǿ����ģʽ������ģʽ�ȣ�
void CDMARecvTask(void *pdata)
{
	uint8_t err;
	uint8_t* ptrRECV = NULL;
	uint8_t* ptrDeal = NULL;
	loginSend = OSSemCreate(0);
	
	ZIPRecv_Q    = OSQCreate(&ZIPRecBuf[0],ZIPRECVBUF_SIZE);//������ZIPRECV��������Ϣ����
	
	while(1)
	{
		OSSemPend(loginSend,0,&err);//�ȴ����͵�¼����
		ptrRECV = OSQPend(ZIPRecv_Q,10000,&err);  //�ȴ�12s�����������Ӧ�����˳�
		if(err == OS_ERR_NONE)
		{
			ptrDeal = RecvDataAnalysis(ptrRECV);
			RecvDatDeal(ptrDeal);
			varOperation.isLoginDeal = 1;//��¼���Ĵ������
			varOperation.isDataFlow  = 0;
		}
		else
		{
			varOperation.isLoginDeal = 1;//û�е�¼������Ҫ����
			if(varOperation.isEngineRun == ENGINE_STOP)//������ת��Ϊ0���ر�CDMA
			{
//				OSTaskSuspend(CDMA_TASK_PRIO);//todo:��Ҫ�ٿ���һ��
//				CDMAPowerOpen_Close(CDMA_CLOSE);
			}
		}
	}
}
//��¼����
void LoginDataSend(void)
{
	uint8_t err;
	uint32_t buff;
	_CDMADataToSend* loginData = CDMNSendInfoInit(100);        //���͵�¼����
	
	loginData->data[loginData->datLength++] = 31;
	loginData->data[loginData->datLength++] = 0x50;
	loginData->data[loginData->datLength++] = 0x01;
	
	buff = t_htonl(SOFTVersion);                            //����̼��汾  
	memcpy(&loginData->data[loginData->datLength],&buff,4);
	loginData->datLength += 4;
	
	buff = t_htonl(sysUpdateVar.ecuVersion);
	memcpy(&loginData->data[loginData->datLength],&buff,4);
	loginData->datLength += 4;
	
	memcpy(&loginData->data[loginData->datLength],varOperation.iccID,20);
	loginData->datLength += 20;
	
	CDMASendDataPack(loginData);//�Ե�¼���Ľ��д�������֡ͷ��У���롢֡β��
	err = OSQPost(CDMASendQ,loginData);
	if(err != OS_ERR_NONE)
	{
		Mem_free(loginData);
	}
	varOperation.isLoginDeal = 0;//���ڴ����¼����
	OSSemPost(loginSend);        //֪ͨ���մ��������Ѿ����͵�¼���ģ��ȴ�������������ݡ�
}


static void RecvDatDeal(uint8_t* ptr)//�Է������ظ��ĵ�¼���Ľ��н���
{
	uint16_t cmdId = 0;
	uint8_t  ipLen = 0;
	uint32_t ecuId = 0;
	uint32_t serverTime  = 0;
	uint32_t softVersion = 0;
//	int      isIpEqual   = 0;
	uint16_t offset = 3;
	
	cmdId = ptr[offset++];
	cmdId = (cmdId<<8) + ptr[offset++];
	if(cmdId != 0x5001)         //�������·��ĵ�¼��Ϣ  todo:�Ժ���ܻ���ģʽ�л��������·���֡���ڴ˴�Ҫ�Լ��޸�
	{
		Mem_free(ptr);
		//todo�����ն����������Ĵ������Ŀ����ڼ���յ���  +ZIPRECV һ��whileѭ���Ϳ�����
		return ;
	}
	serverTime = ptr[offset++];     //�õ�������ʱ��
	serverTime = (serverTime << 8) + ptr[offset++];
	serverTime = (serverTime << 8) + ptr[offset++];
	serverTime = (serverTime << 8) + ptr[offset++];
	
	RTC_Time_Adjust(serverTime);//��¼��ʱ�򣬸�������ʱ�����Уʱ��
	
	softVersion = ptr[offset++];    //�õ�����汾��
	softVersion = (softVersion << 8) + ptr[offset++];
	softVersion = (softVersion << 8) + ptr[offset++];
	softVersion = (softVersion << 8) + ptr[offset++];
	
	ecuId = ptr[offset++];          //�õ�ECU ID
	ecuId = (ecuId << 8) + ptr[offset++];
	ecuId = (ecuId << 8) + ptr[offset++];
	ecuId = (ecuId << 8) + ptr[offset++];
	
	ipLen = ptr[offset++];              //�õ�IP����
	memset(varOperation.newIP_Addr,0,18);//����
	memcpy(varOperation.newIP_Addr,&ptr[offset],ipLen); //�õ�IP��ַ
	
	varOperation.newIP_Potr = ptr[offset + ipLen];      //�õ��˿ں�
	varOperation.newIP_Potr = (varOperation.newIP_Potr << 8) + ptr[offset + ipLen + 1];
	Mem_free(ptr);//������ͷ�
	
	if(softVersion != sysUpdateVar.softVersion) //�ȿ���OTA����
	{
		varOperation.newSoftVersion = softVersion;
		OSSemPend(sendMsg,100,&ipLen);    //�ȴ�200ms  ȷ��CDMA��ǰû�з�������
		varOperation.isDataFlow     = 1;   // OTA�������� ֹͣ��������һ��ֻΪOTA����
		sysUpdateVar.isSoftUpdate   = 0;  
		
		OTA_Updata();
	}
	if(ecuId != sysUpdateVar.ecuVersion)//�ٿ��������ļ�����
	{
		varOperation.newECUVersion = ecuId;
		OSSemPend(sendMsg,100,&ipLen);    //�ȴ�200ms  ȷ��CDMA��ǰû�з�������
		varOperation.isDataFlow     = 1;  //�����ļ�������ֹͣ��������һ��ֻΪ����
		
		ConfigUpdata();
	}
	
	//todo:IP���ģ����ڻ�����Ҫ
//	isIpEqual = strcmp(varOperation.ipAddr,varOperation.newIP_Addr);//�Ƚ�IP�Ƿ����  =0 - ���
//	if((varOperation.newIP_Potr != varOperation.ipPotr) || (isIpEqual != 0))//�˿ںŲ���Ȼ���IP��ַ�����
//	{
//		memset(varOperation.ipAddr,0,18);//��ԭʼIP����
//		memcpy(varOperation.ipAddr,varOperation.newIP_Addr,18);//��IP
//		varOperation.ipPotr = varOperation.newIP_Potr;         //�¶˿�
//		varOperation.isDataFlow = 1; //ֹͣ������
//		OSSemPend(sendMsg,0,&ipLen);//�ȴ�CDMA���Ϳ��У��������䷢�����ݵ�ʱ������TCP����
//		
//		CDMASendCmd((const uint8_t*)"AT+ZIPCLOSE=0\r","ZIPCLOSE",sizeof("AT+ZIPCLOSE=0\r"));//�ر�TCP����
//	}
}
static void SendFrameNum(uint16_t frameNum)
{
	_CDMADataToSend* otaUpdatSend;
	otaUpdatSend = CDMNSendInfoInit(60);//
	otaUpdatSend->data[otaUpdatSend->datLength++] =  3;   //����
	otaUpdatSend->data[otaUpdatSend->datLength++] = (frameNum >> 8) &0x00FF;
	otaUpdatSend->data[otaUpdatSend->datLength++] = frameNum & 0x00FF;
	
	CDMASendDataPack(otaUpdatSend);//����������֡���з��
	
	OSQPost(CDMASendQ,otaUpdatSend);
}
extern uint8_t configData[2048];//�����洢����PID
uint8_t updateBuff[2048];       //������
static void OTA_Updata(void )
{
	uint8_t  err;
	uint8_t* ptrRECV_Soft;
	uint8_t* ptrDeal;
	uint16_t cmdId;
	uint16_t datLength = 0;
	uint16_t i = 0,offset = 0;
	uint8_t  frameNum;            //�˴�һ�����յ�128�ֽڵİ���
	uint16_t currentNum = 0;      //������һ�������
	uint16_t fileCRC    = 0;      //�ļ�CRCУ��
	uint32_t flashAddr  = 0;      //��ַ��Ϣ��д2K������0x800,��Flashһ��д2K�ֽ�
	uint8_t  frameIndex = 0;      //Ҫ�����֡����
	uint8_t  frameLen   = 0;      //ÿһ֡��ÿһС�������ж��ٸ��ֽ�
	
	memset(updateBuff,0,2048);    //������ݽ��ջ�����
	SendFrameNum(0x8000);         //����0x8000�����ļ���С�Լ�CRCУ��
	while(1)
	{
		ptrRECV_Soft = OSQPend(ZIPRecv_Q,0,&err);//�ȴ�12S
		if(err != OS_ERR_NONE)
		{
			varOperation.isDataFlow     = 0;
			return;//�ȴ���ʱ�����˳�OTA����
		}
		ptrDeal   = RecvDataAnalysis(ptrRECV_Soft);
		if(ptrDeal == NULL)//���Ľ���������������ܿ���ֻ�ڿ�����ʱ�����
		{
			varOperation.isDataFlow     = 0;
			return;//���ݴ������˳�OTA����
		}
		datLength = ptrDeal[0];
		datLength = (datLength << 8) + ptrDeal[1];
		
		cmdId     = ptrDeal[3];
		cmdId     = (cmdId << 8) + ptrDeal[4];
		if(cmdId == 0x8000)
		{
			offset = 5;
			varOperation.frameNum = ptrDeal[offset++] + 0x80;//�õ��³����128�ֽڵİ���
			varOperation.frameNum = (varOperation.frameNum << 8) + ptrDeal[offset++];
			varOperation.newSoftCRC = ptrDeal[offset++];//�õ��ļ�У����
			varOperation.newSoftCRC = (varOperation.newSoftCRC << 8) + ptrDeal[offset++];
			currentNum              = 0x8001;
			SendFrameNum(currentNum);//���͵�һ����������֡0x8001
		}
		else if(cmdId>0x8000)        //�������
		{
			if(cmdId != currentNum)  //���յ���֡��ţ����������֡��Ų�ͬ����������ݲ���������
			{
				SendFrameNum(currentNum);
				Mem_free(ptrDeal);
				continue;
			}
			frameNum = (datLength%131) == 0? (datLength/131) : (datLength/131) + 1;//�õ���֡����һ���ж��ٰ�128�ֽڵĳ������
			
			offset = 2;
			for(i=0;i<frameNum;i++)//
			{
				frameLen = ptrDeal[offset++] - 3;//ʵ�ʵ�
				cmdId    = ptrDeal[offset++];
				cmdId    = (cmdId << 8) + ptrDeal[offset++]; 
				memcpy(&updateBuff[frameIndex*128],&ptrDeal[offset],frameLen);
				offset += 128;
				frameIndex ++;
				if((frameIndex>=16) && (cmdId != varOperation.frameNum))
				{
					frameIndex = 0;
					SoftErasePage(flashAddr);
					SoftProgramUpdate(flashAddr,updateBuff,2048);
					//����CRCУ��
					fileCRC = CRC_ComputeFile(fileCRC,updateBuff,2048);
					memset(updateBuff,0,2048);//���������
					flashAddr += 0x800;
				}
				else if(cmdId == varOperation.frameNum)
				{
					SoftErasePage(flashAddr);
					SoftProgramUpdate(flashAddr,updateBuff,((frameIndex - 1)*128 + frameLen));
					//����CRCУ��
					fileCRC = CRC_ComputeFile(fileCRC,updateBuff,((frameIndex - 1)*128 + frameLen));
					memset(updateBuff,0,2048);
					flashAddr += 0x800;
				}
			
			}
			if(cmdId == varOperation.frameNum)
			{
				if(fileCRC != varOperation.newSoftCRC)
				{
					Mem_free(ptrDeal);
					varOperation.isDataFlow     = 0;
					break;//CRCУ����󣬳�������ʧ��
				}
				Mem_free(ptrDeal);
				sysUpdateVar.isSoftUpdate = 1;      //����Sboot,������Ҫ����
				sysUpdateVar.pageNum      = flashAddr/0x800;
				sysUpdateVar.softVersion  = varOperation.newSoftVersion;
				
				SbootParameterSaveToFlash(&sysUpdateVar);//�������������浽Flash��
				
				__disable_fault_irq();          //����
				NVIC_SystemReset();
			}
			currentNum = cmdId + 1;
			SendFrameNum(currentNum);//������һ֡���ݣ�
		}
		Mem_free(ptrDeal);
	}
}

static void GetConfigInfo(void)
{
	_CDMADataToSend* otaUpdatSend;
	otaUpdatSend = CDMNSendInfoInit(60);//��������֡

	otaUpdatSend->data[otaUpdatSend->datLength++] = 11;   //����
	otaUpdatSend->data[otaUpdatSend->datLength++] = 0x40;
	otaUpdatSend->data[otaUpdatSend->datLength++] = 0x00;
	//��ǰ�汾
	otaUpdatSend->data[otaUpdatSend->datLength++] = (varOperation.ecuVersion >> 24) & 0x00FF; 
	otaUpdatSend->data[otaUpdatSend->datLength++] = (varOperation.ecuVersion >> 16) & 0x00FF; 
	otaUpdatSend->data[otaUpdatSend->datLength++] = (varOperation.ecuVersion >> 8) & 0x00FF;   
	otaUpdatSend->data[otaUpdatSend->datLength++] = varOperation.ecuVersion & 0x00FF;
	
	//���������İ汾
	otaUpdatSend->data[otaUpdatSend->datLength++] = (varOperation.newECUVersion >> 24) & 0x00FF;
	otaUpdatSend->data[otaUpdatSend->datLength++] = (varOperation.newECUVersion >> 16) & 0x00FF;
	otaUpdatSend->data[otaUpdatSend->datLength++] = (varOperation.newECUVersion >> 8) & 0x00FF;
	otaUpdatSend->data[otaUpdatSend->datLength++] = varOperation.newECUVersion & 0x00FF;
	
	CDMASendDataPack(otaUpdatSend);//����������з��
	
	OSQPost(CDMASendQ,otaUpdatSend);
}
static void SendConfigNum(uint16_t cmd)
{
	_CDMADataToSend* otaUpdatSend;
	otaUpdatSend = CDMNSendInfoInit(60);//��������֡

	otaUpdatSend->data[otaUpdatSend->datLength++] = 7;   //����
	otaUpdatSend->data[otaUpdatSend->datLength++] = (cmd>>8) &0x00FF;
	otaUpdatSend->data[otaUpdatSend->datLength++] = cmd &0x00FF;
	//���������İ汾
	otaUpdatSend->data[otaUpdatSend->datLength++] = (varOperation.newECUVersion >> 24) & 0x00FF;
	otaUpdatSend->data[otaUpdatSend->datLength++] = (varOperation.newECUVersion >> 16) & 0x00FF;
	otaUpdatSend->data[otaUpdatSend->datLength++] = (varOperation.newECUVersion >> 8) & 0x00FF;
	otaUpdatSend->data[otaUpdatSend->datLength++] = varOperation.newECUVersion & 0x00FF;
	
	CDMASendDataPack(otaUpdatSend);//����������з��
	
	OSQPost(CDMASendQ,otaUpdatSend);
}
static void ConfigUpdata(void )
{
	uint8_t  err;
	uint8_t* ptrRECV_Soft;
	uint8_t* ptrDeal;
	uint16_t  frameLen;
	uint8_t  pidPackNum = 0;//PID �ܰ���
	uint16_t cmdId;
	uint16_t frameIndex = 0;
	uint16_t i = 0,offset = 0;
   uint16_t currentNum = 0x4001; //������һ�����������
	
	memset(updateBuff,0,2048);
	GetConfigInfo();
	while(1)
	{
		ptrRECV_Soft = OSQPend(ZIPRecv_Q,0,&err);//�ȴ�12S   һ��ʱ�ӵδ���2ms
		if(err != OS_ERR_NONE)
		{
			//todo:��flash�������¶�ȡPID��������ʼ������
			varOperation.isDataFlow     = 0;
			return;//�ȴ���ʱ�����˳������ļ�����
		}
		ptrDeal   = RecvDataAnalysis(ptrRECV_Soft);
			
		cmdId     = ptrDeal[3];
		cmdId     = (cmdId << 8) + ptrDeal[4];
		if(cmdId == 0x4000)
		{
			offset = 5;
			varOperation.busType   = ptrDeal[offset++];//��������  CAN���߻���K��
			varOperation.canIdType = ptrDeal[offset++];//CAN ID���ͣ���չ֡���Ǳ�׼֡
			
			varOperation.canRxId = ptrDeal[offset++];  //��·��CAN ����ID
			varOperation.canRxId = (varOperation.canRxId << 8) + ptrDeal[offset++];
			varOperation.canRxId = (varOperation.canRxId << 8) + ptrDeal[offset++];
			varOperation.canRxId = (varOperation.canRxId << 8) + ptrDeal[offset++];
			
			varOperation.canTxId = ptrDeal[offset++];  //��·��CAN ����ID
			varOperation.canTxId = (varOperation.canTxId << 8) + ptrDeal[offset++];
			varOperation.canTxId = (varOperation.canTxId << 8) + ptrDeal[offset++];
			varOperation.canTxId = (varOperation.canTxId << 8) + ptrDeal[offset++];
			
			varOperation.newPidNum = ptrDeal[offset++];//�µ�PID�������
			
			pidPackNum = ptrDeal[offset++];            //һ���ж��ٰ�PID
			
			varOperation.canBaud = ptrDeal[offset++];  //CAN�����ʣ�Э���е� protocolType
			currentNum = 0x4001;
			SendConfigNum(currentNum);//���͵�һ����������֡0x4001
		}else if(cmdId > 0x4000)
		{
			if(cmdId != currentNum)   //���յ���֡��ţ����������֡��Ų�ͬ����������ݲ���������
			{
				SendConfigNum(currentNum);
				Mem_free(ptrDeal);
				continue;
			}
			
			offset = 2;
			frameLen = ptrDeal[offset++] - 3;
			cmdId    = ptrDeal[offset++];
			cmdId    = (cmdId << 8) + ptrDeal[offset++]; 
			memcpy(&updateBuff[frameIndex],&ptrDeal[offset],frameLen);
			frameIndex += frameLen;
			
			if((cmdId - pidPackNum) == 0x4000)
			{
				//todo:�������������ȫ�ֱ������������ò���,����������
				sysUpdateVar.ecuVersion = varOperation.newECUVersion;
				sysUpdateVar.pidNum     = varOperation.newPidNum;
				
				sysUpdateVar.busType    = varOperation.busType;//todo:CAN�ߺ�K�ߵ��л������ڴ���
				sysUpdateVar.canIdType  = varOperation.canIdType;
				sysUpdateVar.canTxId    = varOperation.canTxId;
				sysUpdateVar.canRxId    = varOperation.canRxId;
				sysUpdateVar.canBaud    = varOperation.canBaud;
				
				for(i = 0;i < frameIndex;i += 13)      //���� ָ������� ���ֽ���
				{
					err             = updateBuff[i];
					updateBuff[i]   = updateBuff[i+1];
					updateBuff[i+1] = err;
				}
				
				SaveConfigToFlash(updateBuff,2048);
				SbootParameterSaveToFlash(&sysUpdateVar);
				
				Mem_free(ptrDeal);
				
				__disable_fault_irq();          //����
				NVIC_SystemReset();
				break;
			}
			else
			{
				currentNum = cmdId + 1;
				SendConfigNum(currentNum);//������һ������
			}
		}
		Mem_free(ptrDeal);
	}
}









