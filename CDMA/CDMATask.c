#include "apptask.h"


pCIR_QUEUE sendCDMA_Q;//指向 CDMA 串口发送队列  的指针
pSTORE     receCDMA_S;//指向 CDMA 串口接收数据堆的指针
const uint8_t atCmd[] = "AT\r";
void CDMATask(void *pdata)
{
	sendCDMA_Q = Cir_Queue_Init(1024);//CDMA 串口发送 循环队列
	receCDMA_S = Store_Init(1024);    //CDMA 串口接收 数据堆
	
//	CDMAUart2Init();
//	
//	OSTimeDlyHMSM(0,0,5,0);
	CDMA_POWER_LOW;
	OSTimeDlyHMSM(0,0,3,0);
	CDMA_POWER_HIGH;
	OSTimeDlyHMSM(0,0,3,0);
	
	
	while(1)
	{
		CDMASendDatas(atCmd,sizeof(atCmd));
		OSTimeDlyHMSM(0,0,1,0);
	}
}






