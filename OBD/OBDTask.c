#include "apptask.h"
#include "bsp.h"
#include "obd.h"

extern uint16_t freOBDLed;
extern uint8_t * pPid[102];
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
	uint8_t   pidErrCount = 0;
	               //CAN ����
	OSTimeDlyHMSM(0,0,10,4);     
//	TestServer();                //���Է�����
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
		
		CAN1_RxMsg = OSQPend(canRecieveQ,200,&err); // ���յ� OBD �ظ�
		if(err == OS_ERR_NONE)
		{
			pidErrCount = 0;
			varOperation.flagCAN = 1;
			varOperation.flagECUID = 1;             // ECU ͨ�ųɹ���  �ж�ECU����ʱ�ϵ�
			freOBDLed = LEDSLOW;                    // OBD ��ʼ���ɹ�
			if(CAN1_RxMsg->Data[0] == 0x10)         //�������
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
				if(cmdNum < 100)
				{
					OSMutexPend(CDMASendMutex,0,&err);
					pPid[cmdNum - 1][1] = CAN1_RxMsg->Data[0];//�õ�ָ��ĳ��� + 1
					memcpy(&pPid[cmdNum - 1][2],&CAN1_RxMsg->Data[2],CAN1_RxMsg->Data[0]-1);
					if(cmdNum == varOperation.pidNum)
						varOperation.pidSendFlag = 3;//���ݲɼ����
					OSMutexPost(CDMASendMutex);
				}
				else if(CAN1_RxMsg->Data[0] > cmdLen)//������PIDָ��
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
						CAN1_RxMsg = OSQPend(canRecieveQ,50,&err); //��ʱ�յ����������
						OSMutexPend(CDMASendMutex,0,&err);
						if(err == OS_ERR_NONE)
						{
							if(CAN1_RxMsg->Data[1] == 0x54)	//�������ɹ�
							{
								pPid[100][0] = 4;
								pPid[100][3] = 0;
								SendFaultCmd();
							}
							else                            //�������ʧ��
							{
								pPid[100][0] = 4;
								pPid[100][3] = 1;
							}										
						}else                               //�������ʧ��
						{
							pPid[100][0] = 4;
							pPid[100][3] = 1;
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
			LogReport("\r\n04 - PIDcmd don't report:%d;",cmdNum);//���� PID ָ�ECU ���ظ�
			pidErrCount ++;
			if(pidErrCount>5)//����5�����ݷ�����ȥ
			{
				varOperation.flagCAN = 0;//CAN ��������ͨ
				freOBDLed = LEDFAST; 
				pidErrCount = 6;
				varOperation.pidRun = 0;				
			}
		}
		Mem_free(can1_Txbuff);      //�ͷ��ڴ��
	}
}





