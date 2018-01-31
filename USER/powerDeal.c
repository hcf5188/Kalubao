#include "includes.h"
#include "apptask.h"
#include "delay.h"
#include "obd.h"
#include "bsp.h"


void SpeedPlusSubCompute(void);//�Ӽ��ٴ�������
void RunDataReport(void );     //���������ϱ�

//��������״̬����       todo�����Ź��ӽ�ȥ  �����˾�����
void PowerDeal(void *pdata)
{
	uint8_t timeCount = 0;  //������ ���� - ֹͣ ʱ��

	while(1)
	{
		OSTimeDlyHMSM(0,0,1,0); 
		
		if(carAllRecord.cdmaReStart < 30)//CDMA������������С��30��ι��
			IWDG_ReloadCounter();        // ι��
		timeCount ++;
		if((timeCount >= 60) && (varOperation.isDataFlow == 0))//����1���ӣ����������������ϱ��г���Ϣ
		{
			RunDataReport();
			timeCount = 0;
		}
		SpeedPlusSubCompute();//����Ӽ��١����г�
	}
} 
  
void SpeedPlusSubCompute(void)//�Ӽ��ٴ�������
{
	static float speedRecord = 0;
	static uint16_t ensureOne = 0;//��֤һ�μӼ��ٵĹ����У����������ۼ�
	float speedCount;
	uint16_t speed;
	speed = carAllRecord.carSpeed == 0?gpsMC.speed:carAllRecord.carSpeed;
	speedCount = speed > speedRecord ? speed - speedRecord : speedRecord - speed;
	
	if(speedCount >= 20)//todo����� �ο��Ƚ�ֵ �д�ȷ��
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
	
	speedRecord = speed;//��¼��ǰ����
	
	if(carAllRecord.carSpeedMax < speed)//�õ������
		carAllRecord.carSpeedMax = speed;
	
	carAllRecord.totalMileage += speed;//�������г�
}
//���������ϱ�
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
	ptrCarState[offset++] = 25;//�г���Ϣ�ϱ�
	ptrCarState[offset++] = 0x50;
	ptrCarState[offset++] = 0x06;
	
	//�����ٴ���
	ptrCarState[offset++] = carAllRecord.rapidlyPlusNum & 0xFF;
	//�����ٴ���
	ptrCarState[offset++] = carAllRecord.rapidlySubNum & 0xFF;
	//���ת��
	ptrCarState[offset++] = (carAllRecord.engineSpeedMax >> 8)  & 0x00FF;
	ptrCarState[offset++] = (carAllRecord.engineSpeedMax >> 0)  & 0x00FF;
	//��߳���
	ptrCarState[offset++] = (carAllRecord.carSpeedMax >> 8)  & 0x00FF;
	ptrCarState[offset++] = (carAllRecord.carSpeedMax >> 0)  & 0x00FF;
	//�����
	ptrCarState[offset++] = (carAllRecord.totalMileage >> 24) & 0x000000FF;
	ptrCarState[offset++] = (carAllRecord.totalMileage >> 16) & 0x000000FF;
	ptrCarState[offset++] = (carAllRecord.totalMileage >> 8)  & 0x000000FF;
	ptrCarState[offset++] = (carAllRecord.totalMileage >> 0)  & 0x000000FF;
	//���ͺ�
	ptrCarState[offset++] = (carAllRecord.totalFuel >> 24) & 0x000000FF;
	ptrCarState[offset++] = (carAllRecord.totalFuel >> 16) & 0x000000FF;
	ptrCarState[offset++] = (carAllRecord.totalFuel >> 8)  & 0x000000FF;
	ptrCarState[offset++] = (carAllRecord.totalFuel >> 0)  & 0x000000FF;
	//��Ϣ����
	ptrCarState[offset++] = (carAllRecord.messageNum >> 24) & 0x000000FF;
	ptrCarState[offset++] = (carAllRecord.messageNum >> 16) & 0x000000FF;
	ptrCarState[offset++] = (carAllRecord.messageNum >> 8)  & 0x000000FF;
	ptrCarState[offset++] = (carAllRecord.messageNum >> 0)  & 0x000000FF;
	//��������
	ptrCarState[offset++] = (carAllRecord.netFlow >> 24) & 0x000000FF;
	ptrCarState[offset++] = (carAllRecord.netFlow >> 16) & 0x000000FF;
	ptrCarState[offset++] = (carAllRecord.netFlow >> 8)  & 0x000000FF;
	ptrCarState[offset++] = (carAllRecord.netFlow >> 0)  & 0x000000FF;
	
	OSMutexPend(CDMASendMutex,0,&err);					
	memcpy(&cdmaDataToSend->data[cdmaDataToSend->datLength],ptrCarState,ptrCarState[0]);
	cdmaDataToSend->datLength += ptrCarState[0];
	OSMutexPost(CDMASendMutex);
	
	Mem_free(ptrCarState);	//�ͷ��ڴ��
}



















