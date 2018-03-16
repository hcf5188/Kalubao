#include "apptask.h"
#include "obd.h"
extern u8 RCVData[16];    //��������֡ 
extern u8 RCVData0[16];   //��������֡
extern u8 RCVData1[16]; 
extern u8 RCVData2[16]; 
extern u8 RCVData3[16];
extern u8 RCVData4[16];
extern u8 RCVData5[16];
extern u8 RCVData6[16]; 
extern u8 CANerr_flg,CANerr_state,CANr1,CANr2,CANr3,CANr4,CANr5,CANr6,CANr7;
extern u8 fCanOK;
void DealJ1939Date(void *pdata)
{
	uint8_t  err;
	uint32_t canId;
	CanRxMsg* CAN1_RxMsg;        //ָ����յ��� OBD ��Ϣ
	
	
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
			case  0x0CF00400:           //������ת��
				carAllRecord.engineSpeedTemp = 1;
				carAllRecord.engineSpeed = ((uint16_t) CAN1_RxMsg->Data[4]* 256) + CAN1_RxMsg->Data[3];
			    carAllRecord.engineSpeed /= 8;  //�õ�ʵ��ת��
			
				if(carAllRecord.engineSpeed > carAllRecord.engineSpeedMax)//��÷��������ת��
					carAllRecord.engineSpeedMax = carAllRecord.engineSpeed;
				
				memcpy(RCVData0,CAN1_RxMsg->Data,8);
				CANr1 = 1;
				break;
			case  0x18FEE000:           //��ʻ���룬��������
				carAllRecord.runLen1 =  CAN1_RxMsg->Data[3];
				carAllRecord.runLen1 = (carAllRecord.runLen1 * 256) + CAN1_RxMsg->Data[2];
				carAllRecord.runLen1 = (carAllRecord.runLen1 * 256) + CAN1_RxMsg->Data[1];
				carAllRecord.runLen1 = (carAllRecord.runLen1 * 256) + CAN1_RxMsg->Data[0];
				carAllRecord.runLen2 =  CAN1_RxMsg->Data[7];
				carAllRecord.runLen2 = (carAllRecord.runLen2 * 256) + CAN1_RxMsg->Data[6];
				carAllRecord.runLen2 = (carAllRecord.runLen2 * 256) + CAN1_RxMsg->Data[5];
				carAllRecord.runLen2 = (carAllRecord.runLen2 * 256) + CAN1_RxMsg->Data[4];
				break;
			case  0x18FEF121:           //����
				carAllRecord.carSpeedTemp = 1;
				carAllRecord.carSpeed     = CAN1_RxMsg->Data[2];
				carAllRecord.carSpeed     = (carAllRecord.carSpeed * 256) + CAN1_RxMsg->Data[1];
				if(carAllRecord.carSpeed > carAllRecord.carSpeedMax)//�����߳���
					carAllRecord.carSpeedMax = carAllRecord.carSpeed;
				break;
			case  0x18FEF100:           //����
				if(carAllRecord.carSpeedTemp == 1)break;
				carAllRecord.carSpeed     = CAN1_RxMsg->Data[2];
				carAllRecord.carSpeed     = (carAllRecord.carSpeed * 256) + CAN1_RxMsg->Data[1];
				
				if(carAllRecord.carSpeed     > carAllRecord.carSpeedMax)//�����߳���
					carAllRecord.carSpeedMax = carAllRecord.carSpeed;
				break;
			case  0x18FEE900:           //��ʻ�ͺģ����ͺ�  �������������ͺ�Ӧ��һ���İɣ�
				carAllRecord.allFuelTemp = 1;
				carAllRecord.allFuel     = CAN1_RxMsg->Data[7];
				carAllRecord.allFuel     = (carAllRecord.allFuel * 256) + CAN1_RxMsg->Data[6];
				carAllRecord.allFuel     = (carAllRecord.allFuel * 256) + CAN1_RxMsg->Data[5];
				carAllRecord.allFuel     = (carAllRecord.allFuel * 256) + CAN1_RxMsg->Data[4];
				break;
			
//			case 0x0CF00400: break;
			
			case 0x0CF00300: memcpy(RCVData1,CAN1_RxMsg->Data,8);CANr2=1;break;  // EEC2
					
			case 0x18FF0004: memcpy(RCVData2,CAN1_RxMsg->Data,8);CANr3=1;break;  // 
			
			case 0x18FF0006: memcpy(RCVData3,CAN1_RxMsg->Data,8);CANr4=1;break;  // 
					
			case 0x18FEDF00: memcpy(RCVData4,CAN1_RxMsg->Data,8);CANr5=1;break;  //
			
			case 0x18EBFF00: memcpy(RCVData5,CAN1_RxMsg->Data,8);CANr6=1;break;  //
			
			case 0x18ECFF00: memcpy(RCVData6,CAN1_RxMsg->Data,8);CANr7=1;break;  //
			
			default: break;
		}
		Mem_free(CAN1_RxMsg);
		if(CANr1==1 && CANr2==1 && CANr3==1 && CANr4==1 && CANr5==1 && CANr6==1 && CANr7==1)
		{
			if(fCanOK == 0)
				LogReport("\r\nSaveFuel Data Receive OK!");
			fCanOK = 1;
		}else
			fCanOK = 0;
	}
}

void J1939DataLog(void)
{
//	LogReport("EngineSpeed: 0x0CF00400 - %d;",carAllRecord.engineSpeed);
//	LogReport("RunDistance: 0x18FEE000 - 1:%d,2:%d;",carAllRecord.runLen1,carAllRecord.runLen2);
//	LogReport("CarSpeed: 0x18FEF100 - %d;",carAllRecord.carSpeed);
//	LogReport("RunOil: 0x18FEE900 - %f;",carAllRecord.allFuel);
	signed char coe = 0;
	coe = strengthFuelFlash.coe;
	LogReport("\r\n01-ECUdatLenersion:%s",varOperation.ecuVersion);//��ӡ�汾��Ϣ
	if(coe >= 0)
		LogReport("\r\n02-OilMode:%d;",coe);      //��ӡ����ģʽ
	else
	{
		coe = 0-coe;
		LogReport("\r\n02-OilMode:-%d;",coe);
	}
}









