#include "apptask.h"



void GPSTask(void *pdata)
{
	
	OSTimeDlyHMSM(0,0,6,0);//CDMA还没启动，此处需要延时
	while(1)
	{
		OSTimeDlyHMSM(0,0,0,250);
	}
}








