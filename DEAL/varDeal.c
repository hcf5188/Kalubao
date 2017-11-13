#include "bsp.h"
extern _SystemInformation sysAllData;

#define  FRAME_HEAD_LEN    27     //与指令数据无关的属于帧头的数据长度
_CDMADataToSend* CDMNSendDataInit(void)
{
	_CDMADataToSend* ptr = NULL;
	ptr = Mem_malloc(sizeof(_CDMADataToSend));
	
	ptr->timeCount = 0;
	ptr->datLength = FRAME_HEAD_LEN;
	
	return ptr;
}

void CDMASendDataPack(_CDMADataToSend* ptr)
{
	_PROTOCOL_HEAD *pHead = NULL;
	uint16_t crc = 0;

	pHead = Mem_malloc(sizeof(_PROTOCOL_HEAD));
	
	pHead->magic    = 0x7E;
	pHead->len      = t_htons(ptr->datLength - 3);     //MAP层数据长度
	memcpy(pHead->device,sysAllData.imei,16);          //拷贝设备唯一标识码 IMEI
	pHead->msgid    = t_htonl(sysAllData.sendId);      //发送帧流水号 
	pHead->time_cli = t_htonl(sysAllData.currentTime); //todo: 记录当前包要发送的时间    RTC_GetCounter();
	
	sysAllData.sendId++;
	
	memcpy(ptr->data,pHead,sizeof(_PROTOCOL_HEAD));
	Mem_free(pHead);                              //申请的内存块，用完一定要释放啊
	
	crc = CRC_Compute16(&ptr->data[1],ptr->datLength-1);
	
	ptr->data[ptr->datLength++] = (crc>>8)&0xff;
	ptr->data[ptr->datLength++] = crc&0xff;
	ptr->data[ptr->datLength++] = 0x7E;
}
