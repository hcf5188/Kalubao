#include "includes.h"
#include "bsp.h"

/*************************     处理函数声明          **********************/
static void LoginDataSend(void);
static void RecvDatDeal(uint8_t* ptr);
static void OTA_GetSoftInfo(void );
static void OTA_Updata(void );
static void ConfigUpdata(void );
static void GetConfigInfo(void);

/*************************     本.c文件用到的变量         **********************/
#define ZIPRECVBUF_SIZE  5       //RECV接收消息队列保存消息的最大量
OS_EVENT *ZIPRecv_Q;             //指向RECV消息队列的指针
void *ZIPRecBuf[ZIPRECVBUF_SIZE];

extern _SystemInformation* sysAllData;//系统全局变量
extern OS_EVENT *CDMASendQ;       //通过CDMA向服务器发送采集到的OBD、GPS、登录报文等数据

//本任务用来上发登录报文、处理OTA升级、配置文件升级、模式切换（强动力模式、节油模式等）
void CDMARecvTask(void *pdata)
{
	uint8_t err;
	uint8_t* ptrRECV = NULL;
	uint8_t* ptrDeal = NULL;
	
	ZIPRecv_Q    = OSQCreate(&ZIPRecBuf[0],ZIPRECVBUF_SIZE);//建立“ZIPRECV”处理消息队列
	
	OSTimeDlyHMSM(0,0,25,200);     //todo:此处应该挂起
	LoginDataSend();               //发送登录报文
	while(1)
	{
		ptrRECV = OSQPend(ZIPRecv_Q,2000,&err);
		if(err == OS_ERR_NONE)
		{
			ptrDeal = RecvDataAnalysis(ptrRECV);
			
			RecvDatDeal(ptrDeal);
		}
		else
		{
			OSTimeDlyHMSM(1,0,0,0);//todo:如果是客户手机选定模式切换的话，就不能延时等待了
			//todo:判断时间是否在凌晨1-3点，判断发动机停机
			//LoginDataSend();        //发送登录报文
		}
	}
}
//登录报文
static void LoginDataSend(void)
{
	uint8_t err;
	uint32_t buff;
	_CDMADataToSend* loginData = CDMNSendDataInit(100);        //发送登录报文
	
	loginData->data[loginData->datLength++] = 31;
	loginData->data[loginData->datLength++] = 0x50;
	loginData->data[loginData->datLength++] = 0x01;
	
	buff = t_htonl(SOFTVersion);                            //软件固件版本  
	memcpy(&loginData->data[loginData->datLength],&buff,4);
	loginData->datLength += 4;
	
	buff = t_htonl(sysAllData->ecuVersion);
	memcpy(&loginData->data[loginData->datLength],&buff,4);
	loginData->datLength += 4;
	
	memcpy(&loginData->data[loginData->datLength],sysAllData->iccID,20);
	loginData->datLength += 20;
	
	CDMASendDataPack(loginData);//对登录报文进行打包（添加帧头、校验码、帧尾）
	err = OSQPost(CDMASendQ,loginData);
	if(err != OS_ERR_NONE)
	{
		Mem_free(loginData);
	}
}
extern void CDMASendCmd(const uint8_t sendDat[],char* compString,uint16_t sendLength);

extern OS_EVENT *sendMsg; 

static void RecvDatDeal(uint8_t* ptr)//对服务器回复的登录报文进行解析
{
	uint16_t cmdId = 0;
	uint8_t  ipLen = 0;
	uint32_t ecuId = 0;
	uint32_t serverTime  = 0;
	uint32_t softVersion = 0;
	int      isIpEqual   = 0;
	uint16_t offset = 3;
	
	cmdId = ptr[offset++];
	cmdId = (cmdId<<8) + ptr[offset++];
	if(cmdId != 0x5001)         //服务器下发的登录信息  todo:以后可能会有模式切换的主动下发的帧，在此处要稍加修改
	{
		Mem_free(ptr);
		return ;
	}
	serverTime = ptr[offset++];     //得到服务器时间
	serverTime = (serverTime << 8) + ptr[offset++];
	serverTime = (serverTime << 8) + ptr[offset++];
	serverTime = (serverTime << 8) + ptr[offset++];
	
	softVersion = ptr[offset++];    //得到软件版本号
	softVersion = (softVersion << 8) + ptr[offset++];
	softVersion = (softVersion << 8) + ptr[offset++];
	softVersion = (softVersion << 8) + ptr[offset++];
	
	ecuId = ptr[offset++];          //得到ECU ID
	ecuId = (ecuId << 8) + ptr[offset++];
	ecuId = (ecuId << 8) + ptr[offset++];
	ecuId = (ecuId << 8) + ptr[offset++];
	
	ipLen = ptr[offset++];              //得到IP长度
	memset(sysAllData->newIP_Addr,0,18);//清零
	memcpy(sysAllData->newIP_Addr,&ptr[offset],ipLen); //得到IP地址
	
	sysAllData->newIP_Potr = ptr[offset + ipLen];      //得到端口号
	sysAllData->newIP_Potr = (sysAllData->newIP_Potr << 8) + ptr[offset + ipLen + 1];
	Mem_free(ptr);//用完就释放
	
	if(softVersion != sysAllData->softVersion) //先考虑OTA升级
	{
		sysAllData->newSoftVersion = softVersion;
		sysAllData->isDataFlow     = 1;// OTA进行升级 停止数据流，一心只为OTA升级
		sysAllData->isSoftUpdate   = 0;//todo:升级完成后要置 1 ，告诉SBoot进行程序升级。
		OTA_GetSoftInfo();
		//todo：OTA升级
	}
	if(ecuId != sysAllData->ecuVersion)//再考虑配置文件升级
	{
		sysAllData->newECUVersion = ecuId;
		sysAllData->isEcuUpdate   = 1;//配置文件升级，停止数据流，一心只为配置
		sysAllData->isDataFlow    = 1;
		//todo:配置文件升级
		GetConfigInfo();
	}
	
	isIpEqual = strcmp(sysAllData->ipAddr,sysAllData->newIP_Addr);//比较IP是否相等  =0 - 相等
	if((sysAllData->newIP_Potr != sysAllData->ipPotr) || (isIpEqual != 0))//端口号不相等或者IP地址不相等
	{
		memset(sysAllData->ipAddr,0,18);//将原始IP清零
		memcpy(sysAllData->ipAddr,sysAllData->newIP_Addr,18);//新IP
		sysAllData->ipPotr = sysAllData->newIP_Potr;         //新端口
		sysAllData->isDataFlow = 1; //停止数据流
		OSSemPend(sendMsg,0,&ipLen);//等待CDMA发送空闲，不能在其发送数据的时候，重新TCP连接
		
		CDMASendCmd((const uint8_t*)"AT+ZIPCLOSE=0\r","ZIPCLOSE",sizeof("AT+ZIPCLOSE=0\r"));//关闭TCP连接
	}
}
static void OTA_GetSoftInfo(void )
{
	_CDMADataToSend* otaUpdatSend;
	otaUpdatSend = CDMNSendDataInit(60);//升级请求帧

	otaUpdatSend->data[otaUpdatSend->datLength++] =  7;   //长度
	otaUpdatSend->data[otaUpdatSend->datLength++] = 0x80;
	otaUpdatSend->data[otaUpdatSend->datLength++] = 0x00;
	otaUpdatSend->data[otaUpdatSend->datLength++] = (sysAllData->softVersion >> 8) & 0x00FF;//当前版本
	otaUpdatSend->data[otaUpdatSend->datLength++] = sysAllData->softVersion & 0x00FF;
	otaUpdatSend->data[otaUpdatSend->datLength++] = (sysAllData->newSoftVersion >> 8) & 0x00FF;//请求升级的版本
	otaUpdatSend->data[otaUpdatSend->datLength++] = sysAllData->newSoftVersion & 0x00FF;
	
	CDMASendDataPack(otaUpdatSend);//将0x8000请求包进行封包
	
	OSQPost(CDMASendQ,otaUpdatSend);
	OTA_Updata();
}
static void SendFrameNum(uint16_t frameNum)
{
	_CDMADataToSend* otaUpdatSend;
	otaUpdatSend = CDMNSendDataInit(60);//
	otaUpdatSend->data[otaUpdatSend->datLength++] =  3;   //长度
	otaUpdatSend->data[otaUpdatSend->datLength++] = (frameNum >> 8) &0x00FF;
	otaUpdatSend->data[otaUpdatSend->datLength++] = frameNum & 0x00FF;
	
	CDMASendDataPack(otaUpdatSend);//将程序请求帧进行封包
	
	OSQPost(CDMASendQ,otaUpdatSend);
}
uint8_t updateBuff[2048];
static void OTA_Updata(void )
{
	uint8_t  err;
	uint8_t* ptrRECV_Soft;
	uint8_t* ptrDeal;
	uint16_t cmdId;
	uint16_t datLength = 0;
	uint16_t i = 0,offset = 0;
	uint8_t  frameNum;            //此次一共接收到128字节的包数
	uint16_t currentNum = 0x8001; //发送下一个请求包
	uint16_t fileCRC = 0;         //文件CRC校验
	uint32_t flashAddr  = 0;      //地址信息，写2K便自增0x800,向Flash一次写2K字节
	uint8_t  frameIndex = 0;      //要保存的帧索引
	uint8_t  frameLen   = 0;      //每一帧的每一小包到底有多少个字节
	
	memset(updateBuff,0,2048);
	while(1)
	{
		ptrRECV_Soft = OSQPend(ZIPRecv_Q,6000,&err);//等待12S
		if(err != OS_ERR_NONE)
		{
			return;//等待超时，则退出OTA升级
		}
		ptrDeal   = RecvDataAnalysis(ptrRECV_Soft);
		
		datLength = ptrDeal[0];
		datLength = (datLength << 8) + ptrDeal[1];
		
		cmdId     = ptrDeal[3];
		cmdId     = (cmdId << 8) + ptrDeal[4];
		if(cmdId == 0x8000)
		{
			offset = 7;
			sysAllData->frameNum = ptrDeal[offset++] + 0x80;//得到新程序的128字节的包数
			sysAllData->frameNum = (sysAllData->frameNum << 8) + ptrDeal[offset++];
			sysAllData->newSoftCRC = ptrDeal[9];//得到文件校验码
			sysAllData->newSoftCRC = (sysAllData->newSoftCRC << 8) + ptrDeal[offset++];
			SendFrameNum(currentNum);//发送第一包程序请求帧0x8001
		}
		else if(cmdId>0x8000)        //程序代码
		{
			if(cmdId != currentNum)  //接收到的帧序号，与所申请的帧序号不同，则放弃数据并重新申请
			{
				SendFrameNum(currentNum);
				Mem_free(ptrDeal);
				continue;
			}
			frameNum = (datLength%131) == 0? (datLength/131) : (datLength/131) + 1;//得到此帧数据一共有多少包128字节的程序代码
			
			offset = 2;
			for(i=0;i<frameNum;i++)//
			{
				frameLen = ptrDeal[offset++];
				cmdId    = ptrDeal[offset++];
				cmdId    = (cmdId << 8) + ptrDeal[offset++]; 
				memcpy(&updateBuff[frameIndex*128],&ptrDeal[offset],frameLen);
				offset += 128;
				frameIndex ++;
				if((frameIndex>=16) || (cmdId == sysAllData->frameNum))
				{
					frameIndex = 0;
					SoftErasePage(flashAddr);
					SoftProgramUpdate(flashAddr,updateBuff,2048);
					//计算CRC校验
					fileCRC = CRC_ComputeFile(fileCRC,updateBuff,((frameIndex - 1)*128 + frameLen));
					memset(updateBuff,0,2048);
					flashAddr += 0x800;
				}
			}
			if(cmdId == sysAllData->frameNum)
			{
				if(fileCRC != sysAllData->newSoftCRC)
				{
					Mem_free(ptrDeal);
					break;//CRC校验错误，程序升级失败
				}
				Mem_free(ptrDeal);
				sysAllData->isSoftUpdate = 1;
				sysAllData->pageNum = flashAddr/0x800;
				SbootParameterSaveToFlash(sysAllData);
				//todo:参数保存到Flash中，关闭CDMA，重启
				
			}
			currentNum = cmdId + 1;
			SendFrameNum(currentNum);//请求下一帧数据；
		}
		Mem_free(ptrDeal);
	}
}

static void GetConfigInfo(void)
{
	_CDMADataToSend* otaUpdatSend;
	otaUpdatSend = CDMNSendDataInit(60);//升级请求帧

	otaUpdatSend->data[otaUpdatSend->datLength++] = 7;   //长度
	otaUpdatSend->data[otaUpdatSend->datLength++] = 0x40;
	otaUpdatSend->data[otaUpdatSend->datLength++] = 0x00;
	otaUpdatSend->data[otaUpdatSend->datLength++] = (sysAllData->ecuVersion >> 8) & 0x00FF;   //当前版本
	otaUpdatSend->data[otaUpdatSend->datLength++] = sysAllData->ecuVersion & 0x00FF;
	otaUpdatSend->data[otaUpdatSend->datLength++] = (sysAllData->newECUVersion >> 8) & 0x00FF;//请求升级的版本
	otaUpdatSend->data[otaUpdatSend->datLength++] = sysAllData->newECUVersion & 0x00FF;
	
	CDMASendDataPack(otaUpdatSend);//将0x8000请求包进行封包
	
	OSQPost(CDMASendQ,otaUpdatSend);
	//todo:配置文件升级
	ConfigUpdata();
}
static void ConfigUpdata(void )
{
	uint8_t  err;
	uint8_t* ptrRECV_Soft;
	uint8_t* ptrDeal;
	uint8_t  frameLen;
	uint8_t  pidPackNum = 0;//PID 总包数
	uint8_t  pidNum = 0;    //当前单帧的PID包数
	uint16_t cmdId;
	uint16_t frameIndex = 0;
	uint16_t datLength = 0;
	uint16_t i = 0,offset = 0;
	uint16_t currentNum = 0x4001; //发送下一个配置请求包
	memset(updateBuff,0,2048);
	while(1)
	{
		ptrRECV_Soft = OSQPend(ZIPRecv_Q,6000,&err);//等待12S   一个时钟滴答是2ms
		if(err != OS_ERR_NONE)
		{
			return;//等待超时，则退出配置文件升级
		}
		ptrDeal   = RecvDataAnalysis(ptrRECV_Soft);
		
		datLength = ptrDeal[0];
		datLength = (datLength << 8) + ptrDeal[1];
		
		cmdId     = ptrDeal[3];
		cmdId     = (cmdId << 8) + ptrDeal[4];
		if(cmdId == 0x4000)
		{
			offset = 5;
			sysAllData->busType   = ptrDeal[offset++];//总线类型  CAN总线还是K线
			sysAllData->canIdType = ptrDeal[offset++];//CAN ID类型，扩展帧还是标准帧
			
			sysAllData->canTxId = ptrDeal[offset++];  //CAN 发送ID
			sysAllData->canTxId = (sysAllData->canTxId << 8) + ptrDeal[offset++];
			sysAllData->canTxId = (sysAllData->canTxId << 8) + ptrDeal[offset++];
			sysAllData->canTxId = (sysAllData->canTxId << 8) + ptrDeal[offset++];
			
			sysAllData->canRxId = ptrDeal[offset++];  //CAN 接收ID
			sysAllData->canRxId = (sysAllData->canRxId << 8) + ptrDeal[offset++];
			sysAllData->canRxId = (sysAllData->canRxId << 8) + ptrDeal[offset++];
			sysAllData->canRxId = (sysAllData->canRxId << 8) + ptrDeal[offset++];
			
			sysAllData->newPidNum = ptrDeal[offset++];//新的PID命令个数
			
			pidPackNum = ptrDeal[offset++];           //一共有多少包PID
			
			sysAllData->canBaud = ptrDeal[offset++];  //CAN波特率，协议中的 protocolType
			
			SendFrameNum(currentNum);//发送第一包程序请求帧0x4001
		}else if(cmdId > 0x4001)
		{
			if(cmdId != currentNum)//接收到的帧序号，与所申请的帧序号不同，则放弃数据并重新申请
			{
				SendFrameNum(currentNum);
				Mem_free(ptrDeal);
				continue;
			}
			pidNum = (datLength%198) == 0? (datLength/198) : (datLength/198) + 1;//得到此帧数据一共有多少包15个PID参数的小包
			offset = 2;
			for(i = 0;i < pidNum;i ++)
			{
				frameLen = ptrDeal[offset++];
				cmdId    = ptrDeal[offset++];
				cmdId    = (cmdId << 8) + ptrDeal[offset++]; 
				memcpy(&updateBuff[frameIndex * 195],&ptrDeal[offset],frameLen);
			    offset += 195;
			}
			if((cmdId - pidPackNum) == 0x4000)
			{
				//todo:保存参数，包括全局变量参数和配置参数,启动数据流
				sysAllData->ecuVersion = sysAllData->newECUVersion;
				sysAllData->pidNum     = sysAllData->newPidNum;
				
				SaveConfigToFlash(updateBuff,2048);
				SbootParameterSaveToFlash(sysAllData);
				
				Mem_free(ptrDeal);
				sysAllData->isDataFlow    = 0;//数据流重新流动
				break;
			}
			else
			{
				currentNum = cmdId + 1;
				SendFrameNum(currentNum);//请求下一包数据
			}
		}
		Mem_free(ptrDeal);
	}
	
}









