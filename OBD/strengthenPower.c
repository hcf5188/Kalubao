/*******************************************************************
*                   ��ǿ������Ҫʵ�ִ���
*
*
********************************************************************/

#include "apptask.h"
#include "bsp.h"
#include "obd.h"

//��ȫ�㷨
extern uint8_t strengPower[100];
long ecuMask = 0;//��Ҫ֪�� ECU ����
void SeedToKey(uint8_t* seed, uint8_t* key)
{
	uint8_t i;
	longToUchar seedlokal;
	const long mask = ecuMask;
	
	if(seed[0] == 0 && seed[1] == 0)
		return;
	seedlokal.dword = ((long)seed[0]<<24)+((long)seed[1]<<16)+((long)seed[2]<<8)+(long)seed[3];
	for(i=0; i<35; i++)
	{
		if(seedlokal.dword & 0x80000000)
		{
			seedlokal.dword = seedlokal.dword<<1;
			seedlokal.dword = seedlokal.dword ^ mask;
		}else
			seedlokal.dword=seedlokal.dword<<1;
	}
	for(i=0; i<4; i++)
		key[3 - i] = seedlokal.byte[i];
	return;   
}
//uint8_t safe1[8] = {0x02,0x27,0x09,0x00,0x00,0x00,0x00,0x00};//��ȫ�㷨ָ��1
//uint8_t safe2[8] = {0x06,0x27,0x0A,0x00,0x00,0x00,0x00,0xAA};//��ȫ�㷨ָ��2
//uint8_t safe3[8] = {0x03,0x10,0x86,0xA7,0x00,0x00,0x00,0x00};//��ģʽָ��
//uint8_t safe4[8] = {0x03,0x80,0x90,0x01,0x00,0x00,0x00,0x00};//���빦��ģʽָ��

//��ȫ�㷨����
extern CAN1DataToSend  dataToSend; 
void SafeALG(void)
{
	uint8_t err,i;
	CanRxMsg* CAN1_RxMsg;
	uint8_t pNum,offset;
    uint8_t* cmdToSend;
	uint8_t* seed;
	uint8_t* key;
	signed char coe;
	uint16_t datToWrite = 0x0000;
	
	cmdToSend = Mem_malloc(8);
	seed = Mem_malloc(4);
	key = Mem_malloc(4);
	memset(seed,0,4);memset(key,0,4);
	if(strengthFuelFlash.modeOrder == 2)
	{
		memcpy(cmdToSend,strengthFuelFlash.mode1,8);//��ģʽָ��
		dataToSend.pdat   = cmdToSend; 
		OBD_CAN_SendData(dataToSend.canId,dataToSend.ide,dataToSend.pdat);
		CAN1_RxMsg = OSQPend(canRecieveQ,500,&err);
		if(err != OS_ERR_NONE)
		{
			LogReport("\r\nMode cmd Error.");
			return;
		}
		Mem_free(CAN1_RxMsg);
	}
	memcpy(cmdToSend,strengthFuelFlash.safe1,8);  //��ȫ�㷨ָ��1  ��ECU�õ�����
	dataToSend.pdat   = cmdToSend;                //�˴���ʼ����ȫ�㷨
	OBD_CAN_SendData(dataToSend.canId,dataToSend.ide,dataToSend.pdat);
	CAN1_RxMsg = OSQPend(canRecieveQ,500,&err);
	if(err == OS_ERR_NONE)
	{
		seed[0]=CAN1_RxMsg->Data[3];seed[1]=CAN1_RxMsg->Data[4];
		seed[2]=CAN1_RxMsg->Data[5];seed[3]=CAN1_RxMsg->Data[6];
		SeedToKey(seed,key);                         //����ECU���ص����ӣ��������Կ
		memcpy(cmdToSend,strengthFuelFlash.safe2,8); //��ȫ�㷨ָ��2  ����Կд��ECU
		cmdToSend[0] = 0x06;
		cmdToSend[3] = key[0];cmdToSend[4] = key[1];cmdToSend[5] = key[2];cmdToSend[6] = key[3];
		Mem_free(CAN1_RxMsg);
	}
	Mem_free(seed);Mem_free(key);      //�����˾��ͷ��ڴ��
	dataToSend.pdat   = cmdToSend; 
	OBD_CAN_SendData(dataToSend.canId,dataToSend.ide,dataToSend.pdat);
	CAN1_RxMsg = OSQPend(canRecieveQ,500,&err);
	if(err == OS_ERR_NONE)
	{
		if(CAN1_RxMsg->Data[1] != 0x7F)
			LogReport("\r\n13-NTRU Success.");//��ȫ�㷨ͨ��
		else
		{
			LogReport("\r\n14-NTRU Fail.");   //ûͨ����ȫ�㷨
			varOperation.oilMode = 0;
			Mem_free(CAN1_RxMsg);
			return;
		}
		Mem_free(CAN1_RxMsg);
	}
	//todo��˳���д������·���ָ��˳����е���     ��ȫ�㷨��ģʽָ���ڴ˷���
	if(strengthFuelFlash.modeOrder == 1)
	{
		memcpy(cmdToSend,strengthFuelFlash.mode1,8);//��ģʽָ��
		dataToSend.pdat   = cmdToSend; 
		OBD_CAN_SendData(dataToSend.canId,dataToSend.ide,dataToSend.pdat);
		CAN1_RxMsg = OSQPend(canRecieveQ,500,&err);
		Mem_free(CAN1_RxMsg);
		
		memcpy(cmdToSend,strengthFuelFlash.mode2,8);//���빦��ģʽָ��
		dataToSend.pdat   = cmdToSend;
		OBD_CAN_SendData(dataToSend.canId,dataToSend.ide,dataToSend.pdat);
		CAN1_RxMsg = OSQPend(canRecieveQ,500,&err);
		Mem_free(CAN1_RxMsg);
	}
	Mem_free(cmdToSend);
	cmdToSend = Mem_malloc(100);
	pNum   = 0x21;
	offset = 0;
	cmdToSend[offset++] = 0x10;                  //���޸ĺ�ı궨����д��ECU�����
	if(strengthFuelFlash.modeOrder == 1)
	{
		cmdToSend[offset++] = strengPower[1]*2 + 5;
		cmdToSend[offset++] = 0x3D;
		datToWrite  =  strengthFuelFlash.fuelAddr[2];
		datToWrite  = (datToWrite << 8) + strengthFuelFlash.fuelAddr[3];
		datToWrite  += strengPower[1] * 2 + 2;
		
		cmdToSend[offset++] = strengthFuelFlash.fuelAddr[1];//Ҫд������͵�ַ
		cmdToSend[offset++] = (datToWrite >> 8) & 0xFF;
		cmdToSend[offset++] = datToWrite  & 0xFF;
		cmdToSend[offset++] = strengPower[1] * 2;           //Ҫд�������ĸ���
		coe = strengthFuel.coe;
		for(i = 0;i < strengPower[1];i++)
		{
			datToWrite = strengPower[i*2+3];
			datToWrite = (datToWrite<<8) + strengPower[i*2+2];
			if(strengthFuel.coe >= 0)
				datToWrite += datToWrite * coe / 100;
			else
			{
				coe = 0 - strengthFuel.coe;
				datToWrite -= datToWrite * coe/100;
			}
			if(offset % 8 == 0)
				cmdToSend[offset++] = pNum++;
			cmdToSend[offset++]	= datToWrite & 0xFF;
			if(offset % 8 == 0)
				cmdToSend[offset++] = pNum++;
			cmdToSend[offset++]	= (datToWrite>>8) & 0xFF;
		}
	}else if(strengthFuelFlash.modeOrder == 2)
	{
		cmdToSend[offset++] = strengPower[1]*2 + 7;
		cmdToSend[offset++] = 0x3D;
		datToWrite  =  strengthFuelFlash.fuelAddr[3];
		datToWrite  = (datToWrite << 8) + strengthFuelFlash.fuelAddr[4];
		datToWrite  += strengPower[1] * 2 + 2;
		
		cmdToSend[offset++] = strengthFuelFlash.fuelAddr[1];//Ҫд������͵�ַ
		cmdToSend[offset++] = strengthFuelFlash.fuelAddr[2];
		cmdToSend[offset++] = (datToWrite >> 8) & 0xFF;
		cmdToSend[offset++] = datToWrite  & 0xFF;
		cmdToSend[offset++] = 0x00;  
		cmdToSend[offset++] = pNum++; 
		cmdToSend[offset++] = strengPower[1] * 2;//Ҫд����ֽ���
		coe = strengthFuel.coe;
		for(i = 0;i < strengPower[1];i++)
		{
			datToWrite = strengPower[i*2+3];
			datToWrite = (datToWrite<<8) + strengPower[i*2+2];
			if(strengthFuel.coe >= 0)
				datToWrite += datToWrite * coe / 100;
			else
			{
				coe = 0 - strengthFuel.coe;
				datToWrite -= datToWrite * coe/100;
			}
			cmdToSend[offset++]	= datToWrite & 0xFF; //���޸ĺ�ı궨ֵд�뽫Ҫ���͵�������
			if(offset % 8 ==0)
				cmdToSend[offset++] = pNum++;
			cmdToSend[offset++]	= (datToWrite>>8) & 0xFF;
			if(offset % 8 ==0)
				cmdToSend[offset++] = pNum++;
		}
	}
	CAN1_RxMsg = OSQPend(canRecieveQ,2,&err);
	Mem_free(CAN1_RxMsg);
	
	dataToSend.pdat   = cmdToSend;                 //д����ĺ��ֵ
	OBD_CAN_SendData(dataToSend.canId,dataToSend.ide,dataToSend.pdat);
	CAN1_RxMsg = OSQPend(canRecieveQ,1000,&err);
	Mem_free(CAN1_RxMsg);
	pNum = (offset%8) == 0 ? offset/8:(offset/8) + 1;
	for(i = 1;i < pNum;i ++)
	{
		OSTimeDlyHMSM(0,0,0,2);
		dataToSend.pdat = &cmdToSend[i*8];
		OBD_CAN_SendData(dataToSend.canId,dataToSend.ide,dataToSend.pdat);
	}
	CAN1_RxMsg = OSQPend(canRecieveQ,0,&err);
	if(CAN1_RxMsg->Data[1] == 0x7F)
	{
		LogReport("\r\n15-Strength Fail!");   //��ǿ����ʧ��
		varOperation.oilMode = 0;
	}else
	{
		LogReport("\r\n16-Strength Success.");//��ǿ�����ɹ�
		strengthFuelFlash.coe = strengthFuel.coe;
		varOperation.oilMode = 1;
	}
	Mem_free(CAN1_RxMsg);
	Mem_free(cmdToSend);
}


uint8_t verMany[8]  = {0x30,0x00,0x00,0x00,0x00,0x00,0x00,0x00};//���

uint8_t  ReadECUVersion(uint8_t cmd[])//��ȡECU�汾��
{
	uint8_t err,i=0;
	uint8_t* ptrVer;
	uint8_t cmdLen = 0;
	CanRxMsg* CAN1_RxMsg;
	
	ptrVer = Mem_malloc(60);

	dataToSend.pdat   = cmd;
	cmdLen = cmd[0];
	OBD_CAN_SendData(dataToSend.canId,dataToSend.ide,dataToSend.pdat); 
	CAN1_RxMsg = OSQPend(canRecieveQ,50,&err);
	if((err == OS_ERR_NONE) && (CAN1_RxMsg->Data[0] == 0x10))
	{
		memcpy(ptrVer,&CAN1_RxMsg->Data[cmdLen+2],6-cmdLen);
		Mem_free(CAN1_RxMsg);
	}
	else 
	{
		LogReport("\r\n17-ECUVer read fail!");varOperation.isStrenOilOK = 0;
		return 200;
	}
		
	dataToSend.pdat   = verMany;
	OBD_CAN_SendData(dataToSend.canId,dataToSend.ide,dataToSend.pdat);
	do{
		CAN1_RxMsg = OSQPend(canRecieveQ,200,&err);
		if(err == OS_ERR_NONE)
		{
			memcpy(&ptrVer[7*i + 6-cmdLen],&CAN1_RxMsg->Data[1],7);
			Mem_free(CAN1_RxMsg);
		}
		i++;
	}while(err == OS_ERR_NONE);
	if(i<2)
	{
		LogReport("\r\n18-ECUVer read fail!");varOperation.isStrenOilOK = 0;
		return 200;//ʶ�����
	}
		
	memcpy(varOperation.ecuVersion,ptrVer,19);
	Mem_free(ptrVer);
	for(i=0;i<20;i++)
	{
		if(i<5)
		{
			if((varOperation.ecuVersion[i] < 0x20)||(varOperation.ecuVersion[i] > 0x7E))//��ȡ������ʾ�ַ�
			{
				LogReport("\r\n19-ECUVer read fail!");varOperation.isStrenOilOK = 0;
				return 200;//�汾��ʶ�����
			}	
		}
		if(varOperation.ecuVersion[i]  == 0x20)
			varOperation.ecuVersion[i] =  '\0';
	}	
	LogReport("\r\n20-ECUVer:%s.",varOperation.ecuVersion);//�ϱ��汾��
	
//����ECU�汾�� ȷ��ECU��ȫ�㷨������
	if(strcmp(varOperation.ecuVersion,(const char *)strengthFuelFlash.ecuVer) == 0)//�������
	{
		ecuMask = strengthFuelFlash.mask[0];
		ecuMask = (ecuMask << 8) + strengthFuelFlash.mask[1];
		ecuMask = (ecuMask << 8) + strengthFuelFlash.mask[2];
		ecuMask = (ecuMask << 8) + strengthFuelFlash.mask[3];
		
		return 0;
	}else
	{
		varOperation.isStrenOilOK = 0;   //�����Խ��ж�������
		LogReport("\r\n21-ECUVer Mismatching;");
		return 100;//�汾�Ŷ�ȡ�����ˣ���û��ƥ������������汾��
	}
}
void Get_Q_FromECU(void)
{
	
	CanRxMsg* CAN1_RxMsg;
	uint8_t*  textEcu;
	
	uint8_t  i,err;
	uint16_t dat1,dat2,datFlash;
	uint8_t  qNum = 0;
	uint32_t d_Value = 0;
	
	textEcu = Mem_malloc(8);
	memset(textEcu,0,8);

	if(strengthFuelFlash.modeOrder == 1)
	{
		textEcu[0] = 0x05;textEcu[1] = 0x23;
		memcpy(&textEcu[2],&strengthFuelFlash.fuelAddr[1],strengthFuelFlash.fuelAddr[0]);
		textEcu[5] = 0x02;                  //02 ����������ݳ�����2���ֽ�
	}else if(strengthFuelFlash.modeOrder == 2)
	{
		textEcu[0] = 0x07;textEcu[1] = 0x23;
		memcpy(&textEcu[2],&strengthFuelFlash.fuelAddr[1],strengthFuelFlash.fuelAddr[0]);
		textEcu[6] = 0x00;textEcu[7] = 0x02;//02 ����������ݳ�����2���ֽ�
	}else
	{
		LogReport("Service mode Error!");
		return;
	}
	
	dataToSend.pdat  = textEcu;
	OBD_CAN_SendData(dataToSend.canId,dataToSend.ide,dataToSend.pdat); 
	CAN1_RxMsg = OSQPend(canRecieveQ,50,&err);
	if(err == OS_ERR_NONE)
	{
		qNum = CAN1_RxMsg -> Data[2];                               //�õ��������ĸ���
		if(strengPower[1] != qNum)
		{
			strengPower[0] = 0;
			strengPower[1] = qNum;
		}
		if(strengthFuelFlash.modeOrder == 1)
		{
			dat1 = textEcu[3];                                        //���ڱ���ַ�Ƿ��ۼ�>0xFF,���ǣ���ߵ�ַ����
			dat2 = textEcu[4] + qNum * 2;
			if(dat2 >= 256)
				dat1 ++;
			textEcu[3] = dat1;
			textEcu[4] += (qNum * 2 + 2);
		}else if(strengthFuelFlash.modeOrder == 2)
		{
			dat1 = textEcu[4];                                        //���ڱ���ַ�Ƿ��ۼ�>0xFF,���ǣ���ߵ�ַ����
			dat2 = textEcu[5] + qNum * 2;
			if(dat2 >= 256)
				dat1 ++;
			textEcu[4] = dat1;
			textEcu[5] += (qNum * 2 + 2);
		}
		Mem_free(CAN1_RxMsg);
		
		for(i = 0;i < qNum;i ++)
		{
			dataToSend.pdat  = textEcu;
			OBD_CAN_SendData(dataToSend.canId,dataToSend.ide,dataToSend.pdat); 
			CAN1_RxMsg = OSQPend(canRecieveQ,50,&err);
			if(err == OS_ERR_NONE)
			{	
				dat1 = CAN1_RxMsg -> Data[3];
				dat1 = (dat1 << 8) + CAN1_RxMsg->Data[2];
				Mem_free(CAN1_RxMsg);
				if(strengPower[0] != 0xAF)        //Flash ����û�б����һ��ֵ
				{
					strengPower[i*2 + 2] = dat1 & 0xFF;
					strengPower[i*2 + 3] = (dat1 >> 8) & 0xFF;
				}else                             //�õ���ǰ��������ϵ��
				{
					datFlash = strengPower[i * 2 + 3];
					datFlash = (datFlash << 8) + strengPower[i*2 + 2];
					d_Value = datFlash > dat1 ? datFlash - dat1:dat1 - datFlash;
					d_Value = d_Value * 100 / datFlash;
					strengthFuelFlash.coe = d_Value;   //�õ���ǰ�������İٷֱ�
					strengthFuelFlash.coe = datFlash > dat1? -strengthFuelFlash.coe:strengthFuelFlash.coe;
					strengthFuel.coe = strengthFuelFlash.coe;
					varOperation.isStrenOilOK = 1;     //���Խ��ж�������
					break;
				}
			}else{
				LogReport("PenYou Read Wrong!!!");
				break;
			}	
			if(strengPower[0] != 0xAF && i == (qNum - 1))
			{
				strengPower[0] = 0xAF;
				SoftErasePage(STRENGE_Q);
				Save2KDataToFlash(strengPower,STRENGE_Q,200);      //��ԭʼֵ���浽Flash
				LogReport("PenYou Write to Flash OK!!!");
				varOperation.isStrenOilOK = 1;                     //���Խ��ж�������
				strengthFuelFlash.coe = 0;
			}
			if(strengthFuelFlash.modeOrder == 1)
			{
				textEcu[4]     += 2;      //��ȡ��ַ����
				if(textEcu[4]  == 0x00)
					textEcu[3] ++;
			}else if(strengthFuelFlash.modeOrder == 2)
			{
				textEcu[5]     += 2;      //��ȡ��ַ����
				if(textEcu[5]  == 0x00)
					textEcu[4] ++;
			}
		}
	}else
	{
		varOperation.isStrenOilOK = 0;
		LogReport("PenYou can't read!");
	}

	Mem_free(textEcu);
}

void StrengthFuel(void)
{
	static uint16_t time = 0;
	if(strengthFuelFlash.coe == strengthFuel.coe)//ϵ����ȣ���������������
		return;
	if(varOperation.isStrenOilOK == 0)//
	{
		if(time%10000 == 0)//��ֹ��ͣ�ش�ӡ��Ϣ
			LogReport("ECU can't be discerned!");
		time ++;
		return ;
	}
	if(varOperation.pidTset == 1)//������ڲ��ԣ�Ҳ�����Խ�����ǿ����
		return;
	varOperation.strengthRun = 1;
	//ֹͣCAN���ĵķ���
	//����ȫ�㷨����ģʽ�����µı궨ֵд��ECU
	//��ʱ20�룬����������CAN�����շ�
	SafeALG();
	OSTimeDlyHMSM(0,0,20,0);
	varOperation.strengthRun = 0;
}















































