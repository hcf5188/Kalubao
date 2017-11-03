#include "apptask.h"


pCIR_QUEUE sendCDMA_Q;//ָ�� CDMA ���ڷ��Ͷ���  ��ָ��
pSTORE     receCDMA_S;//ָ�� CDMA ���ڽ������ݶѵ�ָ��
const uint8_t atCmd[] = "AT\r";
void CDMATask(void *pdata)
{
	sendCDMA_Q = Cir_Queue_Init(1024);//CDMA ���ڷ��� ѭ������
	receCDMA_S = Store_Init(1024);    //CDMA ���ڽ��� ���ݶ�
	
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






