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
static void CanTestCmd(uint8_t* ptrDeal);
static void FuelModeChange(uint8_t* ptrDeal);


void ClearFaultCmd(void);

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
			
			if(cmdId == 0x5001)				//接收到登录报文
				RecvLoginDatDeal(ptrDeal);
			
			else if(cmdId == 0x5015)		//模式切换
				FuelModeChange(ptrDeal);
			
			else if(cmdId == 0x5027)		//服务器下发的测试指令
				CanTestCmd(ptrDeal);
			
			else if(((cmdId >= 0x4000)&&(cmdId < 0x5000))||(cmdId == 0x5012)||(cmdId == 0x5018)) //第一、二部分的配置文件  
				ConfigUpdata(ptrDeal);
			
			else if((cmdId >= 0x8000)&&(cmdId < 0x9000)) //程序升级报文    
				OTA_Updata(ptrDeal);
			else if(cmdId == 0x5021)
				ClearFaultCmd();
			Mem_free(ptrDeal);             //释放内存块
			varOperation.isLoginDeal = 1;  //登录报文处理完毕
		}
		else   //等待超时
		{
			varOperation.isLoginDeal = 1; 
			if(sysUpdateVar.isSoftUpdate == 1)// OTA 升级超时
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
	_CDMADataToSend* loginData = CDMNSendInfoInit(100);    //发送登录报文
	
	loginData->data[loginData->datLength++] = 31;
	loginData->data[loginData->datLength++] = 0x50;
	loginData->data[loginData->datLength++] = 0x01;
	
	buff = sysUpdateVar.curSoftVer; 
	buff =t_htonl(buff);			                       //软件固件版本  
	memcpy(&loginData->data[loginData->datLength],&buff,4);
	loginData->datLength += 4;
	
	buff = t_htonl(canDataConfig.pidVersion);
	memcpy(&loginData->data[loginData->datLength],&buff,4);
	loginData->datLength += 4;
	
	memcpy(&loginData->data[loginData->datLength],varOperation.iccID,20);
	loginData->datLength += 20;
	
	CDMASendDataPack(loginData);     //对登录报文进行打包（添加帧头、校验码、帧尾）
	err = OSQPost(CDMASendQ,loginData);
	if(err != OS_ERR_NONE)
	{
		Mem_free(loginData);
	}
	varOperation.isLoginDeal = 0;     //正在处理登录报文
}
static void GetConfigInfo(void)
{
	_CDMADataToSend* otaUpdatSend;
	otaUpdatSend = CDMNSendInfoInit(60);//升级请求帧

	otaUpdatSend->data[otaUpdatSend->datLength++] = 11;  //长度
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
	
	CDMASendDataPack(otaUpdatSend);             //将请求包进行封包
	
	OSQPost(CDMASendQ,otaUpdatSend);
}
extern u32 timeBase;
static void RecvLoginDatDeal(uint8_t* ptr)      //对服务器回复的登录报文进行解析
{
	uint16_t cmdId = 0;
	uint8_t  ipLen = 0;
	uint32_t ecuId = 0;
	uint32_t serverTime  = 0;
	uint32_t softVersion = 0;
	uint16_t offset = 3;
	static uint8_t timetemp = 0;
	static u8 abflag = 0;
	cmdId = ptr[offset++];
	cmdId = (cmdId<<8) + ptr[offset++];

	serverTime = ptr[offset++];                //得到服务器时间
	serverTime = (serverTime << 8) + ptr[offset++];
	serverTime = (serverTime << 8) + ptr[offset++];
	serverTime = (serverTime << 8) + ptr[offset++];
	
	if(timetemp == 0)
	{
		carAllRecord.startTime = serverTime;   //行程起始时间
		timetemp = 1;
	}
	if(abflag == 0)
	{
		abflag = 1;
		timeBase = serverTime;
	}
//		RTC_Time_Adjust(serverTime);   //登录的时候，跟服务器时间进行校时。
	
	softVersion = ptr[offset++];   //得到软件版本号
	softVersion = (softVersion << 8) + ptr[offset++];
	softVersion = (softVersion << 8) + ptr[offset++];
	softVersion = (softVersion << 8) + ptr[offset++];
	
	ecuId = ptr[offset++];          //得到ECU ID
	ecuId = (ecuId << 8) + ptr[offset++];
	ecuId = (ecuId << 8) + ptr[offset++];
	ecuId = (ecuId << 8) + ptr[offset++];
	
	ipLen = ptr[offset++];               //得到IP长度
	memset(varOperation.newIP_Addr,0,18);//清零
	memcpy(varOperation.newIP_Addr,&ptr[offset],ipLen); //得到IP地址
	
	varOperation.newIP_Potr = ptr[offset + ipLen];      //得到端口号
	varOperation.newIP_Potr = (varOperation.newIP_Potr << 8) + ptr[offset + ipLen + 1];
	
	varOperation.oilMode = ptr[offset + ipLen + 2];
	strengthFuel.coe = ptr[offset + ipLen + 2];
	if(softVersion != sysUpdateVar.curSoftVer) //先考虑OTA升级
	{
		varOperation.newSoftVersion = softVersion;
		OSSemPend(sendMsg,100,&ipLen);         //等待200ms  确保CDMA当前没有发送数据
		varOperation.isDataFlow     = 1;       // OTA进行升级 停止数据流，一心只为OTA升级
		sysUpdateVar.isSoftUpdate   = 1;  
		
		SendFrameNum(0x8000);                  //发送0x8000以请求程序文件大小以及CRC校验
	}
	else if(ecuId != canDataConfig.pidVersion && sysUpdateVar.isSoftUpdate ==0)  //再考虑配置文件升级
	{
		varOperation.newPIDVersion = ecuId;
		OSSemPend(sendMsg,100,&ipLen);    //等待200ms  确保 CDMA 当前没有发送数据
		varOperation.isDataFlow     = 1;  //配置文件升级，停止数据流，一心只为配置
		
		GetConfigInfo();                  //请求配置文件 - 发送 0x4000 及版本信息
	}
	else 
	{
		varOperation.isLoginDeal = 1;     //没有登录报文需要处理
		if(OSSemAccept(LoginMes) == 0)
			OSSemPost(LoginMes);
	}
//	todo:IP更改，后期会有需要
//	isIpEqual = strcmp(varOperation.ipAddr,varOperation.newIP_Addr);//比较IP是否相等  = 0 - 相等
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
	otaUpdatSend->data[otaUpdatSend->datLength++] =  3;     //长度
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
		for(i=0;i<frameNum;i++)                //
		{
			frameLen = ptrDeal[offset++] - 3;  //实际的小包程序的字节数
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
			sysUpdateVar.pageNum      = flashAddr / 0x800 + 1;
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
	uint8_t* ptrMode;
	uint16_t i = 0,offset = 0;
    static uint16_t currentNum = 0;//发送下一个配置请求包
	static uint16_t frameIndex = 0;
	static uint8_t  pidPackNum = 0;//PID 总包数
	static uint32_t addrSavePid = 0;
		
	cmdId     = ptrDeal[3];
	cmdId     = (cmdId << 8) + ptrDeal[4];
	if(cmdId == 0x4000)
	{
		offset = 5;
		varOperation.busType   = ptrDeal[offset++];//总线类型  CAN总线还是K线
		varOperation.canIdType = ptrDeal[offset++];//CAN ID类型，扩展帧还是标准帧
		
		varOperation.canRxId = ptrDeal[offset++];  //卡路宝 CAN 接收ID
		varOperation.canRxId = (varOperation.canRxId << 8) + ptrDeal[offset++];
		varOperation.canRxId = (varOperation.canRxId << 8) + ptrDeal[offset++];
		varOperation.canRxId = (varOperation.canRxId << 8) + ptrDeal[offset++];
		
		varOperation.canTxId = ptrDeal[offset++];  //卡路宝 CAN 发送ID
		varOperation.canTxId = (varOperation.canTxId << 8) + ptrDeal[offset++];
		varOperation.canTxId = (varOperation.canTxId << 8) + ptrDeal[offset++];
		varOperation.canTxId = (varOperation.canTxId << 8) + ptrDeal[offset++];
		
		varOperation.newPidNum = ptrDeal[offset++];//新的PID命令个数
		varOperation.newPidNum = varOperation.newPidNum * 256 + ptrDeal[offset++];//新的PID命令个数
		
		pidPackNum = ptrDeal[offset++];            //一帧共有多少PID包配置项
		
		varOperation.canBaud = (CANBAUD_Enum)ptrDeal[offset++];  //CAN波特率，协议中的 protocolType
		
		memcpy(varOperation.pidVerCmd,&ptrDeal[offset],8);  //提取 读取ECU版本号的指令
		
		currentNum = 0x4001;
		frameIndex = 0;
		addrSavePid  = PID1CONFIGADDR;
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
		if(((cmdId - 0x4000) % 11 == 0)&&((cmdId - pidPackNum) != 0x4000))//2K 存110个PID，一个PID占17byte
		{
			for(i = 0;i < frameIndex;i += 17)      //更改 指令发送周期 的字节序
			{
				temp            = updateBuff[i];
				updateBuff[i]   = updateBuff[i+3];
				updateBuff[i+3] = temp;
				temp            = updateBuff[i+1];
				updateBuff[i+1] = updateBuff[i+2];
				updateBuff[i+2] = temp;
			}
			Save2KDataToFlash(updateBuff,addrSavePid,2048);
			addrSavePid += 0x800;
			frameIndex = 0;
			memset(updateBuff,0,2048);
			currentNum = cmdId + 1;   
			SendConfigNum(currentNum);//请求下一包数据
		}
		else if((cmdId - pidPackNum) == 0x4000)
		{
			for(i = 0;i < frameIndex;i += 17)                   //更改 指令发送周期 的字节序
			{
				temp            = updateBuff[i];
				updateBuff[i]   = updateBuff[i+3];
				updateBuff[i+3] = temp;
				temp            = updateBuff[i+1];
				updateBuff[i+1] = updateBuff[i+2];
				updateBuff[i+2] = temp;
			}
			Save2KDataToFlash(updateBuff,addrSavePid,2048);
			addrSavePid = PID2CONFIGADDR;
			frameIndex = 0;
			memset(updateBuff,0,2048);
			SendConfigNum(0x5012);//请求提升动力相关参数 (原来是0x5012请求第二个配置文件)
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
		
		canDataConfig.pidVarNum = frameLen / 14;   //得到上报 ECU 变量的个数
		
		cmdId    = ptrDeal[offset++];
		cmdId    = (cmdId << 8) + ptrDeal[offset++]; 
		memcpy(updateBuff,&ptrDeal[offset],frameLen);
		frameIndex += frameLen;
		
		for(i = 0;i < 500;i += 14)      //更改系数、偏移量的字节序
		{
			temp            = updateBuff[i+6];
			updateBuff[i+6] = updateBuff[i+9];
			updateBuff[i+9] = temp;
			temp            = updateBuff[i+7];
			updateBuff[i+7] = updateBuff[i+8];
			updateBuff[i+8] = temp;
			
			temp             = updateBuff[i+10];
			updateBuff[i+10] = updateBuff[i+13];
			updateBuff[i+13] = temp;
			temp             = updateBuff[i+11];
			updateBuff[i+11] = updateBuff[i+12];
			updateBuff[i+12] = temp;
		}
		
		Save2KDataToFlash(updateBuff,PID2CONFIGADDR,500);//将数据写入第二配置文件区，（flash:0x08061000）
		
		canDataConfig.pidVersion = varOperation.newPIDVersion;
		canDataConfig.pidNum     = varOperation.newPidNum;
		
		canDataConfig.busType    = varOperation.busType;     //todo:CAN线和K线的切换，后期处理
		canDataConfig.canIdType  = varOperation.canIdType;
		canDataConfig.canTxId    = varOperation.canTxId;
		canDataConfig.canRxId    = varOperation.canRxId;
		canDataConfig.canBaud    = varOperation.canBaud;
		memcpy(canDataConfig.pidVerCmd,varOperation.pidVerCmd,8);
		Save2KDataToFlash((uint8_t *)&canDataConfig,PIDCONFIG,(sizeof(_CANDataConfig)+3)); //保存 CAN 通讯参数
		SendConfigNum(0x5018);
	}
	else if(cmdId == 0x5018)
	{
		offset = 5;
		memcpy(strengthFuel.ecuVer,&ptrDeal[offset],16); //版本号
		offset += 16;
		memcpy(strengthFuel.fuelAddr,&ptrDeal[offset],5);//读取喷油量的地址
		offset += 5;
		memcpy(strengthFuel.mask,&ptrDeal[offset],4); //安全算法掩码
		offset += 4;
		strengthFuel.coe = ptrDeal[offset];           //提升动力的系数
		offset ++;
		memcpy(strengthFuel.safe1,&ptrDeal[offset],8);//安全算法cmd 1
		offset += 8;
		memcpy(strengthFuel.safe2,&ptrDeal[offset],8);//安全算法cmd 2
		offset += 8;
		memcpy(strengthFuel.mode1,&ptrDeal[offset],8);//模式 cmd 1
		offset += 8;
		memcpy(strengthFuel.mode2,&ptrDeal[offset],8);//模式 cmd 2
		offset += 8;
		strengthFuel.modeOrder = ptrDeal[offset];     //模式指令与安全算法执行的顺序
		offset++;
		memcpy(strengthFuel.faultCmd1,&ptrDeal[offset],8); //当前故障码
		offset += 8;
		memcpy(strengthFuel.faultCmd2,&ptrDeal[offset],8); //历史故障码
		offset += 8;
		memcpy(strengthFuel.faultClear,&ptrDeal[offset],8);//清除故障码
		
		ptrMode = Mem_malloc(100);
		offset = 0;
		
		memcpy(&ptrMode[offset],strengthFuel.ecuVer,16);//为了拷贝代码，我就用中间数据的方法得了
		offset += 16;
		memcpy(&ptrMode[offset],strengthFuel.fuelAddr,5);
		offset += 5;
		memcpy(&ptrMode[offset],strengthFuel.mask,4);
		offset += 4;
		memcpy(&ptrMode[offset],strengthFuel.safe1,8);
		offset += 8;
		memcpy(&ptrMode[offset],strengthFuel.safe2,8);
		offset += 8;
		memcpy(&ptrMode[offset],strengthFuel.mode1,8);
		offset += 8;
		memcpy(&ptrMode[offset],strengthFuel.mode2,8);
		offset += 8;
		ptrMode[offset++] = strengthFuel.modeOrder;
		offset++;
		memcpy(&ptrMode[offset],strengthFuel.faultCmd1,8);    //当前故障码指令
		offset += 8;
		memcpy(&ptrMode[offset],strengthFuel.faultCmd2,8);    //历史故障码指令
		offset += 8;
		memcpy(&ptrMode[offset],strengthFuel.faultClear,8);   //清除故障码指令
		
		Save2KDataToFlash(ptrMode,PROMOTE_ADDR,100);
		Mem_free(ptrMode);
		
		__disable_fault_irq();                    //重启
		NVIC_SystemReset();
	}
}
#include "obd.h"
void SendPidCmdData(uint8_t* cmdData)
{
	_CDMADataToSend* cmdPidSend;
	cmdPidSend = CDMNSendInfoInit(60);//
	memcpy(&cmdPidSend->data[cmdPidSend->datLength],cmdData,cmdData[0]);
	cmdPidSend->datLength += cmdData[0];

	CDMASendDataPack(cmdPidSend);     //将程序请求帧进行封包
	
	OSQPost(CDMASendQ,cmdPidSend);
	Mem_free(cmdData);
}
extern uint8_t pidManyBag[8];
extern CAN1DataToSend  dataToSend; 
static void CanTestCmd(uint8_t* ptrDeal)//服务器下发的  CAN测试指令
{
	uint32_t canRxId;
	CANBAUD_Enum canBaud;
	uint32_t canTxId;
	uint32_t saveCanID;
	_CDMADataToSend* pidVerCmd;
	uint16_t cmdId;
	uint8_t pidNum;
	uint8_t offset = 3,err = 0,i = 0,j=0,cmdManyPackNum = 0;
	CanRxMsg* CAN1_RxMsg;
	CAN_InitTypeDef CAN_InitStructure;
	uint8_t * can1_Txbuff;
	cmdId = ptrDeal[offset ++];
	cmdId = (cmdId << 8) + ptrDeal[offset ++];
	
	pidVerCmd = CDMNSendDataInit(500); 
	pidVerCmd->data[pidVerCmd->datLength++] = 0;
	pidVerCmd->data[pidVerCmd->datLength++] = 0x50;
	pidVerCmd->data[pidVerCmd->datLength++] = 0x27;
	memcpy(&pidVerCmd->data[pidVerCmd->datLength],&ptrDeal[offset],8);
	offset += 8;
	pidVerCmd->datLength += 8;
	
	canRxId = ptrDeal[offset ++];               //卡路宝ID
	canRxId = (canRxId<<8) + ptrDeal[offset++];
	canRxId = (canRxId<<8) + ptrDeal[offset++];
	canRxId = (canRxId<<8) + ptrDeal[offset++];
	saveCanID = varOperation.canRxId;
	varOperation.canRxId = canRxId;
	
	canTxId = ptrDeal[offset ++];              //ECU ID 
	canTxId = (canTxId<<8) + ptrDeal[offset++];
	canTxId = (canTxId<<8) + ptrDeal[offset++];
	canTxId = (canTxId<<8) + ptrDeal[offset++];
	canBaud = (CANBAUD_Enum)ptrDeal[offset++];
	pidNum  = ptrDeal[offset++];
	
	varOperation.pidRun = 0;                //停止PID发送
	varOperation.pidTset = 1;
	OSTimeDlyHMSM(0,0,2,0);//延时2秒，用于确保OBD任务没有发送接收的任务
	varOperation.flagRecvOK = 0;
	do{
		can1_Txbuff = OSQAccept(canSendQ,&err);//清空消息队列里的消息
		Mem_free(can1_Txbuff);
	}while(err == OS_ERR_NONE);
	
	CAN_DeInit(CAN1);  
	CAN_StructInit(&CAN_InitStructure);
	CAN1_BaudSet(canBaud);  //先用flash中的CAN配置进行测试
	CAN1_ClearFilter();
	CAN_ITConfig(CAN1,CAN_IT_FMP1,ENABLE);//重置CAN滤波器
	
	for(i = 0;i < 200;i ++)//清空消息队列
	{
		CAN1_RxMsg = OSQPend(canRecieveQ,40,&err);
		if(err != OS_ERR_NONE)//消息队列没数据了
			break;
		Mem_free(CAN1_RxMsg);
	}
	
	dataToSend.canId = canTxId;

	for(i = 0;i < pidNum;i ++)
	{
		dataToSend.pdat  = &ptrDeal[offset + i*8];
		OBD_CAN_SendData(dataToSend.canId,CAN_ID_EXT,dataToSend.pdat);//发送 PID 指令
		CAN1_RxMsg = OSQPend(canRecieveQ,25,&err);
		if(err == OS_ERR_NONE)
		{
			if(CAN1_RxMsg->Data[0] == 0x10)         // 多包处理
			{
				cmdManyPackNum = (CAN1_RxMsg->Data[1] - 6) % 7 == 0? (CAN1_RxMsg->Data[1] - 6)/7 : (CAN1_RxMsg->Data[1] - 6)/7 + 1;
				pidVerCmd->data[pidVerCmd->datLength++] = (cmdManyPackNum+1) *7;           //收到的数据长度
				memcpy(&pidVerCmd->data[pidVerCmd->datLength],&CAN1_RxMsg->Data[2],6);
				pidVerCmd->datLength += 6;
				Mem_free(CAN1_RxMsg);
				dataToSend.pdat = pidManyBag;                 //发送 0x30 请求接下来的多包
				OBD_CAN_SendData(dataToSend.canId,dataToSend.ide,dataToSend.pdat);
				for(j=0;j<cmdManyPackNum;j++)
				{
					CAN1_RxMsg = OSQPend(canRecieveQ,25,&err);// 接收多包
					if(err == OS_ERR_NONE)
					{
						memcpy(&pidVerCmd->data[pidVerCmd->datLength],&CAN1_RxMsg->Data[1],7);
						Mem_free(CAN1_RxMsg);
						pidVerCmd->datLength += 7;
					}
					else
						break;
				} 
			}
			else  //单包处理
			{
				pidVerCmd->data[pidVerCmd->datLength++] = 9;
				memcpy(&pidVerCmd->data[pidVerCmd->datLength],CAN1_RxMsg->Data,8);
				pidVerCmd->datLength += 8;
			
				Mem_free(CAN1_RxMsg);
			}
		}
	}
	Mem_free(ptrDeal);
	pidVerCmd->data[34] = pidVerCmd->datLength - 34;
	
	dataToSend.canId = canDataConfig.canTxId;  //恢复初始的PID
	varOperation.canRxId = saveCanID;
	
	CAN_DeInit(CAN1);  
	CAN_StructInit(&CAN_InitStructure);
	CAN1_BaudSet(canDataConfig.canBaud);  //恢复初始配置的波特率
	CAN1_ClearFilter();
	CAN_ITConfig(CAN1,CAN_IT_FMP1,ENABLE);//重置CAN滤波器
	
	varOperation.pidTset = 0;
	varOperation.pidRun = 1;  //启动PID发送
	
	CDMASendDataPack(pidVerCmd);     //将采集到的数据封包
	OSQPost(CDMASendQ,pidVerCmd);
	err = OSQPost(CDMASendQ,pidVerCmd);
	if(err != OS_ERR_NONE)
	{
		Mem_free(pidVerCmd->data);
		Mem_free(pidVerCmd);
	}
		
}

extern uint8_t strengPower[300];
static void FuelModeChange(uint8_t* ptrDeal)          //节油、强动力、普通模式 切换
{
	uint16_t offset  = 5;
	uint8_t* ptrMode = NULL;
	uint16_t newCRC  = 0,nowCRC = 0;
	
	memcpy(strengthFuel.ecuVer,&ptrDeal[offset],16);  //版本号
	offset += 16;
	memcpy(strengthFuel.fuelAddr,&ptrDeal[offset],5); //读取喷油量的地址
	offset += 5;
	memcpy(strengthFuel.mask,&ptrDeal[offset],4);     //安全算法掩码
	offset += 4;
	strengthFuel.coe = ptrDeal[offset];               //提升动力的系数
	offset ++;
	memcpy(strengthFuel.safe1,&ptrDeal[offset],8);    //安全算法cmd 1
	offset += 8;
	memcpy(strengthFuel.safe2,&ptrDeal[offset],8);    //安全算法cmd 2
	offset += 8;
	memcpy(strengthFuel.mode1,&ptrDeal[offset],8);    //模式 cmd 1
	offset += 8;
	memcpy(strengthFuel.mode2,&ptrDeal[offset],8);    //模式 cmd 2
	offset += 8;
	strengthFuel.modeOrder = ptrDeal[offset];         //模式指令与安全算法执行的顺序
	offset++;
	memcpy(strengthFuel.faultCmd1,&ptrDeal[offset],8);    //当前故障码
	offset += 8;
	memcpy(strengthFuel.faultCmd2,&ptrDeal[offset],8);    //历史故障码
	offset += 8;
	memcpy(strengthFuel.faultClear,&ptrDeal[offset],8);   //清除故障码

	if(strengthFuel.coe == strengthFuelFlash.coe)
	{
		ptrMode = Mem_malloc(5);  //
		ptrMode[0] = 4;           //长度
		ptrMode[1] = 0x50;
		ptrMode[2] = 0x15;
		ptrMode[3] = strengthFuel.coe;
		SendPidCmdData(ptrMode);
	}
	Mem_free(ptrMode);
	Mem_free(ptrDeal);
	
	newCRC = CRC_ComputeFile(newCRC,strengthFuel.ecuVer,16);
	newCRC = CRC_ComputeFile(newCRC,strengthFuel.fuelAddr,5);
	newCRC = CRC_ComputeFile(newCRC,strengthFuel.mask,4);
	newCRC = CRC_ComputeFile(newCRC,strengthFuel.safe1,8);
	newCRC = CRC_ComputeFile(newCRC,strengthFuel.safe2,8);
	newCRC = CRC_ComputeFile(newCRC,strengthFuel.mode1,8);
	newCRC = CRC_ComputeFile(newCRC,strengthFuel.mode2,8);
	
	nowCRC = CRC_ComputeFile(nowCRC,strengthFuelFlash.ecuVer,16);
	nowCRC = CRC_ComputeFile(nowCRC,strengthFuelFlash.fuelAddr,5);
	nowCRC = CRC_ComputeFile(nowCRC,strengthFuelFlash.mask,4);
	nowCRC = CRC_ComputeFile(nowCRC,strengthFuelFlash.safe1,8);
	nowCRC = CRC_ComputeFile(nowCRC,strengthFuelFlash.safe2,8);
	nowCRC = CRC_ComputeFile(nowCRC,strengthFuelFlash.mode1,8);
	nowCRC = CRC_ComputeFile(nowCRC,strengthFuelFlash.mode2,8);
	if(nowCRC == newCRC)
		return;
	ptrMode = Mem_malloc(100);
	offset = 0;
	
	memcpy(&ptrMode[offset],strengthFuel.ecuVer,16);
	offset += 16;
	memcpy(&ptrMode[offset],strengthFuel.fuelAddr,5);
	offset += 5;
	memcpy(&ptrMode[offset],strengthFuel.mask,4);
	offset += 4;
	memcpy(&ptrMode[offset],strengthFuel.safe1,8);
	offset += 8;
	memcpy(&ptrMode[offset],strengthFuel.safe2,8);
	offset += 8;
	memcpy(&ptrMode[offset],strengthFuel.mode1,8);
	offset += 8;
	memcpy(&ptrMode[offset],strengthFuel.mode2,8);
	offset += 8;
	ptrMode[offset++] = strengthFuel.modeOrder;
	offset++;
	memcpy(&ptrMode[offset],strengthFuel.faultCmd1,8);    //当前故障码指令
	offset += 8;
	memcpy(&ptrMode[offset],strengthFuel.faultCmd2,8);    //历史故障码指令
	offset += 8;
	memcpy(&ptrMode[offset],strengthFuel.faultClear,8);   //清除故障码指令
	
	Save2KDataToFlash(ptrMode,PROMOTE_ADDR,100);                //将新的增强动力指令写入Flash
	
	memset(strengPower,0,200);                    //清空Flash保存的喷油量的值
	SoftErasePage(STRENGE_Q);
	Save2KDataToFlash(strengPower,STRENGE_Q,200);
	
	__disable_fault_irq();                    //重启
	NVIC_SystemReset();
}

void ClearFaultCmd(void)
{
	uint8_t * ptrOBDSend;
	uint8_t err;
//	static uint32_t timeCount = 0;
	
	ptrOBDSend = Mem_malloc(9);
	ptrOBDSend[0] = 201;
	memcpy(&ptrOBDSend[1],strengthFuelFlash.faultClear,8);
	err = OSQPost(canSendQ,ptrOBDSend);//向OBD推送要发送的PID指令
	if(err != OS_ERR_NONE)
	{
		Mem_free(ptrOBDSend);          //推送不成功，需要释放内存块
	} 
}






































