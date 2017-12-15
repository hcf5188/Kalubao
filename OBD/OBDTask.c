#include "apptask.h"
#include "bsp.h"
#include "obd.h"

void CANTestChannel(void );
void TestServer(void);

extern uint16_t freOBDLed;
uint8_t pidManyBag[8] = {0x30,0x00,0x00,0x00,0x00,0x00,0x00,0x00};

CAN1DataToSend  dataToSend; 

void OBDTask(void *pdata)
{
	INT8U     err;
	uint8_t   i      = 0;
	uint8_t   cmdLen = 0;        //封包的时候要减去指令的长度
	uint8_t   cmdNum = 0;        //指令序号
	uint8_t   cmdManyPackNum = 0;//要接受的多包的数量
	CanRxMsg* CAN1_RxMsg;        //指向接收到的OBD信息
	uint8_t * can1_Txbuff;       //指向要发送的OBD信息
	uint8_t * ptrSaveBuff;
	
	CAN1Config();                //CAN 配置
	OSTimeDlyHMSM(0,0,10,4);
	TestServer();                //测试服务器
	
	while(1)
	{	
		if(varOperation.canTest == 0)//配置文件不成功，则停止CAN
		{
			OSTimeDlyHMSM(0,0,1,0);
			continue;
		}	  
		can1_Txbuff = OSQPend(canSendQ,1000,&err);//收到PID指令，进行发送
		if(err != OS_ERR_NONE)
			continue;
		cmdNum = can1_Txbuff[0];               //记录PID指令序号
		cmdLen = can1_Txbuff[1];               //记录PID指令长度
		
		dataToSend.pdat = &can1_Txbuff[1];     
		OBD_CAN_SendData(dataToSend);          //发送PID指令
		
		CAN1_RxMsg = OSQPend(canRecieveQ,25,&err); //接收到OBD回复
		if(err == OS_ERR_NONE)
		{
			freOBDLed = 500;                    //OBD 初始化成功
			if(CAN1_RxMsg->Data[0] == 0x10)     //多包处理
			{
				ptrSaveBuff = Mem_malloc(CAN1_RxMsg->Data[1]+10);//申请的内存块足够长
				if(ptrSaveBuff != NULL)
				{
					ptrSaveBuff[0] = CAN1_RxMsg->Data[1] - cmdLen + 3;
					ptrSaveBuff[1] = 0x60;
					ptrSaveBuff[2] = cmdNum;
					memcpy(&ptrSaveBuff[3],&CAN1_RxMsg->Data[cmdLen + 2],(6 - cmdLen));
					cmdManyPackNum = (CAN1_RxMsg->Data[1] - 6)%7 == 0? (CAN1_RxMsg->Data[1] - 6)/7 : (CAN1_RxMsg->Data[1] - 6)/7 + 1;
					Mem_free(CAN1_RxMsg);
					
					dataToSend.pdat = pidManyBag;//发送0x30 请求接下来的多包
					OBD_CAN_SendData(dataToSend);
					
					for(i=0;i<cmdManyPackNum;i++)
					{
						CAN1_RxMsg = OSQPend(canRecieveQ,25,&err);//接收多包
						if(err == OS_ERR_NONE)
						{
							memcpy(&ptrSaveBuff[7*i + 9 - cmdLen],&CAN1_RxMsg->Data[1],7);
							Mem_free(CAN1_RxMsg);
						}
						else break;
					}
					if(i == cmdManyPackNum)
					{
						OSMutexPend(CDMASendMutex,0,&err);
						
						memcpy(&cdmaDataToSend->data[cdmaDataToSend->datLength],ptrSaveBuff,ptrSaveBuff[0]);
						cdmaDataToSend->datLength += ptrSaveBuff[0];
						
						OSMutexPost(CDMASendMutex);
					}
					Mem_free(ptrSaveBuff);
				}
			}
			else  //单包处理
			{
				OSMutexPend(CDMASendMutex,0,&err);
				
				cdmaDataToSend->data[cdmaDataToSend->datLength++] = CAN1_RxMsg->Data[0] - cmdLen + 3;
				cdmaDataToSend->data[cdmaDataToSend->datLength++] = 0x60;
				cdmaDataToSend->data[cdmaDataToSend->datLength++] = cmdNum;
				memcpy(&cdmaDataToSend->data[cdmaDataToSend->datLength],&CAN1_RxMsg->Data[cmdLen+1],(CAN1_RxMsg->Data[0] - cmdLen));
				cdmaDataToSend->datLength += CAN1_RxMsg->Data[0] - cmdLen;

				OSMutexPost(CDMASendMutex);
				
				Mem_free(CAN1_RxMsg);
			}
		}
		else
		{
			OSTimeDlyHMSM(0,0,0,4);
		}
		Mem_free(can1_Txbuff);                 //释放内存块
	}
}

extern uint8_t configData[2048];
void TestServer(void)
{
	uint8_t err;
	CanRxMsg* CAN1_RxMsg;
	CanTxMsg CAN1_TxMsg;
	CAN_InitTypeDef   CAN_InitStructure;
	
	OSSemPend(LoginMes,0,&err);
	
	if(varOperation.pidVersion == 0)
	{
		LogReport("01 - ECU Version %d,Need Config.",sysUpdateVar.pidVersion);
		CANTestChannel();
		return	;
	}
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_CAN1, ENABLE);//使能CAN1时钟
	
	varOperation.canTest = 2;//Flash中的CAN配置成功
	
	CAN_DeInit(CAN1);  
	CAN_StructInit(&CAN_InitStructure);
	CAN1_BaudSet(sysUpdateVar.canBaud);   //先用flash中的CAN配置进行测试
	CAN1_ClearFilter();                   //清除滤波器，进入侦听模式，接收一切CAN消息
	CAN_ITConfig(CAN1,CAN_IT_FMP1,ENABLE);//重置CAN滤波器
	
	
	CAN1_TxMsg.IDE     = sysUpdateVar.canIdType;
	CAN1_TxMsg.StdId   = sysUpdateVar.canTxId ;
	CAN1_TxMsg.ExtId   = sysUpdateVar.canTxId ;
	CAN1_TxMsg.DLC     = 0x08;
	
	CAN1_TxMsg.Data[0] = configData[5];CAN1_TxMsg.Data[1] = configData[6];CAN1_TxMsg.Data[2] = configData[7];//todo:测试报文PID是什么？
	CAN1_TxMsg.Data[3] = configData[8];CAN1_TxMsg.Data[4] = configData[9];CAN1_TxMsg.Data[5] = configData[10];
	CAN1_TxMsg.Data[6] = configData[11];CAN1_TxMsg.Data[7] = configData[12];
	CAN_Transmit(CAN1,&CAN1_TxMsg);
	CAN1_RxMsg = OSQPend(canRecieveQ,500,&err);
	if(err ==OS_ERR_NONE)
	{
		dataToSend.ide = sysUpdateVar.canIdType;
		dataToSend.canId   = sysUpdateVar.canTxId;
		Mem_free(CAN1_RxMsg);
		LogReport("01 - ECU Version %d test Success.",sysUpdateVar.pidVersion);
		ReadECUVersion();        //读取ECU版本号	
		Get_Q_FromECU();         //增强动力
	}
	else
	{
		LogReport("ECU Version %d test Error!",sysUpdateVar.pidVersion);
		CANTestChannel();
	}	
}

extern CANBAUD_Enum canBaudEnum[NUMOfCANBaud];
extern uint32_t canIdExt[NUMOfCANID_EXT];
extern uint32_t canIdStd[NUMOfCANID_STD][2];

void CANTestChannel(void )
{
	uint8_t err,i;
	CanRxMsg* CAN1_RxMsg;
	uint8_t*  ptrOK;
	CAN_InitTypeDef   CAN_InitStructure;
	
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_CAN1, ENABLE);//使能CAN1时钟

	varOperation.canTest = 0;
	for(i = 0;i<NUMOfCANBaud;i++)       //获得CAN的波特率
	{
		CAN_DeInit(CAN1);
		CAN_StructInit(&CAN_InitStructure);
		CAN1_BaudSet(canBaudEnum[i]); 
		CAN1_ClearFilter();
		CAN_ITConfig(CAN1,CAN_IT_FMP1,ENABLE);
		
		//只要此波特率下有数据，CAN波特率就确定了
		CAN1_RxMsg = OSQPend(canRecieveQ,500,&err);
		if(err == OS_ERR_NONE)
		{
			varOperation.canBaud = canBaudEnum[i];
			Mem_free(CAN1_RxMsg);
			LogReport("Baud %d Test Success.",canBaudEnum[i]);
			break;
		}
	}//如果波特率没有确定就上报日志并退出
	if((i >= NUMOfCANBaud) && (err != OS_ERR_NONE))
	{
		LogReport("Baud Test Fail!!!");
		varOperation.canTest = 0;
		goto idOK;
	}
	dataToSend.ide = CAN_ID_EXT;
	for(i=0;i<NUMOfCANID_EXT;i++)
	{
		varOperation.canRxId  =(canIdExt[i]>>24) &0x000000FF;
		varOperation.canRxId  =(varOperation.canRxId  << 8)+((canIdExt[i]>>16) & 0x000000FF);
		varOperation.canRxId  =(varOperation.canRxId  << 8)+(canIdExt[i]&0x000000FF);
		varOperation.canRxId  =(varOperation.canRxId  << 8)+((canIdExt[i]>>8) &0x000000FF);
		
		CAN_DeInit(CAN1);
		CAN_StructInit(&CAN_InitStructure);
		CAN1_BaudSet(varOperation.canBaud);
		CAN1_SetFilter(varOperation.canRxId ,CAN_ID_EXT);
		CAN_ITConfig(CAN1,CAN_IT_FMP1,ENABLE);//重置CAN滤波器
		
		varOperation.canTest = 0;
		
		dataToSend.canId = canIdExt[i];
		dataToSend.ide     = CAN_ID_EXT;
		
		ReadECUVersion(); 
		
	    if(varOperation.ecuVersion[0] != 0)
		{
			i=0;
			ptrOK = Mem_malloc(60);
			ptrOK[i++] = 32;
			ptrOK[i++] = 0x50;
			ptrOK[i++] = 0x13;
			ptrOK[i++] = varOperation.canBaud;
			ptrOK[i++] = (varOperation.canRxId>>24) & 0x000000FF;ptrOK[i++] = (varOperation.canRxId>>16) & 0x000000FF;
			ptrOK[i++] = (varOperation.canRxId>>8) & 0x000000FF;ptrOK[i++] = (varOperation.canRxId>>0) & 0x000000FF;
			ptrOK[i++] = (dataToSend.canId>>24) & 0x000000FF;ptrOK[i++] = (dataToSend.canId>>16) & 0x000000FF;
			ptrOK[i++] = (dataToSend.canId>>8) & 0x000000FF;ptrOK[i++] = (dataToSend.canId>>0) & 0x000000FF;
			memcpy(&ptrOK[i],varOperation.ecuVersion,20);
			
			OSMutexPend(CDMASendMutex,0,&err);
				
			memcpy(&cdmaDataToSend->data[cdmaDataToSend->datLength],ptrOK,ptrOK[0]);
			cdmaDataToSend->datLength += ptrOK[0];

			OSMutexPost(CDMASendMutex);
			
			Mem_free(ptrOK);
			break;
		}
	}
	if(varOperation.canTest == 0)
	{
		LogReport("CANID EXT Test Fail!");
		return;
	}
idOK:
	varOperation.canTest = 0;
	return;
}
extern VARConfig* ptrPIDVars;//指向第二配置区


void PIDVarGet(uint8_t cmdId,uint8_t *ptrData)
{
//	uint8_t  i; 
//	uint32_t saveDate;
//	uint8_t  saveLen = 0;
//	uint32_t  temp = 0;
//	for(i = 0;i < varOperation.pidVarNum;i ++)
//	{
//		if((ptrPIDVars + i)->pidId != cmdId)
//			continue;
//		while(saveLen < (ptrPIDVars + i)->bitLen)
//		{
//			if(saveLen < 8)
//				temp = ptrData[ptrPIDVars->startByte];
//			else if(saveLen >= 8 && saveLen < 16)
//			{
//				temp = ptrData[ptrPIDVars->startByte + 1];
//				temp <<= 8;
//			}else if(saveLen >= 16 && saveLen < 24)
//			{
//				temp = ptrData[ptrPIDVars->startByte + 2];
//				temp <<= 16;
//			}else if(saveLen >= 24 && saveLen < 32)
//			{
//				temp = ptrData[ptrPIDVars->startByte + 3];
//				temp <<= 24;
//			}
//			saveDate += temp & (0x00000001 << saveLen);
//			saveLen ++;
//		}
		
		//1 - 车速 2 - 发动机转速 3 - 总喷油量 4 - 主喷油量 5 - 预喷油量 6 - 后喷油量 7 - 当前喷油量
//		switch((ptrPIDVars + i)->varId)
//		{
//			case 1://计算车速
//				carAllRecord.carSpeed = saveDate * ((ptrPIDVars + i)->ceo) + (ptrPIDVars + i)->offset;
//				break;
//			case 2://转速
//				if(carAllRecord.engineSpeedTemp != 1)
//				carAllRecord.engineSpeed = saveDate * ((ptrPIDVars + i)->ceo) + (ptrPIDVars + i)->offset;
//				break;
//			case 3:break;
//			case 4:break;
//			case 5:break;
//			case 6:break;
//			case 7:break;
//			default:break;
//		}
//	}
}
extern const uint8_t weiChai[21][6];

const uint16_t qMax = 0x045C;
void Get_Q_FromECU(void)
{
	uint8_t* ptrSetCmd;
	uint8_t* ptrVer;
	CanRxMsg* CAN1_RxMsg;
	uint8_t  i,err;
	uint16_t dat1,dat2,datSma;
	uint8_t qNum = 0;
	uint8_t offset = 7,bag = 0x21;
	static uint8_t  temp = 0;
	static uint16_t readData = 0;
	//temp = 0;
	uint8_t textEcu[8] = {0x05,0x23,0x05,0x60,0xF8,0x02,0x00,0x00};
	
	ptrSetCmd  = Mem_malloc(8);
	ptrVer = Mem_malloc(80);
	ptrVer[0] = 0x10;ptrVer[1] = 0x2F;ptrVer[2] = 0x3D;ptrVer[3] = 0x05;
	ptrVer[4] = 0x61;ptrVer[5] = 0x24;ptrVer[6] = 0x2A;
	memcpy(ptrSetCmd,textEcu,8);
	dataToSend.pdat   = textEcu;
	OBD_CAN_SendData(dataToSend); 
	CAN1_RxMsg = OSQPend(canRecieveQ,50,&err);
	if(err == OS_ERR_NONE)
	{
		qNum = CAN1_RxMsg->Data[2];
		dat1 = textEcu[3];
		dat2 = textEcu[4]+qNum*2;
		if(dat2 >= 256)
			dat1++;
		textEcu[3] = dat1;
		textEcu[4] += qNum*2;
		Mem_free(CAN1_RxMsg);
		for(i=0;i<qNum;i++)
		{
			textEcu[4] += 2;
			memcpy(ptrSetCmd,textEcu,8);
			dataToSend.pdat   = textEcu;
			OBD_CAN_SendData(dataToSend); 
			CAN1_RxMsg = OSQPend(canRecieveQ,50,&err);
			if(err == OS_ERR_NONE)
			{	
				dat1 = CAN1_RxMsg->Data[3];
				dat1 = (dat1 << 8) + CAN1_RxMsg->Data[2];
				datSma = dat1/20;
				
				dat2 = dat1 + datSma;
				if(i == 0)
				{
					readData = dat1;
				}
				
				if(offset % 8 == 0)
					ptrVer[offset++] = bag++;
				ptrVer[offset++] = dat2 & 0x00FF;
				if(offset % 8 == 0)
					ptrVer[offset++] = bag++;
				ptrVer[offset++] = (dat2>>8) & 0x00FF;
				Mem_free(CAN1_RxMsg);
//				LogReport("ECU_Q_Data-%d:%d,%d",i+1,dat1,dat2);
			}
		}
	}
	//安全算法，加写入数据
	if((temp == 0)&&(readData < 1122))//保证不会重复写入
	{
		temp = 1;
		SafeALG(ptrVer);//安全算法
	}
	else
	{
		Mem_free(ptrVer);
	}
}





































