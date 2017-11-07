#ifndef __GLOBALVAR_H__
#define __GLOBALVAR_H__

#include "includes.h"
typedef enum
{
	LinkOK = 0,
	LinkFault
}LinkStatus;

typedef struct
{
	uint8_t ipAddr[18];//�����洢IP��ַ
	uint8_t ipPotr[6]; //�洢IP�˿ں�
	uint8_t imei[18];  //�洢IMEI��
	LinkStatus link;   //��·����״̬

}CDMAInformation;











//����CRC  �������ݰ�

uint16_t CRC_Compute16( uint8_t* data, uint32_t data_length);
uint16_t CRC_Compute2( uint8_t* data1, uint32_t length1, uint8_t* data2 ,uint32_t length2 );
uint16_t CRC_ComputeFile(uint16_t srcCRC,uint8_t * data,uint32_t dataLength);

void UbloxCheckSum(u8 *buf,u16 len,u8* cka,u8*ckb);//GPSУ��ͼ���




#endif

