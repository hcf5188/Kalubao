#include "apptask.h"
#include "obd.h"

extern CANInformation CANSearchLib[NUMOFCAN];
CanTxMsg CAN1_TxMsg;
CAN1DataToSend  dataToSend;

void CANTestChannel(void )
{
	int i;

	dataToSend.testIsOK = 0;
	
	for(i=0;i<NUMOFCAN;i++)
	{
		CAN1_BaudSet(CANSearchLib[i].canBaud);
		CAN1_ClearFilter();
		
		CAN1_TxMsg.IDE     = CANSearchLib[i].canIde;
		CAN1_TxMsg.StdId   = CANSearchLib[i].canId.dword;
		CAN1_TxMsg.ExtId   = CANSearchLib[i].canId.dword;
		CAN1_TxMsg.DLC     = 0x08;
		
		CAN1_TxMsg.Data[0] = 0x02;
		CAN1_TxMsg.Data[1] = 0x01;
		CAN1_TxMsg.Data[2] = 0x0D;
		CAN1_TxMsg.Data[3] = 0x00;
		CAN1_TxMsg.Data[4] = 0x00;
		CAN1_TxMsg.Data[5] = 0x00;
		CAN1_TxMsg.Data[6] = 0x00;
		CAN1_TxMsg.Data[7] = 0x00;
		CAN_Transmit(CAN1,&CAN1_TxMsg);

		dataToSend.canId = CANSearchLib[i].canId.dword;
		dataToSend.ide   = CANSearchLib[i].canIde;
		OSTimeDlyHMSM(0,0,0,100);   //仿真板的回复时间比较长，有的达到60ms以上，故需要延时时间长一点。
		if(dataToSend.testIsOK == 1)//CAN数据测试通过
			break;
		else if(i == (NUMOFCAN - 1))
			dataToSend.testIsOK = 2;
	}
}

uint8_t pidManyBag[8] = {0x30,0x00,0x00,0x00,0x00,0x00,0x00,0x00};



#define CANRECBUF_SIZE  20       //CAN接收消息队列保存消息的最大量
OS_EVENT *canRecieveQ;           //指向CAN总线消息队列的指针
void *canRecBuf[CANRECBUF_SIZE];

#define CANSENDBUF_SIZE  80   //CAN接收消息队列保存消息的最大量
OS_EVENT *canSendQ;           //指向CAN总线发送消息队列的指针
void *canSendBuf[CANSENDBUF_SIZE];

extern uint16_t freOBDLed;
extern OS_EVENT * CDMASendMutex;//建立互斥型信号量，用来独占处理 发向服务器的消息
extern _CDMADataToSend* cdmaDataToSend;//CDMA发送的数据中（OBD、GPS），是通过它来作为载体
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
	
	
	canSendQ    = OSQCreate(&canSendBuf[0],CANSENDBUF_SIZE);//卡路宝向ECU发送指令的消息队列
	canRecieveQ = OSQCreate(&canRecBuf[0],CANRECBUF_SIZE);  //卡路宝从ECU接收指令的循环队列
	
//	CANTestChannel();//完成CAN的ID、波特率设定，并且读取出ECU的版本号
	
//	dataToSend.canId = 0x7E0;
//	dataToSend.ide   = CAN_ID_STD;
//	dataToSend.testIsOK = 1; 
	
	dataToSend.canId = 0x18DA10FB;//潍柴：0x18DA10FB   渣土车：0x18DA00FA
	dataToSend.ide   = CAN_ID_EXT;
	dataToSend.testIsOK = 1; 

	
	while(1)
	{	
		can1_Txbuff = OSQPend(canSendQ,0,&err);//收到PID指令，进行发送
		
		cmdNum = can1_Txbuff[0];               //记录PID指令序号
		cmdLen = can1_Txbuff[1];               //记录PID指令长度
		
		dataToSend.pdat = &can1_Txbuff[1];     
		OBD_CAN_SendData(dataToSend);          //发送PID指令
		Mem_free(can1_Txbuff);                 //释放内存块

		CAN1_RxMsg = OSQPend(canRecieveQ,25,&err); //接收到OBD回复
		if(err == OS_ERR_NONE)
		{
			freOBDLed = 300;                    //OBD 初始化成功
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
	}
}









