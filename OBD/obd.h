

#ifndef __OBD_H__
#define __OBD_H__

#include "includes.h"

//ECU��ȫ�㷨����

#define ECUMASK47    0x24807961   //Yuchai  V47
#define ECUMASK72    0x24807961   //Yuchai  V72
#define ECUMASK17    0x383B50D9   //Weichai P949V732
#define ECUMASK201   0x383B50D9   //WeiChai P1499V301
#define ECUMASK211   0x62575E4E   //Yuchai  P1072V742 
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
//}CANBAUD_Enum;//CAN �����ʱ�־

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
	uint32_t canId;        //ECU��CAN ID
	uint32_t ide;          //CANͨѶ����չ֡���߱�׼֡
	uint8_t  *pdat;        //ָ��ָ��Ҫ���͵����ݿ�
}CAN1DataToSend,*ptrCAN1DataToSend;

//CAN ID��֡���͡������ʵ�һ���ṹ�壬����������ѯ��
typedef struct
{
	U32ToUchar canId;
	uint8_t    canIde;
	CANBAUD_Enum canBaud;
}CANInformation;



#define NUMOfCANBaud       3    //����CAN�����ʵ�����
#define NUMOfCANID_STD     5     //CANID��׼֡�ĸ���
#define NUMOfCANID_EXT     4     //CANID��չ֡�ĸ���


void OBD_CAN_SendData(CAN1DataToSend sendData);//CAN1��������
void CAN1_BaudSet(CANBAUD_Enum baud);
void CAN1_ClearFilter(void);
void CAN1_SetFilter(uint32_t canId,uint32_t canIde);

void SafeALG(void);       //��ȫ�㷨
void ReadECUVersion(void);//��ȡECU�汾��
void Get_Q_FromECU(void);



#endif








