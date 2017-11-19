#include "bsp.h"


extern _SystemInformation* sysAllData;

#define  FRAME_HEAD_LEN    27     //��ָ�������޹ص�����֡ͷ�����ݳ���


_CDMADataToSend* CDMNSendDataInit(uint16_t length)//Ҫ���͵����ݣ����г�ʼ��
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
	
	realTime = RTC_GetCounter();
	
	pHead = Mem_malloc(sizeof(_PROTOCOL_HEAD));
	
	pHead->magic    = 0x7E;
	pHead->len      = t_htons(ptr->datLength - 3);  //MAP�����ݳ���
	memcpy(pHead->device,sysAllData->imei,16);       //�����豸Ψһ��ʶ�� IMEI
	pHead->msgid    = t_htonl(sysAllData->sendId);   //����֡��ˮ�� 
	pHead->time_cli = t_htonl(realTime);            //todo: ��¼��ǰ��Ҫ���͵�ʱ��    RTC_GetCounter();
	
	sysAllData->sendId++;
	
	memcpy(ptr->data,pHead,sizeof(_PROTOCOL_HEAD));
	Mem_free(pHead);                              //������ڴ�飬����һ��Ҫ�ͷŰ�
	
	crc = CRC_Compute16(&ptr->data[1],ptr->datLength-1);
	
	ptr->data[ptr->datLength++] = (crc>>8)&0xff;
	ptr->data[ptr->datLength++] = crc&0xff;
	ptr->data[ptr->datLength++] = 0x7E;
}

extern uint8_t updateBuff[2048];
extern _SystemInformation* sysAllData;   //ϵͳȫ�ֱ�����Ϣ
const uint8_t ipAddr[] ="116.62.195.99"; //������116.228.88.101   ������116.62.195.99
#define IP_Port          9527            //�˿ں�


void GlobalVarInit(void )//todo��ȫ�ֱ�����ʼ��  ���ϲ��䣬��Flash�ж�ȡ�費��Ҫ���µ� (ECU�汾)
{           
	sysAllData = Mem_malloc(256);
	
	sysAllData->isDataFlow = 1;              //�豸������ʱ��������δ����
	sysAllData->sendId = 0x80000000;         //���͵�֡��ˮ��
	sysAllData->pidNum = 27;                 //PIDָ�����
	sysAllData->softVersion = SOFTVersion;   //����汾��
	sysAllData->ipPotr = IP_Port;            
	memset(sysAllData->ipAddr,0,18);
	memcpy(sysAllData->ipAddr,ipAddr,sizeof(ipAddr));//todo��IP��ַ��������������flash�е�IP���˿ں�
	
}

 uint8_t* RecvDataAnalysis(uint8_t* ptrDataToDeal)//�������յ������ݰ�
{
	uint8_t  i = 0;
	uint16_t datLen   = 0;
	uint16_t crcDat   = 0;
	uint16_t crcCheck = 0;
	uint8_t* ptr = NULL;
	while(1)
	{
		if(ptrDataToDeal[i++] == 0x7E)
			break;
		if(i >= 20)
		{
			Mem_free(ptrDataToDeal);
			return NULL;
		}
	}
	datLen   = ptrDataToDeal[i++];
	datLen   = (datLen << 8) + ptrDataToDeal[i--];
	crcCheck = CRC_Compute16(&ptrDataToDeal[i],datLen);
	
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









