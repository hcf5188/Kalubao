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
	uint8_t   cmdLen = 0;        //封包的时候要减去指令的长度
	uint8_t   cmdNum = 0;        //指令序号
	uint8_t   cmdManyPackNum = 0;//要接受的多包的数量
	CanRxMsg* CAN1_RxMsg;        //指向接收到的OBD信息
	uint8_t * can1_Txbuff;       //指向要发送的OBD信息
	uint8_t * ptrSaveBuff;       //
	uint8_t   pidErrCount = 0;
	               //CAN 配置
	OSTimeDlyHMSM(0,0,10,4);     
//	TestServer();                //测试服务器
	while(1)
	{	
		StrengthFuel();
		if((varOperation.canTest == 0)||(varOperation.pidTset == 1))      //配置文件不成功，或者在测试PID指令，则停止CAN
		{
			OSTimeDlyHMSM(0,0,1,0);	
			continue;
		}	 
		can1_Txbuff = OSQPend(canSendQ,1000,&err);                        //收到 PID 指令，进行发送
		if(err != OS_ERR_NONE)
			continue;
		cmdNum = can1_Txbuff[0];  //记录PID指令序号
		cmdLen = can1_Txbuff[1];  //记录PID指令长度
		
		dataToSend.pdat = &can1_Txbuff[1];   
		OBD_CAN_SendData(dataToSend.canId,dataToSend.ide,dataToSend.pdat);//发送PID指令
		
		CAN1_RxMsg = OSQPend(canRecieveQ,200,&err); // 接收到 OBD 回复
		if(err == OS_ERR_NONE)
		{
			pidErrCount = 0;
			varOperation.flagCAN = 1;
			varOperation.flagECUID = 1;             // ECU 通信成功过  判断ECU运行时断电
			freOBDLed = LEDSLOW;                    // OBD 初始化成功
			if(CAN1_RxMsg->Data[0] == 0x10)         //多包处理
			{
				ptrSaveBuff = Mem_malloc(CAN1_RxMsg->Data[1] + 10);       //申请的内存块足够长
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
					ptrSaveBuff[3] = CAN1_RxMsg -> Data[1];              //故障码长度
					
					memcpy(&ptrSaveBuff[4],&CAN1_RxMsg->Data[2],6);
					cmdManyPackNum = (CAN1_RxMsg->Data[1] - 6) % 7 == 0 ? (CAN1_RxMsg->Data[1] - 6)/7 : (CAN1_RxMsg->Data[1] - 6)/7 + 1;
					Mem_free(CAN1_RxMsg);
					
					dataToSend.pdat = pidManyBag;                    //发送 0x30 请求接下来的多包
					OBD_CAN_SendData(dataToSend.canId,dataToSend.ide,dataToSend.pdat);
					
					for(i = 0;i < cmdManyPackNum;i++)
					{
						CAN1_RxMsg = OSQPend(canRecieveQ,25,&err);   //接收多包
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
						varOperation.datOKLeng += ptrSaveBuff[0];    //记录不可拆卸的包长度
						
						OSMutexPost(CDMASendMutex);
					}
					Mem_free(ptrSaveBuff);
				}
			}
			else  //单包处理
			{
				if(cmdNum < 100)
				{
					OSMutexPend(CDMASendMutex,0,&err);
					pPid[cmdNum - 1][1] = CAN1_RxMsg->Data[0];//得到指令的长度 + 1
					memcpy(&pPid[cmdNum - 1][2],&CAN1_RxMsg->Data[2],CAN1_RxMsg->Data[0]-1);
					if(cmdNum == varOperation.pidNum)
						varOperation.pidSendFlag = 3;//数据采集完成
					OSMutexPost(CDMASendMutex);
				}
				else if(CAN1_RxMsg->Data[0] > cmdLen)//正常的PID指令
				{
					OSMutexPend(CDMASendMutex,0,&err);
					// todo: 限制数据长度，不能越界
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
						ptrSaveBuff[3] = CAN1_RxMsg -> Data[0];              //故障码长度
						memcpy(&ptrSaveBuff[4],&CAN1_RxMsg->Data[1],CAN1_RxMsg->Data[0]);
						
						OSMutexPend(CDMASendMutex,0,&err);
								
						memcpy(&cdmaDataToSend->data[FRAME_HEAD_LEN + varOperation.datOKLeng],ptrSaveBuff,ptrSaveBuff[0]);
						cdmaDataToSend->datLength += ptrSaveBuff[0];
						varOperation.datOKLeng += ptrSaveBuff[0];    //记录不可拆卸的包长度
						
						OSMutexPost(CDMASendMutex);
						Mem_free(ptrSaveBuff);
					}
				}
				else if((CAN1_RxMsg->Data[0]==0x03)&&(CAN1_RxMsg->Data[1]==0x7F))
				{
					if(cmdNum == 201)                       //重汽清除故障码
					{	
						Mem_free(CAN1_RxMsg);
						CAN1_RxMsg = OSQPend(canRecieveQ,50,&err); //延时收到清故障码结果
						OSMutexPend(CDMASendMutex,0,&err);
						if(err == OS_ERR_NONE)
						{
							if(CAN1_RxMsg->Data[1] == 0x54)	//清故障码成功
							{
								pPid[100][0] = 4;
								pPid[100][3] = 0;
								SendFaultCmd();
							}
							else                            //清故障码失败
							{
								pPid[100][0] = 4;
								pPid[100][3] = 1;
							}										
						}else                               //清故障码失败
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
					else if((cmdNum == 234||cmdNum == 235) && strengthFuelFlash.modeOrder == 2)//重汽故障码读取  03 7F 78
					{
						Mem_free(CAN1_RxMsg);
						varOperation.pidRun = 0;//停止发送PID指令
						CAN1_RxMsg = OSQPend(canRecieveQ,2000,&err); // 接收到OBD回复
						if(err == OS_ERR_NONE)
						{	
							if(CAN1_RxMsg->Data[0] == 0x10)          // 多包处理
							{
								ptrSaveBuff = Mem_malloc(CAN1_RxMsg->Data[1] + 10);      //申请的内存块足够长
								if(ptrSaveBuff != NULL)
								{
									ptrSaveBuff[0] = CAN1_RxMsg -> Data[1] + 4;
									ptrSaveBuff[1] = 0x50;
									ptrSaveBuff[2] = cmdNum - 200;
									ptrSaveBuff[3] = CAN1_RxMsg -> Data[1];              //故障码长度
									
									memcpy(&ptrSaveBuff[4],&CAN1_RxMsg->Data[2],6);
									cmdManyPackNum = (CAN1_RxMsg->Data[1] - 6) % 7 == 0 ? (CAN1_RxMsg->Data[1] - 6)/7 : (CAN1_RxMsg->Data[1] - 6)/7 + 1;
									Mem_free(CAN1_RxMsg);
									
									dataToSend.pdat = pidManyBag;                        //发送 0x30 请求接下来的多包
									OBD_CAN_SendData(dataToSend.canId,dataToSend.ide,dataToSend.pdat);
									
									for(i = 0;i < cmdManyPackNum;i++)
									{
										CAN1_RxMsg = OSQPend(canRecieveQ,25,&err);       //接收多包
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
										varOperation.datOKLeng += ptrSaveBuff[0];    //记录不可拆卸的包长度
										
										OSMutexPost(CDMASendMutex);
									}
									Mem_free(ptrSaveBuff);
								}
							}else   //故障码是单包
							{
								ptrSaveBuff = Mem_malloc(8);
								ptrSaveBuff[0] = CAN1_RxMsg -> Data[0] + 4;
								ptrSaveBuff[1] = 0x50;
								ptrSaveBuff[2] = cmdNum - 200;
								ptrSaveBuff[3] = CAN1_RxMsg -> Data[0];              //故障码长度
 								memcpy(&ptrSaveBuff[4],&CAN1_RxMsg->Data[1],CAN1_RxMsg->Data[0]);
								
								OSMutexPend(CDMASendMutex,0,&err);
										
								memcpy(&cdmaDataToSend->data[FRAME_HEAD_LEN + varOperation.datOKLeng],ptrSaveBuff,ptrSaveBuff[0]);
								cdmaDataToSend->datLength += ptrSaveBuff[0];
								varOperation.datOKLeng += ptrSaveBuff[0];    //记录不可拆卸的包长度
								
								OSMutexPost(CDMASendMutex);
								Mem_free(ptrSaveBuff);
							}
							varOperation.pidRun = 1;   //启动 PID 向 OBD 发送指令
						}
					}
				}
				Mem_free(CAN1_RxMsg);
			}
		}
		else
		{
			LogReport("\r\n04 - PIDcmd don't report:%d;",cmdNum);//发送 PID 指令，ECU 不回复
			pidErrCount ++;
			if(pidErrCount>5)//连续5条数据发不出去
			{
				varOperation.flagCAN = 0;//CAN 数据流不通
				freOBDLed = LEDFAST; 
				pidErrCount = 6;
				varOperation.pidRun = 0;				
			}
		}
		Mem_free(can1_Txbuff);      //释放内存块
	}
}





