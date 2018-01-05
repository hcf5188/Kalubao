#include "obd.h"
#include "bsp.h"
#include "apptask.h"

extern OS_EVENT * CANSendMutex;          //CAN���� �������ź���

//CAN1 ���ݷ��ͺ���
static CanTxMsg TxMessage;
void OBD_CAN_SendData(u32 canId,u32 ide,u8* pdat)
{
	u8 err;
	OSMutexPend(CANSendMutex,0,&err);     //������ȼ�����ռCAN
			
	TxMessage.StdId = canId;
	TxMessage.ExtId = canId;  //��չ֡
	
	TxMessage.IDE = ide;
	TxMessage.RTR = CAN_RTR_DATA;
	TxMessage.DLC = 8;//���ݳ���
	
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

//CAN1 �жϽ��մ�����
extern CAN1DataToSend  dataToSend;
extern OS_EVENT *canJ1939Q;       //����J1939��Ϣ����Ϣ����

void CAN1_RX1_IRQHandler(void )
{
	uint8_t err;
	CanRxMsg* CAN1_RxMsg = Mem_malloc(sizeof(CanRxMsg));
	
	OSIntEnter();				//ϵͳ�����жϷ������
	
	CAN_Receive(CAN1, CAN_FIFO1, CAN1_RxMsg);
	if(varOperation.pidTset == 1)
	{
		err = OSQPost(canRecieveQ,CAN1_RxMsg);
		if(err != OS_ERR_NONE)
			Mem_free(CAN1_RxMsg);
	}else
	{
		if(varOperation.canTest == 2) //�յ�CAN�ظ������� - �����ʲ��Գɹ�
		{
			if(CAN1_RxMsg->ExtId != varOperation.canRxId)//�Ӹ�J1939������
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
		else if(varOperation.canTest == 1)//��ʶ�� CAN ID �׶�
		{
			varOperation.canTest = 2;
			err = OSQPost(canRecieveQ,CAN1_RxMsg);
			if(err != OS_ERR_NONE)
				Mem_free(CAN1_RxMsg);
		}
		else if(varOperation.canTest == 0)
		{
			varOperation.canTest = 1; //��ʶ�����ʽ׶�
			err = OSQPost(canRecieveQ,CAN1_RxMsg);
			if(err != OS_ERR_NONE)
				Mem_free(CAN1_RxMsg);
		}
	}
	

	OSIntExit();  //�жϷ��������ϵͳ�����������
}

