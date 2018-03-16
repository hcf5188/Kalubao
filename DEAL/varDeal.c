#include "bsp.h"
#include "apptask.h"

    //与指令数据无关的属于帧头的数据长度

_CDMADataToSend* CDMNSendDataInit(uint16_t length)//要发送的数据，进行初始化
{
	_CDMADataToSend* ptr = NULL;
	ptr = Mem_malloc(sizeof(_CDMADataToSend));
	
	ptr->timeCount = 0;
	ptr->datLength = FRAME_HEAD_LEN;
	ptr->data = Mem_malloc(length);
	
	ptr->data[ptr->datLength ++] = 7;             //ECU 配置文件版本号
	ptr->data[ptr->datLength ++] = 0x60;
	ptr->data[ptr->datLength ++] = 0x00;
	
	ptr->data[ptr->datLength ++] = (canDataConfig.pidVersion >> 24) & 0x000000FF;
	ptr->data[ptr->datLength ++] = (canDataConfig.pidVersion >> 16) & 0x000000FF;
	ptr->data[ptr->datLength ++] = (canDataConfig.pidVersion >>  8) & 0x000000FF;
	ptr->data[ptr->datLength ++] =  canDataConfig.pidVersion & 0x000000FF;

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
uint16_t crc      = 0;
void CDMASendDataPack(_CDMADataToSend* ptr)		//对上传的数据包进行帧头封装、CRC校验等
{
	_PROTOCOL_HEAD *pHead = NULL;
	realTime = RTC_GetCounter();				//得到系统运行的RTC时间
	pHead = Mem_malloc(sizeof(_PROTOCOL_HEAD));
	pHead->magic    = 0x7E;
	pHead->len      = t_htons(ptr->datLength - 3);   //MAP层数据长度
	memcpy(pHead->device,varOperation.imei,16);      //拷贝设备唯一标识码 IMEI
	pHead->msgid    = t_htonl(varOperation.sendId);  //发送帧流水号 
	pHead->time_cli = t_htonl(realTime);             //记录当前包要发送的时间 
	
	varOperation.sendId ++;
	memcpy(ptr->data,pHead,sizeof(_PROTOCOL_HEAD));
	Mem_free(pHead);                                 //申请的内存块，用完一定要释放啊
	
	crc = CRC_Compute16(&ptr->data[1],ptr -> datLength-1);
	
	ptr->data[ptr->datLength++] = (crc>>8) & 0xff;
	ptr->data[ptr->datLength++] = crc & 0xff;
	ptr->data[ptr->datLength++] = 0x7E;
}

/***********************************************************************************/
  
#if (BEN_S_C == 0)                
const uint8_t ipAddr[] = "116.228.90.118"; //本地IP
#define IP_Port          30020             //本地端口号
#elif (BEN_S_C == 1)
const uint8_t ipAddr[] = "116.62.195.99";  //todo: 后期是域名解析 外网：116.62.195.99    9998
#define IP_Port           9998             //端口号 9998 6556
#elif (BEN_S_C == 2)
#define IP_Port           9998
#endif
const uint8_t proIPAddr[] = "tcp.51gonggui.com";// "47.96.8.114"; //生产环境：tcp.51gonggui.com

#define ProIP_Port        9998             //生产环境端口
/***********************************************************************************/


_OBD_PID_Cmd *ptrPIDAllDat;    //指向第一配置区
VARConfig    *ptrPIDVars;      //指向第二配置区

uint8_t configData[6000] = {0};//用来存储配置PID
uint8_t strengPower[300] = {0};//用来存储强动力模式下的数据
uint8_t pid2Config[300]  = {0};//此数组用来保存PID第二配置文件数据
void GlobalVarInit(void )      //todo：全局变量初始化  不断补充，从Flash中读取需不需要更新等 (ECU版本)
{    
	uint8_t *ptrMode;
	uint8_t offset = 0;
	ptrMode = Mem_malloc(80);
	
	//从Flash中载入数据进全局变量
	Flash_ReadDat((uint8_t *)&sysUpdateVar,SBOOT_UPGREAD_ADDR,sizeof(_SystemInformation));
	//从Flash中读取PID参数
	Flash_ReadDat((uint8_t *)&canDataConfig,PIDCONFIG,sizeof(_CANDataConfig));
	
	PIDConfig2DataRead(configData,PID1CONFIGADDR,6000);
	ptrPIDAllDat = (_OBD_PID_Cmd *)configData;
	
	Flash_ReadDat(pid2Config,PID2CONFIGADDR,300); //读取第二配置文件数据
	ptrPIDVars   = (VARConfig*)pid2Config;
	
	Flash_ReadDat(strengPower,STRENGE_Q,300);  //读出喷油量的原始值
	if(strengPower[0] != 0xAF)                 //从未记录过该车的喷油量
		memset(strengPower,0,300);	
	
	Flash_ReadDat(ptrMode,PROMOTE_ADDR,80);    //读取提升动力相关参数
	memcpy(strengthFuelFlash.ecuVer,&ptrMode[offset],16);
	offset += 16;
	memcpy(strengthFuelFlash.fuelAddr,&ptrMode[offset],5);
	offset += 5;
	memcpy(strengthFuelFlash.mask ,&ptrMode[offset],4);
	offset += 4;
	memcpy(strengthFuelFlash.safe1,&ptrMode[offset],8);
	offset += 8;
	memcpy(strengthFuelFlash.safe2,&ptrMode[offset],8);
	offset += 8;
	memcpy(strengthFuelFlash.mode1,&ptrMode[offset],8);
	offset += 8;
	memcpy(strengthFuelFlash.mode2,&ptrMode[offset],8);
	offset += 8;
	strengthFuelFlash.modeOrder = ptrMode[offset++];
	Mem_free(ptrMode);
	
	varOperation.pidNum      = canDataConfig.pidNum;//得到PID指令的个数
	varOperation.isDataFlow  = 1;              //设备启动的时候，数据流未流动
	varOperation.isCDMAStart = CDMA_CLOSE;     //CDMA初始状态为关闭
	varOperation.isEngineRun = ENGINE_RUN;     //初始认为发动机是启动了的
	varOperation.sendId      = 0x80000000;     //发送的帧流水号
	
	varOperation.pidVersion = canDataConfig.pidVersion;
	varOperation.busType    = canDataConfig.busType;
	varOperation.canIdType  = canDataConfig.canIdType;
	varOperation.canTxId    = canDataConfig.canTxId;
	varOperation.canRxId    = canDataConfig.canRxId;
	varOperation.canBaud    = canDataConfig.canBaud;
	varOperation.oilMode    = 0;  //默认正常模式
	varOperation.isStrenOilOK = 0;//默认不可以进行动力提升
	varOperation.strengthRun = 0;
	varOperation.datOKLeng   = 0;
	varOperation.pidRun      = 1;
	memset(varOperation.ecuVersion,0,20);
	
	varOperation.pidVarNum  = canDataConfig.pidVarNum;
	
	varOperation.ipPotr = IP_Port;             //todo:后期是域名解析  初始化端口号
	memset(varOperation.ipAddr,0,18);
#if (BEN_S_C < 2)
	memcpy(varOperation.ipAddr,ipAddr,sizeof(ipAddr));//todo：IP地址，程序升级后用flash中的IP及端口号	//采用域名解析了
#endif
}
 uint8_t* RecvDataAnalysis(uint8_t* ptrDataToDeal)//解析接收到的数据包
{
	uint16_t i = 0;
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

void LogReport(char* fmt,...)          //上传日志文件
{
	u8 datLen,err;
	va_list ap;
	uint8_t * ptrSaveLog;

	if(cdmaDataToSend->datLength > 750)
		return;
	ptrSaveLog = Mem_malloc(255);
	va_start(ap,fmt);
	vsprintf((char*)ptrSaveLog,fmt,ap);
	va_end(ap);
	
	datLen=strlen((const char*)ptrSaveLog);//此次发送数据的长度
	
	if(varOperation.isDataFlow != 1)
	{
		OSMutexPend(CDMASendMutex,0,&err);
		
		if((datLen + cdmaLogData->top) < 900)				
		{
			memcpy(&cdmaLogData->base[cdmaLogData->top],ptrSaveLog,datLen);
			cdmaLogData->top += datLen;
		}
		OSMutexPost(CDMASendMutex);
	}
	
	Mem_free(ptrSaveLog);
}

extern MEM_Check allMemState;         //内存监控变量
//上报内存使用日志文件  用于前期研发阶段的内存块使用情况监视

void MemLog(_CDMADataToSend* ptr)
{
	LogReport("\r\n03-1:%d;2:%d;3:%d;4:%d;5:%d;6:%d;7:%d;",\
			allMemState.memUsedMax1,\
			allMemState.memUsedMax2,\
			allMemState.memUsedMax3,\
			allMemState.memUsedMax4,\
			allMemState.memUsedMax5,\
			allMemState.memUsedMax6,\
			allMemState.memUsedMax7);
}
//车辆运行的数据 初始化
void CARVarInit(void)
{
	carAllRecord.startTime       = 0;//发动机启动时间
	carAllRecord.stopTime        = 0;//发动机停止时间
	carAllRecord.totalMileage    = 0;//此次总行程
	carAllRecord.totalFuel       = 0;//总油耗
	carAllRecord.startlongitude  = 0;//汽车开始 经度
	carAllRecord.startlatitude   = 0;//汽车开始 纬度
	carAllRecord.stoplongitude   = 0;//汽车停止 经度
	carAllRecord.stoplatitude    = 0;//汽车停止 纬度
	carAllRecord.rapidlyPlusNum  = 0;//急加速次数
	carAllRecord.rapidlySubNum   = 0;//急减速次数
	carAllRecord.engineSpeedMax  = 0;//最高转速
	carAllRecord.carSpeedMax     = 0;//最高车速
	carAllRecord.messageNum      = 0;//消息条数
	carAllRecord.cdmaReStart     = 0;//CDMA重启次数
	carAllRecord.netFlow         = 0;//网络流量
	
	carAllRecord.afterFuel       = 0;//后喷油量
	carAllRecord.afterFuel1      = 0;
	carAllRecord.afterFuel2      = 0;
	carAllRecord.afterFuel3      = 0;
	carAllRecord.afterFuelTemp   = 0;
	carAllRecord.allFuel         = 0; //总喷油量
	carAllRecord.allFuelTemp     = 0;
	carAllRecord.beforeFuel      = 0; //预喷油量
	carAllRecord.beforeFuel1     = 0;
	carAllRecord.beforeFuel2     = 0;
	carAllRecord.beforeFuel3     = 0;
	carAllRecord.beforeFuelTemp  = 0;
	carAllRecord.carSpeed        = 0; //车速
	carAllRecord.carSpeedTemp    = 0;
	carAllRecord.curFuel         = 0; //当前喷油量
	carAllRecord.curFuelTemp     = 0;
	carAllRecord.primaryFuel     = 0; //主喷油量
	carAllRecord.primaryFuelTemp = 0;
	carAllRecord.engineSpeed     = 0; //发动机转速
	carAllRecord.engineSpeedTemp = 0; 
	carAllRecord.runLen1         = 0; //行驶距离为 0
	carAllRecord.runLen2         = 0; //车辆距离为 0
	carAllRecord.instantFuel     = 0; //瞬时油耗
}





