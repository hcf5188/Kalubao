#include "apptask.h"



void GPSTask(void *pdata)
{
	
	OSTimeDlyHMSM(0,0,6,0);//CDMA��û�������˴���Ҫ��ʱ
	while(1)
	{
		OSTimeDlyHMSM(0,0,0,250);
	}
}








