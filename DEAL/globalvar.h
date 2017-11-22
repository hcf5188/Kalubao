#ifndef __GLOBALVAR_H__
#define __GLOBALVAR_H__

#include "includes.h"

#define SOFTVersion             0x10001001    //����̼��汾��
#define ECUVersion              0x00000001    //ECU�汾��

#define SBOOT_UPGREAD_ADDR   	0x08007800    //�˵�ַ���SBOOT�����õĲ���

#define PIDConfig_ADDR          0x0802E000    //�˵�ַ��ŷ������·�PID����

#define NEW_SOFT_ADDR           0x08030000    //�˵�ַ����³������

typedef enum
{
	LinkOK = 0,
	LinkFault
}LinkStatus;

#pragma pack(1)             //1�ֽڶ���
__packed typedef struct  //��������صĽṹ��
{
	uint32_t softVersion;   //����汾��       ����OTA����
	uint32_t pageNum;
	uint8_t  isSoftUpdate;  //�����Ƿ���Ҫ����  1 - ��Ҫ����    0 - ����Ҫ����

	uint32_t ecuVersion;    //ECU �汾ID
	uint16_t pidNum;        //PID ָ��ĸ���   �����ļ�
	
	uint8_t  busType;       //��ǰ�õ������ߣ�CAN�߻���K�ߣ�
	uint8_t  canIdType;     //��׼֡������չ֡
	uint32_t canTxId;       //����CAN����ID
	uint32_t canRxId;       //����CAN����ID
	uint8_t  canBaud;       //CANͨѶ������
}_SystemInformation;

__packed typedef struct//������������ʱ��ĸ�������
{
	uint32_t newSoftVersion;//�µ�����汾��   ����OTA����
	uint16_t newSoftCRC;    //�³����ļ���CRCУ��
	uint16_t frameNum;      //����֡��������һ֡128�ֽ�
	
	uint8_t  imei[16];      //�洢IMEI��
	uint8_t  iccID[20];     //�洢SIM����ICCID��
	
	uint8_t  isDataFlow;    //�������Ƿ���������  0 - ���������� 1 - δ����
	uint8_t  isEngineRun;   //�����������Ƿ���  0 - ������     1 - δ����	
	
	uint8_t  isIPUpdate;    //IP��ַ�Ƿ���Ҫ����   1 - ��Ҫ����   0 - ����Ҫ����
	char     ipAddr[18];    //��ǰ���е�IP��ַ
	uint16_t ipPotr;        //��ǰ�������е�IP�˿ں�
	char     newIP_Addr[18];//�洢�������·���IP��ַ
	uint16_t newIP_Potr;    //�洢�������·���IP�˿ں�
	
	uint8_t  gprsStatus;    //��·����״̬
	uint8_t  tcpStatus;     //TCP��·״̬ 
	
	uint32_t sendId;        //Ҫ���͵�ָ����ˮ��
	uint32_t currentTime;   //GPS ʱ��        ��/ʱ/��/��
	
	uint16_t pidNum;        //PID ����������
	uint32_t newECUVersion; //���� ��¼�����·���ECU�汾
	uint8_t  newPidNum;     //PID �µĸ��� 
}SYS_OperationVar;

__packed typedef struct
{
	uint16_t datLength;  //Ҫ���͵����ݳ���
	uint16_t timeCount;  //���ݰ��Ѿ����ڵ�ʱ�䣨ms��
	uint8_t* data;       //Ҫ���͵�����
}_CDMADataToSend;

__packed typedef struct
{
	uint16_t period;     //Ҫ���͵�PID��ָ������
	uint16_t timeCount;  //���ݰ��Ѿ����ڵ�ʱ�䣨ms��
	uint8_t  data[9];    //Ҫ���͵�����
}_OBD_PID_Cmd;


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
uint16_t CRC_Compute2( uint8_t* data1, uint32_t length1,uint8_t* data2 ,uint32_t length2 );
uint16_t CRC_ComputeFile(uint16_t srcCRC,uint8_t * data,uint32_t dataLength);

void UbloxCheckSum(u8 *buf,u16 len,u8* cka,u8*ckb);//GPSУ��ͼ���

void CDMAPowerOpen_Close(void);//��-�ر�CDMA
_CDMADataToSend* CDMNSendDataInit(uint16_t length);//���ǰ�ĳ�ʼ��
void CDMASendDataPack(_CDMADataToSend* ptr);//�Խ�Ҫ���͵����ݽ��д��
uint8_t* RecvDataAnalysis(uint8_t* ptrDataToDeal);//�Խ��յ������ݰ����н�������������Ч����
void GlobalVarInit(void );//ȫ�ֱ�����ʼ��

void SoftErasePage(uint32_t addr);//������ҳ 2K
void SoftProgramUpdate(uint32_t wAddr,uint8_t* ptrBuff,uint16_t datLength);//д�뵥ҳ
void SaveConfigToFlash(uint8_t* ptrBuff,uint16_t datLength);
void SbootParameterSaveToFlash(_SystemInformation* parameter);//����Sboot����
int Flash_ReadDat(uint32_t iAddress, uint8_t *buf, int32_t readLength);
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

