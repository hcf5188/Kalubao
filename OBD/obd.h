




#ifndef __OBD_H__
#define __OBD_H__

#include "includes.h"


typedef enum
{
	CANBAUD_100K = 0,
	CANBAUD_250K,
	CANBAUD_500K,
	CANBAUD_1M
}CANBAUD_Enum;//CAN 波特率标志

typedef union 
{
	uint16_t dword;
	uint8_t  byte[2];
}UintToUchar;

typedef union
{
	uint32_t dword;
	uint16_t byte16[2];
	uint8_t  byte[4];
}U32ToUchar;

typedef union
{
	int32_t dword;
	int16_t byte16[2];
	uint8_t byte[4];
}longToUchar;

typedef struct 
{
	uint32_t canId;        //ECU的CAN ID
	uint32_t ide;          //CAN通讯，扩展帧或者标准帧
	uint8_t  testIsOK;     //初始化程序测试CAN通讯是否成功标志   0 - CAN还没测   1 - CAN通  2 - 遍历CAN，仍不通，只支持K线
	uint8_t  *pdat;        //指针指向要发送的数据块
}CAN1DataToSend,*ptrCAN1DataToSend;

//CAN ID、帧类型、波特率的一个结构体，用来建立查询库
typedef struct
{
	U32ToUchar canId;
	uint8_t    canIde;
	CANBAUD_Enum canBaud;
}CANInformation;


#define NUMOFCAN   4    //测试CAN信息的数量




void OBD_CAN_SendData(CAN1DataToSend sendData);//CAN1发送数据
void CAN1_BaudSet(CANBAUD_Enum baud);
void CAN1_ClearFilter(void);
void CAN1_SetFilter(uint32_t canId,uint32_t canIde);
#endif








