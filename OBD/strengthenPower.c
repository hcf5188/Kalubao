/*******************************************************************
*                   增强动力主要实现代码
*
*
********************************************************************/

#include "apptask.h"
#include "bsp.h"
#include "obd.h"



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
			}else
			{
				seedlokal.dword=seedlokal.dword<<1;
			}
		}
		for(i=0; i<4; i++)
		{
			key[3 - i] = seedlokal.byte[i];
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
	uint8_t pNum;

	uint8_t seed[4] = {0x00,0x00,0x00,0x00};
	uint8_t key[4]  = {0x00,0x00,0x00,0x00};
	
	dataToSend.pdat   = safe1;
	OBD_CAN_SendData(dataToSend.canId,dataToSend.ide,dataToSend.pdat);
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
	OBD_CAN_SendData(dataToSend.canId,dataToSend.ide,dataToSend.pdat);
	CAN1_RxMsg = OSQPend(canRecieveQ,0,&err);
	if(err == OS_ERR_NONE)
	{
		if(CAN1_RxMsg->Data[1] != 0x7F)
			LogReport("NTRU Success.");	
		else
		{
			LogReport("NTRU Fail.");
			varOperation.oilMode = 0;
			goto end;
		}
		Mem_free(CAN1_RxMsg);
	}
	dataToSend.pdat   = safe3;
	OBD_CAN_SendData(dataToSend.canId,dataToSend.ide,dataToSend.pdat);
	CAN1_RxMsg = OSQPend(canRecieveQ,0,&err);
	Mem_free(CAN1_RxMsg);
	dataToSend.pdat   = safe4;
	OBD_CAN_SendData(dataToSend.canId,dataToSend.ide,dataToSend.pdat);
	CAN1_RxMsg = OSQPend(canRecieveQ,0,&err);
	Mem_free(CAN1_RxMsg);
	
	dataToSend.pdat   = ptrVer;
	OBD_CAN_SendData(dataToSend.canId,dataToSend.ide,dataToSend.pdat);
	CAN1_RxMsg = OSQPend(canRecieveQ,0,&err);
	Mem_free(CAN1_RxMsg);
	pNum = (ptrVer[1]+1)%7==0?((ptrVer[1]+1)/7):((ptrVer[1]+1)/7 + 1);
	for(i=1;i<pNum;i++)
	{
		OSTimeDlyHMSM(0,0,0,2);
		dataToSend.pdat = &ptrVer[i*8];
		OBD_CAN_SendData(dataToSend.canId,dataToSend.ide,dataToSend.pdat);
	}
	Mem_free(ptrVer);
	CAN1_RxMsg = OSQPend(canRecieveQ,0,&err);
	if(CAN1_RxMsg->Data[1] == 0x7F)
	{
		LogReport("Strength Oil Fail!");
		varOperation.oilMode = 0;
	}else{
		LogReport("Strength Oil Success.");
		varOperation.oilMode = 1;
	}
end:	
	Mem_free(CAN1_RxMsg);
}


uint8_t verMany[8]  = {0x30,0x00,0x00,0x00,0x00,0x00,0x00,0x00};//多包

uint8_t  ReadECUVersion(uint8_t cmd[])//读取ECU版本号
{
	uint8_t err,i=0;
	uint8_t* ptrVer;
	CanRxMsg* CAN1_RxMsg;
	
	ptrVer = Mem_malloc(60);

	dataToSend.pdat   = cmd;
	OBD_CAN_SendData(dataToSend.canId,dataToSend.ide,dataToSend.pdat); 
	CAN1_RxMsg = OSQPend(canRecieveQ,50,&err);
	if((err == OS_ERR_NONE)&&(CAN1_RxMsg->Data[0] == 0x10))
	{
		memcpy(ptrVer,&CAN1_RxMsg->Data[4],4);
		Mem_free(CAN1_RxMsg);
	}
	else 
		return 200;
	dataToSend.pdat   = verMany;
	OBD_CAN_SendData(dataToSend.canId,dataToSend.ide,dataToSend.pdat);
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
	{
		LogReport("Config ECU version read error!!!");
		return 200;//识别错误
	}
		
	memcpy(varOperation.ecuVersion,ptrVer,19);
	Mem_free(ptrVer);
	for(i=0;i<20;i++)
	{
		if(i<5)
		{
			if((varOperation.ecuVersion[i] < 0x20)||(varOperation.ecuVersion[i] > 0x7E))//读取到非显示字符
			{
				return 200;//版本号识别出错
			}	
		}
		if(varOperation.ecuVersion[i]  == 0x20)
			varOperation.ecuVersion[i] =  '\0';
		
	}	
	LogReport("ECU version is %s.",varOperation.ecuVersion);//上报版本号
	
//根据ECU版本号 确定ECU安全算法的掩码
	if(strcmp(varOperation.ecuVersion,"P949V732") == 0)
	{
		ecuMask = ECUMASKV732;
		return 0;
	}else if(strcmp(varOperation.ecuVersion,"P949V791") == 0)
	{
		ecuMask = ECUMASKV791;
		return 1;
	}
//	else if(strcmp(varOperation.ecuVersion,"P949V792") == 0)//公司没有这个ECU，得不到验证，暂时屏蔽
//	{
//		ecuMask = ECUMASKV792;
//		return 2;
//	}else if(strcmp(varOperation.ecuVersion,"P532V46") == 0)//todo:以下ECU等节油做好了再弄
//	{
//		ecuMask = ECUMASKV46;
//		return 3;
//	}else if(strcmp(varOperation.ecuVersion,"P579V47") == 0)
//	{
//		ecuMask = ECUMASKV47;
//		return 4;
//	}else if(strcmp(varOperation.ecuVersion,"P813V72") == 0)
//	{
//		ecuMask = ECUMASKV72;
//		return 5;
//	}else if(strcmp(varOperation.ecuVersion,"P1499V301") == 0)
//	{
//		ecuMask = ECUMASK201;
//		return 6;
//	}else if(strcmp(varOperation.ecuVersion,"P1072V742") == 0)
//	{
//		ecuMask = ECUMASK211;
//		return 7;
//	}else if(strcmp(varOperation.ecuVersion,"P1158V760") == 0)
//	{
//		ecuMask = ECUMASKV760;
//		return 8;
//	}else if(strcmp(varOperation.ecuVersion,"P903V762") == 0)
//	{
//		ecuMask = ECUMASKV762;
//		return 9;
//	}
	else
	{
		LogReport("ECU Version %s can't be distinguished!!",varOperation.ecuVersion);
		return 100;//版本号读取出来了，但是这个版本号的ECU还不能做强动力
	}
		
}

extern uint8_t strengPower[100];
const uint16_t qMax = 0x045C;
//ECU 增强动力读数据的地址 有新的ECU的时候，将新版本添加进去，数组的个数加1即可
//每加一个版本，一定要测试通过才行。
uint8_t pAddr[10][5] = {
				{0x03,0x05,0x60,0xF8,0xFF},  //潍柴    P949V732  oilADDR
				{0x03,0x05,0x6C,0x60,0xFF},  //        P949V791
				{0x03,0x05,0x6C,0x60,0xFF},  //        P949V792
				
				{0x03,0x1C,0x60,0x38,0xFF},  //        P532V46     
				{0x03,0x1C,0x60,0x38,0xFF},  //        P579V47
				
				{0x03,0x1C,0x9E,0xDC,0xFF},  //        P813V72 
				{0x03,0x05,0x50,0xE0,0xFF},  //        P1499V301    
				{0x03,0x04,0xD4,0x4C,0xFF},  //        P1072V742   
				{0x03,0x04,0xD0,0x94,0xFF},  //重汽    P1158V760
				{0x03,0x04,0xD0,0x94,0xFF}   //锡柴    P903V762
			};
void Get_Q_FromECU(uint8_t ver)
{
	uint8_t* ptrSetCmd;
	uint8_t* ptrVer;
	CanRxMsg* CAN1_RxMsg;
	uint8_t  i,err;
	uint16_t dat1,dat2,datSma,datFlash;
	uint8_t qNum = 0;
	uint8_t offset = 0,bag = 0x21;
	static uint8_t  temp = 0;
    
	uint8_t textEcu[8] = {0x05,0x23,0x00,0x00,0x00,0x02,0x00,0x00};
	
	ptrSetCmd  = Mem_malloc(8);
	ptrVer = Mem_malloc(100);
	ptrVer[0] = 0x10;ptrVer[2] = 0x3D;
	for(i=0;i<pAddr[ver][0];i++)//写入ECU喷油量的地址
	{
		textEcu[i+2] = pAddr[ver][i+1];
	}

	dataToSend.pdat   = textEcu;
	OBD_CAN_SendData(dataToSend.canId,dataToSend.ide,dataToSend.pdat); 
	CAN1_RxMsg = OSQPend(canRecieveQ,50,&err);
	if(err == OS_ERR_NONE)
	{
		qNum = CAN1_RxMsg->Data[2];
		ptrVer[3 + pAddr[ver][0]] = qNum * 2;
		ptrVer[1] = qNum * 2 + pAddr[ver][0] + 2;
		offset = pAddr[ver][0] + 4;
		
		dat1 = textEcu[3];
		dat2 = textEcu[4]+qNum*2;
		if(dat2 >= 256)
			dat1++;
		textEcu[3] = dat1;
		textEcu[4] += (qNum*2 + 2);
		Mem_free(CAN1_RxMsg);
		
		for(i=0;i<pAddr[ver][0];i++)//写入ECU喷油量的地址
		{
			ptrVer[i+3] = textEcu[i+2];
		}
		
		for(i=0;i<qNum;i++)
		{
			memcpy(ptrSetCmd,textEcu,8);
			dataToSend.pdat   = textEcu;
			OBD_CAN_SendData(dataToSend.canId,dataToSend.ide,dataToSend.pdat); 
			CAN1_RxMsg = OSQPend(canRecieveQ,50,&err);
			if(err == OS_ERR_NONE)
			{	
				dat1 = CAN1_RxMsg->Data[3];
				dat1 = (dat1 << 8) + CAN1_RxMsg->Data[2];
				datSma = dat1/20;
				
				dat2 = dat1 + datSma;
			
				LogReport("CURR_OIL:%d,COM_OIL:%d",dat1,dat2);
				if(offset % 8 == 0)
					ptrVer[offset++] = bag++;
				ptrVer[offset++] = dat2 & 0x00FF;
				if(offset % 8 == 0)
					ptrVer[offset++] = bag++;
				ptrVer[offset++] = (dat2>>8) & 0x00FF;
				Mem_free(CAN1_RxMsg);
				if(strengPower[0] != 0x1A)				
				{
					strengPower[i*2 + 1] = dat2 & 0x00FF;
					strengPower[i*2 + 2] = (dat2>>8) & 0x00FF;
				}else
				{
					datFlash = strengPower[i*2 + 2];
					datFlash <<= 8;
					datFlash += strengPower[i*2 + 1];
					if(datFlash != dat2)
					{
						LogReport("PenYou Read With Flash Don't Equal!!!");
						if(datFlash == dat1)
							varOperation.oilMode = 1;//动力已经增强过了
						else
							varOperation.oilMode = 0;//ECU不匹配
						temp = 1;//动力不能增强
						break;
					}
				}
			}else{
				LogReport("PenYou Read Wrong!!!");
				temp = 1;//读取喷油量出错
				break;
			}	
			if(strengPower[0] != 0x1A && i == (qNum - 1))
			{
				strengPower[0] = 0x1A;
				SoftErasePage(STRENGE_Q);
				Save2KDataToFlash(strengPower,STRENGE_Q,200);
				LogReport("PenYou Write to Flash OK!!!");
			}	
			textEcu[4] += 2;
			if(textEcu[4] == 0x02)
				textEcu[3] ++;
		}
	}
	//安全算法，加写入数据
	if(temp == 0)//保证不会重复写入、不会增大到不可限制
	{
		temp = 1;
		SafeALG(ptrVer);//安全算法
	}
	else
	{
		Mem_free(ptrVer);
	}
}






