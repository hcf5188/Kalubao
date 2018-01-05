#include "obd.h"
#include "bsp.h"
#include "apptask.h"

extern OS_EVENT * CANSendMutex;          //CAN发送 互斥型信号量

//CAN1 数据发送函数
static CanTxMsg TxMessage;
void OBD_CAN_SendData(u32 canId,u32 ide,u8* pdat)
{
	u8 err;
	OSMutexPend(CANSendMutex,0,&err);     //提高优先级，独占CAN
			
	TxMessage.StdId = canId;
	TxMessage.ExtId = canId;  //扩展帧
	
	TxMessage.IDE = ide;
	TxMessage.RTR = CAN_RTR_DATA;
	TxMessage.DLC = 8;//数据长度
	
	TxMessage.Data[0] = pdat[0];
	TxMessage.Data[1] = pdat[1];
	TxMessage.Data[2] = pdat[2];
	TxMessage.Data[3] = pdat[3];
	TxMessage.Data[4] = pdat[4];
	TxMessage.Data[5] = pdat[5];
	TxMessage.Data[6] = pdat[6];
	TxMessage.Data[7] = pdat[7];
	
	CAN_Transmit(CAN1,&TxMessage);
	
	OSMutexPost(CANSendMutex);
}

//CAN1 中断接收处理函数
extern CAN1DataToSend  dataToSend;
extern OS_EVENT *canJ1939Q;       //接收J1939消息的消息队列

void CAN1_RX1_IRQHandler(void )
{
	uint8_t err;
	CanRxMsg* CAN1_RxMsg = Mem_malloc(sizeof(CanRxMsg));
	
	OSIntEnter();				//系统进入中断服务程序
	
	CAN_Receive(CAN1, CAN_FIFO1, CAN1_RxMsg);
	if(varOperation.pidTset == 1)
	{
		err = OSQPost(canRecieveQ,CAN1_RxMsg);
		if(err != OS_ERR_NONE)
			Mem_free(CAN1_RxMsg);
	}else
	{
		if(varOperation.canTest == 2) //收到CAN回复的数据 - 波特率测试成功
		{
			if(CAN1_RxMsg->ExtId != varOperation.canRxId)//扔给J1939任务处理
			{
				err = OSQPost(canJ1939Q,CAN1_RxMsg);
				if(err != OS_ERR_NONE)
					Mem_free(CAN1_RxMsg);
			}else 
			{
				err = OSQPost(canRecieveQ,CAN1_RxMsg);
				if(err != OS_ERR_NONE)
					Mem_free(CAN1_RxMsg);
			}
		}	
		else if(varOperation.canTest == 1)//自识别 CAN ID 阶段
		{
			varOperation.canTest = 2;
			err = OSQPost(canRecieveQ,CAN1_RxMsg);
			if(err != OS_ERR_NONE)
				Mem_free(CAN1_RxMsg);
		}
		else if(varOperation.canTest == 0)
		{
			varOperation.canTest = 1; //自识别波特率阶段
			err = OSQPost(canRecieveQ,CAN1_RxMsg);
			if(err != OS_ERR_NONE)
				Mem_free(CAN1_RxMsg);
		}
	}
	

	OSIntExit();  //中断服务结束，系统进行任务调度
}

