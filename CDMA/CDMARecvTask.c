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
			else if(cmdId == 0x5012)                     //�ڶ����ֵ������ļ�
				ConfigUpdata(ptrDeal);
			else if((cmdId >= 0x4000)&&(cmdId < 0x5000)) //��һ���ֵ������ļ�
				ConfigUpdata(ptrDeal);
			else if((cmdId >= 0x8000)&&(cmdId < 0x9000)) //������������    
			{
				OTA_Updata(ptrDeal);
			}
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
	
	cmdId = ptr[offset++];
	cmdId = (cmdId<<8) + ptr[offset++];

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
    static uint16_t currentNum = 0; //������һ�����������
	static uint16_t frameIndex = 0;
	static uint8_t  pidPackNum = 0;//PID �ܰ���
		
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
		
		pidPackNum = ptrDeal[offset++];            //һ����֡PID��������
		
		varOperation.canBaud = ptrDeal[offset++];  //CAN�����ʣ�Э���е� protocolType
		
		currentNum = 0x4001;
		frameIndex = 50;
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
		
		if((cmdId - pidPackNum) == 0x4000)
		{
			//todo:�������������ȫ�ֱ������������ò���,����������
			canDataConfig.pidVersion = varOperation.newPIDVersion;
			canDataConfig.pidNum     = varOperation.newPidNum;
			
			canDataConfig.busType    = varOperation.busType;//todo:CAN�ߺ�K�ߵ��л������ڴ���
			canDataConfig.canIdType  = varOperation.canIdType;
			canDataConfig.canTxId    = varOperation.canTxId;
			canDataConfig.canRxId    = varOperation.canRxId;
			canDataConfig.canBaud    = varOperation.canBaud;
			
			for(i = 50;i < frameIndex;i += 17)      //���� ָ������� ���ֽ���
			{
				temp            = updateBuff[i];
				updateBuff[i]   = updateBuff[i+3];
				updateBuff[i+3] = temp;
				temp            = updateBuff[i+1];
				updateBuff[i+1]   = updateBuff[i+2];
				updateBuff[i+2] = temp;
			}
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
		
		canDataConfig.pidVarNum = frameLen / 13;   //�õ��ϱ� ECU �����ĸ���
		
		cmdId    = ptrDeal[offset++];
		cmdId    = (cmdId << 8) + ptrDeal[offset++]; 
		memcpy(&updateBuff[frameIndex],&ptrDeal[offset],frameLen);
		frameIndex += frameLen;
		
		PIDConfigReadWrite(updateBuff,(uint8_t *)&canDataConfig,sizeof(_CANDataConfig),0);
		
		for(i = (17*canDataConfig.pidNum)+50;i < frameIndex;i += 14)      //����ϵ����ƫ�������ֽ���
			{
				temp            = updateBuff[i+6];
				updateBuff[i+6]   = updateBuff[i+9];
				updateBuff[i+9] = temp;
				temp            = updateBuff[i+7];
				updateBuff[i+7]   = updateBuff[i+8];
				updateBuff[i+8] = temp;
				
				temp            = updateBuff[i+10];
				updateBuff[i+10]   = updateBuff[i+13];
				updateBuff[i+13] = temp;
				temp            = updateBuff[i+11];
				updateBuff[i+11]   = updateBuff[i+12];
				updateBuff[i+12] = temp;
			}
		
		Save2KDataToFlash(updateBuff,PIDConfig_ADDR,2048);      //������д�������ļ�������flash:0x0802E000��
		
		__disable_fault_irq();                    //����
		NVIC_SystemReset();
	}
}









