#include "bsp.h"


extern _SystemInformation* sysAllData;

#define  FRAME_HEAD_LEN    27     //与指令数据无关的属于帧头的数据长度


_CDMADataToSend* CDMNSendDataInit(uint16_t length)//要发送的数据，进行初始化
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
void CDMASendDataPack(_CDMADataToSend* ptr)//对上传的数据包进行帧头封装、CRC校验等
{
	_PROTOCOL_HEAD *pHead = NULL;
	
	realTime = RTC_GetCounter();
	
	pHead = Mem_malloc(sizeof(_PROTOCOL_HEAD));
	
	pHead->magic    = 0x7E;
	pHead->len      = t_htons(ptr->datLength - 3);  //MAP层数据长度
	memcpy(pHead->device,sysAllData->imei,16);       //拷贝设备唯一标识码 IMEI
	pHead->msgid    = t_htonl(sysAllData->sendId);   //发送帧流水号 
	pHead->time_cli = t_htonl(realTime);            //todo: 记录当前包要发送的时间    RTC_GetCounter();
	
	sysAllData->sendId++;
	
	memcpy(ptr->data,pHead,sizeof(_PROTOCOL_HEAD));
	Mem_free(pHead);                              //申请的内存块，用完一定要释放啊
	
	crc = CRC_Compute16(&ptr->data[1],ptr->datLength-1);
	
	ptr->data[ptr->datLength++] = (crc>>8)&0xff;
	ptr->data[ptr->datLength++] = crc&0xff;
	ptr->data[ptr->datLength++] = 0x7E;
}

extern uint8_t updateBuff[2048];
extern _SystemInformation* sysAllData;   //系统全局变量信息
const uint8_t ipAddr[] ="116.62.195.99"; //内网：116.228.88.101   外网：116.62.195.99
#define IP_Port          9527            //端口号


void GlobalVarInit(void )//todo：全局变量初始化  不断补充，从Flash中读取需不需要更新等 (ECU版本)
{           
	sysAllData = Mem_malloc(256);
	
	sysAllData->isDataFlow = 1;              //设备启动的时候，数据流未流动
	sysAllData->sendId = 0x80000000;         //发送的帧流水号
	sysAllData->pidNum = 27;                 //PID指令个数
	sysAllData->softVersion = SOFTVersion;   //软件版本号
	sysAllData->ipPotr = IP_Port;            
	memset(sysAllData->ipAddr,0,18);
	memcpy(sysAllData->ipAddr,ipAddr,sizeof(ipAddr));//todo：IP地址，程序升级后用flash中的IP及端口号
	
}

 uint8_t* RecvDataAnalysis(uint8_t* ptrDataToDeal)//解析接收到的数据包
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









