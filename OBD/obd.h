

#ifndef __OBD_H__
#define __OBD_H__

#include "includes.h"

//ECU安全算法掩码

//有地址的 逐步放开   
#define ECUMASKV732  0x383B50D9   //Weichai P949V732  - 已经实现
#define ECUMASKV791  0x383B50D9   //        P949V791
#define ECUMASKV792  0x383B50D9   //        P949V792


#define ECUMASKV47   0x24807961   //Yuchai  V47  P579V47     
#define ECUMASKV72   0x24807961   //Yuchai  V72  P813V72  

#define ECUMASKV46   0x29835B3D   //        P532V46

#define ECUMASK201   0x383B50D9   //WeiChai P1499V301
#define ECUMASK211   0x62575E4E   //Yuchai  P1072V742 
#define ECUMASKV760  0x3E814311   //重汽    P1158V760
#define ECUMASKV762  0x7C1B2C30   //锡柴    P903V762

//没地址 还不敢放开
#define ECUMASK212   0x26121983   //Yuchai  P1382V762 
#define ECUMASK213   0x3E814311   //CNHTC   P1158V760
#define ECUMASK214   0x14071355   //DFCV    P1186V770 
#define ECUMASK315   0x01080003   //JND     P1664V200
#define ECUMASK216   0xF4EF7493   //SFH     P1287V770
//#define ECUMASKOT    0x29835B3D 
//#define ECUMASKCAMC  0x19A59E07


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
	uint32_t   canID;
	uint8_t    pidVerCmd[8];
}CmdVersion;

void OBD_CAN_SendData(u32 canId,u32 ide,u8* pdat);//CAN1发送数据
void CAN1_BaudSet(CANBAUD_Enum baud);
void CAN1_ClearFilter(void);
void CAN1_SetFilter(uint32_t canId,uint32_t canIde);

void SafeALG(void);                   //安全算法、过模式、进入功能模式、写入ECU新标定值
uint8_t ReadECUVersion(uint8_t cmd[]);//读取ECU版本号
void Get_Q_FromECU(void);             //得到喷油量数据，并计算出当前的模式
void StrengthFuel(void);              //将新的标定写入
void J1939DataLog(void);              //上报J1939采集到的数据

void PIDVarGet(uint8_t cmdId,uint8_t *ptrData);//第二配置文件数据解析



#endif






