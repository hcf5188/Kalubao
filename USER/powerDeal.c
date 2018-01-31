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
	uint8_t timeCount = 0;  //发动机 启动 - 停止 时间

	while(1)
	{
		OSTimeDlyHMSM(0,0,1,0); 
		
		if(carAllRecord.cdmaReStart < 30)//CDMA连续重启次数小余30，喂狗
			IWDG_ReloadCounter();        // 喂狗
		timeCount ++;
		if((timeCount >= 60) && (varOperation.isDataFlow == 0))//运行1分钟，数据流在流动，上报行程信息
		{
			RunDataReport();
			timeCount = 0;
		}
		SpeedPlusSubCompute();//计算加减速、总行程
	}
} 
  
void SpeedPlusSubCompute(void)//加减速次数计算
{
	static float speedRecord = 0;
	static uint16_t ensureOne = 0;//保证一次加减速的过程中，次数不会累加
	float speedCount;
	uint16_t speed;
	speed = carAllRecord.carSpeed == 0?gpsMC.speed:carAllRecord.carSpeed;
	speedCount = speed > speedRecord ? speed - speedRecord : speedRecord - speed;
	
	if(speedCount >= 20)//todo：这个 参考比较值 有待确定
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
	
	speedRecord = speed;//记录当前车速
	
	if(carAllRecord.carSpeedMax < speed)//得到最大车速
		carAllRecord.carSpeedMax = speed;
	
	carAllRecord.totalMileage += speed;//计算总行程
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
	ptrCarState[offset++] = 25;//行程信息上报
	ptrCarState[offset++] = 0x50;
	ptrCarState[offset++] = 0x06;
	
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
	
	Mem_free(ptrCarState);	//释放内存块
}



















