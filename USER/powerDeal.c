#include "includes.h"
#include "apptask.h"
#include "delay.h"
#include "obd.h"
#include "bsp.h"


void SpeedPlusSubCompute(void);//加减速次数计算
void RunDataReport(void );     //运行数据上报

//整车运行状态管理       todo：看门狗加进去  死机了就重启

void PowerDeal(void *pdata)
{
	uint8_t  timeTemp  = 0;
	uint8_t  locaTemp  = 0;
	uint32_t timeCount = 0;  //发动机 启动 - 停止 时间
	
	
	while(1)
	{
		OSTimeDlyHMSM(0,0,1,200);   //2s 巡检一次
		if(carAllRecord.cdmaReStart < 30)//CDMA连续重启次数小余30，喂狗
			IWDG_ReloadCounter();     // 喂狗

		timeCount = 0;
		
		if(((timeTemp & 0x01) == 0) && (carAllRecord.engineSpeed > 100))//发动机启动
		{
			timeTemp |= 0x01;
			timeTemp &= 0xFD;
			carAllRecord.startTime = RTC_GetCounter();
		}//发动机停止
		if((carAllRecord.engineSpeed < 100) && ((timeTemp & 0x02) == 0))
		{
			timeTemp |= 0x02;
			timeTemp &= 0xFE;
			carAllRecord.stopTime = RTC_GetCounter();
			timeCount = carAllRecord.stopTime>carAllRecord.startTime?carAllRecord.stopTime-carAllRecord.startTime:0;
		}//起始经纬度
		if(((locaTemp & 0x01) == 0)&&(gpsMC.longitude != 0)&&((timeTemp & 0x01) != 0))
		{
			locaTemp |= 0x01;
			carAllRecord.startlongitude = gpsMC.longitude;
			carAllRecord.startlatitude  = gpsMC.latitude;
		}//车停后的经纬度
		if((gpsMC.longitude != 0)&&((timeTemp & 0x02) != 0))
		{
			locaTemp &= 0xFE;
			carAllRecord.stoplongitude = gpsMC.longitude;
			carAllRecord.stoplatitude  = gpsMC.latitude;
		}
		if(timeCount > 60)    //发动机启停超1分钟  上报运行数据
			RunDataReport();
		
		SpeedPlusSubCompute();//计算加减速、总行程
	}
} 


void SpeedPlusSubCompute(void)//加减速次数计算
{
	static float speedRecord = 0;
	static uint16_t ensureOne = 0;//保证一次加减速的过程中，次数不会累加
	float speedCount;
	
	speedCount = gpsMC.speed > speedRecord ? gpsMC.speed - speedRecord : speedRecord - gpsMC.speed;
	
	if(speedCount >= 2)//todo：这个 参考比较值 有待确定
	{
		if(ensureOne == 0)
		{
			if((gpsMC.speed > speedRecord)&&(ensureOne != 1))
			{
				carAllRecord.rapidlyPlusNum ++;
				ensureOne = 1;
			}
			else if((gpsMC.speed < speedRecord)&&(ensureOne != 2))
			{
				carAllRecord.rapidlySubNum ++;
				ensureOne = 2;
			}	
		}
	}else 
		ensureOne = 0;
	
	speedRecord = gpsMC.speed;//记录当前车速
	
	if(carAllRecord.carSpeedMax < gpsMC.speed)//得到最大车速
		carAllRecord.carSpeedMax = gpsMC.speed;
	
	carAllRecord.totalMileage += gpsMC.speed;//计算总行程
}
//运行数据上报
void RunDataReport(void)
{
	uint8_t* ptrCarState;
	uint8_t offset  = 0,err;
	
	ptrCarState = Mem_malloc(sizeof(CARRunRecord) + 10);
	if(ptrCarState == NULL)
	{
		LogReport("MEM Error! %d not enough!",sizeof(CARRunRecord) + 10);
		return;
	}
	ptrCarState[offset++] = 49;//行程信息上报
	ptrCarState[offset++] = 0x50;
	ptrCarState[offset++] = 0x06;
	//行程开始时间
	ptrCarState[offset++] = (carAllRecord.startTime >> 24) & 0x000000FF;
	ptrCarState[offset++] = (carAllRecord.startTime >> 16) & 0x000000FF;
	ptrCarState[offset++] = (carAllRecord.startTime >> 8)  & 0x000000FF;
	ptrCarState[offset++] = (carAllRecord.startTime >> 0)  & 0x000000FF;
	//行程截止时间
	ptrCarState[offset++] = (carAllRecord.stopTime >> 24) & 0x000000FF;
	ptrCarState[offset++] = (carAllRecord.stopTime >> 16) & 0x000000FF;
	ptrCarState[offset++] = (carAllRecord.stopTime >> 8)  & 0x000000FF;
	ptrCarState[offset++] = (carAllRecord.stopTime >> 0)  & 0x000000FF;
	//总里程
	ptrCarState[offset++] = (carAllRecord.totalMileage >> 24) & 0x000000FF;
	ptrCarState[offset++] = (carAllRecord.totalMileage >> 16) & 0x000000FF;
	ptrCarState[offset++] = (carAllRecord.totalMileage >> 8)  & 0x000000FF;
	ptrCarState[offset++] = (carAllRecord.totalMileage >> 0)  & 0x000000FF;
	//总油耗
	ptrCarState[offset++] = (carAllRecord.totalFuel >> 24) & 0x000000FF;
	ptrCarState[offset++] = (carAllRecord.totalFuel >> 16) & 0x000000FF;
	ptrCarState[offset++] = (carAllRecord.totalFuel >> 8)  & 0x000000FF;
	ptrCarState[offset++] = (carAllRecord.totalFuel >> 0)  & 0x000000FF;
	
//	carAllRecord.startlongitude = 12163188;
//	carAllRecord.startlatitude  = 3106432;
//	carAllRecord.stoplongitude  = 12163188;
//	carAllRecord.stoplatitude   = 3106432;
	
	//开始经度
	ptrCarState[offset++] = (carAllRecord.startlongitude >> 24) & 0x000000FF;
	ptrCarState[offset++] = (carAllRecord.startlongitude >> 16) & 0x000000FF;
	ptrCarState[offset++] = (carAllRecord.startlongitude >> 8)  & 0x000000FF;
	ptrCarState[offset++] = (carAllRecord.startlongitude >> 0)  & 0x000000FF;
	//开始纬度
	ptrCarState[offset++] = (carAllRecord.startlatitude >> 24) & 0x000000FF;
	ptrCarState[offset++] = (carAllRecord.startlatitude >> 16) & 0x000000FF;
	ptrCarState[offset++] = (carAllRecord.startlatitude >> 8)  & 0x000000FF;
	ptrCarState[offset++] = (carAllRecord.startlatitude >> 0)  & 0x000000FF;
	//结束经度
	ptrCarState[offset++] = (carAllRecord.stoplongitude >> 24) & 0x000000FF;
	ptrCarState[offset++] = (carAllRecord.stoplongitude >> 16) & 0x000000FF;
	ptrCarState[offset++] = (carAllRecord.stoplongitude >> 8)  & 0x000000FF;
	ptrCarState[offset++] = (carAllRecord.stoplongitude >> 0)  & 0x000000FF;
	//结束纬度
	ptrCarState[offset++] = (carAllRecord.stoplatitude >> 24) & 0x000000FF;
	ptrCarState[offset++] = (carAllRecord.stoplatitude >> 16) & 0x000000FF;
	ptrCarState[offset++] = (carAllRecord.stoplatitude >> 8)  & 0x000000FF;
	ptrCarState[offset++] = (carAllRecord.stoplatitude >> 0)  & 0x000000FF;
	//急加速次数
	ptrCarState[offset++] = carAllRecord.rapidlyPlusNum & 0xFF;
	//急减速次数
	ptrCarState[offset++] = carAllRecord.rapidlySubNum & 0xFF;
	//最高转速
	ptrCarState[offset++] = (carAllRecord.engineSpeedMax >> 8)  & 0x00FF;
	ptrCarState[offset++] = (carAllRecord.engineSpeedMax >> 0)  & 0x00FF;
	//最高车速
	ptrCarState[offset++] = (carAllRecord.carSpeedMax >> 8)  & 0x00FF;
	ptrCarState[offset++] = (carAllRecord.carSpeedMax >> 0)  & 0x00FF;
	//消息条数
	ptrCarState[offset++] = (carAllRecord.messageNum >> 24) & 0x000000FF;
	ptrCarState[offset++] = (carAllRecord.messageNum >> 16) & 0x000000FF;
	ptrCarState[offset++] = (carAllRecord.messageNum >> 8)  & 0x000000FF;
	ptrCarState[offset++] = (carAllRecord.messageNum >> 0)  & 0x000000FF;
	//网络流量
	ptrCarState[offset++] = (carAllRecord.netFlow >> 24) & 0x000000FF;
	ptrCarState[offset++] = (carAllRecord.netFlow >> 16) & 0x000000FF;
	ptrCarState[offset++] = (carAllRecord.netFlow >> 8)  & 0x000000FF;
	ptrCarState[offset++] = (carAllRecord.netFlow >> 0)  & 0x000000FF;
	
	OSMutexPend(CDMASendMutex,0,&err);					
	memcpy(&cdmaDataToSend->data[cdmaDataToSend->datLength],ptrCarState,ptrCarState[0]);
	cdmaDataToSend->datLength += ptrCarState[0];
	OSMutexPost(CDMASendMutex);
	
	Mem_free(ptrCarState);	
}



















