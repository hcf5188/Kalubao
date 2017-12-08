#include "includes.h"
#include "apptask.h"
#include "delay.h"
#include "obd.h"
//整车电源管理                 todo：看门狗加进去  死机了就重启
void PowerDeal(void *pdata)
{
	
	while(1)
	{
		OSTimeDlyHMSM(0,0,0,100);
	}
}







