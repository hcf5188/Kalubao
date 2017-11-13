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
	uint8_t  ipAddr[18];    //用来存储IP地址
	uint8_t  ipPotr[6];     //存储IP端口号
	uint8_t  imei[16];      //存储IMEI号
	uint8_t  gprsStatus;    //链路连接状态
	uint8_t  tcpStatus;     //TCP链路状态  
	
	uint32_t  sendId;        //要发送的指令流水号
	uint32_t currentTime;   //GPS 时间      日/时/分/秒
	
	uint16_t softVersion;   //软件版本号   用于OTA升级
	
}_SystemInformation;

__packed typedef struct
{
	uint16_t datLength;    //要发送的数据长度
	uint16_t timeCount;    //数据包已经存在的时间（ms）
	uint8_t  data[900];    //要发送的数据
}_CDMADataToSend;

__packed typedef struct
{
	uint16_t period;     //要发送的PID的指令周期
	uint16_t timeCount;  //数据包已经存在的时间（ms）
	uint8_t  data[9];    //要发送的数据
}_OBD_PID_Cmd;



#pragma pack(1)
typedef struct 
{
	uint8_t  magic;     //帧头标识符  0x7E
	uint16_t len;       //此帧MAP层数据长度
	char     device[16];//设备IMEI号（唯一标识）
	uint32_t msgid;     //发送的指令流水号
	uint32_t time_cli;  //数据发送的时间
}_PROTOCOL_HEAD;
#pragma pack () 


//计算CRC  处理数据包

uint16_t CRC_Compute16( uint8_t* data, uint32_t data_length);
uint16_t CRC_Compute2( uint8_t* data1, uint32_t length1, uint8_t* data2 ,uint32_t length2 );
uint16_t CRC_ComputeFile(uint16_t srcCRC,uint8_t * data,uint32_t dataLength);

void UbloxCheckSum(u8 *buf,u16 len,u8* cka,u8*ckb);//GPS校验和计算

_CDMADataToSend* CDMNSendDataInit(void);//封包前的初始化
void CDMASendDataPack(_CDMADataToSend* ptr);//对将要发送的数据进行打包



// 短整型大小端互换
#define BigLittleSwap16(A)  ((((uint16_t)(A) & 0xff00) >> 8) | (((uint16_t)(A) & 0x00ff) << 8))
// 长整型大小端互换
#define BigLittleSwap32(A)  ((((uint32_t)(A) & 0xff000000) >> 24) | (((uint32_t)(A) & 0x00ff0000) >> 8) | (((uint32_t)(A) & 0x0000ff00) << 8) | (((uint32_t)(A) & 0x000000ff) << 24))

// 本机大端返回1，小端返回0
uint32_t checkCPUendian(void);
// 模拟htonl函数，本机字节序转网络字节序
uint32_t t_htonl(uint32_t h);
// 模拟ntohl函数，网络字节序转本机字节序
uint32_t t_ntohl(uint32_t n);
// 模拟htons函数，本机字节序转网络字节序
uint16_t t_htons(uint16_t  h);
// 模拟ntohs函数，网络字节序转本机字节序
uint16_t t_ntohs(uint16_t  n);



#endif

