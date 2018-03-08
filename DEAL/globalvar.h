#ifndef __GLOBALVAR_H__
#define __GLOBALVAR_H__

#include "includes.h"

#define SBOOT_UPGREAD_ADDR   	0x08007800    //�˵�ַ���SBOOT�����õĲ���

#define NEW_SOFT_ADDR           0x08030000    //�˵�ַ����³������

#define STRENGE_Q               0x08060000    //�˵�ַ����ǿ����ģʽ�µ���������ԭʼֵ����Сֵ������ֵ��

#define PIDCONFIG               0x08060800    //CAN ͨѶ���������ַ

#define PID2CONFIGADDR          0x08061000    //�ڶ������ļ���

#define PID1CONFIGADDR          0x08063000    //�˵�ַ��ŷ������·�PID����

#define PROMOTE_ADDR            0x08065000    //�˵�ַ��������������ָ���ȫ�㷨��ģʽ����ַ������ȵȣ�

#define CDMA_OPEN               0             //��CDMA
#define CDMA_CLOSE              1             //�ر�CDMA
#define ENGINE_RUN              0             //��������������
#define ENGINE_STOP             1             //������ֹͣ����

#define LEDFAST                 200           //С�ƿ���
#define LEDSLOW                 1000          //С������

typedef enum
{
	LinkOK = 0,
	LinkFault
}LinkStatus;

typedef enum
{
	CANBAUD_100K = 0,
	CANBAUD_250K,
	CANBAUD_500K,
	CANBAUD_1M
}CANBAUD_Enum;//CAN �����ʱ�־

typedef struct     //��������صĽṹ��   Ϊ��������ʦ��Sboot���ݣ��˲���һ�ֽڶ���
{
	uint8_t  isSoftUpdate;  //�����Ƿ���Ҫ����  1 - ��Ҫ����    0 - ����Ҫ����
	uint8_t  isSoftSucc;    //������SBOOT���ϴ������Ƿ�ɹ�
	uint16_t curSoftVer;    //��ǰ����汾��
	uint16_t newSoftVer;    //��������汾��
    uint16_t pageNum;       //�̼���ռ�õ�ҳ��
	uint16_t newSoftCRC;    //�¹̼�CRC
	uint32_t softByteSize;  //�¹̼�ռ�õ��ֽ���
}_SystemInformation;


#pragma pack(1)             //1�ֽڶ���

__packed typedef struct 
{
	uint32_t pidVersion;    //ECU �汾ID
	uint16_t pidNum;        //PID ָ��ĸ���  -  �����ļ�
	uint16_t pidVarNum;     //��Ҫ����� ECU �����ĸ���
	
	uint8_t  busType;       //��ǰ�õ������ߣ�CAN�߻���K�ߣ�
	uint8_t  canIdType;     //��׼֡������չ֡
	uint32_t canTxId;       //����CAN����ID
	uint32_t canRxId;       //����CAN����ID
	CANBAUD_Enum  canBaud;  //CANͨѶ������
    uint8_t  pidVerCmd[8];     //��ʶ��Ľ������
}_CANDataConfig;


__packed typedef struct//������������ʱ��ĸ�������
{
	uint32_t newSoftVersion;//�µ�����汾��   ����OTA����
	uint16_t newSoftCRC;    //�³����ļ���CRCУ��
	uint16_t frameNum;      //����֡��������һ֡128�ֽ�
	uint8_t  USB_NormalMode;//USB����ģʽ��������ģʽ 0-����ģʽ��1 - USBģʽ
	
	uint8_t  imei[16];      //�洢IMEI��
	uint8_t  iccID[20];     //�洢SIM����ICCID��
	
	uint8_t  isDataFlow;    //�������Ƿ���������  0 - ���������� 1 - δ����
	uint8_t  isEngineRun;   //�����������Ƿ���  0 - ������     1 - δ����
	uint8_t  isCDMAStart;   //CDMA��Դ�Ƿ���    0 - ������    1 - �ر�
	uint8_t  isLoginDeal;   //��¼�����Ƿ����ڴ���
	
	uint8_t  isIPUpdate;    //IP��ַ�Ƿ���Ҫ����   1 - ��Ҫ����   0 - ����Ҫ����
	char     ipAddr[18];    //��ǰ���е�IP��ַ
	uint16_t ipPotr;        //��ǰ�������е�IP�˿ں�
	char     newIP_Addr[18];//�洢�������·���IP��ַ
	uint16_t newIP_Potr;    //�洢�������·���IP�˿ں�
	
	uint8_t  gprsStatus;    //��·����״̬
	uint8_t  tcpStatus;     //TCP��·״̬ 
	
	uint32_t sendId;        //Ҫ���͵�ָ����ˮ��
	uint32_t currentTime;   //GPS ʱ��        ��/ʱ/��/��
	
	char     ecuVersion[20];//ECU�汾��
	uint16_t pidNum;        //PID ����������
	uint32_t pidVersion;    //��ǰ�������е�ECU�汾��
	uint32_t newPIDVersion; //���� ��¼�����·���ECU�汾
	uint16_t  newPidNum;     //PID �µĸ��� 
	
	uint16_t pidVarNum;     //��Ҫ����� ECU �����ĸ���
	
	uint8_t  busType;       //��ǰ�õ������ߣ�CAN�߻���K�ߣ�
	uint8_t  canIdType;     //��׼֡������չ֡
	uint32_t canTxId;       //����CAN����ID
	uint32_t canRxId;       //����CAN����ID
	CANBAUD_Enum  canBaud;  //CANͨѶ������
	
	uint16_t canTest;       //CAN ���Բ����ʡ�ID
	uint8_t  pidTset;       //���Է������·���PIDָ��
	uint8_t  strengthRun;   //������������
	uint8_t  pidRun;        //Ҫ��Ҫ����PIDָ��
	uint16_t datOKLeng;
	
	signed char  oilMode;   //0 - ����ģʽ    1 - ǿ����ģʽ   2 - ����ģʽ
	uint8_t  isStrenOilOK;  //�Ƿ���Խ�����������
	uint8_t  isUSBSendDat;  //USB�Ƿ�Ҫ��������  0 - ��Ҫ   1 - ���ڷ���
	uint8_t   pidVerCmd[8];
}SYS_OperationVar;

__packed typedef struct//�ڶ��������ļ��Ľṹ��
{
	uint8_t  dataHL;        //���ݵĴ�С��  1 - ���    2 - С��
	uint8_t  pidId;         //��Ӧ��PID���
	uint8_t  varId;         //�������� 1-���� 2-ת�� 3-�������� 4-�������� 5-Ԥ������ 6-�������� 7-��ǰ������
	uint8_t  startByte;     //��ʼ�ֽ�
	uint8_t  startBit;      //��ʼλ
	uint8_t  bitLen;        //��λ����
	float    ceo;           //ϵ��
	float    offset;        //ƫ����

}VARConfig;
__packed typedef struct     //�ڴ��ر���
{
	uint8_t ecuVer[16];     //ECU�汾
	uint8_t fuelAddr[5];    //��ȡ���������ߵĵ�ַ
	uint8_t mask[4];        //��ȫ�㷨����
	signed char coe;            //��ǿ����ϵ��
	uint8_t safe1[8];       //��ȫ�㷨ָ��1
	uint8_t safe2[8];       //��ȫ�㷨ָ��2
	uint8_t mode1[8];       //ģʽָ��1
	uint8_t mode2[8];       //ģʽָ��2
	uint8_t modeOrder;      //ģʽָ��ִ��˳��
	
}STRENFUEL_Struct;
__packed typedef struct     //�ڴ��ر���
{
	uint16_t memUsedNum1;    //�ڴ��1��ǰ����ʹ�õ�����
	uint16_t memUsedMax1;    //�ڴ��1��ʷͬһʱ��ʹ����������
	uint8_t memUsedNum2;
	uint8_t memUsedMax2;
	uint8_t memUsedNum3;
	uint8_t memUsedMax3;
	uint8_t memUsedNum4;
	uint8_t memUsedMax4;
	uint8_t memUsedNum5;
	uint8_t memUsedMax5;
	uint8_t memUsedNum6;
	uint8_t memUsedMax6;
	uint8_t memUsedNum7;
	uint8_t memUsedMax7;
}MEM_Check;

__packed typedef struct 
{
	uint32_t startTime;       //����������ʱ��
	uint32_t stopTime;        //������ֹͣʱ��
	uint32_t totalMileage;    //�˴����г�
	uint32_t totalFuel;       //���ͺ�
	uint32_t startlongitude;  //������ʼ ����
	uint32_t startlatitude;   //������ʼ γ��
	uint32_t stoplongitude;   //����ֹͣ ����
	uint32_t stoplatitude;    //����ֹͣ γ��
	uint8_t  rapidlyPlusNum;  //�����ٴ���
	uint8_t  rapidlySubNum;   //�����ٴ���
	uint16_t engineSpeedMax;  //���ת��
	uint16_t carSpeedMax;     //��߳���
	uint32_t messageNum;      //��Ϣ����
	uint8_t  cdmaReStart;     //CDMA��������
	uint32_t netFlow;         //��������
	
	uint16_t carSpeed;        //����
	uint8_t  carSpeedTemp;
	uint16_t engineSpeed;     //������ת��
	uint8_t  engineSpeedTemp;
	uint32_t allFuel;         //��������
	uint8_t  allFuelTemp;
	uint16_t primaryFuel;     //��������
	uint8_t  primaryFuelTemp; 
	uint16_t beforeFuel;      //Ԥ������
	uint16_t beforeFuel1;
	uint16_t beforeFuel2;
	uint16_t beforeFuel3;
	uint8_t  beforeFuelTemp;
	uint16_t afterFuel;       //��������
	uint16_t afterFuel1;
	uint16_t afterFuel2;
	uint16_t afterFuel3;
	uint8_t  afterFuelTemp; 
	uint16_t curFuel;         //��ǰ������
	uint8_t  curFuelTemp;
	uint16_t  instantFuel;     //˲ʱ�ͺ�
	uint32_t runLen1;         //��ʻ����
	uint32_t runLen2;         //��������
}CARRunRecord;

__packed typedef struct
{
	uint16_t datLength;  //Ҫ���͵����ݳ���
	uint16_t timeCount;  //���ݰ��Ѿ����ڵ�ʱ�䣨ms��
	uint8_t* data;       //Ҫ���͵�����
}_CDMADataToSend;

__packed typedef struct
{
	uint32_t period;     //Ҫ���͵�PID��ָ������
	uint32_t timeCount;  //���ݰ��Ѿ����ڵ�ʱ�䣨ms��
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

void CDMAPowerOpen_Close(uint8_t flag);            //��-�ر�CDMA  flag 0 - ��CDMA    1 - �ر�CDMA
void CDMAConfigInit(void );                        //��ʼ������CDMA
_CDMADataToSend* CDMNSendDataInit(uint16_t length);//���ǰ�ĳ�ʼ��
_CDMADataToSend* CDMNSendInfoInit(uint16_t length);//����6000�ı���
void CDMASendDataPack(_CDMADataToSend* ptr);       //�Խ�Ҫ���͵����ݽ��д��
uint8_t* RecvDataAnalysis(uint8_t* ptrDataToDeal); //�Խ��յ������ݰ����н�������������Ч����
void GlobalVarInit(void );                         //ȫ�ֱ�����ʼ��
void LoginDataSend(void);                          //��¼����
void SendPidCmdData(uint8_t* cmdData);
void LogReport(char* fmt,...);                     //�ϴ���־�ļ�
void MemLog(_CDMADataToSend* ptr);                 //�ڴ�ʹ�õ���־�ļ�

void SoftErasePage(uint32_t addr);//������ҳ 2K
void SoftProgramUpdate(uint32_t wAddr,uint8_t* ptrBuff,uint16_t datLength);//д�뵥ҳ
void Save2KDataToFlash(uint8_t* ptrBuff,uint32_t flashAddr,uint16_t datLength);
void SbootParameterSaveToFlash(_SystemInformation* parameter);//����Sboot����
int Flash_ReadDat( uint8_t *buf,uint32_t iAddress, int32_t readLength);
int PIDConfig2DataRead(uint8_t *buf,uint32_t iAddress, int32_t readLength);

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

