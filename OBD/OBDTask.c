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
uint8_t a[8] = {0x02,0x01,0x0D,0x00,0x00,0x00,0x00,0x00};


void OBDTask(void *pdata)
{
	OSTimeDlyHMSM(0,0,2,0);
	CANTestChannel();
	dataToSend.pdat = a;
	
	while(1)
	{
		GPIO_ResetBits(GPIO_LED,LED_GPIO_OBD);
		OSTimeDlyHMSM(0,0,0,150);
		GPIO_SetBits(GPIO_LED,LED_GPIO_OBD);
		OSTimeDlyHMSM(0,0,0,150);
		if(dataToSend.testIsOK == 1)
			OBD_CAN_SendData(dataToSend);
	}
}









