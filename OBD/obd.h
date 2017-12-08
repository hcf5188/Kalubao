




#ifndef __OBD_H__
#define __OBD_H__

#include "includes.h"


//typedef enum
//{
//	CANBAUD_100K = 0,
//	CANBAUD_250K,
//	CANBAUD_500K,
//	CANBAUD_1M
//}CANBAUD_Enum;//CAN 波特率标志

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
	long dword;
	int16_t byte16[2];
	uint8_t byte[4];
}longToUchar;

typedef struct 
{
	uint32_t canId;        //ECU的CAN ID
	uint32_t ide;          //CAN通讯，扩展帧或者标准帧
	uint8_t  *pdat;        //指针指向要发送的数据块
}CAN1DataToSend,*ptrCAN1DataToSend;

//CAN ID、帧类型、波特率的一个结构体，用来建立查询库
typedef struct
{
	U32ToUchar canId;
	uint8_t    canIde;
	CANBAUD_Enum canBaud;
}CANInformation;


typedef struct
{
	float   carSpeed;        //车速
	uint8_t carSpeedTemp;
	float   engineSpeed;     //发动机转速
	uint8_t engineSpeedTemp;
	float   allFuel;         //总喷油量
	uint8_t allFuelTemp;
	float   primaryFuel;     //主喷油量
	uint8_t primaryFuelTemp; 
	float   beforeFuel;      //预喷油量
	uint8_t beforeFuelTemp;
	float   afterFuel;       //后喷油量
	uint8_t afterFuelTemp; 
	float   nowFuel;         //当前喷油量
	uint8_t nowFuelTemp;
	float   runLen1;          //行驶距离
	float   runLen2;          //车辆距离
}CARStateVar;

#define NUMOfCANBaud       3    //测试CAN波特率的数量
#define NUMOfCANID_STD     5     //CANID标准帧的个数
#define NUMOfCANID_EXT     4     //CANID扩展帧的个数


void OBD_CAN_SendData(CAN1DataToSend sendData);//CAN1发送数据
void CAN1_BaudSet(CANBAUD_Enum baud);
void CAN1_ClearFilter(void);
void CAN1_SetFilter(uint32_t canId,uint32_t canIde);
void CARVarInit(void);


extern CARStateVar carStateVar;     //汽车有价值的状态信息


#endif








