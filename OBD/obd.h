




#ifndef __OBD_H__
#define __OBD_H__

#include "includes.h"


typedef enum
{
	CANBAUD_100K = 0,
	CANBAUD_250K,
	CANBAUD_500K,
	CANBAUD_1M
}CANBAUD_Enum;//CAN �����ʱ�־

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
	uint32_t canId;        //ECU��CAN ID
	uint32_t ide;          //CANͨѶ����չ֡���߱�׼֡
	uint8_t  testIsOK;     //��ʼ���������CANͨѶ�Ƿ�ɹ���־   0 - CAN��û��   1 - CANͨ  2 - ����CAN���Բ�ͨ��ֻ֧��K��
	uint8_t  *pdat;        //ָ��ָ��Ҫ���͵����ݿ�
}CAN1DataToSend,*ptrCAN1DataToSend;

//CAN ID��֡���͡������ʵ�һ���ṹ�壬����������ѯ��
typedef struct
{
	U32ToUchar canId;
	uint8_t    canIde;
	CANBAUD_Enum canBaud;
}CANInformation;


#define NUMOFCAN   4    //����CAN��Ϣ������




void OBD_CAN_SendData(CAN1DataToSend sendData);//CAN1��������
void CAN1_BaudSet(CANBAUD_Enum baud);
void CAN1_ClearFilter(void);
void CAN1_SetFilter(uint32_t canId,uint32_t canIde);
#endif








