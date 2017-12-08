#include "apptask.h"
#include "obd.h"

void DealJ1939Date(void *pdata)
{
	uint8_t  err;
	uint32_t canId;
	CanRxMsg* CAN1_RxMsg;        //指向接收到的OBD信息
	
	
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
			case  0x0CF00400://发动机转速
				carStateVar.engineSpeedTemp = 1;
				carStateVar.engineSpeed = CAN1_RxMsg->Data[4];
				carStateVar.engineSpeed = (carStateVar.engineSpeed * 256) + CAN1_RxMsg->Data[5];
				break;
			case  0x18FEE000://行驶距离，车辆距离
				carStateVar.runLen1 =  CAN1_RxMsg->Data[3];
				carStateVar.runLen1 = (carStateVar.runLen1 * 256) + CAN1_RxMsg->Data[2];
				carStateVar.runLen1 = (carStateVar.runLen1 * 256) + CAN1_RxMsg->Data[1];
				carStateVar.runLen1 = (carStateVar.runLen1 * 256) + CAN1_RxMsg->Data[0];
				carStateVar.runLen2 =  CAN1_RxMsg->Data[7];
				carStateVar.runLen2 = (carStateVar.runLen2 * 256) + CAN1_RxMsg->Data[6];
				carStateVar.runLen2 = (carStateVar.runLen2 * 256) + CAN1_RxMsg->Data[5];
				carStateVar.runLen2 = (carStateVar.runLen2 * 256) + CAN1_RxMsg->Data[4];
				break;
			case  0x18FEF121://车速
				carStateVar.carSpeedTemp = 1;
				carStateVar.carSpeed     = CAN1_RxMsg->Data[3];
				carStateVar.carSpeed     = (carStateVar.carSpeed * 256) + CAN1_RxMsg->Data[2];
				break;
			case  0x18FEF100://车速
				if(carStateVar.carSpeedTemp == 1)break;
				carStateVar.carSpeed     = CAN1_RxMsg->Data[3];
				carStateVar.carSpeed     = (carStateVar.carSpeed * 256) + CAN1_RxMsg->Data[2];
				carStateVar.carSpeedTemp = 1;
				break;
			case  0x18FEE900://行驶油耗，总油耗
				carStateVar.allFuelTemp = 1;
				carStateVar.allFuel     = CAN1_RxMsg->Data[7];
				carStateVar.allFuel     = (carStateVar.allFuel * 256) + CAN1_RxMsg->Data[6];
				carStateVar.allFuel     = (carStateVar.allFuel * 256) + CAN1_RxMsg->Data[5];
				carStateVar.allFuel     = (carStateVar.allFuel * 256) + CAN1_RxMsg->Data[4];
				break;
			default: break;
		}
		Mem_free(CAN1_RxMsg);
	}
}

CARStateVar carStateVar;     //汽车有价值的状态信息

void CARVarInit(void)
{
	carStateVar.afterFuel       = 0;
	carStateVar.afterFuelTemp   = 0;
	carStateVar.allFuel         = 0;
	carStateVar.allFuelTemp     = 0;
	carStateVar.beforeFuel      = 0;
	carStateVar.beforeFuelTemp  = 0;
	carStateVar.carSpeed        = 0;
	carStateVar.carSpeedTemp    = 0;
	carStateVar.nowFuel         = 0;
	carStateVar.nowFuelTemp     = 0;
	carStateVar.primaryFuel     = 0;
	carStateVar.primaryFuelTemp = 0;
	carStateVar.engineSpeed     = 0;
	carStateVar.engineSpeedTemp = 0;
	carStateVar.runLen1         = 0;  //行驶距离为 0
	carStateVar.runLen2         = 0;  //车辆距离为 0
}
//安全算法


 #define ECUMASK47    0x24807961   //Yuchai  V47
 #define ECUMASK72    0x24807961   //Yuchai  V72
 #define ECUMASK17    0x383B50D9   //Weichai P949V732
 #define ECUMASK201   0x383B50D9   //WeiChai P1499V301
 #define ECUMASK211   0x62575E4E   //Yuchai  P1072V742 
 #define ECUMASK212   0x26121983   //Yuchai  P1382V762 
 #define ECUMASK213   0x3E814311   //CNHTC   P1158V760
 #define ECUMASK214   0x14071355   //DFCV    P1186V770 
 #define ECUMASK315   0x01080003   //JND     P1664V200
 #define ECUMASK216   0xF4EF7493   //SFH     P1287V770
 #define ECUMASKOT    0x29835B3D 
 #define ECUMASKCAMC  0x19A59E07

long ecuMask = 0x383B50D9;//需要知道 ECU 掩码
void SeedToKey(uint8_t* seed, uint8_t* key, uint8_t lenth)
{
	uint8_t i;
	longToUchar seedlokal;
	const long mask = ecuMask;
	
	if(seed[0] == 0 && seed[1] == 0)
		lenth = 0;
	else
	{
		seedlokal.dword = ((long)seed[0]<<24)+((long)seed[1]<<16)+((long)seed[2]<<8)+(long)seed[3];
		for(i=0; i<35; i++)
		{
			if(seedlokal.dword & 0x80000000)
			{
				seedlokal.dword=seedlokal.dword<<1;
				seedlokal.dword=seedlokal.dword ^ mask;
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
		lenth = 6;
	}
	return;   
}






