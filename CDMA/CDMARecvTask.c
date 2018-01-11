#include "includes.h"
#include "bsp.h"
#include "apptask.h"

/*************************     ����������          **********************/

static void RecvLoginDatDeal(uint8_t* ptr);
static void SendFrameNum(uint16_t frameNum);
static void OTA_Updata(uint8_t* ptrDeal );
static void ConfigUpdata(uint8_t* ptrDeal  );
static void GetConfigInfo(void);
extern void CDMASendCmd(const uint8_t sendDat[],char* compString,uint16_t sendLength);
static void CanTestCmd(uint8_t* ptrDeal);
static void FuelModeChange(uint8_t* ptrDeal);

//�����������Ϸ���¼���ġ�����OTA�����������ļ�������ģʽ�л���ǿ����ģʽ������ģʽ�ȣ�
void CDMARecvTask(void *pdata)
{
	uint8_t  err;
	uint16_t cmdId;
	uint8_t* ptrRECV = NULL;
	uint8_t* ptrDeal = NULL;
	
	while(1)
	{
		ptrRECV = OSQPend(ZIPRecv_Q,30000,&err);  //�ȴ�60s�����������Ӧ�����˳�
		if(err == OS_ERR_NONE)
		{
			ptrDeal = RecvDataAnalysis(ptrRECV);  //�����յ������ݽ��мӹ�
			if(ptrDeal == NULL)                   //���յ����������
				continue; 
			
			cmdId = ptrDeal[3];
			cmdId = (cmdId<<8) + ptrDeal[4];
			
			if(cmdId == 0x5001)                          //���յ���¼����
				RecvLoginDatDeal(ptrDeal);
			else if(cmdId == 0x5015)                     //ģʽ�л�
				FuelModeChange(ptrDeal);
			else if(cmdId == 0x5017)                     //�������·��Ĳ���ָ��
				CanTestCmd(ptrDeal);
			else if((cmdId >= 0x4000)&&(cmdId < 0x5000)) //��һ���ֵ������ļ�
				ConfigUpdata(ptrDeal);
			else if(cmdId == 0x5012)                     //�ڶ����ֵ������ļ�
				ConfigUpdata(ptrDeal);
			else if((cmdId >= 0x8000)&&(cmdId < 0x9000)) //������������    
				OTA_Updata(ptrDeal);
			
			Mem_free(ptrDeal);             //�ͷ��ڴ��
			varOperation.isLoginDeal = 1;  //��¼���Ĵ������
		}
		else   //�ȴ���ʱ
		{
			varOperation.isLoginDeal = 1; 
			if(sysUpdateVar.isSoftUpdate == 1)//OTA������ʱ
			{
				sysUpdateVar.isSoftUpdate = 0;
				varOperation.isDataFlow   = 0;//����������
				if(OSSemAccept(LoginMes) == 0)//����CAN
					OSSemPost(LoginMes);	
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
	
	buff = sysUpdateVar.curSoftVer; 
	buff =t_htonl(buff);			//����̼��汾  
	memcpy(&loginData->data[loginData->datLength],&buff,4);
	loginData->datLength += 4;
	
	buff = t_htonl(canDataConfig.pidVersion);
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
}
static void GetConfigInfo(void)
{
	_CDMADataToSend* otaUpdatSend;
	otaUpdatSend = CDMNSendInfoInit(60);//��������֡

	otaUpdatSend->data[otaUpdatSend->datLength++] = 11;   //����
	otaUpdatSend->data[otaUpdatSend->datLength++] = 0x40;
	otaUpdatSend->data[otaUpdatSend->datLength++] = 0x00;
	//��ǰ�汾
	otaUpdatSend->data[otaUpdatSend->datLength++] = (varOperation.pidVersion >> 24) & 0x00FF; 
	otaUpdatSend->data[otaUpdatSend->datLength++] = (varOperation.pidVersion >> 16) & 0x00FF; 
	otaUpdatSend->data[otaUpdatSend->datLength++] = (varOperation.pidVersion >> 8) & 0x00FF;   
	otaUpdatSend->data[otaUpdatSend->datLength++] = varOperation.pidVersion & 0x00FF;
	
	//���������İ汾
	otaUpdatSend->data[otaUpdatSend->datLength++] = (varOperation.newPIDVersion >> 24) & 0x00FF;
	otaUpdatSend->data[otaUpdatSend->datLength++] = (varOperation.newPIDVersion >> 16) & 0x00FF;
	otaUpdatSend->data[otaUpdatSend->datLength++] = (varOperation.newPIDVersion >> 8) & 0x00FF;
	otaUpdatSend->data[otaUpdatSend->datLength++] = varOperation.newPIDVersion & 0x00FF;
	
	CDMASendDataPack(otaUpdatSend);//����������з��
	
	OSQPost(CDMASendQ,otaUpdatSend);
}

static void RecvLoginDatDeal(uint8_t* ptr)//�Է������ظ��ĵ�¼���Ľ��н���
{
	uint16_t cmdId = 0;
	uint8_t  ipLen = 0;
	uint32_t ecuId = 0;
	uint32_t serverTime  = 0;
	uint32_t softVersion = 0;
	uint16_t offset = 3;
	static uint8_t timetemp = 0;
	cmdId = ptr[offset++];
	cmdId = (cmdId<<8) + ptr[offset++];

	serverTime = ptr[offset++];     //�õ�������ʱ��
	serverTime = (serverTime << 8) + ptr[offset++];
	serverTime = (serverTime << 8) + ptr[offset++];
	serverTime = (serverTime << 8) + ptr[offset++];
	
	if(timetemp == 0)
	{
		carAllRecord.startTime = serverTime; //�г���ʼʱ��
		timetemp = 1;
	}
	
	RTC_Time_Adjust(serverTime);   //��¼��ʱ�򣬸�������ʱ�����Уʱ��
	
	softVersion = ptr[offset++];   //�õ�����汾��
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
	
	varOperation.oilMode = ptr[offset + ipLen + 2];
	
	if(softVersion != sysUpdateVar.curSoftVer) //�ȿ���OTA����
	{
		varOperation.newSoftVersion = softVersion;
		OSSemPend(sendMsg,100,&ipLen);    //�ȴ�200ms  ȷ��CDMA��ǰû�з�������
		varOperation.isDataFlow     = 1;  // OTA�������� ֹͣ��������һ��ֻΪOTA����
		sysUpdateVar.isSoftUpdate   = 1;  
		
		SendFrameNum(0x8000);             //����0x8000����������ļ���С�Լ�CRCУ��
	}
	else if(ecuId != canDataConfig.pidVersion && sysUpdateVar.isSoftUpdate ==0)  //�ٿ��������ļ�����
	{
		varOperation.newPIDVersion = ecuId;
		OSSemPend(sendMsg,100,&ipLen);    //�ȴ�200ms  ȷ��CDMA��ǰû�з�������
		varOperation.isDataFlow     = 1;  //�����ļ�������ֹͣ��������һ��ֻΪ����
		
		GetConfigInfo();                  //���������ļ� - ����0x4000���汾��Ϣ
	}
	else 
	{
		varOperation.isLoginDeal = 1;  //û�е�¼������Ҫ����
		if(OSSemAccept(LoginMes) == 0)
			OSSemPost(LoginMes);
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
uint8_t updateBuff[2048];       //������
static void OTA_Updata(uint8_t* ptrDeal)
{
	uint16_t cmdId;
	uint16_t datLength = 0;
	uint16_t i = 0;
	uint8_t  frameNum;           //�˴�һ�����յ�128�ֽڵİ���
	uint8_t  frameLen;           //ÿһ֡��ÿһС�������ж��ٸ��ֽ�
	uint16_t offset;
	
	static uint16_t currentNum = 0;      //������һ�������
	static uint16_t fileCRC    = 0;      //�ļ�CRCУ��
	static uint32_t flashAddr  = 0;      //��ַ��Ϣ��д2K������0x800,��Flashһ��д2K�ֽ�
	static uint8_t  frameIndex = 0;      //Ҫ�����֡����
	
	
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
		
		currentNum = 0x8001;
		fileCRC    = 0;
		flashAddr  = 0;
		frameIndex = 0;
		SendFrameNum(currentNum);//���͵�һ����������֡0x8001
		memset(updateBuff,0,2048);
	}
	else if(cmdId>0x8000)        //�������
	{
		if(cmdId != currentNum)  //���յ���֡��ţ����������֡��Ų�ͬ����������ݲ���������
		{
//			SendFrameNum(currentNum);//todo�����½������ݣ�
			return;
		}
			
		frameNum = (datLength%131) == 0? (datLength/131) : (datLength/131) + 1;//�õ���֡����һ���ж��ٰ�128�ֽڵĳ������
		
		offset = 2;
		for(i=0;i<frameNum;i++)//
		{
			frameLen = ptrDeal[offset++] - 3;//ʵ�ʵ�С��������ֽ���
			cmdId    = ptrDeal[offset++];
			cmdId    = (cmdId << 8) + ptrDeal[offset++]; 
			memcpy(&updateBuff[frameIndex*128],&ptrDeal[offset],frameLen);
			offset += 128;
			frameIndex ++;
			if((frameIndex >= 16) && (cmdId != varOperation.frameNum))
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
				flashAddr += ((frameIndex - 1)*128 + frameLen);
			}
		}
		if(cmdId == varOperation.frameNum)
		{
			if(fileCRC != varOperation.newSoftCRC)//CRCУ����󣬳�������ʧ��
			{
				Mem_free(ptrDeal);
				varOperation.isDataFlow     = 0;
//				SendFrameNum(0x8000);      //todo:����������
				return;	
			}
			Mem_free(ptrDeal);
			sysUpdateVar.isSoftUpdate = 1;      //����Sboot,������Ҫ����
			sysUpdateVar.pageNum      = flashAddr/0x800 + 1;
			sysUpdateVar.softByteSize = flashAddr;
			sysUpdateVar.newSoftCRC   = fileCRC;
			sysUpdateVar.newSoftVer   = varOperation.newSoftVersion;
			
			SbootParameterSaveToFlash(&sysUpdateVar);//�������������浽Flash��
			
			__disable_fault_irq();          //����
			NVIC_SystemReset();
		}
		currentNum = cmdId + 1;
		SendFrameNum(currentNum);//������һ֡���ݣ�
	}
}


static void SendConfigNum(uint16_t cmd)
{
	_CDMADataToSend* otaUpdatSend;
	otaUpdatSend = CDMNSendInfoInit(60);//��������֡

	otaUpdatSend->data[otaUpdatSend->datLength++] = 7;   //����
	otaUpdatSend->data[otaUpdatSend->datLength++] = (cmd>>8) &0x00FF;
	otaUpdatSend->data[otaUpdatSend->datLength++] = cmd &0x00FF;
	//���������İ汾
	otaUpdatSend->data[otaUpdatSend->datLength++] = (varOperation.newPIDVersion >> 24) & 0x00FF;
	otaUpdatSend->data[otaUpdatSend->datLength++] = (varOperation.newPIDVersion >> 16) & 0x00FF;
	otaUpdatSend->data[otaUpdatSend->datLength++] = (varOperation.newPIDVersion >> 8) & 0x00FF;
	otaUpdatSend->data[otaUpdatSend->datLength++] = varOperation.newPIDVersion & 0x00FF;
	
	CDMASendDataPack(otaUpdatSend);//����������з��
	
	OSQPost(CDMASendQ,otaUpdatSend);
}
static void ConfigUpdata(uint8_t* ptrDeal )
{
	uint8_t  temp;
	uint16_t frameLen;
	uint16_t cmdId;
	
	uint16_t i = 0,offset = 0;
    static uint16_t currentNum = 0;//������һ�����������
	static uint16_t frameIndex = 0;
	static uint8_t  pidPackNum = 0;//PID �ܰ���
	static uint32_t addrSavePid = 0;
		
	cmdId     = ptrDeal[3];
	cmdId     = (cmdId << 8) + ptrDeal[4];
	if(cmdId == 0x4000)
	{
		offset = 5;
		varOperation.busType   = ptrDeal[offset++];//��������  CAN���߻���K��
		varOperation.canIdType = ptrDeal[offset++];//CAN ID���ͣ���չ֡���Ǳ�׼֡
		
		varOperation.canRxId = ptrDeal[offset++];  //��·�� CAN ����ID
		varOperation.canRxId = (varOperation.canRxId << 8) + ptrDeal[offset++];
		varOperation.canRxId = (varOperation.canRxId << 8) + ptrDeal[offset++];
		varOperation.canRxId = (varOperation.canRxId << 8) + ptrDeal[offset++];
		
		varOperation.canTxId = ptrDeal[offset++];  //��·�� CAN ����ID
		varOperation.canTxId = (varOperation.canTxId << 8) + ptrDeal[offset++];
		varOperation.canTxId = (varOperation.canTxId << 8) + ptrDeal[offset++];
		varOperation.canTxId = (varOperation.canTxId << 8) + ptrDeal[offset++];
		
		varOperation.newPidNum = ptrDeal[offset++];//�µ�PID�������
		varOperation.newPidNum = varOperation.newPidNum * 256 + ptrDeal[offset++];//�µ�PID�������
		
		pidPackNum = ptrDeal[offset++];            //һ֡���ж���PID��������
		
		varOperation.canBaud = (CANBAUD_Enum)ptrDeal[offset++];  //CAN�����ʣ�Э���е� protocolType
		
		memcpy(varOperation.pidVerCmd,&ptrDeal[offset],8);  //��ȡ ��ȡECU�汾�ŵ�ָ��
		
		currentNum = 0x4001;
		frameIndex = 0;
		addrSavePid  = PID1CONFIGADDR;
		memset(updateBuff,0,2048);
		SendConfigNum(currentNum);//���͵�һ����������֡0x4001
		
	}else if(cmdId > 0x4000 && cmdId < 0x5000)
	{
		if(cmdId != currentNum)       //���յ���֡��ţ����������֡��Ų�ͬ����������ݲ���������
		{
			//SendConfigNum(currentNum);//todo�����·��ͣ�
			return;
		}
		
		offset = 2;
		frameLen = ptrDeal[offset++] - 3;
		cmdId    = ptrDeal[offset++];
		cmdId    = (cmdId << 8) + ptrDeal[offset++]; 
		memcpy(&updateBuff[frameIndex],&ptrDeal[offset],frameLen);
		frameIndex += frameLen;
		if(((cmdId - 0x4000) % 11 == 0)&&((cmdId - pidPackNum) != 0x4000))//2K ��110��PID��һ��PIDռ17byte
		{
			for(i = 0;i < frameIndex;i += 17)      //���� ָ������� ���ֽ���
			{
				temp            = updateBuff[i];
				updateBuff[i]   = updateBuff[i+3];
				updateBuff[i+3] = temp;
				temp            = updateBuff[i+1];
				updateBuff[i+1] = updateBuff[i+2];
				updateBuff[i+2] = temp;
			}
			Save2KDataToFlash(updateBuff,addrSavePid,2048);
			addrSavePid += 0x800;
			frameIndex = 0;
			memset(updateBuff,0,2048);
			currentNum = cmdId + 1;   
			SendConfigNum(currentNum);//������һ������
		}
		else if((cmdId - pidPackNum) == 0x4000)
		{
			for(i = 0;i < frameIndex;i += 17)                   //���� ָ������� ���ֽ���
			{
				temp            = updateBuff[i];
				updateBuff[i]   = updateBuff[i+3];
				updateBuff[i+3] = temp;
				temp            = updateBuff[i+1];
				updateBuff[i+1]   = updateBuff[i+2];
				updateBuff[i+2] = temp;
			}
			Save2KDataToFlash(updateBuff,addrSavePid,2048);
			addrSavePid = PID2CONFIGADDR;
			frameIndex = 0;
			memset(updateBuff,0,2048);
			SendConfigNum(0x5012);//����ڶ��������ļ�
		}
		else  
		{
			currentNum = cmdId + 1;   
			SendConfigNum(currentNum);//������һ������
		}
	}
	else if(cmdId == 0x5012)
	{
		offset = 2;
		frameLen = ptrDeal[offset++] - 3;
		
		canDataConfig.pidVarNum = frameLen / 14;   //�õ��ϱ� ECU �����ĸ���
		
		cmdId    = ptrDeal[offset++];
		cmdId    = (cmdId << 8) + ptrDeal[offset++]; 
		memcpy(updateBuff,&ptrDeal[offset],frameLen);
		frameIndex += frameLen;
		
		for(i = 0;i < 500;i += 14)      //����ϵ����ƫ�������ֽ���
		{
			temp            = updateBuff[i+6];
			updateBuff[i+6] = updateBuff[i+9];
			updateBuff[i+9] = temp;
			temp            = updateBuff[i+7];
			updateBuff[i+7] = updateBuff[i+8];
			updateBuff[i+8] = temp;
			
			temp             = updateBuff[i+10];
			updateBuff[i+10] = updateBuff[i+13];
			updateBuff[i+13] = temp;
			temp             = updateBuff[i+11];
			updateBuff[i+11] = updateBuff[i+12];
			updateBuff[i+12] = temp;
		}
		
		Save2KDataToFlash(updateBuff,PID2CONFIGADDR,500);//������д��ڶ������ļ�������flash:0x08061000��
		
		canDataConfig.pidVersion = varOperation.newPIDVersion;
		canDataConfig.pidNum     = varOperation.newPidNum;
		
		canDataConfig.busType    = varOperation.busType;     //todo:CAN�ߺ�K�ߵ��л������ڴ���
		canDataConfig.canIdType  = varOperation.canIdType;
		canDataConfig.canTxId    = varOperation.canTxId;
		canDataConfig.canRxId    = varOperation.canRxId;
		canDataConfig.canBaud    = varOperation.canBaud;
		memcpy(canDataConfig.pidVerCmd,varOperation.pidVerCmd,8);
		Save2KDataToFlash((uint8_t *)&canDataConfig,PIDCONFIG,(sizeof(_CANDataConfig)+3)); //���� CAN ͨѶ����
		
		__disable_fault_irq();                    //����
		NVIC_SystemReset();
	}
}
#include "obd.h"
static void SendPidCmdData(uint8_t* cmdData)
{
	_CDMADataToSend* cmdPidSend;
	cmdPidSend = CDMNSendInfoInit(60);//
	memcpy(&cmdPidSend->data[cmdPidSend->datLength],cmdData,cmdData[0]);
	cmdPidSend->datLength += cmdData[0];

	CDMASendDataPack(cmdPidSend);//����������֡���з��
	
	OSQPost(CDMASendQ,cmdPidSend);
	Mem_free(cmdData);
}
extern uint8_t pidManyBag[8];
extern CAN1DataToSend  dataToSend; 
static void CanTestCmd(uint8_t* ptrDeal)//�������·���  CAN����ָ��
{
	uint32_t flowId;
	uint32_t canRxId;
//	uint32_t recordId;
	uint32_t canTxId;
	uint8_t*  pidVerCmd;
	uint16_t cmdId;
	uint8_t offset = 3,err = 0,i = 0,cmdManyPackNum = 0;
	CanRxMsg* CAN1_RxMsg;
	CAN_InitTypeDef   CAN_InitStructure;
	
//	recordId = dataToSend.canId;//��¼֮ǰ��  CANID
	
	pidVerCmd = Mem_malloc(8);
	
	cmdId     = ptrDeal[offset ++];
	cmdId     = (cmdId << 8) + ptrDeal[offset ++];
	
	flowId = ptrDeal[offset ++];//��ˮID
	flowId = (flowId<<8) + ptrDeal[offset++];
	flowId = (flowId<<8) + ptrDeal[offset++];
	flowId = (flowId<<8) + ptrDeal[offset++];
	
	canRxId = ptrDeal[offset ++];//��·��ID
	canRxId = (canRxId<<8) + ptrDeal[offset++];
	canRxId = (canRxId<<8) + ptrDeal[offset++];
	canRxId = (canRxId<<8) + ptrDeal[offset++];
	
	canTxId = ptrDeal[offset ++];//ECU ID 
	canTxId = (canTxId<<8) + ptrDeal[offset++];
	canTxId = (canTxId<<8) + ptrDeal[offset++];
	canTxId = (canTxId<<8) + ptrDeal[offset++];
	
	memcpy(pidVerCmd,&ptrDeal[offset],8);//Ҫ���Ե� PID ָ��
	
	varOperation.pidTset = 1;
	
	CAN_DeInit(CAN1);  
	CAN_StructInit(&CAN_InitStructure);
	CAN1_BaudSet(canDataConfig.canBaud);  //����flash�е�CAN���ý��в���
	CAN1_SetFilter(canRxId ,CAN_ID_EXT); 
	CAN_ITConfig(CAN1,CAN_IT_FMP1,ENABLE);//����CAN�˲���
	
	for(i = 0;i < 200;i ++)//�����Ϣ����
	{
		CAN1_RxMsg = OSQPend(canRecieveQ,40,&err);
		if(err != OS_ERR_NONE)//��Ϣ����û������
			break;
		Mem_free(CAN1_RxMsg);
	}
	dataToSend.canId = canTxId;
	dataToSend.pdat  = pidVerCmd;
	OBD_CAN_SendData(dataToSend.canId,CAN_ID_EXT,dataToSend.pdat);//����PIDָ��

	CAN1_RxMsg = OSQPend(canRecieveQ,500,&err); // ���յ�OBD�ظ�
	if(err == OS_ERR_NONE)
	{
		if(CAN1_RxMsg->Data[0] == 0x10)     // �������
		{
			pidVerCmd = Mem_malloc(CAN1_RxMsg->Data[1] + 15);// ������ڴ���㹻��
			if(pidVerCmd != NULL)
			{
				pidVerCmd[0] = CAN1_RxMsg -> Data[1] + 8;
				pidVerCmd[1] = 0x50;
				pidVerCmd[2] = 0x17;
				
				pidVerCmd[3] = (flowId>>24) & 0xFF;//ָ����ˮ��
				pidVerCmd[4] = (flowId>>16) & 0xFF;
				pidVerCmd[5] = (flowId>>8)  & 0xFF;
				pidVerCmd[6] = (flowId>>0)  & 0xFF;
				
				pidVerCmd[7] = CAN1_RxMsg->Data[1];//�յ������ݳ���
				
				memcpy(&pidVerCmd[8],&CAN1_RxMsg->Data[2],6);
				cmdManyPackNum = (CAN1_RxMsg->Data[1] - 6) % 7 == 0? (CAN1_RxMsg->Data[1] - 6)/7 : (CAN1_RxMsg->Data[1] - 6)/7 + 1;
				Mem_free(CAN1_RxMsg);
				dataToSend.pdat = pidManyBag;//���� 0x30 ����������Ķ��
				OBD_CAN_SendData(dataToSend.canId,dataToSend.ide,dataToSend.pdat);
				for(i=0;i<cmdManyPackNum;i++)
				{
					CAN1_RxMsg = OSQPend(canRecieveQ,25,&err);// ���ն��
					if(err == OS_ERR_NONE)
					{
						memcpy(&pidVerCmd[7*i + 14],&CAN1_RxMsg->Data[1],7);
						Mem_free(CAN1_RxMsg);
					}
					else 
						break;
				} 
				if(i == cmdManyPackNum)
				{
					SendPidCmdData(pidVerCmd);
				}
				Mem_free(pidVerCmd);
			}
		}
		else  //��������
		{
			offset = 0;
			pidVerCmd = Mem_malloc(16);
			pidVerCmd[offset++] = 16;
			pidVerCmd[offset++] = 0x50;
			pidVerCmd[offset++] = 0x17;
			
			pidVerCmd[offset++] = (flowId>>24) & 0xFF;//ָ����ˮ��
			pidVerCmd[offset++] = (flowId>>16) & 0xFF;
			pidVerCmd[offset++] = (flowId>>8)  & 0xFF;
			pidVerCmd[offset++] = (flowId>>0)  & 0xFF;
			
			pidVerCmd[offset++] = 8;//�յ������ݳ���
			memcpy(&pidVerCmd[offset++],CAN1_RxMsg->Data,8);
			SendPidCmdData(pidVerCmd);
			Mem_free(pidVerCmd);
			Mem_free(CAN1_RxMsg);
		}
	}else//ECU �޻ظ�
	{	offset = 0;
		pidVerCmd = Mem_malloc(16);
		pidVerCmd[offset++] = 13;
		pidVerCmd[offset++] = 0x50;
		pidVerCmd[offset++] = 0x17;
		
		pidVerCmd[offset++] = (flowId>>24) & 0xFF;//ָ����ˮ��
		pidVerCmd[offset++] = (flowId>>16) & 0xFF;
		pidVerCmd[offset++] = (flowId>>8)  & 0xFF;
		pidVerCmd[offset++] = (flowId>>0)  & 0xFF;
		
		pidVerCmd[offset++] = 5;//ERROR  -  ����ָ�����
		pidVerCmd[offset++] = 'E';pidVerCmd[offset++] = 'R';pidVerCmd[offset++] = 'R';
		pidVerCmd[offset++] = 'O';pidVerCmd[offset++] = 'R';
		SendPidCmdData(pidVerCmd);
		Mem_free(pidVerCmd);
		Mem_free(CAN1_RxMsg);
	}
	if(varOperation.canTest == 2)
	{
		CAN_DeInit(CAN1);  
		CAN_StructInit(&CAN_InitStructure);
		CAN1_BaudSet(canDataConfig.canBaud);  //����flash�е�CAN���ý��в���
		CAN1_ClearFilter();           
		CAN_ITConfig(CAN1,CAN_IT_FMP1,ENABLE);//����CAN�˲���
	}
	varOperation.pidTset = 0;
	Mem_free(pidVerCmd);
}

static void FuelModeChange(uint8_t* ptrDeal)         //���͡�ǿ��������ͨģʽ �л�
{
	uint8_t* ptrMode;
	ptrMode = Mem_malloc(4);
	ptrMode[0] = 4;
	ptrMode[1] = 0x50;
	ptrMode[2] = 0x15;
	ptrMode[3] = ptrDeal[5];
	SendPidCmdData(ptrMode);
	
	varOperation.oilMode = ptrDeal[5];

	Mem_free(ptrMode);
	Mem_free(ptrDeal);
}






