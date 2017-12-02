#include "bsp.h"
#include "apptask.h"

_SystemInformation sysUpdateVar; //用来保存升级用
SYS_OperationVar  varOperation;   //程序正常运行的全局变量参数

#define  FRAME_HEAD_LEN    27     //与指令数据无关的属于帧头的数据长度


_CDMADataToSend* CDMNSendDataInit(uint16_t length)//要发送的数据，进行初始化
{
	_CDMADataToSend* ptr = NULL;
	ptr = Mem_malloc(sizeof(_CDMADataToSend));
	
	ptr->timeCount = 0;
	ptr->datLength = FRAME_HEAD_LEN;
	ptr->data = Mem_malloc(length);
	
	ptr->data[ptr->datLength ++] = 7;     //ECU 配置文件版本号
	ptr->data[ptr->datLength ++] = 0x60;
	ptr->data[ptr->datLength ++] = 0x00;
	
	ptr->data[ptr->datLength ++] = (sysUpdateVar.ecuVersion >> 24) &0x000000FF;
	ptr->data[ptr->datLength ++] = (sysUpdateVar.ecuVersion >> 16) &0x000000FF;
	ptr->data[ptr->datLength ++] = (sysUpdateVar.ecuVersion >>  8) &0x000000FF;
	ptr->data[ptr->datLength ++] =  sysUpdateVar.ecuVersion & 0x000000FF;

	return ptr;
}
_CDMADataToSend* CDMNSendInfoInit(uint16_t length)//要发送的数据，进行初始化
{
	_CDMADataToSend* ptr = NULL;
	ptr = Mem_malloc(sizeof(_CDMADataToSend));
	
	ptr->timeCount = 0;
	ptr->datLength = FRAME_HEAD_LEN;
	ptr->data = Mem_malloc(length);

	return ptr;
}
uint32_t realTime = 0;
uint16_t crc = 0;
void CDMASendDataPack(_CDMADataToSend* ptr)//对上传的数据包进行帧头封装、CRC校验等
{
	_PROTOCOL_HEAD *pHead = NULL;
	
	realTime = RTC_GetCounter();//得到系统运行的RTC时间
	
	pHead = Mem_malloc(sizeof(_PROTOCOL_HEAD));
	
	pHead->magic    = 0x7E;
	pHead->len      = t_htons(ptr->datLength - 3);   //MAP层数据长度
	memcpy(pHead->device,varOperation.imei,16);      //拷贝设备唯一标识码 IMEI
	pHead->msgid    = t_htonl(varOperation.sendId);  //发送帧流水号 
	pHead->time_cli = t_htonl(realTime);             //记录当前包要发送的时间 
	
	varOperation.sendId++;
	
	memcpy(ptr->data,pHead,sizeof(_PROTOCOL_HEAD));
	Mem_free(pHead);                              //申请的内存块，用完一定要释放啊
	
	crc = CRC_Compute16(&ptr->data[1],ptr->datLength-1);
	
	ptr->data[ptr->datLength++] = (crc>>8)&0xff;
	ptr->data[ptr->datLength++] = crc&0xff;
	ptr->data[ptr->datLength++] = 0x7E;
}

extern _SystemInformation sysUpdateVar;  //系统全局变量信息

const uint8_t ipAddr[] ="116.62.195.99"; //todo: 后期是域名解析 内网：116.228.88.101  29999  外网：116.62.195.99
#define IP_Port          9527            //端口号

_OBD_PID_Cmd *ptrPIDAllDat;    //指向第一配置区
VARConfig    *ptrPIDVars;      //指向第二配置区

uint8_t configData[2048] = {0};//用来存储配置PID
void GlobalVarInit(void )//todo：全局变量初始化  不断补充，从Flash中读取需不需要更新等 (ECU版本)
{    
	//从Flash中载入数据进全局变量
	Flash_ReadDat(SBOOT_UPGREAD_ADDR,(uint8_t *)&sysUpdateVar,sizeof(_SystemInformation));
	//从Flash中读取PID参数
	Flash_ReadDat(PIDConfig_ADDR,configData,2048);
	ptrPIDAllDat = (_OBD_PID_Cmd *)configData;
		
	varOperation.pidNum = sysUpdateVar.pidNum; //得到PID指令的个数
	varOperation.isDataFlow   = 1;             //设备启动的时候，数据流未流动
	varOperation.isCDMAStart  = CDMA_CLOSE;    //CDMA初始状态为关闭
	varOperation.isEngineRun  = ENGINE_RUN;    //初始认为发动机是启动了的
	varOperation.sendId       = 0x80000000;    //发送的帧流水号
	
	varOperation.ecuVersion = sysUpdateVar.ecuVersion;
	varOperation.busType    = sysUpdateVar.busType;
	varOperation.canIdType  = sysUpdateVar.canIdType;
	varOperation.canTxId    = sysUpdateVar.canTxId;
	varOperation.canRxId    = sysUpdateVar.canRxId;
	varOperation.canBaud    = sysUpdateVar.canBaud;
	
	varOperation.pidVarNum  = sysUpdateVar.pidVarNum;
	
	ptrPIDVars              = (VARConfig*)&configData[varOperation.pidNum * 13];//得到第二配置文件的地址
	
	varOperation.ipPotr = IP_Port;             //todo:后期是域名解析  初始化端口号
	memset(varOperation.ipAddr,0,18);
	memcpy(varOperation.ipAddr,ipAddr,sizeof(ipAddr));//todo：IP地址，程序升级后用flash中的IP及端口号	
}

 uint8_t* RecvDataAnalysis(uint8_t* ptrDataToDeal)//解析接收到的数据包
{
	uint16_t  i = 0;
	uint16_t datLen   = 0;
	uint16_t crcDat   = 0;
	uint16_t crcCheck = 0;
	uint8_t* ptr = NULL;
	while(1)
	{
		if(ptrDataToDeal[i++] == 0x7E)
			break;
		if(i >= 60)
		{
			Mem_free(ptrDataToDeal);
			return NULL;
		}
	}
	datLen   = ptrDataToDeal[i++];
	datLen   = (datLen << 8) + ptrDataToDeal[i--];
	crcCheck = CRC_Compute16(&ptrDataToDeal[i],datLen + 2);
	
	crcDat   = ptrDataToDeal[i + datLen +2];
	crcDat   = (crcDat << 8) + ptrDataToDeal[i + datLen +3];
	if(crcCheck != crcDat)
	{
		Mem_free(ptrDataToDeal);
		return NULL;
	}
	ptr = Mem_malloc(datLen);
	datLen = datLen - 24;
	ptr[0] = (datLen >> 8)  & 0x00FF;
	ptr[1] = datLen & 0x00FF;
	memcpy(&ptr[2],&ptrDataToDeal[i + 26],datLen);
	
	Mem_free(ptrDataToDeal);
	return ptr;
}

extern uint16_t freOBDLed; //红
extern uint16_t freCDMALed;//黄
extern uint16_t freGPSLed; //绿

extern OS_EVENT *sendMsg;
void RevolvingSpeedDeal(void)//todo:发动机转速处理
{
	static uint8_t openClose = 0;
	static uint8_t loginFlag = 0;
//	uint8_t err;
	uint32_t currentTime = 0;  //当前时间
	uint32_t currentHour = 0;//当前小时     0点-1点   发送登录报文，请求升级
	
	currentTime = RTC_GetCounter();
	currentHour = (currentTime/3600) %24;  //得到当前的小时数
	if(currentHour > 0)
		loginFlag = 0;
	
	if(varOperation.isEngineRun == ENGINE_RUN)//发动机正在运行中
	{
		if(openClose != 1)
		{
			openClose = 1;
			//todo:打开CDMA电源、GPS电源、数据流动标志、三个小灯
			freOBDLed  = 100;
			freCDMALed = 100;
			freGPSLed  = 100;
			OSTaskResume(CDMA_LED_PRIO);
			OSTaskResume(GPS_LED_PRIO);
			OSTaskResume(OBD_LED_PRIO);
			
			CDMAPowerOpen_Close(CDMA_OPEN);//打开CDMA电源
			CDMAConfigInit();              //初始化CDMA
			OSTaskResume(CDMA_TASK_PRIO);  //回复CDMA发送任务
			
			GPS_POWER_ON;  //打开
		}
	}
	else if(varOperation.isEngineRun == ENGINE_STOP)//发动机已停止运行
	{
		if(currentHour == 0 && loginFlag == 0 && openClose == 0)//零点到,CDMA已关闭，发送登录报文
		{
			loginFlag = 1;
			openClose = 1;
			CDMAPowerOpen_Close(CDMA_OPEN);//打开CDMA电源
			CDMAConfigInit();              //初始化CDMA
			OSTaskResume(CDMA_TASK_PRIO);  //回复CDMA发送任务
			LoginDataSend();               //发送登录报文
		}
		if(openClose != 0 && varOperation.isLoginDeal == 1)//需要关闭并且无正在处理的登录报文
		{
			openClose = 0;
			varOperation.isDataFlow  = 1; //禁止数据流
			GPS_POWER_OFF;                //关闭GPS电源
			
			OSTimeDlyHMSM(0,0,5,0);       //5s的时间，应该能将所有要发送的数据都发送完毕了
			
			//todo：关闭CDMA电源、GPS电源、数据流动标志、三个小灯任务挂起并关闭
		
			OSTaskSuspend(GPS_TASK_PRIO);     //挂起GPS任务
			
			OSTaskSuspend(CDMA_TASK_PRIO);    //挂起CDMA任务
			CDMAPowerOpen_Close(CDMA_CLOSE);  //关闭CDMA电源
			
			//挂起LED灯
			OSTaskSuspend(CDMA_LED_PRIO);
			OSTaskSuspend(GPS_LED_PRIO);
			OSTaskSuspend(OBD_LED_PRIO);
			GPIO_SetBits(GPIOB,GPIO_Pin_0 | GPIO_Pin_3 | GPIO_Pin_5 | GPIO_Pin_4);//关闭所有小灯
			
			//关闭LED灯光提示  发送蜂鸣器关机提示音  
		}
	}
}








