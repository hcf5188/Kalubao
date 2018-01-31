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
	uint8_t * ptrSaveBuff;       //
	varOperation.canTest = 2; 
	CAN1Config();                //CAN 配置
	OSTimeDlyHMSM(0,0,10,4);     
	TestServer();                //测试服务器
	while(1)
	{	
		StrengthFuel();
		if((varOperation.canTest == 0)||(varOperation.pidTset == 1))//配置文件不成功，则停止CAN，或者在测试PID指令
		{
			OSTimeDlyHMSM(0,0,1,0);	
			continue;
		}	 
		can1_Txbuff = OSQPend(canSendQ,1000,&err);//收到PID指令，进行发送
		if(err != OS_ERR_NONE)
			continue;
		cmdNum = can1_Txbuff[0];  //记录PID指令序号
		cmdLen = can1_Txbuff[1];  //记录PID指令长度
		
		dataToSend.pdat = &can1_Txbuff[1];   
		OBD_CAN_SendData(dataToSend.canId,dataToSend.ide,dataToSend.pdat);//发送PID指令
		
		CAN1_RxMsg = OSQPend(canRecieveQ,500,&err); // 接收到OBD回复
		if(err == OS_ERR_NONE)
		{
			freOBDLed = LEDSLOW;                    // OBD 初始化成功
			if(CAN1_RxMsg->Data[0] == 0x10)         // 多包处理
			{
				ptrSaveBuff = Mem_malloc(CAN1_RxMsg->Data[1] + 10);// 申请的内存块足够长
				if(ptrSaveBuff != NULL)
				{
					ptrSaveBuff[0] = CAN1_RxMsg -> Data[1] + 3;
					ptrSaveBuff[1] = 0x60;
					ptrSaveBuff[2] = cmdNum;
					memcpy(&ptrSaveBuff[3],&CAN1_RxMsg->Data[2],6);
					cmdManyPackNum = (CAN1_RxMsg->Data[1] - 6) % 7 == 0? (CAN1_RxMsg->Data[1] - 6)/7 : (CAN1_RxMsg->Data[1] - 6)/7 + 1;
					Mem_free(CAN1_RxMsg);
					
					dataToSend.pdat = pidManyBag;   //发送 0x30 请求接下来的多包
					OBD_CAN_SendData(dataToSend.canId,dataToSend.ide,dataToSend.pdat);
					
					for(i=0;i<cmdManyPackNum;i++)
					{
						CAN1_RxMsg = OSQPend(canRecieveQ,25,&err);   // 接收多包
						if(err == OS_ERR_NONE)
						{
							memcpy(&ptrSaveBuff[7*i + 9],&CAN1_RxMsg->Data[1],7);
							Mem_free(CAN1_RxMsg);
						}
						else 
							break;
					} 
					if(i == cmdManyPackNum)
					{
						OSMutexPend(CDMASendMutex,0,&err);
						
						memcpy(&cdmaDataToSend->data[cdmaDataToSend->datLength],ptrSaveBuff,ptrSaveBuff[0]);
						cdmaDataToSend->datLength += ptrSaveBuff[0];
						
						OSMutexPost(CDMASendMutex);
					}
					PIDVarGet(cmdNum,&ptrSaveBuff[3]);
					Mem_free(ptrSaveBuff);
				}
			}
			else  //单包处理
			{
				if(CAN1_RxMsg->Data[0] > cmdLen)
				{
					OSMutexPend(CDMASendMutex,0,&err);
					
					cdmaDataToSend->data[cdmaDataToSend->datLength++] = CAN1_RxMsg->Data[0] - cmdLen + 3;
					cdmaDataToSend->data[cdmaDataToSend->datLength++] = 0x60;
					cdmaDataToSend->data[cdmaDataToSend->datLength++] = cmdNum;
					memcpy(&cdmaDataToSend->data[cdmaDataToSend->datLength],&CAN1_RxMsg->Data[cmdLen+1],(CAN1_RxMsg->Data[0] - cmdLen));
					cdmaDataToSend -> datLength += CAN1_RxMsg->Data[0] - cmdLen;

					OSMutexPost(CDMASendMutex);
					PIDVarGet(cmdNum,&CAN1_RxMsg->Data[cmdLen + 1]);
				}
				else if((CAN1_RxMsg->Data[0]==0x03)&&(CAN1_RxMsg->Data[1]==0x7F))
				{
					LogReport("\r\n05-ECU report:03 7F;PID:%d err:%d,%d,%d,%d,%d,%d;",cmdNum,
					CAN1_RxMsg->Data[2],CAN1_RxMsg->Data[3],CAN1_RxMsg->Data[4],CAN1_RxMsg->Data[5],
					CAN1_RxMsg->Data[6],CAN1_RxMsg->Data[7]);
				}
				Mem_free(CAN1_RxMsg);
			}
		}
		else
		{
			freOBDLed = LEDFAST; 
			LogReport("\r\n04-PIDcmd don't report:%d;",cmdNum);//发送PID指令，ECU不回复
		}
		Mem_free(can1_Txbuff);                 //释放内存块
	}
}

extern uint8_t configData[6000];

void TestServer(void)//用服务器下发的ID、Baud等等进行CAN配置
{
	uint8_t   err,temp;
	CanRxMsg* CAN1_RxMsg;
	CAN_InitTypeDef CAN_InitStructure;
	
	OSSemPend(LoginMes,0,&err);           //1394606080
	if((varOperation.pidVersion == 0xFFFFFFFF )||(varOperation.pidNum == 0xFFFF)||(varOperation.busType == 0xFF))
	{
		varOperation.canTest = 0;
		LogReport("\r\n06-Unknown Equipment;");
		CANTestChannel();
		return;
	}	
	
	if(varOperation.pidVersion == 0)
	{
		LogReport("\r\n07-ECUID don't Config:%d;",canDataConfig.pidVersion);
		CANTestChannel();
		return	;
	}
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_CAN1, ENABLE);//使能CAN1时钟
	
	varOperation.canTest = 2;             //Flash中的CAN配置成功
	
	CAN_DeInit(CAN1);  
	CAN_StructInit(&CAN_InitStructure);
	CAN1_BaudSet(canDataConfig.canBaud);  //先用flash中的CAN配置进行测试
	CAN1_SetFilter(varOperation.canRxId ,CAN_ID_EXT); 
	CAN_ITConfig(CAN1,CAN_IT_FMP1,ENABLE);//重置CAN滤波器
	
	dataToSend.canId = canDataConfig.canTxId;
	dataToSend.ide   = canDataConfig.canIdType;
	dataToSend.pdat  = &configData[9];    //CAN PID 的第一包数据
	
	OBD_CAN_SendData(dataToSend.canId,dataToSend.ide,dataToSend.pdat);
	
	CAN1_RxMsg = OSQPend(canRecieveQ,500,&err);
	if(err == OS_ERR_NONE)
	{
		Mem_free(CAN1_RxMsg);
		LogReport("\r\n08-ECUID Right:%d;",canDataConfig.pidVersion);//ECU有回应，配置文件正确
		
		dataToSend.ide   = canDataConfig.canIdType;
		dataToSend.canId = canDataConfig.canTxId;
		
		temp = ReadECUVersion(canDataConfig.pidVerCmd);
		if(temp < 100)                  //读取ECU版本号，读取不成功则不进行动力提升
		{
			Get_Q_FromECU();            //读取喷油量的原始值，得到当前车辆系数
		
			CAN_DeInit(CAN1);
			CAN_StructInit(&CAN_InitStructure);
			CAN1_BaudSet(canDataConfig.canBaud);  
			CAN1_SetFilter(varOperation.canRxId ,CAN_ID_EXT);
			CAN_ITConfig(CAN1,CAN_IT_FMP1,ENABLE);
			OSTimeDlyHMSM(0,0,20,0);    //等待 20s 读取故障码，若不等待的话，读出来都是拒绝应答
			
			CAN_DeInit(CAN1);  
			CAN_StructInit(&CAN_InitStructure);
			CAN1_BaudSet(canDataConfig.canBaud);   
			CAN1_ClearFilter();                    
			CAN_ITConfig(CAN1,CAN_IT_FMP1,ENABLE);
			varOperation.canTest = 2;  //启动CAN，开始与ECU进行数据交互
		}
		else if(temp == 200)           //读取版本号失败（ 配置文件有误 ）
		{
			CAN_DeInit(CAN1);  
			CAN_StructInit(&CAN_InitStructure);
			CAN1_BaudSet(canDataConfig.canBaud);   
			CAN1_ClearFilter();                    
			CAN_ITConfig(CAN1,CAN_IT_FMP1,ENABLE);
			varOperation.canTest = 2; 
			CANTestChannel();
		}
		else//版本号读取出来了，但是不能提升动力，那就正常跑吧
		{
			OSTimeDlyHMSM(0,0,20,0);   //读取故障码是需要时间的
		}
	}
	else
	{
		LogReport("\r\n09-ECUID Error:%d!",canDataConfig.pidVersion);
		CANTestChannel();
	}	
}

extern CANBAUD_Enum canBaudEnum[NUMOfCANBaud];
extern const CmdVersion canIdExt[NUMOfCANID_EXT];
//服务器的配置文件出错，或者还没进行配置，系统进入自识别，识别成功则将信息上报
void CANTestChannel(void)
{
	uint8_t   err,i,temp;
	CanRxMsg* CAN1_RxMsg;
	uint8_t*  ptrOK;
	CAN_InitTypeDef CAN_InitStructure;
	
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
			canDataConfig.canBaud = canBaudEnum[i];
			Mem_free(CAN1_RxMsg);
			LogReport("\r\n10-Test Baud:%d;",canDataConfig.canBaud);
			break;
		}
	}//如果波特率没有确定就上报日志并退出
	if((i >= NUMOfCANBaud) && (err != OS_ERR_NONE))
	{
		LogReport("\r\n11-Baud Test Fail!");
		varOperation.canTest = 0;
		
		goto idOK;
	}
	dataToSend.ide = CAN_ID_EXT;
	for(i=0;i<NUMOfCANID_EXT;i++)
	{
		varOperation.canRxId  =(canIdExt[i].canID>>24) & 0x000000FF;
		varOperation.canRxId  =(varOperation.canRxId  << 8)+((canIdExt[i].canID>>16) & 0x000000FF);
		varOperation.canRxId  =(varOperation.canRxId  << 8)+(canIdExt[i].canID & 0x000000FF);
		varOperation.canRxId  =(varOperation.canRxId  << 8)+((canIdExt[i].canID>>8) & 0x000000FF);
		
		CAN_DeInit(CAN1);
		CAN_StructInit(&CAN_InitStructure);
		CAN1_BaudSet(canDataConfig.canBaud);
		CAN1_SetFilter(varOperation.canRxId ,CAN_ID_EXT);
		CAN_ITConfig(CAN1,CAN_IT_FMP1,ENABLE);//重置CAN滤波器
		
		varOperation.canTest = 0;
		dataToSend.canId = canIdExt[i].canID;
		dataToSend.ide     = CAN_ID_EXT;
		temp = ReadECUVersion((uint8_t *)canIdExt[i].pidVerCmd); 
	    if(temp != 200)//版本号读取成功
		{
			i=0;
			ptrOK = Mem_malloc(60);
			ptrOK[i++] = 32;
			ptrOK[i++] = 0x50;
			ptrOK[i++] = 0x13;
			ptrOK[i++] = canDataConfig.canBaud;
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
			memcpy(canDataConfig.pidVerCmd,canIdExt[i].pidVerCmd,4);//todo:保存当前读取版本号的指令
			break;
		}
	}
	if(varOperation.canTest == 0)
	{
		LogReport("\r\n12-CAN Test Fail!");
		varOperation.isStrenOilOK = 0;
		return;
	}
idOK:
	varOperation.canTest = 0;//不再获取 PID 信息
	varOperation.isStrenOilOK = 0;//一旦进入自识别，就不能再进行动力提升
	return;
}
extern VARConfig* ptrPIDVars;//指向第二配置区

void PIDVarGet(uint8_t cmdId,uint8_t ptrData[])
{
	uint8_t  i,j,err;	
	uint64_t saveDate;
	uint8_t  byteNum = 0;
	uint8_t  temp = 0;
	uint16_t temp1 = 0;
	uint8_t* ptr;
	static uint8_t  curFuelTimes = 0;//四次一计算
	static uint32_t allFuelCom   = 0;//累加喷油量
	for(i = 0;i < varOperation.pidVarNum;i ++)
	{
		if((ptrPIDVars + i)->pidId != cmdId)
			continue;
		//获得字节数
		byteNum = ((ptrPIDVars + i)->bitLen +(ptrPIDVars + i)->startBit)%8 ==0?\
						((ptrPIDVars + i)->bitLen + (ptrPIDVars + i)->startBit) / 8 : \
						((ptrPIDVars + i)->bitLen + (ptrPIDVars + i)->startBit) / 8 + 1;
		if((ptrPIDVars + i)->dataHL == 1)//大端在前
		{
			j = 0;
			do{
				if(j == 0)
				{
					temp = ptrData[ptrPIDVars->startByte + j];
					if(((ptrPIDVars + i)->bitLen +(ptrPIDVars + i)->startBit)<8)
					{
						temp &= (0xff>>(8 - ((ptrPIDVars + i)->bitLen +(ptrPIDVars + i)->startBit)));
						temp >>= (ptrPIDVars + i)->startBit;
						saveDate = temp;
						break;
					}
					temp >>= (ptrPIDVars + i)->startBit;
					saveDate = temp;
				}else if(j == (byteNum - 1))
				{
					saveDate <<= (((ptrPIDVars + i)->bitLen +(ptrPIDVars + i)->startBit + 8) - byteNum * 8);
					temp = ptrData[ptrPIDVars->startByte + j];
					temp &= 0xff>>(byteNum * 8 - ((ptrPIDVars + i)->bitLen +(ptrPIDVars + i)->startBit));
					saveDate += temp;
				}else
				{
					saveDate <<= 8;
					temp = ptrData[ptrPIDVars->startByte + j];
					saveDate += temp;
				}
				j++;
			}while(j < byteNum);	
		}else if((ptrPIDVars + i)->dataHL == 2)//小端在前
		{
			j = byteNum - 1;
			do{
				if(j ==( byteNum - 1))
				{
					temp = ptrData[ptrPIDVars->startByte + j];
					if(((ptrPIDVars + i)->bitLen +(ptrPIDVars + i)->startBit)<8)
					{
						temp &= (0xff>>(8 - ((ptrPIDVars + i)->bitLen +(ptrPIDVars + i)->startBit)));
						temp >>= (ptrPIDVars + i)->startBit;
						saveDate = temp;
						break;
					}
					temp &= (0xff>>(byteNum*8 - ((ptrPIDVars + i)->bitLen +(ptrPIDVars + i)->startBit)));
					saveDate = temp;
				}else if(j == 0)
				{
					saveDate <<= (8 - (ptrPIDVars + i)->startBit);
					temp = ptrData[ptrPIDVars->startByte + j];
					temp >>= (ptrPIDVars + i)->startBit;
					saveDate += temp;
				}else
				{
					saveDate <<= 8;
					temp = ptrData[ptrPIDVars->startByte + j];
					saveDate += temp;
				}
				if(j == 0)
				{
					break;
				}
				j--;
			}while(1);	
		}
		switch((ptrPIDVars + i)->varId)
		{
			case 1://计算车速
				if(carAllRecord.carSpeedTemp != 1)
					carAllRecord.carSpeed = (uint16_t)(saveDate * ((ptrPIDVars + i)->ceo) + (ptrPIDVars + i)->offset);
				break;
			case 2://转速
				if(carAllRecord.engineSpeedTemp != 1)
					carAllRecord.engineSpeed = (uint16_t)(saveDate * ((ptrPIDVars + i)->ceo) + (ptrPIDVars + i)->offset);
				if(carAllRecord.engineSpeed > carAllRecord.engineSpeedMax)//获得发动机最高转速
					carAllRecord.engineSpeedMax = carAllRecord.engineSpeed;
				break;
			case 3://总喷油量
				if(carAllRecord.allFuelTemp != 1)
				{
					carAllRecord.allFuel = (uint16_t)(saveDate * ((ptrPIDVars + i)->ceo) + (ptrPIDVars + i)->offset);
					carAllRecord.totalFuel += carAllRecord.allFuel;
				}break;
			case 4://主喷油量
				carAllRecord.primaryFuel = (uint16_t)(saveDate * ((ptrPIDVars + i)->ceo) + (ptrPIDVars + i)->offset);
				carAllRecord.totalFuel += carAllRecord.primaryFuel;
				break;
			case 5://预喷油量 1
				carAllRecord.beforeFuel1 = (uint16_t)(saveDate * ((ptrPIDVars + i)->ceo) + (ptrPIDVars + i)->offset);
				carAllRecord.totalFuel += carAllRecord.beforeFuel1;
				allFuelCom +=carAllRecord.beforeFuel1;
				break;
			case 6://预喷油量 2
				carAllRecord.beforeFuel2 = (uint16_t)(saveDate * ((ptrPIDVars + i)->ceo) + (ptrPIDVars + i)->offset);
				carAllRecord.totalFuel += carAllRecord.beforeFuel2;
				allFuelCom +=carAllRecord.beforeFuel2;
				break;
			case 7://预喷油量 3
				carAllRecord.beforeFuel3 = (uint16_t)(saveDate * ((ptrPIDVars + i)->ceo) + (ptrPIDVars + i)->offset);
				carAllRecord.totalFuel += carAllRecord.beforeFuel3;
				allFuelCom +=carAllRecord.beforeFuel3;
				break;
			case 8://后喷油量 1
				carAllRecord.afterFuel1 = (uint16_t)(saveDate * ((ptrPIDVars + i)->ceo) + (ptrPIDVars + i)->offset);
				carAllRecord.totalFuel += carAllRecord.afterFuel1;
				break;
			case 9://后喷油量 2
				carAllRecord.afterFuel2 = (uint16_t)(saveDate * ((ptrPIDVars + i)->ceo) + (ptrPIDVars + i)->offset);
				carAllRecord.totalFuel += carAllRecord.afterFuel2;
				break;
			case 10://后喷油量3
				carAllRecord.afterFuel3 = (uint16_t)(saveDate * ((ptrPIDVars + i)->ceo) + (ptrPIDVars + i)->offset);
				carAllRecord.totalFuel += carAllRecord.afterFuel3;
				break;
			case 11://当前喷油量
				carAllRecord.curFuel    = (uint16_t)(saveDate * ((ptrPIDVars + i)->ceo) + (ptrPIDVars + i)->offset);
				carAllRecord.totalFuel += carAllRecord.curFuel;
				allFuelCom             += carAllRecord.curFuel;
				curFuelTimes ++;
				if(curFuelTimes >= 4)//计算瞬时油耗  1s 的
				{
					carAllRecord.instantFuel = allFuelCom * carAllRecord.engineSpeed / 7;
					LogReport("\r\n60-oil:%d,r:%d;",allFuelCom,carAllRecord.engineSpeed);
					ptr = Mem_malloc(9);
					
					ptr[0] = 0x09;ptr[1] = 0x50;ptr[2] = 0x16;
					
					temp1 = t_htons(carAllRecord.instantFuel);	 
					memcpy(&ptr[3],&temp1,2);
					if(carAllRecord.carSpeed > 0)//todo: 需要确认单位
						temp1 = t_htons(carAllRecord.carSpeed);
					else 
						temp1 = t_htons(gpsMC.speed);
					memcpy(&ptr[5],&temp1,2);        //当前车速
					temp1 = t_htons(carAllRecord.engineSpeed);
					
					memcpy(&ptr[7],&temp1,2);        //当前转速
					
					OSMutexPend(CDMASendMutex,0,&err);
					memcpy(&cdmaDataToSend->data[cdmaDataToSend->datLength],ptr,ptr[0]);
					cdmaDataToSend->datLength += ptr[0];
					OSMutexPost(CDMASendMutex);
					
					Mem_free(ptr);
					
					carAllRecord.totalFuel += carAllRecord.instantFuel;
					temp = 0;
					allFuelCom   = 0;
					curFuelTimes = 0;
				}
				break;
			default:break;
		}
	}
}











