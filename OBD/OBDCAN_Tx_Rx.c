#include "obd.h"

//CAN1 ���ݷ��ͺ���
static CanTxMsg TxMessage;
void OBD_CAN_SendData(CAN1DataToSend sendData)
{
	if(sendData.ide == CAN_ID_STD)         //��׼֡
		TxMessage.StdId = sendData.canId;
	else 
		TxMessage.ExtId = sendData.canId;  //��չ֡
	TxMessage.IDE = sendData.ide;
	TxMessage.RTR = CAN_RTR_DATA;
	TxMessage.DLC = 8;//���ݳ���
	
	TxMessage.Data[0] = sendData.pdat[0];
	TxMessage.Data[1] = sendData.pdat[1];
	TxMessage.Data[2] = sendData.pdat[2];
	TxMessage.Data[3] = sendData.pdat[3];
	TxMessage.Data[4] = sendData.pdat[4];
	TxMessage.Data[5] = sendData.pdat[5];
	TxMessage.Data[6] = sendData.pdat[6];
	TxMessage.Data[7] = sendData.pdat[7];
	
	CAN_Transmit(CAN1,&TxMessage);
}

//CAN1 �жϽ��մ�����
extern CAN1DataToSend  dataToSend;
CanRxMsg CAN1_RxMsg;
void CAN1_RX1_IRQHandler(void )
{
	OSIntEnter();//ϵͳ�����жϷ������
	
	CAN_Receive(CAN1, CAN_FIFO1, &CAN1_RxMsg);
	if(dataToSend.testIsOK == 0) //�յ�CAN�ظ������� - ���Գɹ�
	{
		dataToSend.testIsOK = 1;    
		if(dataToSend.ide == CAN_ID_STD)
			CAN1_SetFilter(CAN1_RxMsg.StdId,CAN_ID_STD);//��׼֡
		else
			CAN1_SetFilter(CAN1_RxMsg.ExtId,CAN_ID_EXT);//��չ֡
	}
	
	OSIntExit();  //�жϷ��������ϵͳ�����������
}

