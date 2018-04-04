#include "apptask.h"
#include "bsp.h"
#include "obd.h"

void ElecUP(void);         //��һ���ϵ��߼�
void CANTestChannel(void); //ECU ��ʶ��
extern uint16_t adcValue;
extern CAN1DataToSend  dataToSend;
extern u8 CANerr_flg,CANerr_state,CANr1,CANr2,CANr3,CANr4,CANr5,CANr6,CANr7;
extern u8 fCanOK;
extern uint16_t freOBDLed;

void OBD_ON_OFFDeal(void *pdata)
{
	uint8_t err,temp;
	uint8_t * can1_Txbuff;
	
	OSSemPend(LoginMes,0,&err);   //�ȴ���������
	CAN1Config(); //����CAN
	ElecUP();
	OSTimeDlyHMSM(0,0,5,0);
	while(1)
	{
		OSTimeDlyHMSM(0,0,2,0);
		if(varOperation.flagCAN == 1||varOperation.canTest == 0)//�����������ɼ��� / ϵͳ������ʶ��
		{
			if(varOperation.canTest == 0)
				LogReport("TestECU Fail;");
			continue;
		}
		if(varOperation.flagJ1939 == 0 && varOperation.flagCAN == 0)//ECU����
		{
			do{
				can1_Txbuff = OSQAccept(canSendQ,&err);//�����Ϣ���������Ϣ
				Mem_free(can1_Txbuff);
			}while(err == OS_ERR_NONE);
			CANr1=0;CANr2=0;CANr3=0;CANr4=0;CANr5=0;CANr6=0;CANr7=0;fCanOK=0;
			LogReport("\r\n08-ECU OFF!");
	
			IsCanCommunicationOK();
			if(varOperation.canTest == 1)
			{
				Get_Q_FromECU();                          //��ȡ��������ֵ
				varOperation.pidRun = 1;                  //����PID���ݷ���
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
void IsCanCommunicationOK(void)//�ö�ȡ�汾�ŵ�ָ��Ͳ���֡
{
	CanRxMsg* CAN1_RxMsg;
	uint8_t err;
	
	dataToSend.canId = canDataConfig.canTxId;
	dataToSend.ide   = canDataConfig.canIdType;
	dataToSend.pdat  = canDataConfig.pidVerCmd;    //�ö�ȡ�汾�ŵ�ָ����в���
	varOperation.flagRecvOK = 0;
	do{
		OBD_CAN_SendData(dataToSend.canId,dataToSend.ide,dataToSend.pdat);
		CAN1_RxMsg = OSQPend(canRecieveQ,500,&err);
		
		LogReport("\r\nTest Mode!;");
		
		Mem_free(CAN1_RxMsg);
		OSTimeDlyHMSM(0,0,3,0);
			
	}while(err != OS_ERR_NONE);
	
}
void ElecUP(void)                       //��·����һ���ϵ��������������
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
		CAN1_BaudSet(canDataConfig.canBaud);  //����flash�е�CAN���ý��в���
		CAN1_ClearFilter();  
		CAN_ITConfig(CAN1,CAN_IT_FMP1,ENABLE);//����CAN�˲���
	
		dataToSend.ide   = canDataConfig.canIdType;
		dataToSend.canId = canDataConfig.canTxId;
abc:		
		isCANOK = ReadECUVersion(canDataConfig.pidVerCmd);
		if(isCANOK < 100)				//��ȡECU�汾�ţ���ȡ���ɹ��򲻽��ж�������
		{
			freOBDLed = LEDSLOW;
			varOperation.canTest = 1;
			Get_Q_FromECU();            //��ȡ��������ԭʼֵ���õ���ǰ����ϵ��
			OSTimeDlyHMSM(0,0,10,0);    //�궨��������Ҫ�ȴ� 20s ��ȡ�����룬�����ȴ��Ļ������������Ǿܾ�Ӧ��
			varOperation.pidRun = 1;    //���Կ�ʼ������
		}
		else if(isCANOK == 200)         //��ȡ�汾��ʧ�ܣ� �����ļ����� ��
		{
			IsCanCommunicationOK();     //�������״̬
			goto abc;
		}
			
		else                            //�汾�Ŷ�ȡ�����ˣ����ǲ��������������Ǿ������ܰ�
		{
			varOperation.canTest = 2;   
			OSTimeDlyHMSM(0,0,2,0);     //��ȡ����������Ҫʱ���
			varOperation.pidRun = 1;    //���Կ�ʼ������
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
	
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_CAN1, ENABLE);//ʹ�� CAN1 ʱ��

	TestStart:
	CAN_DeInit(CAN1);
	CAN_StructInit(&CAN_InitStructure);
	CAN1_BaudSet(CANBAUD_250K); 
	CAN1_ClearFilter();
	CAN_ITConfig(CAN1,CAN_IT_FMP1,ENABLE);
		
	//ֻҪ�˲���������J1939���ݣ�CAN �����ʾ�ȷ����
	CAN1_RxMsg = OSQPend(canJ1939Q,500,&err);
	if(err == OS_ERR_NONE)  //14230Э�飬250K��J1939������
	{
		canDataConfig.canBaud = CANBAUD_250K;
		Mem_free(CAN1_RxMsg);
		LogReport("\r\n10-Test Baud:%d;",canDataConfig.canBaud);
		for(i = 0;i < 3;i ++)//ģʽ1��ʱ��һ����3��CANID
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
	else   // 14229 Э�飬 J1939 û����
	{
		CAN_DeInit(CAN1);
		CAN_StructInit(&CAN_InitStructure);
		CAN1_BaudSet(CANBAUD_500K); 
		CAN1_ClearFilter();
		CAN_ITConfig(CAN1,CAN_IT_FMP1,ENABLE);
		for(i = 3;i < 5;i ++)//ģʽ1��ʱ��һ����3��CANID
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
	memcpy(canDataConfig.pidVerCmd,canIdExt[i].pidVerCmd,4);//todo:���浱ǰ��ȡ�汾�ŵ�ָ��
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
	varOperation.isStrenOilOK = 0; //todo:һ��������ʶ�𣬾Ͳ����ٽ��ж�������
	varOperation.pidRun       = 0; //todo:һ��������ʶ�𣬽����ٷ��� PID ����
	if(temp == 200)
	{
		OSTimeDlyHMSM(0,0,5,0);
		goto TestStart;
		
	}
	return;
}
void ECU_ElecOff()//���������У�ECU������
{
	varOperation.pidRun = 0;
}





