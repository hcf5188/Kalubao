#include "apptask.h"
#include "obd.h"

CanTxMsg CAN1_TxMsg;
CAN1DataToSend  dataToSend;


uint8_t pidManyBag[8] = {0x30,0x00,0x00,0x00,0x00,0x00,0x00,0x00};

#define CANRECBUF_SIZE  20       //CAN������Ϣ���б�����Ϣ�������
OS_EVENT *canRecieveQ;           //ָ��CAN������Ϣ���е�ָ��
void *canRecBuf[CANRECBUF_SIZE];

#define CANSENDBUF_SIZE  80      //CAN������Ϣ���б�����Ϣ�������
OS_EVENT *canSendQ;              //ָ��CAN���߷�����Ϣ���е�ָ��
void *canSendBuf[CANSENDBUF_SIZE];

extern uint16_t freOBDLed;
extern OS_EVENT * CDMASendMutex;       //�����������ź�����������ռ���� �������������Ϣ
extern _CDMADataToSend* cdmaDataToSend;//CDMA���͵������У�OBD��GPS������ͨ��������Ϊ����
extern SYS_OperationVar  varOperation;
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
	
	CAN1Config();                //CAN ����
	
	canSendQ    = OSQCreate(&canSendBuf[0],CANSENDBUF_SIZE);//��·����ECU����ָ�����Ϣ����
	canRecieveQ = OSQCreate(&canRecBuf[0],CANRECBUF_SIZE);  //��·����ECU����ָ���ѭ������
	
//	CANTestChannel();//���CAN��ID���������趨�����Ҷ�ȡ��ECU�İ汾��
	
//	dataToSend.canId = 0x7E0;
//	dataToSend.ide   = CAN_ID_STD;
//	dataToSend.testIsOK = 1; 
	
//	dataToSend.canId = varOperation.canTxId;//Ϋ��0x18DA10FB   ��������0x18DA00FA
//	dataToSend.ide   = varOperation.canIdType;
	
	while(1)
	{	
		can1_Txbuff = OSQPend(canSendQ,0,&err);//�յ�PIDָ����з���
		
		cmdNum = can1_Txbuff[0];               //��¼PIDָ�����
		cmdLen = can1_Txbuff[1];               //��¼PIDָ���
		
		dataToSend.pdat = &can1_Txbuff[1];     
		OBD_CAN_SendData(dataToSend);          //����PIDָ��
		

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
		else
		{
			OSTimeDlyHMSM(0,0,0,4);
		}
		
		Mem_free(can1_Txbuff);                 //�ͷ��ڴ��
	}
}

extern CANBAUD_Enum canBaudEnum[NUMOfCANBaud];
extern uint32_t canIdExt[NUMOfCANID_EXT];

void CANTestChannel(void )
{
	uint8_t err,i;
	CanRxMsg* CAN1_RxMsg;
	uint32_t canRxID;

	varOperation.canTest = 0;
	for(i = 0;i<NUMOfCANBaud;i++)       //���CAN�Ĳ�����
	{
		CAN1_BaudSet(canBaudEnum[i]); 
		CAN1_RxMsg = OSQPend(canRecieveQ,500,&err);
		if(err == OS_ERR_NONE)
		{
			Mem_free(CAN1_RxMsg);
			break;
		}
	}dataToSend.ide = CAN_ID_EXT;
	for(i=0;i<NUMOfCANID_EXT;i++)
	{
		canRxID =(canIdExt[i]>>24) &0x000000FF;
		canRxID =(canRxID << 8)+((canIdExt[i]>>16) &0x000000FF);
		canRxID =(canRxID << 8)+(canIdExt[i]&0x000000FF);
		canRxID =(canRxID << 8)+((canIdExt[i]>>8) &0x000000FF);
		CAN1_SetFilter(canRxID,CAN_ID_EXT);
		dataToSend.canId = canIdExt[i];
		
		CAN1_TxMsg.IDE     = CAN_ID_EXT;
		CAN1_TxMsg.StdId   = dataToSend.canId;
		CAN1_TxMsg.ExtId   = dataToSend.canId;
		CAN1_TxMsg.DLC     = 0x08;
		
		CAN1_TxMsg.Data[0] = 0x02;
		CAN1_TxMsg.Data[1] = 0x1A;
		CAN1_TxMsg.Data[2] = 0x94;
		CAN1_TxMsg.Data[3] = 0x00;
		CAN1_TxMsg.Data[4] = 0x00;
		CAN1_TxMsg.Data[5] = 0x00;
		CAN1_TxMsg.Data[6] = 0x00;
		CAN1_TxMsg.Data[7] = 0x00;
		
		CAN_Transmit(CAN1,&CAN1_TxMsg);
		
	}
	for(i=0;i<NUMOfCANID_EXT;i++)
	{
	
		
	}
}
extern VARConfig    *ptrPIDVars;      //ָ��ڶ�������
float  carSpeed;        //����
float  rotateSpeed;     //ת��
float  allFuel;         //��������
float  primaryFuel;     //��������
float  beforeFuel;      //Ԥ������
float  afterFuel;       //��������
float  nowFuel;         //��ǰ������

void PIDVarGet(uint8_t cmdId,uint8_t *ptrData)
{
	uint8_t i; 
	for(i = 0;i < varOperation.pidVarNum;i ++)
	{
		if(ptrPIDVars->pidId != cmdId)
			continue;
		//1-���� 2-ת�� 3-�������� 4-�������� 5-Ԥ������ 6-�������� 7-��ǰ������
		switch(ptrPIDVars->varId)
		{
			case 1://���㳵��
				if(ptrPIDVars->bitLen == 8)
					carSpeed = ptrData[ptrPIDVars->startByte]*ptrPIDVars->ceo + ptrPIDVars->offset;
				else if(ptrPIDVars->bitLen == 16)
					carSpeed =(ptrData[ptrPIDVars->startByte]*256 + ptrData[ptrPIDVars->startByte+1])*ptrPIDVars->ceo + ptrPIDVars->offset;
				else 
				{
				
				}break;
				
			case 2:break;
			case 3:break;
			case 4:break;
			case 5:break;
			case 6:break;
			case 7:break;
			default:break;
		}
	}
}








