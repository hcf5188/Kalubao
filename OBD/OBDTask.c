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
	uint8_t   cmdLen = 0;        //�����ʱ��Ҫ��ȥָ��ĳ���
	uint8_t   cmdNum = 0;        //ָ�����
	uint8_t   cmdManyPackNum = 0;//Ҫ���ܵĶ��������
	CanRxMsg* CAN1_RxMsg;        //ָ����յ���OBD��Ϣ
	uint8_t * can1_Txbuff;       //ָ��Ҫ���͵�OBD��Ϣ
	uint8_t * ptrSaveBuff;
	
	CAN1Config();                //CAN ����
	OSTimeDlyHMSM(0,0,10,4);
	TestServer();                //���Է�����
	
	while(1)
	{	
		if(varOperation.canTest == 0)//�����ļ����ɹ�����ֹͣCAN
		{
			OSTimeDlyHMSM(0,0,1,0);
			continue;
		}	  
		can1_Txbuff = OSQPend(canSendQ,1000,&err);//�յ�PIDָ����з���
		if(err != OS_ERR_NONE)
			continue;
		cmdNum = can1_Txbuff[0];               //��¼PIDָ�����
		cmdLen = can1_Txbuff[1];               //��¼PIDָ���
		
		dataToSend.pdat = &can1_Txbuff[1];     
		OBD_CAN_SendData(dataToSend);          //����PIDָ��
		
		CAN1_RxMsg = OSQPend(canRecieveQ,500,&err); //���յ�OBD�ظ�
		if(err == OS_ERR_NONE)
		{
			freOBDLed = 500;                    //OBD ��ʼ���ɹ�
			if(CAN1_RxMsg->Data[0] == 0x10)     //�������
			{
				ptrSaveBuff = Mem_malloc(CAN1_RxMsg->Data[1] + 10);//������ڴ���㹻��
				if(ptrSaveBuff != NULL)
				{
					ptrSaveBuff[0] = CAN1_RxMsg -> Data[1] + 3;
					ptrSaveBuff[1] = 0x60;
					ptrSaveBuff[2] = cmdNum;
					memcpy(&ptrSaveBuff[3],&CAN1_RxMsg->Data[2],6);
					cmdManyPackNum = (CAN1_RxMsg->Data[1] - 6)%7 == 0? (CAN1_RxMsg->Data[1] - 6)/7 : (CAN1_RxMsg->Data[1] - 6)/7 + 1;
					Mem_free(CAN1_RxMsg);
					
					dataToSend.pdat = pidManyBag;//����0x30 ����������Ķ��
					OBD_CAN_SendData(dataToSend);
					
					for(i=0;i<cmdManyPackNum;i++)
					{
						CAN1_RxMsg = OSQPend(canRecieveQ,25,&err);//���ն��
						if(err == OS_ERR_NONE)
						{
							memcpy(&ptrSaveBuff[7*i + 9],&CAN1_RxMsg->Data[1],7);
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
					PIDVarGet(cmdNum,&ptrSaveBuff[3]);
					Mem_free(ptrSaveBuff);
				}
			}
			else  //��������
			{
				if(CAN1_RxMsg->Data[0]>cmdLen)
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
					LogReport("03 7F - ECU refuse to reply! PID-%d;",cmdNum);
				}
				
				Mem_free(CAN1_RxMsg);
			}
		}
		else
		{
			LogReport("PID cmd %d read error!!!",cmdNum);
		}
		Mem_free(can1_Txbuff);                 //�ͷ��ڴ��
	}
}

extern uint8_t configData[2048];

void TestServer(void)//�÷������·���ID��Baud�ȵȽ���CAN����
{
	uint8_t err;
	CanRxMsg* CAN1_RxMsg;
	CAN_InitTypeDef   CAN_InitStructure;
	
	OSSemPend(LoginMes,0,&err);//1394606080
	if((varOperation.pidVersion ==0xFFFFFFFF )||(varOperation.pidNum==0xFFFF)||(varOperation.busType==0xFF))
	{
		varOperation.canTest = 0;
		LogReport("Please binDing this equipment!");
		return;
	}	
	
	if(varOperation.pidVersion == 0)
	{
		LogReport("01 - ECU Version %d,Need Config.",canDataConfig.pidVersion);
		CANTestChannel();
		return	;
	}
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_CAN1, ENABLE);//ʹ��CAN1ʱ��
	
	varOperation.canTest = 2;//Flash�е�CAN���óɹ�
	
	CAN_DeInit(CAN1);  
	CAN_StructInit(&CAN_InitStructure);
	if(canDataConfig.canBaud == 8||canDataConfig.canBaud == 9)
		CAN1_BaudSet(CANBAUD_250K);   //����flash�е�CAN���ý��в���
	else if(canDataConfig.canBaud == 6||canDataConfig.canBaud == 7)
		CAN1_BaudSet(CANBAUD_500K);
	CAN1_ClearFilter();                   //����˲�������������ģʽ������һ��CAN��Ϣ
	CAN_ITConfig(CAN1,CAN_IT_FMP1,ENABLE);//����CAN�˲���
	
	dataToSend.canId = canDataConfig.canTxId;
	dataToSend.ide   = canDataConfig.canIdType;
	dataToSend.pdat  = &configData[59];//CAN PID �ĵ�һ������
	
	OBD_CAN_SendData(dataToSend);
	
	CAN1_RxMsg = OSQPend(canRecieveQ,500,&err);
	if(err == OS_ERR_NONE)
	{
		dataToSend.ide = canDataConfig.canIdType;
		dataToSend.canId   = canDataConfig.canTxId;
		Mem_free(CAN1_RxMsg);
		LogReport("01 - PID Version %d test Success.",canDataConfig.pidVersion);
		if(ReadECUVersion() == 0)//��ȡECU�汾�ţ���ȡ���ɹ��򲻽��ж�������
			Get_Q_FromECU();     //��ǿ����
		else 
		{//û��ƥ��İ汾�ţ�û�ܽ��ж�����ǿ
//			CAN_DeInit(CAN1);
//			CAN_StructInit(&CAN_InitStructure);
//			CAN1_BaudSet(varOperation.canBaud);
//			CAN1_SetFilter(varOperation.canRxId ,CAN_ID_EXT);
//			CAN_ITConfig(CAN1,CAN_IT_FMP1,ENABLE);//����CAN�˲���
//			varOperation.canTest = 0;
		}
		CAN_DeInit(CAN1);
		CAN_StructInit(&CAN_InitStructure);
		CAN1_BaudSet(varOperation.canBaud);
		CAN1_SetFilter(varOperation.canRxId ,CAN_ID_EXT);
		CAN_ITConfig(CAN1,CAN_IT_FMP1,ENABLE);//����CAN�˲���
		varOperation.canTest = 0;
		OSTimeDlyHMSM(0,0,20,0);
		CAN_DeInit(CAN1);  
		CAN_StructInit(&CAN_InitStructure);
		if(canDataConfig.canBaud == 8||canDataConfig.canBaud == 9)
			CAN1_BaudSet(CANBAUD_250K);   //����flash�е�CAN���ý��в���
		else if(canDataConfig.canBaud == 6||canDataConfig.canBaud == 7)
			CAN1_BaudSet(CANBAUD_500K);
		CAN1_ClearFilter();                   //����˲�������������ģʽ������һ��CAN��Ϣ
		CAN_ITConfig(CAN1,CAN_IT_FMP1,ENABLE);//����CAN�˲���
		varOperation.canTest = 2;
	}
	else
	{
		LogReport("ECU Version %d test Error!",canDataConfig.pidVersion);
		CANTestChannel();
	}	
}

extern CANBAUD_Enum canBaudEnum[NUMOfCANBaud];
extern uint32_t canIdExt[NUMOfCANID_EXT];
extern uint32_t canIdStd[NUMOfCANID_STD][2];
//�������������ļ��������߻�û�������ã�ϵͳ������ʶ��ʶ��ɹ�����Ϣ�ϱ�
void CANTestChannel(void )
{
	uint8_t err,i;
	CanRxMsg* CAN1_RxMsg;
	uint8_t*  ptrOK;
	CAN_InitTypeDef   CAN_InitStructure;
	
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_CAN1, ENABLE);//ʹ��CAN1ʱ��

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
	}//���������û��ȷ�����ϱ���־���˳�
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
		varOperation.canRxId  =(varOperation.canRxId  << 8)+(canIdExt[i] & 0x000000FF);
		varOperation.canRxId  =(varOperation.canRxId  << 8)+((canIdExt[i]>>8) & 0x000000FF);
		
		CAN_DeInit(CAN1);
		CAN_StructInit(&CAN_InitStructure);
		CAN1_BaudSet(varOperation.canBaud);
		CAN1_SetFilter(varOperation.canRxId ,CAN_ID_EXT);
		CAN_ITConfig(CAN1,CAN_IT_FMP1,ENABLE);//����CAN�˲���
		
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
	varOperation.canTest = 0;//���ٻ�ȡ PID ��Ϣ
	return;
}
extern VARConfig* ptrPIDVars;//ָ��ڶ�������

void PIDVarGet(uint8_t cmdId,uint8_t ptrData[])
{
	uint8_t  i,j; 
	uint64_t  saveDate;
	uint8_t  byteNum = 0;
	uint8_t  temp = 0;
	
	for(i = 0;i < varOperation.pidVarNum;i ++)
	{
		if((ptrPIDVars + i)->pidId != cmdId)
			continue;
		//����ֽ���
		byteNum = ((ptrPIDVars + i)->bitLen +(ptrPIDVars + i)->startBit)%8 ==0?\
						((ptrPIDVars + i)->bitLen + (ptrPIDVars + i)->startBit) / 8 : \
						((ptrPIDVars + i)->bitLen + (ptrPIDVars + i)->startBit) / 8 + 1;
		if((ptrPIDVars + i)->dataHL == 1)//�����ǰ
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
					temp &= 0xff>>(byteNum *8 - ((ptrPIDVars + i)->bitLen +(ptrPIDVars + i)->startBit));
					saveDate += temp;
				}else
				{
					saveDate <<= 8;
					temp = ptrData[ptrPIDVars->startByte + j];
					saveDate += temp;
				}
				j++;
			}while(j < byteNum);	
		}else if((ptrPIDVars + i)->dataHL == 2)//С����ǰ
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
			case 1://���㳵��
				if(carAllRecord.carSpeedTemp != 1)
					carAllRecord.carSpeed = (uint16_t)(saveDate * ((ptrPIDVars + i)->ceo) + (ptrPIDVars + i)->offset);
				break;
			case 2://ת��
				if(carAllRecord.engineSpeedTemp != 1)
					carAllRecord.engineSpeed = (uint16_t)(saveDate * ((ptrPIDVars + i)->ceo) + (ptrPIDVars + i)->offset);
				break;
			case 3://��������
				if(carAllRecord.allFuelTemp != 1)
					{
						carAllRecord.allFuel = (uint16_t)(saveDate * ((ptrPIDVars + i)->ceo) + (ptrPIDVars + i)->offset);
						carAllRecord.totalFuel += carAllRecord.allFuel;
					}
					break;
			case 4://��������
				carAllRecord.primaryFuel = (uint16_t)(saveDate * ((ptrPIDVars + i)->ceo) + (ptrPIDVars + i)->offset);
				carAllRecord.totalFuel += carAllRecord.primaryFuel;
				break;
			case 5://Ԥ������ 1
				carAllRecord.beforeFuel1 = (uint16_t)(saveDate * ((ptrPIDVars + i)->ceo) + (ptrPIDVars + i)->offset);
				carAllRecord.totalFuel += carAllRecord.beforeFuel1;
				break;
			case 6://Ԥ������ 2
				carAllRecord.beforeFuel2 = (uint16_t)(saveDate * ((ptrPIDVars + i)->ceo) + (ptrPIDVars + i)->offset);
				carAllRecord.totalFuel += carAllRecord.beforeFuel2;
				break;
			case 7://Ԥ������ 3
				carAllRecord.beforeFuel3 = (uint16_t)(saveDate * ((ptrPIDVars + i)->ceo) + (ptrPIDVars + i)->offset);
				carAllRecord.totalFuel += carAllRecord.beforeFuel3;
				break;
			case 8://�������� 1
				carAllRecord.afterFuel1 = (uint16_t)(saveDate * ((ptrPIDVars + i)->ceo) + (ptrPIDVars + i)->offset);
				carAllRecord.totalFuel += carAllRecord.afterFuel1;
				break;
			case 9://�������� 2
				carAllRecord.afterFuel2 = (uint16_t)(saveDate * ((ptrPIDVars + i)->ceo) + (ptrPIDVars + i)->offset);
				carAllRecord.totalFuel += carAllRecord.afterFuel2;
				break;
			case 10://��������3
				carAllRecord.afterFuel3 = (uint16_t)(saveDate * ((ptrPIDVars + i)->ceo) + (ptrPIDVars + i)->offset);
				carAllRecord.totalFuel += carAllRecord.afterFuel3;
				break;
			case 11://��ǰ������
				carAllRecord.nowFuel = (uint16_t)(saveDate * ((ptrPIDVars + i)->ceo) + (ptrPIDVars + i)->offset);
				carAllRecord.totalFuel += carAllRecord.nowFuel;
				break;
			default:break;
		}
	}
}

//extern const uint8_t weiChai[21][6];

extern uint8_t strengPower[100];
const uint16_t qMax = 0x045C;
void Get_Q_FromECU(void)
{
	uint8_t* ptrSetCmd;
	uint8_t* ptrVer;
	CanRxMsg* CAN1_RxMsg;
	uint8_t  i,err;
	uint16_t dat1,dat2,datSma,datFlash;
	uint8_t qNum = 0;
	uint8_t offset = 7,bag = 0x21;
	static uint8_t  temp = 0;
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
		ptrVer[6] = qNum * 2;
		ptrVer[1] = ptrVer[6] + 5;
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
			
				LogReport("CURR_OIL:%d,COM_OIL:%d",dat1,dat2);
				if(offset % 8 == 0)
					ptrVer[offset++] = bag++;
				ptrVer[offset++] = dat2 & 0x00FF;
				if(offset % 8 == 0)
					ptrVer[offset++] = bag++;
				ptrVer[offset++] = (dat2>>8) & 0x00FF;
				Mem_free(CAN1_RxMsg);
				if(strengPower[0] != 0x1A)				
				{
					strengPower[i*2 + 1] = dat2 & 0x00FF;
					strengPower[i*2 + 2] = (dat2>>8) & 0x00FF;
				}
				else
				{
					datFlash = strengPower[i*2 + 2];
					datFlash <<= 8;
					datFlash += strengPower[i*2 + 1];
					if(datFlash != dat2)
					{
						LogReport("PenYou Read With Flash Don't Equal!!!");
						temp = 1;
						break;
					}
				}
			}
			else{
				LogReport("PenYou Read Wrong!!!");
				temp = 1;//��ȡ����������
				break;
			}	
			if(strengPower[0] != 0x1A && i == (qNum - 1))
			{
				strengPower[0] = 0x1A;
				SoftErasePage(STRENGE_Q);
				Save2KDataToFlash(strengPower,STRENGE_Q,100);
				LogReport("PenYou Write to Flash OK!!!");
			}	
		}
	}
	//��ȫ�㷨����д������
	if(temp == 0)//��֤�����ظ�д�롢�������󵽲�������
	{
		temp = 1;
		
		SafeALG(ptrVer);//��ȫ�㷨
	}
	else
	{
		Mem_free(ptrVer);
	}
}









