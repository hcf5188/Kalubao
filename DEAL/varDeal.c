#include "bsp.h"


_SystemInformation sysUpdateVar; //��������������
SYS_OperationVar  varOperation;   //�����������е�ȫ�ֱ�������

#define  FRAME_HEAD_LEN    27     //��ָ�������޹ص�����֡ͷ�����ݳ���


_CDMADataToSend* CDMNSendDataInit(uint16_t length)//Ҫ���͵����ݣ����г�ʼ��
{
	_CDMADataToSend* ptr = NULL;
	ptr = Mem_malloc(sizeof(_CDMADataToSend));
	
	ptr->timeCount = 0;
	ptr->datLength = FRAME_HEAD_LEN;
	ptr->data = Mem_malloc(length);
	
	ptr->data[ptr->datLength ++] = 7;
	ptr->data[ptr->datLength ++] = 0x60;
	ptr->data[ptr->datLength ++] = 0x00;
	
	ptr->data[ptr->datLength ++] = (sysUpdateVar.ecuVersion >> 24) &0x000000FF;
	ptr->data[ptr->datLength ++] = (sysUpdateVar.ecuVersion >> 16) &0x000000FF;
	ptr->data[ptr->datLength ++] = (sysUpdateVar.ecuVersion >>  8) &0x000000FF;
	ptr->data[ptr->datLength ++] =  sysUpdateVar.ecuVersion & 0x000000FF;

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
	Mem_free(pHead);                              //������ڴ�飬����һ��Ҫ�ͷŰ�
	
	crc = CRC_Compute16(&ptr->data[1],ptr->datLength-1);
	
	ptr->data[ptr->datLength++] = (crc>>8)&0xff;
	ptr->data[ptr->datLength++] = crc&0xff;
	ptr->data[ptr->datLength++] = 0x7E;
}


extern _SystemInformation sysUpdateVar;   //ϵͳȫ�ֱ�����Ϣ

const uint8_t ipAddr[] ="116.228.88.101"; //������116.228.88.101  29999  ������116.62.195.99
#define IP_Port          29999            //�˿ں�

_OBD_PID_Cmd *ptrPIDAllDat;    //ָ��

uint8_t configData[2048] = {0};//�����洢����PID


void GlobalVarInit(void )//todo��ȫ�ֱ�����ʼ��  ���ϲ��䣬��Flash�ж�ȡ�費��Ҫ���µ� (ECU�汾)
{           
	//��Flash���������ݽ�ȫ�ֱ���
	Flash_ReadDat(SBOOT_UPGREAD_ADDR,(uint8_t *)&sysUpdateVar,sizeof(_SystemInformation));
	//��Flash�ж�ȡPID����
	Flash_ReadDat(PIDConfig_ADDR,configData,2048);
	ptrPIDAllDat = (_OBD_PID_Cmd *)configData;
		
	varOperation.pidNum = sysUpdateVar.pidNum;//�õ�PIDָ��ĸ���
	varOperation.isDataFlow   = 1;             //�豸������ʱ��������δ����
	varOperation.isEngineRun  = 0;             //��ʼ��Ϊ�������������˵�
	varOperation.sendId = 0x80000000;          //���͵�֡��ˮ��
	
	varOperation.ipPotr = IP_Port;             //��ʼ���˿ں�
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









