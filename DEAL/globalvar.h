#ifndef __GLOBALVAR_H__
#define __GLOBALVAR_H__

#include "includes.h"
typedef enum
{
	LinkOK = 0,
	LinkFault
}LinkStatus;

__packed typedef struct
{
	uint8_t  ipAddr[18];    //�����洢IP��ַ
	uint8_t  ipPotr[6];     //�洢IP�˿ں�
	uint8_t  imei[16];      //�洢IMEI��
	uint8_t  gprsStatus;    //��·����״̬
	uint8_t  tcpStatus;     //TCP��·״̬  
	
	uint32_t  sendId;        //Ҫ���͵�ָ����ˮ��
	uint32_t currentTime;   //GPS ʱ��      ��/ʱ/��/��
	
	uint16_t softVersion;   //����汾��   ����OTA����
	
}_SystemInformation;

__packed typedef struct
{
	uint16_t datLength;    //Ҫ���͵����ݳ���
	uint16_t timeCount;    //���ݰ��Ѿ����ڵ�ʱ�䣨ms��
	uint8_t  data[900];    //Ҫ���͵�����
}_CDMADataToSend;

__packed typedef struct
{
	uint16_t period;     //Ҫ���͵�PID��ָ������
	uint16_t timeCount;  //���ݰ��Ѿ����ڵ�ʱ�䣨ms��
	uint8_t  data[9];    //Ҫ���͵�����
}_OBD_PID_Cmd;



#pragma pack(1)
typedef struct 
{
	uint8_t  magic;     //֡ͷ��ʶ��  0x7E
	uint16_t len;       //��֡MAP�����ݳ���
	char     device[16];//�豸IMEI�ţ�Ψһ��ʶ��
	uint32_t msgid;     //���͵�ָ����ˮ��
	uint32_t time_cli;  //���ݷ��͵�ʱ��
}_PROTOCOL_HEAD;
#pragma pack () 


//����CRC  �������ݰ�

uint16_t CRC_Compute16( uint8_t* data, uint32_t data_length);
uint16_t CRC_Compute2( uint8_t* data1, uint32_t length1, uint8_t* data2 ,uint32_t length2 );
uint16_t CRC_ComputeFile(uint16_t srcCRC,uint8_t * data,uint32_t dataLength);

void UbloxCheckSum(u8 *buf,u16 len,u8* cka,u8*ckb);//GPSУ��ͼ���

_CDMADataToSend* CDMNSendDataInit(void);//���ǰ�ĳ�ʼ��
void CDMASendDataPack(_CDMADataToSend* ptr);//�Խ�Ҫ���͵����ݽ��д��



// �����ʹ�С�˻���
#define BigLittleSwap16(A)  ((((uint16_t)(A) & 0xff00) >> 8) | (((uint16_t)(A) & 0x00ff) << 8))
// �����ʹ�С�˻���
#define BigLittleSwap32(A)  ((((uint32_t)(A) & 0xff000000) >> 24) | (((uint32_t)(A) & 0x00ff0000) >> 8) | (((uint32_t)(A) & 0x0000ff00) << 8) | (((uint32_t)(A) & 0x000000ff) << 24))

// ������˷���1��С�˷���0
uint32_t checkCPUendian(void);
// ģ��htonl�����������ֽ���ת�����ֽ���
uint32_t t_htonl(uint32_t h);
// ģ��ntohl�����������ֽ���ת�����ֽ���
uint32_t t_ntohl(uint32_t n);
// ģ��htons�����������ֽ���ת�����ֽ���
uint16_t t_htons(uint16_t  h);
// ģ��ntohs�����������ֽ���ת�����ֽ���
uint16_t t_ntohs(uint16_t  n);



#endif

