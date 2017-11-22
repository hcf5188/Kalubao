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
		OSTimeDlyHMSM(0,0,0,100);   //�����Ļظ�ʱ��Ƚϳ����еĴﵽ60ms���ϣ�����Ҫ��ʱʱ�䳤һ�㡣
		if(dataToSend.testIsOK == 1)//CAN���ݲ���ͨ��
			break;
		else if(i == (NUMOFCAN - 1))
			dataToSend.testIsOK = 2;
	}
}

uint8_t pidManyBag[8] = {0x30,0x00,0x00,0x00,0x00,0x00,0x00,0x00};



#define CANRECBUF_SIZE  20       //CAN������Ϣ���б�����Ϣ�������
OS_EVENT *canRecieveQ;           //ָ��CAN������Ϣ���е�ָ��
void *canRecBuf[CANRECBUF_SIZE];

#define CANSENDBUF_SIZE  80   //CAN������Ϣ���б�����Ϣ�������
OS_EVENT *canSendQ;           //ָ��CAN���߷�����Ϣ���е�ָ��
void *canSendBuf[CANSENDBUF_SIZE];

extern uint16_t freOBDLed;
extern OS_EVENT * CDMASendMutex;//�����������ź�����������ռ���� �������������Ϣ
extern _CDMADataToSend* cdmaDataToSend;//CDMA���͵������У�OBD��GPS������ͨ��������Ϊ����
void OBDTask(void *pdata)
{
	INT8U     err;
	uint8_t   i      = 0;
	uint8_t   cmdLen = 0;        //�����ʱ��Ҫ��ȥָ��ĳ���
	uint8_t   cmdNum = 0;        //ָ�����
	uint8_t   cmdManyPackNum = 0;//Ҫ���ܵĶ��������
	
	CanRxMsg* CAN1_RxMsg;        //ָ����յ���OBD��Ϣ
	uint8_t * can1_Txbuff;       //ָ��Ҫ���͵�OBD��Ϣ
	uint8_t * ptrSaveBuff;
	
	
	canSendQ    = OSQCreate(&canSendBuf[0],CANSENDBUF_SIZE);//��·����ECU����ָ�����Ϣ����
	canRecieveQ = OSQCreate(&canRecBuf[0],CANRECBUF_SIZE);  //��·����ECU����ָ���ѭ������
	
//	CANTestChannel();//���CAN��ID���������趨�����Ҷ�ȡ��ECU�İ汾��
	
//	dataToSend.canId = 0x7E0;
//	dataToSend.ide   = CAN_ID_STD;
//	dataToSend.testIsOK = 1; 
	
	dataToSend.canId = 0x18DA10FB;//Ϋ��0x18DA10FB   ��������0x18DA00FA
	dataToSend.ide   = CAN_ID_EXT;
	dataToSend.testIsOK = 1; 

	
	while(1)
	{	
		can1_Txbuff = OSQPend(canSendQ,0,&err);//�յ�PIDָ����з���
		
		cmdNum = can1_Txbuff[0];               //��¼PIDָ�����
		cmdLen = can1_Txbuff[1];               //��¼PIDָ���
		
		dataToSend.pdat = &can1_Txbuff[1];     
		OBD_CAN_SendData(dataToSend);          //����PIDָ��
		Mem_free(can1_Txbuff);                 //�ͷ��ڴ��

		CAN1_RxMsg = OSQPend(canRecieveQ,25,&err); //���յ�OBD�ظ�
		if(err == OS_ERR_NONE)
		{
			freOBDLed = 300;                    //OBD ��ʼ���ɹ�
			if(CAN1_RxMsg->Data[0] == 0x10)     //�������
			{
				ptrSaveBuff = Mem_malloc(CAN1_RxMsg->Data[1]+10);//������ڴ���㹻��
				if(ptrSaveBuff != NULL)
				{
					ptrSaveBuff[0] = CAN1_RxMsg->Data[1] - cmdLen + 3;
					ptrSaveBuff[1] = 0x60;
					ptrSaveBuff[2] = cmdNum;
					memcpy(&ptrSaveBuff[3],&CAN1_RxMsg->Data[cmdLen + 2],(6 - cmdLen));
					cmdManyPackNum = (CAN1_RxMsg->Data[1] - 6)%7 == 0? (CAN1_RxMsg->Data[1] - 6)/7 : (CAN1_RxMsg->Data[1] - 6)/7 + 1;
					Mem_free(CAN1_RxMsg);
					
					dataToSend.pdat = pidManyBag;//����0x30 ����������Ķ��
					OBD_CAN_SendData(dataToSend);
					
					for(i=0;i<cmdManyPackNum;i++)
					{
						CAN1_RxMsg = OSQPend(canRecieveQ,25,&err);//���ն��
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
			else  //��������
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









