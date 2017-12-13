#include "apptask.h"
#include "obd.h"

void DealJ1939Date(void *pdata)
{
	uint8_t  err;
	uint32_t canId;
	CanRxMsg* CAN1_RxMsg;        //ָ����յ���OBD��Ϣ
	
	
	while(1)
	{
		CAN1_RxMsg = OSQPend(canJ1939Q,0,&err);
		if(err != OS_ERR_NONE)
			continue;
		if(CAN1_RxMsg->IDE == 0x04)
			canId = CAN1_RxMsg->ExtId;
		else
			canId = CAN1_RxMsg->StdId;
		switch(canId)
		{
			case  0x0CF00400://������ת��
				carAllRecord.engineSpeedTemp = 1;
				carAllRecord.engineSpeed = CAN1_RxMsg->Data[4];
				carAllRecord.engineSpeed = (carAllRecord.engineSpeed * 256) + CAN1_RxMsg->Data[5];
				if(carAllRecord.engineSpeed > carAllRecord.engineSpeedMax)//��÷��������ת��
					carAllRecord.engineSpeedMax = carAllRecord.engineSpeed;
				break;
			case  0x18FEE000://��ʻ���룬��������
				carAllRecord.runLen1 =  CAN1_RxMsg->Data[3];
				carAllRecord.runLen1 = (carAllRecord.runLen1 * 256) + CAN1_RxMsg->Data[2];
				carAllRecord.runLen1 = (carAllRecord.runLen1 * 256) + CAN1_RxMsg->Data[1];
				carAllRecord.runLen1 = (carAllRecord.runLen1 * 256) + CAN1_RxMsg->Data[0];
				carAllRecord.runLen2 =  CAN1_RxMsg->Data[7];
				carAllRecord.runLen2 = (carAllRecord.runLen2 * 256) + CAN1_RxMsg->Data[6];
				carAllRecord.runLen2 = (carAllRecord.runLen2 * 256) + CAN1_RxMsg->Data[5];
				carAllRecord.runLen2 = (carAllRecord.runLen2 * 256) + CAN1_RxMsg->Data[4];
				break;
			case  0x18FEF121://����
				carAllRecord.carSpeedTemp = 1;
				carAllRecord.carSpeed     = CAN1_RxMsg->Data[3];
				carAllRecord.carSpeed     = (carAllRecord.carSpeed * 256) + CAN1_RxMsg->Data[2];
				if(carAllRecord.carSpeed > carAllRecord.carSpeedMax)//�����߳���
					carAllRecord.carSpeedMax = carAllRecord.carSpeed;
				break;
			case  0x18FEF100://����
				if(carAllRecord.carSpeedTemp == 1)break;
				carAllRecord.carSpeed     = CAN1_RxMsg->Data[3];
				carAllRecord.carSpeed     = (carAllRecord.carSpeed * 256) + CAN1_RxMsg->Data[2];
				
				if(carAllRecord.carSpeed > carAllRecord.carSpeedMax)//�����߳���
					carAllRecord.carSpeedMax = carAllRecord.carSpeed;
				break;
			case  0x18FEE900://��ʻ�ͺģ����ͺ�  �������������ͺ�Ӧ��һ���İɣ�
				carAllRecord.allFuelTemp = 1;
				carAllRecord.allFuel     = CAN1_RxMsg->Data[7];
				carAllRecord.allFuel     = (carAllRecord.allFuel * 256) + CAN1_RxMsg->Data[6];
				carAllRecord.allFuel     = (carAllRecord.allFuel * 256) + CAN1_RxMsg->Data[5];
				carAllRecord.allFuel     = (carAllRecord.allFuel * 256) + CAN1_RxMsg->Data[4];
				break;
			default: break;
		}
		Mem_free(CAN1_RxMsg);
	}
}

void CARVarInit(void)
{
	carAllRecord.startTime      = 0;//����������ʱ��
	carAllRecord.stopTime       = 0;//������ֹͣʱ��
	carAllRecord.totalMileage   = 0;//�˴����г�
	carAllRecord.totalFuel      = 0;//���ͺ�
	carAllRecord.startlongitude = 0;//������ʼ ����
	carAllRecord.startlatitude  = 0;//������ʼ γ��
	carAllRecord.stoplongitude  = 0;//����ֹͣ ����
	carAllRecord.stoplatitude   = 0;//����ֹͣ γ��
	carAllRecord.rapidlyPlusNum = 0;//�����ٴ���
	carAllRecord.rapidlySubNum  = 0;//�����ٴ���
	carAllRecord.engineSpeedMax = 0;//���ת��
	carAllRecord.carSpeedMax    = 0;//��߳���
	carAllRecord.messageNum     = 0;//��Ϣ����
	carAllRecord.netFlow        = 0;//��������
	
	carAllRecord.afterFuel       = 0;//��������
	carAllRecord.afterFuelTemp   = 0;
	carAllRecord.allFuel         = 0;//��������
	carAllRecord.allFuelTemp     = 0;
	carAllRecord.beforeFuel      = 0;//Ԥ������
	carAllRecord.beforeFuelTemp  = 0;
	carAllRecord.carSpeed        = 0;//����
	carAllRecord.carSpeedTemp    = 0;
	carAllRecord.nowFuel         = 0;//��ǰ������
	carAllRecord.nowFuelTemp     = 0;
	carAllRecord.primaryFuel     = 0;//��������
	carAllRecord.primaryFuelTemp = 0;
	carAllRecord.engineSpeed     = 0;//������ת��
	carAllRecord.engineSpeedTemp = 0;
	carAllRecord.runLen1         = 0;//��ʻ����Ϊ 0
	carAllRecord.runLen2         = 0;//��������Ϊ 0
}


//��ȫ�㷨
long ecuMask = 0;//��Ҫ֪�� ECU ����
void SeedToKey(uint8_t* seed, uint8_t* key)
{
	uint8_t i;
	longToUchar seedlokal;
	const long mask = ecuMask;
	
	if(seed[0] == 0 && seed[1] == 0)
		return;
	else
	{
		seedlokal.dword = ((long)seed[0]<<24)+((long)seed[1]<<16)+((long)seed[2]<<8)+(long)seed[3];
		for(i=0; i<35; i++)
		{
			if(seedlokal.dword & 0x80000000)
			{
				seedlokal.dword = seedlokal.dword<<1;
				seedlokal.dword = seedlokal.dword ^ mask;
			} 
			else
			{
				seedlokal.dword=seedlokal.dword<<1;
			}
		}
		for(i=0; i<4; i++)
		{
			key[3-i] = seedlokal.byte[i];
		}
	}
	return;   
}

uint8_t safe1[8] = {0x02,0x27,0x09,0x00,0x00,0x00,0x00,0x00};
uint8_t safe2[8] = {0x06,0x27,0x0A,0x00,0x00,0x00,0x00,0xAA};

//��ȫ�㷨����
extern CAN1DataToSend  dataToSend; 
void SafeALG(void)
{
	uint8_t err;
	CanRxMsg* CAN1_RxMsg;
	CAN_InitTypeDef   CAN_InitStructure;
	uint8_t seed[4] = {0x00,0x00,0x00,0x00};
	uint8_t key[4]  = {0x00,0x00,0x00,0x00};
	
	CAN_DeInit(CAN1);  
	CAN_StructInit(&CAN_InitStructure);
	//����flash�е�CAN���ý��в���
	CAN1_BaudSet(CANBAUD_250K);
	CAN1_SetFilter(0x18DAFA00,CAN_ID_EXT);
	CAN_ITConfig(CAN1,CAN_IT_FMP1,ENABLE);//����CAN�˲���

	
	CAN1_RxMsg = OSQPend(canRecieveQ,50,&err);Mem_free(CAN1_RxMsg);
	CAN1_RxMsg = OSQPend(canRecieveQ,50,&err);Mem_free(CAN1_RxMsg);
	CAN1_RxMsg = OSQPend(canRecieveQ,50,&err);Mem_free(CAN1_RxMsg);
	CAN1_RxMsg = OSQPend(canRecieveQ,50,&err);Mem_free(CAN1_RxMsg);
	CAN1_RxMsg = OSQPend(canRecieveQ,50,&err);Mem_free(CAN1_RxMsg);
	CAN1_RxMsg = OSQPend(canRecieveQ,50,&err);Mem_free(CAN1_RxMsg);

	dataToSend.pdat   = safe1;
	OBD_CAN_SendData(dataToSend);
	CAN1_RxMsg = OSQPend(canRecieveQ,0,&err);
	if(err == OS_ERR_NONE)
	{
		seed[0]=CAN1_RxMsg->Data[3];seed[1]=CAN1_RxMsg->Data[4];
		seed[2]=CAN1_RxMsg->Data[5];seed[3]=CAN1_RxMsg->Data[6];
		SeedToKey(seed,key);
		safe2[3] = key[0];safe2[4] = key[1];safe2[5] = key[2];safe2[6] = key[3];
		Mem_free(CAN1_RxMsg);
	}
	dataToSend.pdat   = safe2;
	OBD_CAN_SendData(dataToSend);
	CAN1_RxMsg = OSQPend(canRecieveQ,0,&err);
	if(err == OS_ERR_NONE)
	{
		if(CAN1_RxMsg->Data[1] != 0x7F)
			LogReport("NTRU Success.");
		else
			LogReport("NTRU Fail.");
		Mem_free(CAN1_RxMsg);
	}
}

uint8_t ver[8]   = {0x02,0x1A,0x94,0x00,0x00,0x00,0x00,0x00};
uint8_t ver1[8]  = {0x30,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
void ReadECUVersion(void)//��ȡECU�汾��
{
	uint8_t err,i=0;
	uint8_t* ptrVer;
	CanRxMsg* CAN1_RxMsg;	
	
	CAN_InitTypeDef   CAN_InitStructure;
	
	ptrVer = Mem_malloc(60);
	
	CAN_DeInit(CAN1);  
	CAN_StructInit(&CAN_InitStructure);
	//����flash�е�CAN���ý��в���
	CAN1_BaudSet(CANBAUD_250K);
	CAN1_SetFilter(0x18DAFA00,CAN_ID_EXT);
	CAN_ITConfig(CAN1,CAN_IT_FMP1,ENABLE);//����CAN�˲���
		
	dataToSend.canId = 0x18DA00FA;
	dataToSend.ide   =0x04;
	
	dataToSend.pdat   = ver;
	OBD_CAN_SendData(dataToSend); 
	CAN1_RxMsg = OSQPend(canRecieveQ,50,&err);
	if(err == OS_ERR_NONE)
	{
		memcpy(ptrVer,&CAN1_RxMsg->Data[4],4);
		Mem_free(CAN1_RxMsg);
	}
	dataToSend.pdat   = ver1;
	OBD_CAN_SendData(dataToSend);
	do{
		CAN1_RxMsg = OSQPend(canRecieveQ,50,&err);
		if(err == OS_ERR_NONE)
		{
			memcpy(&ptrVer[7*i + 4],&CAN1_RxMsg->Data[1],7);
			Mem_free(CAN1_RxMsg);
		}
		i++;
	}while(err == OS_ERR_NONE);
	if(i<2)
		return;
	memcpy(varOperation.ecuVersion,ptrVer,19);
	Mem_free(ptrVer);
	for(i=0;i<20;i++)
	{
		if(varOperation.ecuVersion[i] == 0x20)
			varOperation.ecuVersion[i] = '\0';
	}	
	LogReport("ECU version is %s.",varOperation.ecuVersion);//�ϱ��汾��
	
//����ECU�汾�� ȷ��ECU��ȫ�㷨������
	if(strcmp(varOperation.ecuVersion,"V47") == 0)
	{
		ecuMask = ECUMASK47;
	}else if(strcmp(varOperation.ecuVersion,"V72") == 0)
	{
		ecuMask = ECUMASK72;
	}else if(strcmp(varOperation.ecuVersion,"P949V732") == 0)
	{
		ecuMask = ECUMASK17;
	}else if(strcmp(varOperation.ecuVersion,"P1499V301") == 0)
	{
		ecuMask = ECUMASK201;
	}else if(strcmp(varOperation.ecuVersion,"P1072V742") == 0)
	{
		ecuMask = ECUMASK211;
	}else if(strcmp(varOperation.ecuVersion,"P1382V762") == 0)
	{
		ecuMask = ECUMASK212;
	}else if(strcmp(varOperation.ecuVersion,"P1158V760") == 0)
	{
		ecuMask = ECUMASK213;
	}else if(strcmp(varOperation.ecuVersion,"P1186V770") == 0)
	{
		ecuMask = ECUMASK214;
	}else if(strcmp(varOperation.ecuVersion,"P1664V200") == 0)
	{
		ecuMask = ECUMASK315;
	}else if(strcmp(varOperation.ecuVersion,"P1287V770") == 0)
	{
		ecuMask = ECUMASK216;
	}
}








































