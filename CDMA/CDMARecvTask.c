#include "includes.h"
#include "bsp.h"
#include "apptask.h"

/*************************     处理函数声明          **********************/

static void RecvLoginDatDeal(uint8_t* ptr);
static void SendFrameNum(uint16_t frameNum);
static void OTA_Updata(uint8_t* ptrDeal );
static void ConfigUpdata(uint8_t* ptrDeal  );
static void GetConfigInfo(void);
extern void CDMASendCmd(const uint8_t sendDat[],char* compString,uint16_t sendLength);


//本任务用来上发登录报文、处理OTA升级、配置文件升级、模式切换（强动力模式、节油模式等）
void CDMARecvTask(void *pdata)
{
	uint8_t  err;
	uint16_t cmdId;
	uint8_t* ptrRECV = NULL;
	uint8_t* ptrDeal = NULL;
	
	while(1)
	{
		ptrRECV = OSQPend(ZIPRecv_Q,30000,&err);  //等待60s后服务器无响应，则退出
		if(err == OS_ERR_NONE)
		{
			ptrDeal = RecvDataAnalysis(ptrRECV);  //将接收到的数据进行加工
			if(ptrDeal == NULL)                   //接收到错误的数据
				continue; 
			
			cmdId = ptrDeal[3];
			cmdId = (cmdId<<8) + ptrDeal[4];
			
			if(cmdId == 0x5001)                          //接收到登录报文
				RecvLoginDatDeal(ptrDeal);
			else if(cmdId == 0x5012)                     //第二部分的配置文件
				ConfigUpdata(ptrDeal);
			else if((cmdId >= 0x4000)&&(cmdId < 0x5000)) //第一部分的配置文件
				ConfigUpdata(ptrDeal);
			else if((cmdId >= 0x8000)&&(cmdId < 0x9000)) //程序升级报文    
			{
				OTA_Updata(ptrDeal);
			}
			Mem_free(ptrDeal);             //释放内存块
			varOperation.isLoginDeal = 1;  //登录报文处理完毕
		}
		else   //等待超时
		{
			varOperation.isLoginDeal = 1; 
			if(sysUpdateVar.isSoftUpdate == 1)//OTA升级超时
			{
				sysUpdateVar.isSoftUpdate = 0;
				varOperation.isDataFlow   = 0;//重启数据流
				if(OSSemAccept(LoginMes) == 0)//启动CAN
					OSSemPost(LoginMes);	
			} 
		}
	}
}
//登录报文
void LoginDataSend(void)
{
	uint8_t err;
	uint32_t buff;
	_CDMADataToSend* loginData = CDMNSendInfoInit(100);        //发送登录报文
	
	loginData->data[loginData->datLength++] = 31;
	loginData->data[loginData->datLength++] = 0x50;
	loginData->data[loginData->datLength++] = 0x01;
	
	buff = sysUpdateVar.curSoftVer; 
	buff =t_htonl(buff);			//软件固件版本  
	memcpy(&loginData->data[loginData->datLength],&buff,4);
	loginData->datLength += 4;
	
	buff = t_htonl(canDataConfig.pidVersion);
	memcpy(&loginData->data[loginData->datLength],&buff,4);
	loginData->datLength += 4;
	
	memcpy(&loginData->data[loginData->datLength],varOperation.iccID,20);
	loginData->datLength += 20;
	
	CDMASendDataPack(loginData);//对登录报文进行打包（添加帧头、校验码、帧尾）
	err = OSQPost(CDMASendQ,loginData);
	if(err != OS_ERR_NONE)
	{
		Mem_free(loginData);
	}
	varOperation.isLoginDeal = 0;//正在处理登录报文
}
static void GetConfigInfo(void)
{
	_CDMADataToSend* otaUpdatSend;
	otaUpdatSend = CDMNSendInfoInit(60);//升级请求帧

	otaUpdatSend->data[otaUpdatSend->datLength++] = 11;   //长度
	otaUpdatSend->data[otaUpdatSend->datLength++] = 0x40;
	otaUpdatSend->data[otaUpdatSend->datLength++] = 0x00;
	//当前版本
	otaUpdatSend->data[otaUpdatSend->datLength++] = (varOperation.pidVersion >> 24) & 0x00FF; 
	otaUpdatSend->data[otaUpdatSend->datLength++] = (varOperation.pidVersion >> 16) & 0x00FF; 
	otaUpdatSend->data[otaUpdatSend->datLength++] = (varOperation.pidVersion >> 8) & 0x00FF;   
	otaUpdatSend->data[otaUpdatSend->datLength++] = varOperation.pidVersion & 0x00FF;
	
	//请求升级的版本
	otaUpdatSend->data[otaUpdatSend->datLength++] = (varOperation.newPIDVersion >> 24) & 0x00FF;
	otaUpdatSend->data[otaUpdatSend->datLength++] = (varOperation.newPIDVersion >> 16) & 0x00FF;
	otaUpdatSend->data[otaUpdatSend->datLength++] = (varOperation.newPIDVersion >> 8) & 0x00FF;
	otaUpdatSend->data[otaUpdatSend->datLength++] = varOperation.newPIDVersion & 0x00FF;
	
	CDMASendDataPack(otaUpdatSend);//将请求包进行封包
	
	OSQPost(CDMASendQ,otaUpdatSend);
}

static void RecvLoginDatDeal(uint8_t* ptr)//对服务器回复的登录报文进行解析
{
	uint16_t cmdId = 0;
	uint8_t  ipLen = 0;
	uint32_t ecuId = 0;
	uint32_t serverTime  = 0;
	uint32_t softVersion = 0;
	uint16_t offset = 3;
	
	cmdId = ptr[offset++];
	cmdId = (cmdId<<8) + ptr[offset++];

	serverTime = ptr[offset++];     //得到服务器时间
	serverTime = (serverTime << 8) + ptr[offset++];
	serverTime = (serverTime << 8) + ptr[offset++];
	serverTime = (serverTime << 8) + ptr[offset++];
	
	RTC_Time_Adjust(serverTime);//登录的时候，跟服务器时间进行校时。
	
	softVersion = ptr[offset++];    //得到软件版本号
	softVersion = (softVersion << 8) + ptr[offset++];
	softVersion = (softVersion << 8) + ptr[offset++];
	softVersion = (softVersion << 8) + ptr[offset++];
	
	ecuId = ptr[offset++];          //得到ECU ID
	ecuId = (ecuId << 8) + ptr[offset++];
	ecuId = (ecuId << 8) + ptr[offset++];
	ecuId = (ecuId << 8) + ptr[offset++];
	
	ipLen = ptr[offset++];              //得到IP长度
	memset(varOperation.newIP_Addr,0,18);//清零
	memcpy(varOperation.newIP_Addr,&ptr[offset],ipLen); //得到IP地址
	
	varOperation.newIP_Potr = ptr[offset + ipLen];      //得到端口号
	varOperation.newIP_Potr = (varOperation.newIP_Potr << 8) + ptr[offset + ipLen + 1];
	
	if(softVersion != sysUpdateVar.curSoftVer) //先考虑OTA升级
	{
		varOperation.newSoftVersion = softVersion;
		OSSemPend(sendMsg,100,&ipLen);    //等待200ms  确保CDMA当前没有发送数据
		varOperation.isDataFlow     = 1;  // OTA进行升级 停止数据流，一心只为OTA升级
		sysUpdateVar.isSoftUpdate   = 1;  
		
		SendFrameNum(0x8000);             //发送0x8000以请求程序文件大小以及CRC校验
	}
	else if(ecuId != canDataConfig.pidVersion && sysUpdateVar.isSoftUpdate ==0)  //再考虑配置文件升级
	{
		varOperation.newPIDVersion = ecuId;
		OSSemPend(sendMsg,100,&ipLen);    //等待200ms  确保CDMA当前没有发送数据
		varOperation.isDataFlow     = 1;  //配置文件升级，停止数据流，一心只为配置
		
		GetConfigInfo();                  //请求配置文件 - 发送0x4000及版本信息
	}
	else 
	{
		varOperation.isLoginDeal = 1;  //没有登录报文需要处理
		if(OSSemAccept(LoginMes) == 0)
			OSSemPost(LoginMes);
	}
	
	//todo:IP更改，后期会有需要
//	isIpEqual = strcmp(varOperation.ipAddr,varOperation.newIP_Addr);//比较IP是否相等  =0 - 相等
//	if((varOperation.newIP_Potr != varOperation.ipPotr) || (isIpEqual != 0))//端口号不相等或者IP地址不相等
//	{
//		memset(varOperation.ipAddr,0,18);//将原始IP清零
//		memcpy(varOperation.ipAddr,varOperation.newIP_Addr,18);//新IP
//		varOperation.ipPotr = varOperation.newIP_Potr;         //新端口
//		varOperation.isDataFlow = 1; //停止数据流
//		OSSemPend(sendMsg,0,&ipLen);//等待CDMA发送空闲，不能在其发送数据的时候，重新TCP连接
//		
//		CDMASendCmd((const uint8_t*)"AT+ZIPCLOSE=0\r","ZIPCLOSE",sizeof("AT+ZIPCLOSE=0\r"));//关闭TCP连接
//	}
}
static void SendFrameNum(uint16_t frameNum)
{
	_CDMADataToSend* otaUpdatSend;
	otaUpdatSend = CDMNSendInfoInit(60);//
	otaUpdatSend->data[otaUpdatSend->datLength++] =  3;   //长度
	otaUpdatSend->data[otaUpdatSend->datLength++] = (frameNum >> 8) &0x00FF;
	otaUpdatSend->data[otaUpdatSend->datLength++] = frameNum & 0x00FF;
	
	CDMASendDataPack(otaUpdatSend);//将程序请求帧进行封包
	
	OSQPost(CDMASendQ,otaUpdatSend);
}
uint8_t updateBuff[2048];       //升级用
static void OTA_Updata(uint8_t* ptrDeal)
{
	uint16_t cmdId;
	uint16_t datLength = 0;
	uint16_t i = 0;
	uint8_t  frameNum;           //此次一共接收到128字节的包数
	uint8_t  frameLen;           //每一帧的每一小包到底有多少个字节
	uint16_t offset;
	
	static uint16_t currentNum = 0;      //发送下一个请求包
	static uint16_t fileCRC    = 0;      //文件CRC校验
	static uint32_t flashAddr  = 0;      //地址信息，写2K便自增0x800,向Flash一次写2K字节
	static uint8_t  frameIndex = 0;      //要保存的帧索引
	
	
	datLength = ptrDeal[0];
	datLength = (datLength << 8) + ptrDeal[1];
	
	cmdId     = ptrDeal[3];
	cmdId     = (cmdId << 8) + ptrDeal[4];
	if(cmdId == 0x8000)
	{
		offset = 5;
		varOperation.frameNum = ptrDeal[offset++] + 0x80;//得到新程序的128字节的包数
		varOperation.frameNum = (varOperation.frameNum << 8) + ptrDeal[offset++];
		varOperation.newSoftCRC = ptrDeal[offset++];//得到文件校验码
		varOperation.newSoftCRC = (varOperation.newSoftCRC << 8) + ptrDeal[offset++];
		
		currentNum = 0x8001;
		fileCRC    = 0;
		flashAddr  = 0;
		frameIndex = 0;
		SendFrameNum(currentNum);//发送第一包程序请求帧0x8001
		memset(updateBuff,0,2048);
	}
	else if(cmdId>0x8000)        //程序代码
	{
		if(cmdId != currentNum)  //接收到的帧序号，与所申请的帧序号不同，则放弃数据并重新申请
		{
//			SendFrameNum(currentNum);//todo：重新接收数据？
			return;
		}
			
		frameNum = (datLength%131) == 0? (datLength/131) : (datLength/131) + 1;//得到此帧数据一共有多少包128字节的程序代码
		
		offset = 2;
		for(i=0;i<frameNum;i++)//
		{
			frameLen = ptrDeal[offset++] - 3;//实际的小包程序的字节数
			cmdId    = ptrDeal[offset++];
			cmdId    = (cmdId << 8) + ptrDeal[offset++]; 
			memcpy(&updateBuff[frameIndex*128],&ptrDeal[offset],frameLen);
			offset += 128;
			frameIndex ++;
			if((frameIndex >= 16) && (cmdId != varOperation.frameNum))
			{
				frameIndex = 0;
				SoftErasePage(flashAddr);
				SoftProgramUpdate(flashAddr,updateBuff,2048);
				//计算CRC校验
				fileCRC = CRC_ComputeFile(fileCRC,updateBuff,2048);
				memset(updateBuff,0,2048);//清空数据区
				flashAddr += 0x800;
			}
			else if(cmdId == varOperation.frameNum)
			{
				SoftErasePage(flashAddr);
				SoftProgramUpdate(flashAddr,updateBuff,((frameIndex - 1)*128 + frameLen));
				//计算CRC校验
				fileCRC = CRC_ComputeFile(fileCRC,updateBuff,((frameIndex - 1)*128 + frameLen));
				memset(updateBuff,0,2048);
				flashAddr += ((frameIndex - 1)*128 + frameLen);
			}
		}
		if(cmdId == varOperation.frameNum)
		{
			if(fileCRC != varOperation.newSoftCRC)//CRC校验错误，程序升级失败
			{
				Mem_free(ptrDeal);
				varOperation.isDataFlow     = 0;
//				SendFrameNum(0x8000);      //todo:重新升级？
				return;	
			}
			Mem_free(ptrDeal);
			sysUpdateVar.isSoftUpdate = 1;      //告诉Sboot,程序需要升级
			sysUpdateVar.pageNum      = flashAddr/0x800 + 1;
			sysUpdateVar.softByteSize = flashAddr;
			sysUpdateVar.newSoftCRC   = fileCRC;
			sysUpdateVar.newSoftVer   = varOperation.newSoftVersion;
			
			SbootParameterSaveToFlash(&sysUpdateVar);//将升级参数保存到Flash中
			
			__disable_fault_irq();          //重启
			NVIC_SystemReset();
		}
		currentNum = cmdId + 1;
		SendFrameNum(currentNum);//请求下一帧数据；
	}
}


static void SendConfigNum(uint16_t cmd)
{
	_CDMADataToSend* otaUpdatSend;
	otaUpdatSend = CDMNSendInfoInit(60);//升级请求帧

	otaUpdatSend->data[otaUpdatSend->datLength++] = 7;   //长度
	otaUpdatSend->data[otaUpdatSend->datLength++] = (cmd>>8) &0x00FF;
	otaUpdatSend->data[otaUpdatSend->datLength++] = cmd &0x00FF;
	//请求升级的版本
	otaUpdatSend->data[otaUpdatSend->datLength++] = (varOperation.newPIDVersion >> 24) & 0x00FF;
	otaUpdatSend->data[otaUpdatSend->datLength++] = (varOperation.newPIDVersion >> 16) & 0x00FF;
	otaUpdatSend->data[otaUpdatSend->datLength++] = (varOperation.newPIDVersion >> 8) & 0x00FF;
	otaUpdatSend->data[otaUpdatSend->datLength++] = varOperation.newPIDVersion & 0x00FF;
	
	CDMASendDataPack(otaUpdatSend);//将请求包进行封包
	
	OSQPost(CDMASendQ,otaUpdatSend);
}
static void ConfigUpdata(uint8_t* ptrDeal )
{
	uint8_t  temp;
	uint16_t frameLen;
	uint16_t cmdId;
	
	uint16_t i = 0,offset = 0;
    static uint16_t currentNum = 0; //发送下一个配置请求包
	static uint16_t frameIndex = 0;
	static uint8_t  pidPackNum = 0;//PID 总包数
		
	cmdId     = ptrDeal[3];
	cmdId     = (cmdId << 8) + ptrDeal[4];
	if(cmdId == 0x4000)
	{
		offset = 5;
		varOperation.busType   = ptrDeal[offset++];//总线类型  CAN总线还是K线
		varOperation.canIdType = ptrDeal[offset++];//CAN ID类型，扩展帧还是标准帧
		
		varOperation.canRxId = ptrDeal[offset++];  //卡路宝CAN 接收ID
		varOperation.canRxId = (varOperation.canRxId << 8) + ptrDeal[offset++];
		varOperation.canRxId = (varOperation.canRxId << 8) + ptrDeal[offset++];
		varOperation.canRxId = (varOperation.canRxId << 8) + ptrDeal[offset++];
		
		varOperation.canTxId = ptrDeal[offset++];  //卡路宝CAN 发送ID
		varOperation.canTxId = (varOperation.canTxId << 8) + ptrDeal[offset++];
		varOperation.canTxId = (varOperation.canTxId << 8) + ptrDeal[offset++];
		varOperation.canTxId = (varOperation.canTxId << 8) + ptrDeal[offset++];
		
		varOperation.newPidNum = ptrDeal[offset++];//新的PID命令个数
		
		pidPackNum = ptrDeal[offset++];            //一共有帧PID包配置项
		
		varOperation.canBaud = ptrDeal[offset++];  //CAN波特率，协议中的 protocolType
		
		currentNum = 0x4001;
		frameIndex = 50;
		memset(updateBuff,0,2048);
		SendConfigNum(currentNum);//发送第一包程序请求帧0x4001
		
	}else if(cmdId > 0x4000 && cmdId < 0x5000)
	{
		if(cmdId != currentNum)       //接收到的帧序号，与所申请的帧序号不同，则放弃数据并重新申请
		{
			//SendConfigNum(currentNum);//todo：重新发送？
			return;
		}
		
		offset = 2;
		frameLen = ptrDeal[offset++] - 3;
		cmdId    = ptrDeal[offset++];
		cmdId    = (cmdId << 8) + ptrDeal[offset++]; 
		memcpy(&updateBuff[frameIndex],&ptrDeal[offset],frameLen);
		frameIndex += frameLen;
		
		if((cmdId - pidPackNum) == 0x4000)
		{
			//todo:保存参数，包括全局变量参数和配置参数,启动数据流
			canDataConfig.pidVersion = varOperation.newPIDVersion;
			canDataConfig.pidNum     = varOperation.newPidNum;
			
			canDataConfig.busType    = varOperation.busType;//todo:CAN线和K线的切换，后期处理
			canDataConfig.canIdType  = varOperation.canIdType;
			canDataConfig.canTxId    = varOperation.canTxId;
			canDataConfig.canRxId    = varOperation.canRxId;
			canDataConfig.canBaud    = varOperation.canBaud;
			
			for(i = 50;i < frameIndex;i += 17)      //更改 指令发送周期 的字节序
			{
				temp            = updateBuff[i];
				updateBuff[i]   = updateBuff[i+3];
				updateBuff[i+3] = temp;
				temp            = updateBuff[i+1];
				updateBuff[i+1]   = updateBuff[i+2];
				updateBuff[i+2] = temp;
			}
			SendConfigNum(0x5012);//请求第二个配置文件
		}
		else
		{
			currentNum = cmdId + 1;   
			SendConfigNum(currentNum);//请求下一包数据
		}
	}
	else if(cmdId == 0x5012)
	{
		offset = 2;
		frameLen = ptrDeal[offset++] - 3;
		
		canDataConfig.pidVarNum = frameLen / 13;   //得到上报 ECU 变量的个数
		
		cmdId    = ptrDeal[offset++];
		cmdId    = (cmdId << 8) + ptrDeal[offset++]; 
		memcpy(&updateBuff[frameIndex],&ptrDeal[offset],frameLen);
		frameIndex += frameLen;
		
		PIDConfigReadWrite(updateBuff,(uint8_t *)&canDataConfig,sizeof(_CANDataConfig),0);
		
		for(i = (17*canDataConfig.pidNum)+50;i < frameIndex;i += 14)      //更改系数、偏移量的字节序
			{
				temp            = updateBuff[i+6];
				updateBuff[i+6]   = updateBuff[i+9];
				updateBuff[i+9] = temp;
				temp            = updateBuff[i+7];
				updateBuff[i+7]   = updateBuff[i+8];
				updateBuff[i+8] = temp;
				
				temp            = updateBuff[i+10];
				updateBuff[i+10]   = updateBuff[i+13];
				updateBuff[i+13] = temp;
				temp            = updateBuff[i+11];
				updateBuff[i+11]   = updateBuff[i+12];
				updateBuff[i+12] = temp;
			}
		
		Save2KDataToFlash(updateBuff,PIDConfig_ADDR,2048);      //将数据写入配置文件区，（flash:0x0802E000）
		
		__disable_fault_irq();                    //重启
		NVIC_SystemReset();
	}
}









