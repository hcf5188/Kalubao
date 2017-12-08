#include "apptask.h"
#include "obd.h"

void CANTestChannel(void );
void TestServer(void);

extern uint16_t freOBDLed;
extern _CDMADataToSend* cdmaDataToSend;//CDMA���͵������У�OBD��GPS������ͨ��������Ϊ����
uint8_t pidManyBag[8] = {0x30,0x00,0x00,0x00,0x00,0x00,0x00,0x00};

CAN1DataToSend  dataToSend; 

void OBDTask(void *pdata)
{
	INT8U     err;
	uint8_t   i      = 0;
	uint8_t   cmdLen = 0;        //�����ʱ��Ҫ��ȥָ��ĳ���
	uint8_t   cmdNum = 0;        //ָ�����
	uint8_t   cmdManyPackNum = 0;//Ҫ���ܵĶ��������
	
	CanRxMsg* CAN1_RxMsg;        //ָ����յ���OBD��Ϣ
	uint8_t * can1_Txbuff;       //ָ��Ҫ���͵�OBD��Ϣ
	uint8_t * ptrSaveBuff;
	
	CARVarInit();                //��CAN��صı����ĳ�ʼ��
	CAN1Config();                //CAN ����
	OSTimeDlyHMSM(0,0,1,4);
	TestServer();                //���Է�����

	while(1)
	{	
		can1_Txbuff = OSQPend(canSendQ,0,&err);//�յ�PIDָ����з���
		
		cmdNum = can1_Txbuff[0];               //��¼PIDָ�����
		cmdLen = can1_Txbuff[1];               //��¼PIDָ���
		
		dataToSend.pdat = &can1_Txbuff[1];     
		OBD_CAN_SendData(dataToSend);          //����PIDָ��
		

		CAN1_RxMsg = OSQPend(canRecieveQ,25,&err); //���յ�OBD�ظ�
		if(err == OS_ERR_NONE)
		{
			freOBDLed = 500;                    //OBD ��ʼ���ɹ�
			if(CAN1_RxMsg->Data[0] == 0x10)     //�������
			{
				ptrSaveBuff = Mem_malloc(CAN1_RxMsg->Data[1]+10);//������ڴ���㹻��
				if(ptrSaveBuff != NULL)
				{
					ptrSaveBuff[0] = CAN1_RxMsg->Data[1] - cmdLen + 3;
					ptrSaveBuff[1] = 0x60;
					ptrSaveBuff[2] = cmdNum;
					memcpy(&ptrSaveBuff[3],&CAN1_RxMsg->Data[cmdLen + 2],(6 - cmdLen));
					cmdManyPackNum = (CAN1_RxMsg->Data[1] - 6)%7 == 0? (CAN1_RxMsg->Data[1] - 6)/7 : (CAN1_RxMsg->Data[1] - 6)/7 + 1;
					Mem_free(CAN1_RxMsg);
					
					dataToSend.pdat = pidManyBag;//����0x30 ����������Ķ��
					OBD_CAN_SendData(dataToSend);
					
					for(i=0;i<cmdManyPackNum;i++)
					{
						CAN1_RxMsg = OSQPend(canRecieveQ,25,&err);//���ն��
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
			else  //��������
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
		Mem_free(can1_Txbuff);                 //�ͷ��ڴ��
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
	
	if(varOperation.ecuVersion == 0)
	{
		LogReport("01 - ECU Version %d,Need Config.",sysUpdateVar.ecuVersion);
		CANTestChannel();
		return	;
	}		
	
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_CAN1, ENABLE);//ʹ��CAN1ʱ��
	
	CAN_DeInit(CAN1);  
	CAN_StructInit(&CAN_InitStructure);
	//����flash�е�CAN���ý��в���
	CAN1_BaudSet(sysUpdateVar.canBaud);
	
	CAN1_SetFilter(sysUpdateVar.canRxId,sysUpdateVar.canIdType);
	CAN_ITConfig(CAN1,CAN_IT_FMP1,ENABLE);//����CAN�˲���
	
	CAN1_TxMsg.IDE     = sysUpdateVar.canIdType;
	CAN1_TxMsg.StdId   = sysUpdateVar.canTxId ;
	CAN1_TxMsg.ExtId   = sysUpdateVar.canTxId ;
	CAN1_TxMsg.DLC     = 0x08;
	
	CAN1_TxMsg.Data[0] = configData[5];CAN1_TxMsg.Data[1] = configData[6];CAN1_TxMsg.Data[2] = configData[7];//todo:���Ա���PID��ʲô��
	CAN1_TxMsg.Data[3] = configData[8];CAN1_TxMsg.Data[4] = configData[9];CAN1_TxMsg.Data[5] = configData[10];
	CAN1_TxMsg.Data[6] = configData[11];CAN1_TxMsg.Data[7] = configData[12];
	CAN_Transmit(CAN1,&CAN1_TxMsg);
	CAN1_RxMsg = OSQPend(canRecieveQ,500,&err);
	if(err ==OS_ERR_NONE)
	{
		dataToSend.ide = sysUpdateVar.canIdType;
		dataToSend.canId   = sysUpdateVar.canTxId;
		Mem_free(CAN1_RxMsg);
		LogReport("01 - ECU Version %d test Success.",sysUpdateVar.ecuVersion);
		varOperation.canTest = 2;//Flash�е�CAN���óɹ�
		
		CAN_DeInit(CAN1);  
		CAN_StructInit(&CAN_InitStructure);
		//����flash�е�CAN���ý��в���
		CAN1_BaudSet(sysUpdateVar.canBaud);
		
		CAN1_ClearFilter();  //����˲�������������ģʽ������һ��CAN��Ϣ
		CAN_ITConfig(CAN1,CAN_IT_FMP1,ENABLE);//����CAN�˲���
		//todo:������������
	}
	else
	{
		LogReport("ECU Version %d test Error!",sysUpdateVar.ecuVersion);
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
	CanTxMsg  CAN1_TxMsg;
    uint8_t* ptrTestCan;
	uint8_t cmdManyPackNum;
	CAN_InitTypeDef   CAN_InitStructure;
	
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_CAN1, ENABLE);//ʹ��CAN1ʱ��

//baud:	
	varOperation.canTest = 0;
	for(i = 0;i<NUMOfCANBaud;i++)       //���CAN�Ĳ�����
	{
		CAN_DeInit(CAN1);
		CAN_StructInit(&CAN_InitStructure);
		CAN1_BaudSet(canBaudEnum[i]); 
		CAN1_ClearFilter();
		CAN_ITConfig(CAN1,CAN_IT_FMP1,ENABLE);
		
		//ֻҪ�˲������������ݣ�CAN�����ʾ�ȷ����
		CAN1_RxMsg = OSQPend(canRecieveQ,500,&err);
		if(err == OS_ERR_NONE)
		{
			varOperation.canBaud = canBaudEnum[i];
			Mem_free(CAN1_RxMsg);
			LogReport("Baud %d Test Success.",canBaudEnum[i]);
			break;
		}
	}//���������û��ȷ������һֱȥȷ��������
	if((i >= NUMOfCANBaud) && (err != OS_ERR_NONE))
	{
		LogReport("Baud Test Fail!!!");
		varOperation.canTest = 0;
		goto idOK;
	}
		
//reID:	
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
		CAN_ITConfig(CAN1,CAN_IT_FMP1,ENABLE);//����CAN�˲���
		
		dataToSend.canId = canIdExt[i];
		
		CAN1_TxMsg.IDE     = CAN_ID_EXT;
		CAN1_TxMsg.StdId   = dataToSend.canId;
		CAN1_TxMsg.ExtId   = dataToSend.canId;
		CAN1_TxMsg.DLC     = 0x08;
		
		CAN1_TxMsg.Data[0] = 0x02;CAN1_TxMsg.Data[1] = 0x1A;CAN1_TxMsg.Data[2] = 0x94;
		CAN1_TxMsg.Data[3] = 0x00;CAN1_TxMsg.Data[4] = 0x00;CAN1_TxMsg.Data[5] = 0x00;
		CAN1_TxMsg.Data[6] = 0x00;CAN1_TxMsg.Data[7] = 0x00;
		CAN_Transmit(CAN1,&CAN1_TxMsg);
		
		CAN1_RxMsg = OSQPend(canRecieveQ,50,&err);
		if(err == OS_ERR_NONE)
		{
			varOperation.canIdType = CAN_ID_EXT;
			varOperation.canTxId   = dataToSend.canId;
			if(CAN1_RxMsg->Data[0] == 0x10)     //�������
			{
				ptrTestCan = Mem_malloc(CAN1_RxMsg->Data[1]+15);//������ڴ���㹻��
				if(ptrTestCan != NULL)
				{
					ptrTestCan[0] = 31;
					ptrTestCan[1] = 0x50;
					ptrTestCan[2] = 0x13;
					ptrTestCan[3] = varOperation.canBaud;
					ptrTestCan[4] = (varOperation.canRxId>>24) &0x000000FF;//��·��ID
					ptrTestCan[5] = (varOperation.canRxId>>16) &0x000000FF;
					ptrTestCan[6] = (varOperation.canRxId>>8) &0x000000FF;
					ptrTestCan[7] = (varOperation.canRxId>>0) &0x000000FF;
					
					ptrTestCan[8]  = (varOperation.canTxId>>24) &0x000000FF;//ECUID
					ptrTestCan[9]  = (varOperation.canTxId>>16) &0x000000FF;
					ptrTestCan[10] = (varOperation.canTxId>>8) &0x000000FF;
					ptrTestCan[11] = (varOperation.canTxId>>0) &0x000000FF;
					
					memcpy(&ptrTestCan[12],&CAN1_RxMsg->Data[4],4);
					cmdManyPackNum = (CAN1_RxMsg->Data[1] - 6)%7 == 0? (CAN1_RxMsg->Data[1] - 6)/7 : (CAN1_RxMsg->Data[1] - 6)/7 + 1;
					Mem_free(CAN1_RxMsg);
					
					dataToSend.pdat = pidManyBag;//����0x30 ����������Ķ��
					OBD_CAN_SendData(dataToSend);
					
					for(i=0;i<cmdManyPackNum;i++)
					{
						CAN1_RxMsg = OSQPend(canRecieveQ,25,&err);//���ն��
						if(err == OS_ERR_NONE)
						{
							memcpy(&ptrTestCan[7*i + 16],&CAN1_RxMsg->Data[1],7);
							Mem_free(CAN1_RxMsg);
						}
						else break;
					}
					if(i == cmdManyPackNum)
					{
						OSMutexPend(CDMASendMutex,0,&err);
						
						memcpy(&cdmaDataToSend->data[cdmaDataToSend->datLength],ptrTestCan,ptrTestCan[0]);
						cdmaDataToSend->datLength += ptrTestCan[0];
						
						OSMutexPost(CDMASendMutex);
					}
					Mem_free(ptrTestCan);
					LogReport("CANID - %d Test Success type-%d .",dataToSend.canId,CAN_ID_EXT);
					goto idOK;
				}
			}
			else
			{
				LogReport("CANID %d receive wrong data!",varOperation.canTxId);
				varOperation.canTest = 0;
			}
			Mem_free(CAN1_RxMsg);
		}
	}
	LogReport("CANID EXT Test Fail!");
	varOperation.canTest = 0;
//	if(i >= NUMOfCANID_STD&&err != OS_ERR_NONE)
//		goto reID;
idOK:
	return;
}
extern VARConfig* ptrPIDVars;//ָ��ڶ�������


void PIDVarGet(uint8_t cmdId,uint8_t *ptrData)
{
	uint8_t  i; 
	uint32_t saveDate;
	uint8_t  saveLen = 0;
	uint32_t  temp = 0;
	for(i = 0;i < varOperation.pidVarNum;i ++)
	{
		if((ptrPIDVars + i)->pidId != cmdId)
			continue;
		while(saveLen < (ptrPIDVars + i)->bitLen)
		{
			if(saveLen < 8)
				temp = ptrData[ptrPIDVars->startByte];
			else if(saveLen >= 8 && saveLen < 16)
			{
				temp = ptrData[ptrPIDVars->startByte + 1];
				temp <<= 8;
			}else if(saveLen >= 16 && saveLen < 24)
			{
				temp = ptrData[ptrPIDVars->startByte + 2];
				temp <<= 16;
			}else if(saveLen >= 24 && saveLen < 32)
			{
				temp = ptrData[ptrPIDVars->startByte + 3];
				temp <<= 24;
			}
			saveDate += temp & (0x00000001 << saveLen);
			saveLen ++;
		}
		
		//1-���� 2-������ת�� 3-�������� 4-�������� 5-Ԥ������ 6-�������� 7-��ǰ������
		switch((ptrPIDVars + i)->varId)
		{
			case 1://���㳵��
				carStateVar.carSpeed = saveDate * ((ptrPIDVars + i)->ceo) + (ptrPIDVars + i)->offset;
				break;
			case 2://ת��
				if(carStateVar.engineSpeedTemp != 1)
				carStateVar.engineSpeed = saveDate * ((ptrPIDVars + i)->ceo) + (ptrPIDVars + i)->offset;
				break;
			case 3:break;
			case 4:break;
			case 5:break;
			case 6:break;
			case 7:break;
			default:break;
		}
	}
}








