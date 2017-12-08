#include "obd.h"
#include "bsp.h"
#include "apptask.h"

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
extern OS_EVENT *canJ1939Q;       //����J1939��Ϣ����Ϣ����

void CAN1_RX1_IRQHandler(void )
{
	uint8_t err;
	CanRxMsg* CAN1_RxMsg = Mem_malloc(sizeof(CanRxMsg));
	
	OSIntEnter();				//ϵͳ�����жϷ������
	
	CAN_Receive(CAN1, CAN_FIFO1, CAN1_RxMsg);
	
	if(varOperation.canTest == 2) //�յ�CAN�ظ������� - �����ʲ��Գɹ�
	{
		if(CAN1_RxMsg->ExtId != varOperation.canRxId)//�Ӹ�J1939������
		{
			err = OSQPost(canJ1939Q,CAN1_RxMsg);
			if(err != OS_ERR_NONE)
			{
				Mem_free(CAN1_RxMsg);
			}
		}
		err = OSQPost(canRecieveQ,CAN1_RxMsg);
		if(err != OS_ERR_NONE)
		{
			Mem_free(CAN1_RxMsg);
		}
	}	
	else if(varOperation.canTest == 1)//��ʶ��׶�
	{
		varOperation.canTest = 2;
		err = OSQPost(canRecieveQ,CAN1_RxMsg);
		if(err != OS_ERR_NONE)
		{
			Mem_free(CAN1_RxMsg);
		}
	}
	else if(varOperation.canTest == 0)
	{
		varOperation.canTest = 1; //���Խ׶�
		err = OSQPost(canRecieveQ,CAN1_RxMsg);
		if(err != OS_ERR_NONE)
		{
			Mem_free(CAN1_RxMsg);
		}
	}
		
	
	
	
	OSIntExit();  //�жϷ��������ϵͳ�����������
}

