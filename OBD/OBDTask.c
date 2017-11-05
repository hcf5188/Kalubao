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
uint8_t PID01[8]   = {0x02,0x1A,0x94,0x00,0x00,0x00,0x00,0x00};
uint8_t PID01_1[8] = {0x30,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
uint8_t PID02[8]   = {0x03,0x21,0x00,0x86,0x00,0x00,0x00,0x00};
uint8_t PID03[8]   = {0x03,0x21,0x00,0x85,0x00,0x00,0x00,0x00};
uint8_t PID04[8]   = {0x03,0x21,0x00,0x61,0x00,0x00,0x00,0x00};


#define CANRECBUF_SIZE  10       //CAN接收消息队列保存消息的最大量
OS_EVENT *canRecieveQ;           //指向CAN总线消息队列的指针
void *canRecBuf[CANRECBUF_SIZE];

uint8_t startBuffSave[50];
extern uint16_t freOBDLed;
void OBDTask(void *pdata)
{
	INT8U     err;
	CanRxMsg* CAN1_RxMsg;
	static uint8_t index = 0;
	CAN1Config();
	canRecieveQ = OSQCreate(&canRecBuf[0],CANRECBUF_SIZE);
	
	OSTimeDlyHMSM(0,0,6,0);//CDMA还没启动，此处需要延时
	
//	CANTestChannel();
	
	dataToSend.canId = 0x18DA10FB;
	dataToSend.ide   = CAN_ID_EXT;
	dataToSend.testIsOK = 1; 
	dataToSend.pdat = PID01;
	
	while(1)
	{
		CAN1_RxMsg = OSQPend(canRecieveQ,200,&err);
		if(err == OS_ERR_NONE)
		{
			freOBDLed = 200;
			if(dataToSend.testIsOK == 1)
			{   
				if(CAN1_RxMsg->Data[0] == 0x10)
				{
					dataToSend.pdat = PID01_1;
					OBD_CAN_SendData(dataToSend);
					memcpy(startBuffSave,CAN1_RxMsg->Data,8);
				}else if(CAN1_RxMsg->Data[0] > 0x20)
				{
					memcpy(&startBuffSave[(CAN1_RxMsg->Data[0] - 0x20) * 8],CAN1_RxMsg->Data,8);
				}
			}
		}
		else if(err == OS_ERR_TIMEOUT)
		{
			if(dataToSend.testIsOK == 1)
			{
				switch(index)
				{
					case 0:dataToSend.pdat = PID01;break;
					case 1:dataToSend.pdat = PID02;break;
					case 2:dataToSend.pdat = PID03;break;
					case 3:dataToSend.pdat = PID04;break;
				}
				OBD_CAN_SendData(dataToSend);
				memset(startBuffSave,0,45);
				index++;
				if(index >= 4)
					index = 0;
			}
		}
		
		Mem_free(CAN1_RxMsg);//处理完以后，要释放内存块
	}
}









