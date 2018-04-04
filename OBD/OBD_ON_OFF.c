#include "apptask.h"
#include "bsp.h"
#include "obd.h"

void ElecUP(void);         //第一次上电逻辑
void CANTestChannel(void); //ECU 自识别
extern uint16_t adcValue;
extern CAN1DataToSend  dataToSend;
extern u8 CANerr_flg,CANerr_state,CANr1,CANr2,CANr3,CANr4,CANr5,CANr6,CANr7;
extern u8 fCanOK;
extern uint16_t freOBDLed;

void OBD_ON_OFFDeal(void *pdata)
{
	uint8_t err,temp;
	uint8_t * can1_Txbuff;
	
	OSSemPend(LoginMes,0,&err);   //等待连上网络
	CAN1Config(); //配置CAN
	ElecUP();
	OSTimeDlyHMSM(0,0,5,0);
	while(1)
	{
		OSTimeDlyHMSM(0,0,2,0);
		if(varOperation.flagCAN == 1||varOperation.canTest == 0)//数据流正常采集中 / 系统进入自识别
		{
			if(varOperation.canTest == 0)
				LogReport("TestECU Fail;");
			continue;
		}
		if(varOperation.flagJ1939 == 0 && varOperation.flagCAN == 0)//ECU掉电
		{
			do{
				can1_Txbuff = OSQAccept(canSendQ,&err);//清空消息队列里的消息
				Mem_free(can1_Txbuff);
			}while(err == OS_ERR_NONE);
			CANr1=0;CANr2=0;CANr3=0;CANr4=0;CANr5=0;CANr6=0;CANr7=0;fCanOK=0;
			LogReport("\r\n08-ECU OFF!");
	
			IsCanCommunicationOK();
			if(varOperation.canTest == 1)
			{
				Get_Q_FromECU();                          //获取喷油量的值
				varOperation.pidRun = 1;                  //启动PID数据发送
				varOperation.flagRecvOK = 0;
			}	
			else if(temp == 0)
			{
				varOperation.pidRun = 1;
				varOperation.flagRecvOK = 0;
			}
			LogReport("\r\nECU ON!");
			OSTimeDlyHMSM(0,0,15,0);
		}
	}
}
void IsCanCommunicationOK(void)//用读取版本号的指令发送测试帧
{
	CanRxMsg* CAN1_RxMsg;
	uint8_t err;
	
	dataToSend.canId = canDataConfig.canTxId;
	dataToSend.ide   = canDataConfig.canIdType;
	dataToSend.pdat  = canDataConfig.pidVerCmd;    //用读取版本号的指令进行测试
	varOperation.flagRecvOK = 0;
	do{
		OBD_CAN_SendData(dataToSend.canId,dataToSend.ide,dataToSend.pdat);
		CAN1_RxMsg = OSQPend(canRecieveQ,500,&err);
		
		LogReport("\r\nTest Mode!;");
		
		Mem_free(CAN1_RxMsg);
		OSTimeDlyHMSM(0,0,3,0);
			
	}while(err != OS_ERR_NONE);
	
}
void ElecUP(void)                       //卡路宝第一次上电或者升级后重启
{
	uint8_t isCANOK = 1;
	CAN_InitTypeDef CAN_InitStructure;
	varOperation.pidRun = 0;
	
	OSTimeDlyHMSM(0,0,1,0);
	OSTimeDlyHMSM(0,0,3,0);

	if((varOperation.pidVersion == 0xFFFFFFFF )||(varOperation.pidNum == 0xFFFF)||(varOperation.busType == 0xFF))
	{
		LogReport("\r\n06-Unknown Equipment;");
		CANTestChannel();
		return;
	}	
	else if(varOperation.pidVersion == 0)
	{
		LogReport("\r\n07-ECUID don't Config:%d;",canDataConfig.pidVersion);
		CANTestChannel();
		return	;
	}else
	{
		CAN_DeInit(CAN1);  
		CAN_StructInit(&CAN_InitStructure);  
		CAN1_BaudSet(canDataConfig.canBaud);  //先用flash中的CAN配置进行测试
		CAN1_ClearFilter();  
		CAN_ITConfig(CAN1,CAN_IT_FMP1,ENABLE);//重置CAN滤波器
	
		dataToSend.ide   = canDataConfig.canIdType;
		dataToSend.canId = canDataConfig.canTxId;
abc:		
		isCANOK = ReadECUVersion(canDataConfig.pidVerCmd);
		if(isCANOK < 100)				//读取ECU版本号，读取不成功则不进行动力提升
		{
			freOBDLed = LEDSLOW;
			varOperation.canTest = 1;
			Get_Q_FromECU();            //读取喷油量的原始值，得到当前车辆系数
			OSTimeDlyHMSM(0,0,10,0);    //标定结束后，需要等待 20s 读取故障码，若不等待的话，读出来都是拒绝应答
			varOperation.pidRun = 1;    //可以开始数据流
		}
		else if(isCANOK == 200)         //读取版本号失败（ 配置文件有误 ）
		{
			IsCanCommunicationOK();     //进入测试状态
			goto abc;
		}
			
		else                            //版本号读取出来了，但是不能提升动力，那就正常跑吧
		{
			varOperation.canTest = 2;   
			OSTimeDlyHMSM(0,0,2,0);     //读取故障码是需要时间的
			varOperation.pidRun = 1;    //可以开始数据流
		}		
	}
		

	
}
extern const CmdVersion canIdExt[5];
void CANTestChannel(void)
{
	uint8_t   err,i,temp = 0;
	CanRxMsg* CAN1_RxMsg;
	uint8_t*  ptrOK;
	CAN_InitTypeDef CAN_InitStructure;
	
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_CAN1, ENABLE);//使能 CAN1 时钟

	TestStart:
	CAN_DeInit(CAN1);
	CAN_StructInit(&CAN_InitStructure);
	CAN1_BaudSet(CANBAUD_250K); 
	CAN1_ClearFilter();
	CAN_ITConfig(CAN1,CAN_IT_FMP1,ENABLE);
		
	//只要此波特率下有J1939数据，CAN 波特率就确定了
	CAN1_RxMsg = OSQPend(canJ1939Q,500,&err);
	if(err == OS_ERR_NONE)  //14230协议，250K，J1939有数据
	{
		canDataConfig.canBaud = CANBAUD_250K;
		Mem_free(CAN1_RxMsg);
		LogReport("\r\n10-Test Baud:%d;",canDataConfig.canBaud);
		for(i = 0;i < 3;i ++)//模式1的时候，一共有3个CANID
		{
			varOperation.canRxId  =(canIdExt[i].canID >> 24) & 0x000000FF;
			varOperation.canRxId  =(varOperation.canRxId  << 8)+((canIdExt[i].canID >> 16) & 0x000000FF);
			varOperation.canRxId  =(varOperation.canRxId  << 8)+(canIdExt[i].canID & 0x000000FF);
			varOperation.canRxId  =(varOperation.canRxId  << 8)+((canIdExt[i].canID >> 8) & 0x000000FF);
			
			dataToSend.canId = canIdExt[i].canID;
			dataToSend.ide     = CAN_ID_EXT;
			temp = ReadECUVersion((uint8_t *)canIdExt[i].pidVerCmd); 
			if(temp < 200)
				goto TestSuccess;
			else if(i == 2)
				goto TestError;
		}
	}
	else   // 14229 协议， J1939 没数据
	{
		CAN_DeInit(CAN1);
		CAN_StructInit(&CAN_InitStructure);
		CAN1_BaudSet(CANBAUD_500K); 
		CAN1_ClearFilter();
		CAN_ITConfig(CAN1,CAN_IT_FMP1,ENABLE);
		for(i = 3;i < 5;i ++)//模式1的时候，一共有3个CANID
		{
			varOperation.canRxId  =(canIdExt[i].canID >> 24) & 0x000000FF;
			varOperation.canRxId  =(varOperation.canRxId  << 8)+((canIdExt[i].canID >> 16) & 0x000000FF);
			varOperation.canRxId  =(varOperation.canRxId  << 8)+(canIdExt[i].canID & 0x000000FF);
			varOperation.canRxId  =(varOperation.canRxId  << 8)+((canIdExt[i].canID >> 8) & 0x000000FF);
			
			dataToSend.canId = canIdExt[i].canID;
			dataToSend.ide     = CAN_ID_EXT;
			temp = ReadECUVersion((uint8_t *)canIdExt[i].pidVerCmd);
			if(temp < 200)
				goto TestSuccess;
			else if(i == 4)
				goto TestError;
		}
	}
TestSuccess:	
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
	if(varOperation.isDataFlow != 1)
	{
		OSMutexPend(CDMASendMutex,0,&err);
		memcpy(&cdmaDataToSend->data[FRAME_HEAD_LEN + varOperation.datOKLeng],ptrOK,ptrOK[0]);
		cdmaDataToSend->datLength += ptrOK[0];
		varOperation.datOKLeng += ptrOK[0];
		OSMutexPost(CDMASendMutex);
	}
	Mem_free(ptrOK);
	memcpy(canDataConfig.pidVerCmd,canIdExt[i].pidVerCmd,4);//todo:保存当前读取版本号的指令
	goto endTest;
TestError:			
	ptrOK = Mem_malloc(10);
	ptrOK[0] = 3;
	ptrOK[1] = 0x50;
	ptrOK[2] = 0x20;
	if(varOperation.isDataFlow != 1)
	{
		OSMutexPend(CDMASendMutex,0,&err);
		memcpy(&cdmaDataToSend->data[FRAME_HEAD_LEN + varOperation.datOKLeng],ptrOK,ptrOK[0]);
		cdmaDataToSend->datLength += ptrOK[0];
		varOperation.datOKLeng    += ptrOK[0];
		OSMutexPost(CDMASendMutex);
	}
	Mem_free(ptrOK);
	
endTest:
	varOperation.canTest      = 0;
	varOperation.isStrenOilOK = 0; //todo:一旦进入自识别，就不能再进行动力提升
	varOperation.pidRun       = 0; //todo:一旦进入自识别，将不再发出 PID 数据
	if(temp == 200)
	{
		OSTimeDlyHMSM(0,0,5,0);
		goto TestStart;
		
	}
	return;
}
void ECU_ElecOff()//正常运行中，ECU掉电了
{
	varOperation.pidRun = 0;
}





