#ifndef __GLOBALVAR_H__
#define __GLOBALVAR_H__

#include "includes.h"

#define SBOOT_UPGREAD_ADDR   	0x08007800    //此地址存放SBOOT升级用的参数

#define NEW_SOFT_ADDR           0x08030000    //此地址存放新程序代码

#define STRENGE_Q               0x08060000    //此地址保存强动力模式下的喷油量（原始值、减小值、增大值）

#define PIDCONFIG               0x08060800    //CAN 通讯参数保存地址

#define PID2CONFIGADDR          0x08061000    //第二配置文件区

#define PID1CONFIGADDR          0x08063000    //此地址存放服务器下发PID参数

#define PROMOTE_ADDR            0x08065000    //此地址保存提升动力的指令（安全算法、模式、地址、掩码等等）

#define CDMA_OPEN               0             //打开CDMA
#define CDMA_CLOSE              1             //关闭CDMA
#define ENGINE_RUN              0             //发动机正常运行
#define ENGINE_STOP             1             //发动机停止运行

#define LEDFAST                 200           //小灯快闪
#define LEDSLOW                 1000          //小灯慢闪

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
}CANBAUD_Enum;//CAN 波特率标志

typedef struct     //与升级相关的结构体   为了与王老师的Sboot兼容，此不可一字节对齐
{
	uint8_t  isSoftUpdate;  //程序是否需要升级  1 - 需要升级    0 - 不需要升级
	uint8_t  isSoftSucc;    //（兼容SBOOT）上次启动是否成功
	uint16_t curSoftVer;    //当前软件版本号
	uint16_t newSoftVer;    //更新软件版本号
    uint16_t pageNum;       //固件所占用的页数
	uint16_t newSoftCRC;    //新固件CRC
	uint32_t softByteSize;  //新固件占用的字节数
}_SystemInformation;


#pragma pack(1)             //1字节对齐

__packed typedef struct 
{
	uint32_t pidVersion;    //ECU 版本ID
	uint16_t pidNum;        //PID 指令的个数  -  配置文件
	uint16_t pidVarNum;     //需要计算的 ECU 变量的个数
	
	uint8_t  busType;       //当前用到的总线（CAN线还是K线）
	uint8_t  canIdType;     //标准帧还是扩展帧
	uint32_t canTxId;       //保存CAN发送ID
	uint32_t canRxId;       //保存CAN接收ID
	CANBAUD_Enum  canBaud;  //CAN通讯波特率
    uint8_t  pidVerCmd[8];     //自识别的结果保存
}_CANDataConfig;


__packed typedef struct//程序正常运行时候的各个参数
{
	uint32_t newSoftVersion;//新的软件版本号   用于OTA升级
	uint16_t newSoftCRC;    //新程序文件的CRC校验
	uint16_t frameNum;      //程序帧的数量，一帧128字节
	uint8_t  USB_NormalMode;//USB升级模式还是正常模式 0-正常模式，1 - USB模式
	
	uint8_t  imei[16];      //存储IMEI号
	uint8_t  iccID[20];     //存储SIM卡的ICCID号
	
	uint8_t  isDataFlow;    //数据流是否流动起来  0 - 流动起来， 1 - 未流动
	uint8_t  isEngineRun;   //发动机引擎是否开启  0 - 开启，     1 - 未开启
	uint8_t  isCDMAStart;   //CDMA电源是否开启    0 - 开启，    1 - 关闭
	uint8_t  isLoginDeal;   //登录报文是否正在处理
	
	uint8_t  isIPUpdate;    //IP地址是否需要更新   1 - 需要更新   0 - 不需要更新
	char     ipAddr[18];    //当前运行的IP地址
	uint16_t ipPotr;        //当前程序运行的IP端口号
	char     newIP_Addr[18];//存储服务器下发的IP地址
	uint16_t newIP_Potr;    //存储服务器下发的IP端口号
	
	uint8_t  gprsStatus;    //链路连接状态
	uint8_t  tcpStatus;     //TCP链路状态 
	
	uint32_t sendId;        //要发送的指令流水号
	uint32_t currentTime;   //GPS 时间        日/时/分/秒
	
	char     ecuVersion[20];//ECU版本号
	uint16_t pidNum;        //PID 参数的数量
	uint32_t pidVersion;    //当前程序运行的ECU版本号
	uint32_t newPIDVersion; //保存 登录报文下发的ECU版本
	uint16_t  newPidNum;     //PID 新的个数 
	
	uint16_t pidVarNum;     //需要运算的 ECU 变量的个数
	
	uint8_t  busType;       //当前用到的总线（CAN线还是K线）
	uint8_t  canIdType;     //标准帧还是扩展帧
	uint32_t canTxId;       //保存CAN发送ID
	uint32_t canRxId;       //保存CAN接收ID
	CANBAUD_Enum  canBaud;  //CAN通讯波特率
	
	uint16_t canTest;       //CAN 测试波特率、ID
	uint8_t  pidTset;       //测试服务器下发的PID指令
	uint8_t  strengthRun;   //正在提升动力
	uint8_t  pidRun;        //要不要发送PID指令
	uint16_t datOKLeng;
	
	signed char  oilMode;   //0 - 正常模式    1 - 强动力模式   2 - 节油模式
	uint8_t  isStrenOilOK;  //是否可以进行提升动力
	uint8_t  isUSBSendDat;  //USB是否要发送数据  0 - 不要   1 - 正在发送
	uint8_t   pidVerCmd[8];
}SYS_OperationVar;

__packed typedef struct//第二个配置文件的结构体
{
	uint8_t  dataHL;        //数据的大小端  1 - 大端    2 - 小端
	uint8_t  pidId;         //对应的PID序号
	uint8_t  varId;         //变量类型 1-车速 2-转速 3-总喷油量 4-主喷油量 5-预喷油量 6-后喷油量 7-当前喷油量
	uint8_t  startByte;     //开始字节
	uint8_t  startBit;      //开始位
	uint8_t  bitLen;        //总位长度
	float    ceo;           //系数
	float    offset;        //偏移量

}VARConfig;
__packed typedef struct     //内存监控变量
{
	uint8_t ecuVer[16];     //ECU版本
	uint8_t fuelAddr[5];    //读取喷油量曲线的地址
	uint8_t mask[4];        //安全算法掩码
	signed char coe;            //增强动力系数
	uint8_t safe1[8];       //安全算法指令1
	uint8_t safe2[8];       //安全算法指令2
	uint8_t mode1[8];       //模式指令1
	uint8_t mode2[8];       //模式指令2
	uint8_t modeOrder;      //模式指令执行顺序
	
}STRENFUEL_Struct;
__packed typedef struct     //内存监控变量
{
	uint16_t memUsedNum1;    //内存块1当前正在使用的数量
	uint16_t memUsedMax1;    //内存块1历史同一时间使用最大的数量
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
	uint32_t startTime;       //发动机启动时间
	uint32_t stopTime;        //发动机停止时间
	uint32_t totalMileage;    //此次总行程
	uint32_t totalFuel;       //总油耗
	uint32_t startlongitude;  //汽车开始 经度
	uint32_t startlatitude;   //汽车开始 纬度
	uint32_t stoplongitude;   //汽车停止 经度
	uint32_t stoplatitude;    //汽车停止 纬度
	uint8_t  rapidlyPlusNum;  //急加速次数
	uint8_t  rapidlySubNum;   //急减速次数
	uint16_t engineSpeedMax;  //最高转速
	uint16_t carSpeedMax;     //最高车速
	uint32_t messageNum;      //消息条数
	uint8_t  cdmaReStart;     //CDMA重启次数
	uint32_t netFlow;         //网络流量
	
	uint16_t carSpeed;        //车速
	uint8_t  carSpeedTemp;
	uint16_t engineSpeed;     //发动机转速
	uint8_t  engineSpeedTemp;
	uint32_t allFuel;         //总喷油量
	uint8_t  allFuelTemp;
	uint16_t primaryFuel;     //主喷油量
	uint8_t  primaryFuelTemp; 
	uint16_t beforeFuel;      //预喷油量
	uint16_t beforeFuel1;
	uint16_t beforeFuel2;
	uint16_t beforeFuel3;
	uint8_t  beforeFuelTemp;
	uint16_t afterFuel;       //后喷油量
	uint16_t afterFuel1;
	uint16_t afterFuel2;
	uint16_t afterFuel3;
	uint8_t  afterFuelTemp; 
	uint16_t curFuel;         //当前喷油量
	uint8_t  curFuelTemp;
	uint16_t  instantFuel;     //瞬时油耗
	uint32_t runLen1;         //行驶距离
	uint32_t runLen2;         //车辆距离
}CARRunRecord;

__packed typedef struct
{
	uint16_t datLength;  //要发送的数据长度
	uint16_t timeCount;  //数据包已经存在的时间（ms）
	uint8_t* data;       //要发送的数据
}_CDMADataToSend;

__packed typedef struct
{
	uint32_t period;     //要发送的PID的指令周期
	uint32_t timeCount;  //数据包已经存在的时间（ms）
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

void CDMAPowerOpen_Close(uint8_t flag);            //打开-关闭CDMA  flag 0 - 打开CDMA    1 - 关闭CDMA
void CDMAConfigInit(void );                        //初始化配置CDMA
_CDMADataToSend* CDMNSendDataInit(uint16_t length);//封包前的初始化
_CDMADataToSend* CDMNSendInfoInit(uint16_t length);//不带6000的报文
void CDMASendDataPack(_CDMADataToSend* ptr);       //对将要发送的数据进行打包
uint8_t* RecvDataAnalysis(uint8_t* ptrDataToDeal); //对接收到的数据包进行解析，并返回有效数据
void GlobalVarInit(void );                         //全局变量初始化
void LoginDataSend(void);                          //登录报文
void SendPidCmdData(uint8_t* cmdData);
void LogReport(char* fmt,...);                     //上传日志文件
void MemLog(_CDMADataToSend* ptr);                 //内存使用的日志文件

void SoftErasePage(uint32_t addr);//擦除单页 2K
void SoftProgramUpdate(uint32_t wAddr,uint8_t* ptrBuff,uint16_t datLength);//写入单页
void Save2KDataToFlash(uint8_t* ptrBuff,uint32_t flashAddr,uint16_t datLength);
void SbootParameterSaveToFlash(_SystemInformation* parameter);//保存Sboot参数
int Flash_ReadDat( uint8_t *buf,uint32_t iAddress, int32_t readLength);
int PIDConfig2DataRead(uint8_t *buf,uint32_t iAddress, int32_t readLength);

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

