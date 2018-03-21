#include "apptask.h"
#include "bsp.h"
#include "obd.h"

void CANTestChannel(void );
void TestServer(void);
void Deal5016(uint8_t cmdNum,uint8_t *p);

extern uint16_t freOBDLed;
extern uint8_t * pPid[52];
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
	uint8_t * ptrSaveBuff;       //
	CAN1Config();                //CAN ����
	OSTimeDlyHMSM(0,0,10,4);     
	TestServer();                //���Է�����
	while(1)
	{	
		StrengthFuel();
		if((varOperation.canTest == 0)||(varOperation.pidTset == 1))      //�����ļ����ɹ��������ڲ���PIDָ���ֹͣCAN
		{
			OSTimeDlyHMSM(0,0,1,0);	
			continue;
		}	 
		can1_Txbuff = OSQPend(canSendQ,1000,&err);                        //�յ� PID ָ����з���
		if(err != OS_ERR_NONE)
			continue;
		cmdNum = can1_Txbuff[0];  //��¼PIDָ�����
		cmdLen = can1_Txbuff[1];  //��¼PIDָ���
		
		dataToSend.pdat = &can1_Txbuff[1];   
		OBD_CAN_SendData(dataToSend.canId,dataToSend.ide,dataToSend.pdat);//����PIDָ��
		
		CAN1_RxMsg = OSQPend(canRecieveQ,400,&err); // ���յ� OBD �ظ�
		if(err == OS_ERR_NONE)
		{
			freOBDLed = LEDSLOW;                    // OBD ��ʼ���ɹ�
			if(CAN1_RxMsg->Data[0] == 0x10)         // �������
			{
				ptrSaveBuff = Mem_malloc(CAN1_RxMsg->Data[1] + 10);       //������ڴ���㹻��
				if(ptrSaveBuff != NULL)
				{
					ptrSaveBuff[0] = CAN1_RxMsg -> Data[1] + 4;
					if(cmdNum == 236)
					{
						ptrSaveBuff[1] = 0x50;
						ptrSaveBuff[2] = 0x24;
					}
					else{
						ptrSaveBuff[1] = 0x60;
						ptrSaveBuff[2] = cmdNum;
					}
					ptrSaveBuff[3] = CAN1_RxMsg -> Data[1];              //�����볤��
					
					memcpy(&ptrSaveBuff[4],&CAN1_RxMsg->Data[2],6);
					cmdManyPackNum = (CAN1_RxMsg->Data[1] - 6) % 7 == 0 ? (CAN1_RxMsg->Data[1] - 6)/7 : (CAN1_RxMsg->Data[1] - 6)/7 + 1;
					Mem_free(CAN1_RxMsg);
					
					dataToSend.pdat = pidManyBag;                    //���� 0x30 ����������Ķ��
					OBD_CAN_SendData(dataToSend.canId,dataToSend.ide,dataToSend.pdat);
					
					for(i = 0;i < cmdManyPackNum;i++)
					{
						CAN1_RxMsg = OSQPend(canRecieveQ,25,&err);   //���ն��
						if(err == OS_ERR_NONE)
						{
							memcpy(&ptrSaveBuff[7*i + 10],&CAN1_RxMsg->Data[1],7);
							Mem_free(CAN1_RxMsg);
						}
						else 
							break;
					} 
					if(i == cmdManyPackNum && varOperation.isDataFlow != 1)
					{
						OSMutexPend(CDMASendMutex,0,&err);
						
						memcpy(&cdmaDataToSend->data[FRAME_HEAD_LEN + varOperation.datOKLeng],ptrSaveBuff,ptrSaveBuff[0]);
						cdmaDataToSend->datLength += ptrSaveBuff[0];
						varOperation.datOKLeng += ptrSaveBuff[0];    //��¼���ɲ�ж�İ�����
						
						OSMutexPost(CDMASendMutex);
					}
					Mem_free(ptrSaveBuff);
				}
			}
			else  //��������
			{
				if(CAN1_RxMsg->Data[0] > cmdLen)//������PIDָ��
				{
					OSMutexPend(CDMASendMutex,0,&err);
					// todo: �������ݳ��ȣ�����Խ��
					if(cmdNum != 236)
					{
						pPid[cmdNum - 1][3]  = CAN1_RxMsg->Data[0] - cmdLen;
						if((cmdNum<12&&pPid[cmdNum - 1][0]<58)||(cmdNum>11&&cmdNum<60&&pPid[cmdNum - 1][0]<30))
						{
							memcpy(&pPid[cmdNum - 1][pPid[cmdNum - 1][0]],&CAN1_RxMsg->Data[cmdLen + 1],(CAN1_RxMsg->Data[0] - cmdLen));
							pPid[cmdNum - 1][0] += CAN1_RxMsg->Data[0] - cmdLen;
							cdmaDataToSend->datLength += CAN1_RxMsg->Data[0] - cmdLen;
						}
						OSMutexPost(CDMASendMutex);
						Deal5016(cmdNum,&CAN1_RxMsg->Data[cmdLen + 1]);
					}
					else if(cmdNum == 236)	
					{
						ptrSaveBuff = Mem_malloc(8);
						ptrSaveBuff[0] = CAN1_RxMsg -> Data[0] + 4;
						ptrSaveBuff[1] = 0x50;
						ptrSaveBuff[2] = cmdNum - 200;
						ptrSaveBuff[3] = CAN1_RxMsg -> Data[0];              //�����볤��
						memcpy(&ptrSaveBuff[4],&CAN1_RxMsg->Data[1],CAN1_RxMsg->Data[0]);
						
						OSMutexPend(CDMASendMutex,0,&err);
								
						memcpy(&cdmaDataToSend->data[FRAME_HEAD_LEN + varOperation.datOKLeng],ptrSaveBuff,ptrSaveBuff[0]);
						cdmaDataToSend->datLength += ptrSaveBuff[0];
						varOperation.datOKLeng += ptrSaveBuff[0];    //��¼���ɲ�ж�İ�����
						
						OSMutexPost(CDMASendMutex);
						Mem_free(ptrSaveBuff);
					}
				}
				else if((CAN1_RxMsg->Data[0]==0x03)&&(CAN1_RxMsg->Data[1]==0x7F))
				{
					if(cmdNum == 201)                       //�������������
					{	
						Mem_free(CAN1_RxMsg);
						CAN1_RxMsg = OSQPend(canRecieveQ,50,&err); // ��ʱ�յ����������
						OSMutexPend(CDMASendMutex,0,&err);
						if(err == OS_ERR_NONE)
						{
							if(CAN1_RxMsg->Data[1] == 0x54)	//�������ɹ�
							{
								pPid[49][0] = 4;
								pPid[49][3] = 0;
								SendFaultCmd();
							}
							else                            //�������ʧ��
							{
								pPid[49][0] = 4;
								pPid[49][3] = 1;
							}										
						}else                               //�������ʧ��
						{
							pPid[49][0] = 4;
							pPid[49][3] = 1;
						}
						OSMutexPost(CDMASendMutex);						
					}
					else if(cmdNum < 100)
						LogReport("\r\n05-ECU report:03 7F;PID:%d err:%d,%d,%d,%d,%d,%d;",cmdNum,
							CAN1_RxMsg->Data[2],CAN1_RxMsg -> Data[3],CAN1_RxMsg->Data[4],CAN1_RxMsg->Data[5],
							CAN1_RxMsg->Data[6],CAN1_RxMsg -> Data[7]);
					else if((cmdNum == 234||cmdNum == 235) && strengthFuelFlash.modeOrder == 2)//�����������ȡ  03 7F 78
					{
						Mem_free(CAN1_RxMsg);
						varOperation.pidRun = 0;//ֹͣ����PIDָ��
						CAN1_RxMsg = OSQPend(canRecieveQ,2000,&err); // ���յ�OBD�ظ�
						if(err == OS_ERR_NONE)
						{	
							if(CAN1_RxMsg->Data[0] == 0x10)          // �������
							{
								ptrSaveBuff = Mem_malloc(CAN1_RxMsg->Data[1] + 10);      //������ڴ���㹻��
								if(ptrSaveBuff != NULL)
								{
									ptrSaveBuff[0] = CAN1_RxMsg -> Data[1] + 4;
									ptrSaveBuff[1] = 0x50;
									ptrSaveBuff[2] = cmdNum - 200;
									ptrSaveBuff[3] = CAN1_RxMsg -> Data[1];              //�����볤��
									
									memcpy(&ptrSaveBuff[4],&CAN1_RxMsg->Data[2],6);
									cmdManyPackNum = (CAN1_RxMsg->Data[1] - 6) % 7 == 0 ? (CAN1_RxMsg->Data[1] - 6)/7 : (CAN1_RxMsg->Data[1] - 6)/7 + 1;
									Mem_free(CAN1_RxMsg);
									
									dataToSend.pdat = pidManyBag;                        //���� 0x30 ����������Ķ��
									OBD_CAN_SendData(dataToSend.canId,dataToSend.ide,dataToSend.pdat);
									
									for(i = 0;i < cmdManyPackNum;i++)
									{
										CAN1_RxMsg = OSQPend(canRecieveQ,25,&err);       //���ն��
										if(err == OS_ERR_NONE)
										{
											memcpy(&ptrSaveBuff[7*i + 10],&CAN1_RxMsg->Data[1],7);
											Mem_free(CAN1_RxMsg);
										}
										else 
											break;
									} 
									if(i == cmdManyPackNum && varOperation.isDataFlow != 1)
									{
										OSMutexPend(CDMASendMutex,0,&err);
										
										memcpy(&cdmaDataToSend->data[FRAME_HEAD_LEN + varOperation.datOKLeng],ptrSaveBuff,ptrSaveBuff[0]);
										cdmaDataToSend->datLength += ptrSaveBuff[0];
										varOperation.datOKLeng += ptrSaveBuff[0];    //��¼���ɲ�ж�İ�����
										
										OSMutexPost(CDMASendMutex);
									}
									Mem_free(ptrSaveBuff);
								}
							}else   //�������ǵ���
							{
								ptrSaveBuff = Mem_malloc(8);
								ptrSaveBuff[0] = CAN1_RxMsg -> Data[0] + 4;
								ptrSaveBuff[1] = 0x50;
								ptrSaveBuff[2] = cmdNum - 200;
								ptrSaveBuff[3] = CAN1_RxMsg -> Data[0];              //�����볤��
 								memcpy(&ptrSaveBuff[4],&CAN1_RxMsg->Data[1],CAN1_RxMsg->Data[0]);
								
								OSMutexPend(CDMASendMutex,0,&err);
										
								memcpy(&cdmaDataToSend->data[FRAME_HEAD_LEN + varOperation.datOKLeng],ptrSaveBuff,ptrSaveBuff[0]);
								cdmaDataToSend->datLength += ptrSaveBuff[0];
								varOperation.datOKLeng += ptrSaveBuff[0];    //��¼���ɲ�ж�İ�����
								
								OSMutexPost(CDMASendMutex);
								Mem_free(ptrSaveBuff);
							}
							varOperation.pidRun = 1;   //���� PID �� OBD ����ָ��
						}
					}
				}
				Mem_free(CAN1_RxMsg);
			}
		}
		else
		{
			freOBDLed = LEDFAST; 
			LogReport("\r\n04 - PIDcmd don't report:%d;",cmdNum);//���� PID ָ�ECU ���ظ�
		}
		Mem_free(can1_Txbuff);      //�ͷ��ڴ��
	}
}

extern uint8_t configData[6000];

void TestServer(void)               //�÷������·��� ID��Baud �ȵȽ��� CAN ����
{
	uint8_t   err,temp;
	CanRxMsg* CAN1_RxMsg;
	CAN_InitTypeDef CAN_InitStructure;
	varOperation.pidRun = 0;
	OSSemPend(LoginMes,0,&err);     //
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
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_CAN1, ENABLE);//ʹ��CAN1ʱ��
	
	varOperation.canTest = 2;             //Flash�е�CAN���óɹ�
	
	CAN_DeInit(CAN1);  
	CAN_StructInit(&CAN_InitStructure);  
	CAN1_BaudSet(canDataConfig.canBaud);  //����flash�е�CAN���ý��в���
	CAN1_SetFilter(varOperation.canRxId ,CAN_ID_EXT); 
	CAN_ITConfig(CAN1,CAN_IT_FMP1,ENABLE);//����CAN�˲���
	
	dataToSend.canId = canDataConfig.canTxId;
	dataToSend.ide   = canDataConfig.canIdType;
	dataToSend.pdat  = &configData[9];    //CAN PID �ĵ�һ������
	
	OBD_CAN_SendData(dataToSend.canId,dataToSend.ide,dataToSend.pdat);
	
	CAN1_RxMsg = OSQPend(canRecieveQ,500,&err);
	if(err == OS_ERR_NONE)
	{
		Mem_free(CAN1_RxMsg);
		LogReport("\r\n08-ECUID Right:%d;",canDataConfig.pidVersion);    //ECU�л�Ӧ�������ļ���ȷ
		
		dataToSend.ide   = canDataConfig.canIdType;
		dataToSend.canId = canDataConfig.canTxId;
		
		temp = ReadECUVersion(canDataConfig.pidVerCmd);
		if(temp < 100)                  //��ȡECU�汾�ţ���ȡ���ɹ��򲻽��ж�������
		{
			Get_Q_FromECU();            //��ȡ��������ԭʼֵ���õ���ǰ����ϵ��
		
			CAN_DeInit(CAN1);
			CAN_StructInit(&CAN_InitStructure);
			CAN1_BaudSet(canDataConfig.canBaud);  
			CAN1_SetFilter(varOperation.canRxId ,CAN_ID_EXT);
			CAN_ITConfig(CAN1,CAN_IT_FMP1,ENABLE);
			OSTimeDlyHMSM(0,0,5,0);    //�궨��������Ҫ�ȴ� 20s ��ȡ�����룬�����ȴ��Ļ������������Ǿܾ�Ӧ��
			
			CAN_DeInit(CAN1);  
			CAN_StructInit(&CAN_InitStructure);
			CAN1_BaudSet(canDataConfig.canBaud);   
			CAN1_ClearFilter();                    
			CAN_ITConfig(CAN1,CAN_IT_FMP1,ENABLE);
			varOperation.canTest = 2;  //����CAN����ʼ��ECU�������ݽ���
		}
		else if(temp == 200)           //��ȡ�汾��ʧ�ܣ� �����ļ����� ��
		{
			CAN_DeInit(CAN1);  
			CAN_StructInit(&CAN_InitStructure);
			CAN1_BaudSet(canDataConfig.canBaud);   
			CAN1_ClearFilter();                    
			CAN_ITConfig(CAN1,CAN_IT_FMP1,ENABLE);
			varOperation.canTest = 2; 
			CANTestChannel();
		}
		else     //�汾�Ŷ�ȡ�����ˣ����ǲ��������������Ǿ������ܰ�
		{
			OSTimeDlyHMSM(0,0,20,0);   //��ȡ����������Ҫʱ���
		}
	}
	else
	{
		LogReport("\r\n09-ECUID Error:%d!",canDataConfig.pidVersion);
		CANTestChannel();
	}
    varOperation.pidRun = 1;
} 

extern CANBAUD_Enum canBaudEnum[NUMOfCANBaud];
extern const CmdVersion canIdExt[NUMOfCANID_EXT];
//�������������ļ��������߻�û�������ã�ϵͳ������ʶ��ʶ��ɹ�����Ϣ�ϱ�
void CANTestChannel(void)
{
	uint8_t   err,i,temp;
	CanRxMsg* CAN1_RxMsg;
	uint8_t*  ptrOK;
	CAN_InitTypeDef CAN_InitStructure;
	
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_CAN1, ENABLE);//ʹ�� CAN1 ʱ��
	varOperation.canTest = 0;
	for(i = 0;i<NUMOfCANBaud;i++)       //��� CAN �Ĳ�����
	{
		CAN_DeInit(CAN1);
		CAN_StructInit(&CAN_InitStructure);
		CAN1_BaudSet(canBaudEnum[i]); 
		CAN1_ClearFilter();
		CAN_ITConfig(CAN1,CAN_IT_FMP1,ENABLE);
		
		//ֻҪ�˲������������ݣ�CAN �����ʾ�ȷ����
		CAN1_RxMsg = OSQPend(canRecieveQ,500,&err);
		if(err == OS_ERR_NONE)
		{
			canDataConfig.canBaud = canBaudEnum[i];
			Mem_free(CAN1_RxMsg);
			LogReport("\r\n10-Test Baud:%d;",canDataConfig.canBaud);
			break;
		}
	}   //���������û��ȷ�����ϱ���־���˳�
	if((i >= NUMOfCANBaud) && (err != OS_ERR_NONE))
	{
		LogReport("\r\n11-Baud Test Fail!");
		
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
		varOperation.canTest = 0;
		
		goto idOK;
	}
	dataToSend.ide = CAN_ID_EXT;
	for(i=0;i<NUMOfCANID_EXT;i++)
	{
		varOperation.canRxId  =(canIdExt[i].canID >> 24) & 0x000000FF;
		varOperation.canRxId  =(varOperation.canRxId  << 8)+((canIdExt[i].canID >> 16) & 0x000000FF);
		varOperation.canRxId  =(varOperation.canRxId  << 8)+(canIdExt[i].canID & 0x000000FF);
		varOperation.canRxId  =(varOperation.canRxId  << 8)+((canIdExt[i].canID >> 8) & 0x000000FF);
		
		CAN_DeInit(CAN1);
		CAN_StructInit(&CAN_InitStructure);
		CAN1_BaudSet(canDataConfig.canBaud);
		CAN1_SetFilter(varOperation.canRxId ,CAN_ID_EXT);
		CAN_ITConfig(CAN1,CAN_IT_FMP1,ENABLE);//����CAN�˲���
		
		varOperation.canTest = 0;
		dataToSend.canId = canIdExt[i].canID;
		dataToSend.ide     = CAN_ID_EXT;
		temp = ReadECUVersion((uint8_t *)canIdExt[i].pidVerCmd); 
	    if(temp != 200)                       //�汾�Ŷ�ȡ�ɹ�
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
			OSTimeDlyHMSM(0,0,15,0);
			LoginDataSend(); 
			break;
		}
	}
	if(varOperation.canTest == 0)
	{
		LogReport("\r\n12-CAN Test Fail!");
		varOperation.isStrenOilOK = 0;
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
		return;
	}
idOK:
	varOperation.canTest = 0;      //���ٻ�ȡ PID ��Ϣ
	varOperation.isStrenOilOK = 0; //һ��������ʶ�𣬾Ͳ����ٽ��ж�������
	
	return;
}

void Deal5016(uint8_t cmdNum,uint8_t *p)
{
	uint8_t err; 
	if(cmdNum == 1)
	{
		OSMutexPend(CDMASendMutex,0,&err);
		pPid[50][0] = 9;
		memcpy(&pPid[50][5],p,2);
		OSMutexPost(CDMASendMutex);
	}
	if(cmdNum == 2)
	{
		OSMutexPend(CDMASendMutex,0,&err);
		pPid[50][0] = 9;
		memcpy(&pPid[50][7],p,2);
		OSMutexPost(CDMASendMutex);
	}	
}
extern VARConfig* ptrPIDVars;      //ָ��ڶ�������
void PIDVarGet(uint8_t cmdId,uint8_t ptrData[])
{
	uint8_t  i,j,err;	
	uint64_t saveDate;
	uint8_t  byteNum = 0;
	uint8_t  temp = 0;
	uint16_t temp1 = 0;
	uint8_t* ptr;
	static uint8_t  curFuelTimes = 0;     //�Ĵ�һ����
	static uint32_t allFuelCom   = 0;     //�ۼ�������
	for(i = 0;i < varOperation.pidVarNum;i ++)
	{
		if((ptrPIDVars + i)->pidId != cmdId)
			continue;
		//����ֽ���
		byteNum = ((ptrPIDVars + i)->bitLen +(ptrPIDVars + i)->startBit)%8 ==0?       \
						((ptrPIDVars + i)->bitLen + (ptrPIDVars + i)->startBit) / 8 : \
						((ptrPIDVars + i)->bitLen + (ptrPIDVars + i)->startBit) / 8 + 1;
		if((ptrPIDVars + i)->dataHL == 1)     //�����ǰ
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
					temp >>= (ptrPIDVars + i) -> startBit;
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
					temp = ptrData[ptrPIDVars -> startByte + j];
					saveDate += temp;
				}
				j++;
			}while(j < byteNum);	
		}else if((ptrPIDVars + i)->dataHL == 2)      //С����ǰ
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
				if(carAllRecord.engineSpeed > carAllRecord.engineSpeedMax)//��÷��������ת��
					carAllRecord.engineSpeedMax = carAllRecord.engineSpeed;
				break;
			case 3://��������
				if(carAllRecord.allFuelTemp != 1)
				{
					carAllRecord.allFuel = (uint16_t)(saveDate * ((ptrPIDVars + i)->ceo) + (ptrPIDVars + i)->offset);
					carAllRecord.totalFuel += carAllRecord.allFuel;
				}break;
			case 4://��������
				carAllRecord.primaryFuel = (uint16_t)(saveDate * ((ptrPIDVars + i)->ceo) + (ptrPIDVars + i)->offset);
				carAllRecord.totalFuel += carAllRecord.primaryFuel;
				break;
			case 5://Ԥ������ 1
				carAllRecord.beforeFuel1 = (uint16_t)(saveDate * ((ptrPIDVars + i)->ceo) + (ptrPIDVars + i)->offset);
				carAllRecord.totalFuel += carAllRecord.beforeFuel1;
				allFuelCom             += carAllRecord.beforeFuel1;
				break;
			case 6://Ԥ������ 2
				carAllRecord.beforeFuel2 = (uint16_t)(saveDate * ((ptrPIDVars + i)->ceo) + (ptrPIDVars + i)->offset);
				carAllRecord.totalFuel += carAllRecord.beforeFuel2;
				allFuelCom += carAllRecord.beforeFuel2;
				break;
			case 7://Ԥ������ 3
				carAllRecord.beforeFuel3 = (uint16_t)(saveDate * ((ptrPIDVars + i)->ceo) + (ptrPIDVars + i)->offset);
				carAllRecord.totalFuel += carAllRecord.beforeFuel3;
				allFuelCom += carAllRecord.beforeFuel3;
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
				carAllRecord.curFuel    = (uint16_t)(saveDate * ((ptrPIDVars + i)->ceo) + (ptrPIDVars + i)->offset);
				carAllRecord.totalFuel += carAllRecord.curFuel;
				allFuelCom             += carAllRecord.curFuel;
				curFuelTimes ++;
				if(curFuelTimes >= 4)//����˲ʱ�ͺ�  1s ��
				{
					carAllRecord.instantFuel = allFuelCom * carAllRecord.engineSpeed / 7;
					LogReport("\r\n60-oil:%d,r:%d;",allFuelCom,carAllRecord.engineSpeed);
					ptr = Mem_malloc(6);
					
					memset(ptr,0,6);
					
					temp1 = t_htons(carAllRecord.instantFuel);	 
					memcpy(&ptr[0],&temp1,2);
					if(carAllRecord.carSpeed > 0)    //todo:��Ҫȷ�ϵ�λ
						temp1 = t_htons(carAllRecord.carSpeed);
					else 
						temp1 = t_htons(gpsMC.speed);
					memcpy(&ptr[2],&temp1,2);        //��ǰ����
					
					temp1 = t_htons(carAllRecord.engineSpeed);
					memcpy(&ptr[4],&temp1,2);        //��ǰת��
					
					if(varOperation.isDataFlow != 1)
					{
						OSMutexPend(CDMASendMutex,0,&err);
						
						memcpy(&pPid[50][pPid[50][0]],ptr,6);
						pPid[50][0] += 6;
						cdmaDataToSend->datLength += 6;
						
						OSMutexPost(CDMASendMutex);
					}
					
					
					Mem_free(ptr);
					
					carAllRecord.totalFuel += carAllRecord.instantFuel;
					temp         = 0;
					allFuelCom   = 0;
					curFuelTimes = 0;
				}
				break;
			default:break;
		}
	}
}











