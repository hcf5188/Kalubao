#ifndef __GLOBALVAR_H__
#define __GLOBALVAR_H__

#include "includes.h"

#define SOFTVersion             0x10001001    //软件固件版本号
#define ECUVersion              0x00000001    //ECU版本号

#define SBOOT_UPGREAD_ADDR   	0x08007800    //此地址存放SBOOT升级用的参数

#define PIDConfig_ADDR          0x0802E000    //此地址存放服务器下发PID参数

#define NEW_SOFT_ADDR           0x08030000    //此地址存放新程序代码

typedef enum
{
	LinkOK = 0,
	LinkFault
}LinkStatus;

#pragma pack(1)             //1字节对齐
__packed typedef struct  //与升级相关的结构体
{
	uint32_t softVersion;   //软件版本号       用于OTA升级
	uint32_t pageNum;
	uint8_t  isSoftUpdate;  //程序是否需要升级  1 - 需要升级    0 - 不需要升级

	uint32_t ecuVersion;    //ECU 版本ID
	uint16_t pidNum;        //PID 指令的个数   配置文件
	
	uint8_t  busType;       //当前用到的总线（CAN线还是K线）
	uint8_t  canIdType;     //标准帧还是扩展帧
	uint32_t canTxId;       //保存CAN发送ID
	uint32_t canRxId;       //保存CAN接收ID
	uint8_t  canBaud;       //CAN通讯波特率
}_SystemInformation;

__packed typedef struct//程序正常运行时候的各个参数
{
	uint32_t newSoftVersion;//新的软件版本号   用于OTA升级
	uint16_t newSoftCRC;    //新程序文件的CRC校验
	uint16_t frameNum;      //程序帧的数量，一帧128字节
	
	uint8_t  imei[16];      //存储IMEI号
	uint8_t  iccID[20];     //存储SIM卡的ICCID号
	
	uint8_t  isDataFlow;    //数据流是否流动起来  0 - 流动起来， 1 - 未流动
	uint8_t  isEngineRun;   //发动机引擎是否开启  0 - 开启，     1 - 未开启	
	
	uint8_t  isIPUpdate;    //IP地址是否需要更新   1 - 需要更新   0 - 不需要更新
	char     ipAddr[18];    //当前运行的IP地址
	uint16_t ipPotr;        //当前程序运行的IP端口号
	char     newIP_Addr[18];//存储服务器下发的IP地址
	uint16_t newIP_Potr;    //存储服务器下发的IP端口号
	
	uint8_t  gprsStatus;    //链路连接状态
	uint8_t  tcpStatus;     //TCP链路状态 
	
	uint32_t sendId;        //要发送的指令流水号
	uint32_t currentTime;   //GPS 时间        日/时/分/秒
	
	uint16_t pidNum;        //PID 参数的数量
	uint32_t newECUVersion; //保存 登录报文下发的ECU版本
	uint8_t  newPidNum;     //PID 新的个数 
}SYS_OperationVar;

__packed typedef struct
{
	uint16_t datLength;  //要发送的数据长度
	uint16_t timeCount;  //数据包已经存在的时间（ms）
	uint8_t* data;       //要发送的数据
}_CDMADataToSend;

__packed typedef struct
{
	uint16_t period;     //要发送的PID的指令周期
	uint16_t timeCount;  //数据包已经存在的时间（ms）
	uint8_t  data[9];    //要发送的数据
}_OBD_PID_Cmd;


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
uint16_t CRC_Compute2( uint8_t* data1, uint32_t length1,uint8_t* data2 ,uint32_t length2 );
uint16_t CRC_ComputeFile(uint16_t srcCRC,uint8_t * data,uint32_t dataLength);

void UbloxCheckSum(u8 *buf,u16 len,u8* cka,u8*ckb);//GPS校验和计算

void CDMAPowerOpen_Close(void);//打开-关闭CDMA
_CDMADataToSend* CDMNSendDataInit(uint16_t length);//封包前的初始化
void CDMASendDataPack(_CDMADataToSend* ptr);//对将要发送的数据进行打包
uint8_t* RecvDataAnalysis(uint8_t* ptrDataToDeal);//对接收到的数据包进行解析，并返回有效数据
void GlobalVarInit(void );//全局变量初始化

void SoftErasePage(uint32_t addr);//擦除单页 2K
void SoftProgramUpdate(uint32_t wAddr,uint8_t* ptrBuff,uint16_t datLength);//写入单页
void SaveConfigToFlash(uint8_t* ptrBuff,uint16_t datLength);
void SbootParameterSaveToFlash(_SystemInformation* parameter);//保存Sboot参数
int Flash_ReadDat(uint32_t iAddress, uint8_t *buf, int32_t readLength);
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

