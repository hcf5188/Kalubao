/*******************************************************************
*                   增强动力主要实现代码
*
*
********************************************************************/

#include "apptask.h"
#include "bsp.h"
#include "obd.h"

//安全算法
extern uint8_t strengPower[100];
long ecuMask = 0;//需要知道 ECU 掩码
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
//uint8_t safe1[8] = {0x02,0x27,0x09,0x00,0x00,0x00,0x00,0x00};//安全算法指令1
//uint8_t safe2[8] = {0x06,0x27,0x0A,0x00,0x00,0x00,0x00,0xAA};//安全算法指令2
//uint8_t safe3[8] = {0x03,0x10,0x86,0xA7,0x00,0x00,0x00,0x00};//过模式指令
//uint8_t safe4[8] = {0x03,0x80,0x90,0x01,0x00,0x00,0x00,0x00};//进入功能模式指令

//安全算法测试
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
		memcpy(cmdToSend,strengthFuelFlash.mode1,8);//过模式指令
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
	memcpy(cmdToSend,strengthFuelFlash.safe1,8);  //安全算法指令1  从ECU得到种子
	dataToSend.pdat   = cmdToSend;                //此处开始过安全算法
	OBD_CAN_SendData(dataToSend.canId,dataToSend.ide,dataToSend.pdat);
	CAN1_RxMsg = OSQPend(canRecieveQ,500,&err);
	if(err == OS_ERR_NONE)
	{
		seed[0]=CAN1_RxMsg->Data[3];seed[1]=CAN1_RxMsg->Data[4];
		seed[2]=CAN1_RxMsg->Data[5];seed[3]=CAN1_RxMsg->Data[6];
		SeedToKey(seed,key);                         //根据ECU返回的种子，计算出秘钥
		memcpy(cmdToSend,strengthFuelFlash.safe2,8); //安全算法指令2  将秘钥写入ECU
		cmdToSend[0] = 0x06;
		cmdToSend[3] = key[0];cmdToSend[4] = key[1];cmdToSend[5] = key[2];cmdToSend[6] = key[3];
		Mem_free(CAN1_RxMsg);
	}
	Mem_free(seed);Mem_free(key);      //用完了就释放内存块
	dataToSend.pdat   = cmdToSend; 
	OBD_CAN_SendData(dataToSend.canId,dataToSend.ide,dataToSend.pdat);
	CAN1_RxMsg = OSQPend(canRecieveQ,500,&err);
	if(err == OS_ERR_NONE)
	{
		if(CAN1_RxMsg->Data[1] != 0x7F)
			LogReport("\r\n13-NTRU Success.");//安全算法通过
		else
		{
			LogReport("\r\n14-NTRU Fail.");   //没通过安全算法
			varOperation.oilMode = 0;
			Mem_free(CAN1_RxMsg);
			return;
		}
		Mem_free(CAN1_RxMsg);
	}
	//todo：顺序有待根据下发的指令顺序进行调整     安全算法的模式指令在此发送
	if(strengthFuelFlash.modeOrder == 1)
	{
		memcpy(cmdToSend,strengthFuelFlash.mode1,8);//过模式指令
		dataToSend.pdat   = cmdToSend; 
		OBD_CAN_SendData(dataToSend.canId,dataToSend.ide,dataToSend.pdat);
		CAN1_RxMsg = OSQPend(canRecieveQ,500,&err);
		Mem_free(CAN1_RxMsg);
		
		memcpy(cmdToSend,strengthFuelFlash.mode2,8);//进入功能模式指令
		dataToSend.pdat   = cmdToSend;
		OBD_CAN_SendData(dataToSend.canId,dataToSend.ide,dataToSend.pdat);
		CAN1_RxMsg = OSQPend(canRecieveQ,500,&err);
		Mem_free(CAN1_RxMsg);
	}
	Mem_free(cmdToSend);
	cmdToSend = Mem_malloc(100);
	pNum   = 0x21;
	offset = 0;
	cmdToSend[offset++] = 0x10;                  //将修改后的标定数据写入ECU，多包
	if(strengthFuelFlash.modeOrder == 1)
	{
		cmdToSend[offset++] = strengPower[1]*2 + 5;
		cmdToSend[offset++] = 0x3D;
		datToWrite  =  strengthFuelFlash.fuelAddr[2];
		datToWrite  = (datToWrite << 8) + strengthFuelFlash.fuelAddr[3];
		datToWrite  += strengPower[1] * 2 + 2;
		
		cmdToSend[offset++] = strengthFuelFlash.fuelAddr[1];//要写入的喷油地址
		cmdToSend[offset++] = (datToWrite >> 8) & 0xFF;
		cmdToSend[offset++] = datToWrite  & 0xFF;
		cmdToSend[offset++] = strengPower[1] * 2;           //要写入后面跟的个数
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
		
		cmdToSend[offset++] = strengthFuelFlash.fuelAddr[1];//要写入的喷油地址
		cmdToSend[offset++] = strengthFuelFlash.fuelAddr[2];
		cmdToSend[offset++] = (datToWrite >> 8) & 0xFF;
		cmdToSend[offset++] = datToWrite  & 0xFF;
		cmdToSend[offset++] = 0x00;  
		cmdToSend[offset++] = pNum++; 
		cmdToSend[offset++] = strengPower[1] * 2;//要写入的字节数
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
			cmdToSend[offset++]	= datToWrite & 0xFF; //将修改后的标定值写入将要发送的数组中
			if(offset % 8 ==0)
				cmdToSend[offset++] = pNum++;
			cmdToSend[offset++]	= (datToWrite>>8) & 0xFF;
			if(offset % 8 ==0)
				cmdToSend[offset++] = pNum++;
		}
	}
	CAN1_RxMsg = OSQPend(canRecieveQ,2,&err);
	Mem_free(CAN1_RxMsg);
	
	dataToSend.pdat   = cmdToSend;                 //写入更改后的值
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
		LogReport("\r\n15-Strength Fail!");   //增强动力失败
		varOperation.oilMode = 0;
	}else
	{
		LogReport("\r\n16-Strength Success.");//增强动力成功
		strengthFuelFlash.coe = strengthFuel.coe;
		varOperation.oilMode = 1;
	}
	Mem_free(CAN1_RxMsg);
	Mem_free(cmdToSend);
}


uint8_t verMany[8]  = {0x30,0x00,0x00,0x00,0x00,0x00,0x00,0x00};//多包

uint8_t  ReadECUVersion(uint8_t cmd[])//读取ECU版本号
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
				LogReport("\r\n19-ECUVer read fail!");varOperation.isStrenOilOK = 0;
				return 200;//版本号识别出错
			}	
		}
		if(varOperation.ecuVersion[i]  == 0x20)
			varOperation.ecuVersion[i] =  '\0';
	}	
	LogReport("\r\n20-ECUVer:%s.",varOperation.ecuVersion);//上报版本号
	
//根据ECU版本号 确定ECU安全算法的掩码
	if(strcmp(varOperation.ecuVersion,(const char *)strengthFuelFlash.ecuVer) == 0)//获得掩码
	{
		ecuMask = strengthFuelFlash.mask[0];
		ecuMask = (ecuMask << 8) + strengthFuelFlash.mask[1];
		ecuMask = (ecuMask << 8) + strengthFuelFlash.mask[2];
		ecuMask = (ecuMask << 8) + strengthFuelFlash.mask[3];
		
		return 0;
	}else
	{
		varOperation.isStrenOilOK = 0;   //不可以进行动力提升
		LogReport("\r\n21-ECUVer Mismatching;");
		return 100;//版本号读取出来了，还没有匹配的提升动力版本号
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
		textEcu[5] = 0x02;                  //02 代表的是数据长度是2个字节
	}else if(strengthFuelFlash.modeOrder == 2)
	{
		textEcu[0] = 0x07;textEcu[1] = 0x23;
		memcpy(&textEcu[2],&strengthFuelFlash.fuelAddr[1],strengthFuelFlash.fuelAddr[0]);
		textEcu[6] = 0x00;textEcu[7] = 0x02;//02 代表的是数据长度是2个字节
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
		qNum = CAN1_RxMsg -> Data[2];                               //得到喷油量的个数
		if(strengPower[1] != qNum)
		{
			strengPower[0] = 0;
			strengPower[1] = qNum;
		}
		if(strengthFuelFlash.modeOrder == 1)
		{
			dat1 = textEcu[3];                                        //用于辨别地址是否累加>0xFF,若是，则高地址自增
			dat2 = textEcu[4] + qNum * 2;
			if(dat2 >= 256)
				dat1 ++;
			textEcu[3] = dat1;
			textEcu[4] += (qNum * 2 + 2);
		}else if(strengthFuelFlash.modeOrder == 2)
		{
			dat1 = textEcu[4];                                        //用于辨别地址是否累加>0xFF,若是，则高地址自增
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
				if(strengPower[0] != 0xAF)        //Flash 中有没有保存第一次值
				{
					strengPower[i*2 + 2] = dat1 & 0xFF;
					strengPower[i*2 + 3] = (dat1 >> 8) & 0xFF;
				}else                             //得到当前喷油量的系数
				{
					datFlash = strengPower[i * 2 + 3];
					datFlash = (datFlash << 8) + strengPower[i*2 + 2];
					d_Value = datFlash > dat1 ? datFlash - dat1:dat1 - datFlash;
					d_Value = d_Value * 100 / datFlash;
					strengthFuelFlash.coe = d_Value;   //得到当前喷油量的百分比
					strengthFuelFlash.coe = datFlash > dat1? -strengthFuelFlash.coe:strengthFuelFlash.coe;
					strengthFuel.coe = strengthFuelFlash.coe;
					varOperation.isStrenOilOK = 1;     //可以进行动力提升
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
				Save2KDataToFlash(strengPower,STRENGE_Q,200);      //将原始值保存到Flash
				LogReport("PenYou Write to Flash OK!!!");
				varOperation.isStrenOilOK = 1;                     //可以进行动力提升
				strengthFuelFlash.coe = 0;
			}
			if(strengthFuelFlash.modeOrder == 1)
			{
				textEcu[4]     += 2;      //读取地址管理
				if(textEcu[4]  == 0x00)
					textEcu[3] ++;
			}else if(strengthFuelFlash.modeOrder == 2)
			{
				textEcu[5]     += 2;      //读取地址管理
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
	if(strengthFuelFlash.coe == strengthFuel.coe)//系数相等，不进行提升动力
		return;
	if(varOperation.isStrenOilOK == 0)//
	{
		if(time%10000 == 0)//防止不停地打印信息
			LogReport("ECU can't be discerned!");
		time ++;
		return ;
	}
	if(varOperation.pidTset == 1)//如果正在测试，也不可以进行增强动力
		return;
	varOperation.strengthRun = 1;
	//停止CAN报文的发送
	//过安全算法、过模式、将新的标定值写入ECU
	//延时20秒，进入正常的CAN报文收发
	SafeALG();
	OSTimeDlyHMSM(0,0,20,0);
	varOperation.strengthRun = 0;
}















































