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
				carAllRecord.engineSpeedTemp = 1;
				carAllRecord.engineSpeed = CAN1_RxMsg->Data[4];
				carAllRecord.engineSpeed = (carAllRecord.engineSpeed * 256) + CAN1_RxMsg->Data[3];
				if(carAllRecord.engineSpeed > carAllRecord.engineSpeedMax)//获得发动机最高转速
					carAllRecord.engineSpeedMax = carAllRecord.engineSpeed;
				break;
			case  0x18FEE000://行驶距离，车辆距离
				carAllRecord.runLen1 =  CAN1_RxMsg->Data[3];
				carAllRecord.runLen1 = (carAllRecord.runLen1 * 256) + CAN1_RxMsg->Data[2];
				carAllRecord.runLen1 = (carAllRecord.runLen1 * 256) + CAN1_RxMsg->Data[1];
				carAllRecord.runLen1 = (carAllRecord.runLen1 * 256) + CAN1_RxMsg->Data[0];
				carAllRecord.runLen2 =  CAN1_RxMsg->Data[7];
				carAllRecord.runLen2 = (carAllRecord.runLen2 * 256) + CAN1_RxMsg->Data[6];
				carAllRecord.runLen2 = (carAllRecord.runLen2 * 256) + CAN1_RxMsg->Data[5];
				carAllRecord.runLen2 = (carAllRecord.runLen2 * 256) + CAN1_RxMsg->Data[4];
				break;
			case  0x18FEF121://车速
				carAllRecord.carSpeedTemp = 1;
				carAllRecord.carSpeed     = CAN1_RxMsg->Data[2];
				carAllRecord.carSpeed     = (carAllRecord.carSpeed * 256) + CAN1_RxMsg->Data[1];
				if(carAllRecord.carSpeed > carAllRecord.carSpeedMax)//获得最高车速
					carAllRecord.carSpeedMax = carAllRecord.carSpeed;
				break;
			case  0x18FEF100://车速
				if(carAllRecord.carSpeedTemp == 1)break;
				carAllRecord.carSpeed     = CAN1_RxMsg->Data[2];
				carAllRecord.carSpeed     = (carAllRecord.carSpeed * 256) + CAN1_RxMsg->Data[1];
				
				if(carAllRecord.carSpeed > carAllRecord.carSpeedMax)//获得最高车速
					carAllRecord.carSpeedMax = carAllRecord.carSpeed;
				break;
			case  0x18FEE900://行驶油耗，总油耗  总喷油量和总油耗应该一样的吧？
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
void J1939DataLog(void)
{
	LogReport("EngineSpeed: 0x0CF00400 - %d;",carAllRecord.engineSpeed);
	LogReport("RunDistance: 0x18FEE000 - 1:%d,2:%d;",carAllRecord.runLen1,carAllRecord.runLen2);
	LogReport("EngineSpeed: 0x18FEF100 - %d;",carAllRecord.carSpeed);
	LogReport("EngineSpeed: 0x18FEE900 - %f;",carAllRecord.allFuel);
}
//车辆运行的数据 初始化
void CARVarInit(void)
{
	carAllRecord.startTime      = 0;//发动机启动时间
	carAllRecord.stopTime       = 0;//发动机停止时间
	carAllRecord.totalMileage   = 0;//此次总行程
	carAllRecord.totalFuel      = 0;//总油耗
	carAllRecord.startlongitude = 0;//汽车开始 经度
	carAllRecord.startlatitude  = 0;//汽车开始 纬度
	carAllRecord.stoplongitude  = 0;//汽车停止 经度
	carAllRecord.stoplatitude   = 0;//汽车停止 纬度
	carAllRecord.rapidlyPlusNum = 0;//急加速次数
	carAllRecord.rapidlySubNum  = 0;//急减速次数
	carAllRecord.engineSpeedMax = 0;//最高转速
	carAllRecord.carSpeedMax    = 0;//最高车速
	carAllRecord.messageNum     = 0;//消息条数
	carAllRecord.netFlow        = 0;//网络流量
	
	carAllRecord.afterFuel       = 0;//后喷油量
	carAllRecord.afterFuelTemp   = 0;
	carAllRecord.allFuel         = 0;//总喷油量
	carAllRecord.allFuelTemp     = 0;
	carAllRecord.beforeFuel      = 0;//预喷油量
	carAllRecord.beforeFuelTemp  = 0;
	carAllRecord.carSpeed        = 0;//车速
	carAllRecord.carSpeedTemp    = 0;
	carAllRecord.nowFuel         = 0;//当前喷油量
	carAllRecord.nowFuelTemp     = 0;
	carAllRecord.primaryFuel     = 0;//主喷油量
	carAllRecord.primaryFuelTemp = 0;
	carAllRecord.engineSpeed     = 0;//发动机转速
	carAllRecord.engineSpeedTemp = 0;
	carAllRecord.runLen1         = 0;//行驶距离为 0
	carAllRecord.runLen2         = 0;//车辆距离为 0
}


//安全算法
long ecuMask = 0;//需要知道 ECU 掩码
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
uint8_t safe3[8] = {0x03,0x10,0x86,0xA7,0x00,0x00,0x00,0x00};
uint8_t safe4[8] = {0x03,0x80,0x90,0x01,0x00,0x00,0x00,0x00};
uint8_t safe5[8] = {0x10,0x2F,0x3D,0x05,0x61,0x24,0x2A,0x00};


//安全算法测试
extern CAN1DataToSend  dataToSend; 
void SafeALG(uint8_t* ptrVer)
{
	uint8_t err,i;
	CanRxMsg* CAN1_RxMsg;

	uint8_t seed[4] = {0x00,0x00,0x00,0x00};
	uint8_t key[4]  = {0x00,0x00,0x00,0x00};
	
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
	dataToSend.pdat   = safe3;
	OBD_CAN_SendData(dataToSend);
	CAN1_RxMsg = OSQPend(canRecieveQ,0,&err);
	Mem_free(CAN1_RxMsg);
	dataToSend.pdat   = safe4;
	OBD_CAN_SendData(dataToSend);
	CAN1_RxMsg = OSQPend(canRecieveQ,0,&err);
	Mem_free(CAN1_RxMsg);
	
	dataToSend.pdat   = ptrVer;
	OBD_CAN_SendData(dataToSend);
	CAN1_RxMsg = OSQPend(canRecieveQ,0,&err);
	Mem_free(CAN1_RxMsg);
	for(i=1;i<7;i++)
	{
		OSTimeDlyHMSM(0,0,0,2);
		dataToSend.pdat = &ptrVer[i*8];
		OBD_CAN_SendData(dataToSend);
		
	}
	Mem_free(ptrVer);
	CAN1_RxMsg = OSQPend(canRecieveQ,0,&err);
	Mem_free(CAN1_RxMsg);
}

uint8_t ver[8]   = {0x02,0x1A,0x94,0x00,0x00,0x00,0x00,0x00};
uint8_t ver1[8]  = {0x30,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
void ReadECUVersion(void)//读取ECU版本号
{
	uint8_t err,i=0;
	uint8_t* ptrVer;
	CanRxMsg* CAN1_RxMsg;	
	
	ptrVer = Mem_malloc(60);
	
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
	LogReport("ECU version is %s.",varOperation.ecuVersion);//上报版本号
	
//根据ECU版本号 确定ECU安全算法的掩码
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








































