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
	uint8_t  timeTemp  = 0;
	uint8_t  locaTemp  = 0;
	uint32_t timeCount = 0;//������ ���� - ֹͣ ʱ��
	
	
	while(1)
	{
		timeCount = 0;
		if(((timeTemp & 0x01) == 0) && (carAllRecord.engineSpeed > 100))//����������
		{
			timeTemp |= 0x01;
			timeTemp &= 0xFD;
			carAllRecord.startTime = RTC_GetCounter();
		}//������ֹͣ
		if((carAllRecord.engineSpeed < 100) && ((timeTemp & 0x02) == 0))
		{
			timeTemp |= 0x02;
			timeTemp &= 0xFE;
			carAllRecord.stopTime = RTC_GetCounter();
			timeCount = carAllRecord.stopTime>carAllRecord.startTime?carAllRecord.stopTime-carAllRecord.startTime:0;
		}//��ʼ��γ��
		if(((locaTemp & 0x01) == 0)&&(gpsMC.longitude != 0)&&((timeTemp & 0x01) != 0))
		{
			locaTemp |= 0x01;
			carAllRecord.startlongitude = gpsMC.longitude;
			carAllRecord.startlatitude  = gpsMC.latitude;
		}//��ͣ��ľ�γ��
		if((gpsMC.longitude != 0)&&((timeTemp & 0x02) != 0))
		{
			locaTemp &= 0xFE;
			carAllRecord.stoplongitude = gpsMC.longitude;
			carAllRecord.stoplatitude  = gpsMC.latitude;
		}
		if(timeCount > 60)    //��������ͣ��1����
			RunDataReport();
		
		SpeedPlusSubCompute();//����Ӽ��١����г�
		OSTimeDlyHMSM(0,0,0,20);
	}
}


void SpeedPlusSubCompute(void)//�Ӽ��ٴ�������
{
	static float speedRecord = 0;
	static uint8_t time  = 0;
	static uint16_t ensureOne = 0;//��֤һ�μӼ��ٵĹ����У����������ۼ�
	float speedCount;
	time ++;
	if(time < 5)
		return;
	time = 0;
	speedCount = carAllRecord.carSpeed > speedRecord ? carAllRecord.carSpeed - speedRecord : speedRecord - carAllRecord.carSpeed;
	if(speedCount > 10)//todo����� �ο��Ƚ�ֵ �д�ȷ��
	{
		if(ensureOne == 0)
		{
			ensureOne = 1;
			if(carAllRecord.carSpeed > speedRecord)
				carAllRecord.rapidlyPlusNum ++;
			else
				carAllRecord.rapidlySubNum ++;
		}
	}else ensureOne = 0;
	speedRecord = carAllRecord.carSpeed;
	
	carAllRecord.totalMileage += carAllRecord.carSpeed;//�������г�
}
//���������ϱ�
void RunDataReport(void )
{
	uint8_t* ptrCarState;
	uint8_t offset  = 0,err;
	
	ptrCarState = Mem_malloc(sizeof(CARRunRecord) + 10);
	if(ptrCarState == NULL)
	{
		LogReport("MEM Error! %d not enough!",sizeof(CARRunRecord) + 10);
		return;
	}
	ptrCarState[offset++] = 49;//�г���Ϣ�ϱ�
	ptrCarState[offset++] = 0x50;
	ptrCarState[offset++] = 0x06;
	//�г̿�ʼʱ��
	ptrCarState[offset++] = (carAllRecord.startTime >> 24) &0x000000FF;
	ptrCarState[offset++] = (carAllRecord.startTime >> 16) &0x000000FF;
	ptrCarState[offset++] = (carAllRecord.startTime >> 8)  &0x000000FF;
	ptrCarState[offset++] = (carAllRecord.startTime >> 0)  &0x000000FF;
	//�г̽�ֹʱ��
	ptrCarState[offset++] = (carAllRecord.stopTime >> 24) &0x000000FF;
	ptrCarState[offset++] = (carAllRecord.stopTime >> 16) &0x000000FF;
	ptrCarState[offset++] = (carAllRecord.stopTime >> 8)  &0x000000FF;
	ptrCarState[offset++] = (carAllRecord.stopTime >> 0)  &0x000000FF;
	//�����
	ptrCarState[offset++] = (carAllRecord.totalMileage >> 24) &0x000000FF;
	ptrCarState[offset++] = (carAllRecord.totalMileage >> 16) &0x000000FF;
	ptrCarState[offset++] = (carAllRecord.totalMileage >> 8)  &0x000000FF;
	ptrCarState[offset++] = (carAllRecord.totalMileage >> 0)  &0x000000FF;
	//���ͺ�
	ptrCarState[offset++] = (carAllRecord.totalFuel >> 24) &0x000000FF;
	ptrCarState[offset++] = (carAllRecord.totalFuel >> 16) &0x000000FF;
	ptrCarState[offset++] = (carAllRecord.totalFuel >> 8)  &0x000000FF;
	ptrCarState[offset++] = (carAllRecord.totalFuel >> 0)  &0x000000FF;
	//��ʼ����
	ptrCarState[offset++] = (carAllRecord.startlongitude >> 24) &0x000000FF;
	ptrCarState[offset++] = (carAllRecord.startlongitude >> 16) &0x000000FF;
	ptrCarState[offset++] = (carAllRecord.startlongitude >> 8)  &0x000000FF;
	ptrCarState[offset++] = (carAllRecord.startlongitude >> 0)  &0x000000FF;
	//��ʼγ��
	ptrCarState[offset++] = (carAllRecord.startlatitude >> 24) &0x000000FF;
	ptrCarState[offset++] = (carAllRecord.startlatitude >> 16) &0x000000FF;
	ptrCarState[offset++] = (carAllRecord.startlatitude >> 8)  &0x000000FF;
	ptrCarState[offset++] = (carAllRecord.startlatitude >> 0)  &0x000000FF;
	//��������
	ptrCarState[offset++] = (carAllRecord.stoplongitude >> 24) &0x000000FF;
	ptrCarState[offset++] = (carAllRecord.stoplongitude >> 16) &0x000000FF;
	ptrCarState[offset++] = (carAllRecord.stoplongitude >> 8)  &0x000000FF;
	ptrCarState[offset++] = (carAllRecord.stoplongitude >> 0)  &0x000000FF;
	//����γ��
	ptrCarState[offset++] = (carAllRecord.stoplatitude >> 24) &0x000000FF;
	ptrCarState[offset++] = (carAllRecord.stoplatitude >> 16) &0x000000FF;
	ptrCarState[offset++] = (carAllRecord.stoplatitude >> 8)  &0x000000FF;
	ptrCarState[offset++] = (carAllRecord.stoplatitude >> 0)  &0x000000FF;
	//�����ٴ���
	ptrCarState[offset++] = carAllRecord.rapidlyPlusNum  &0x000000FF;
	//�����ٴ���
	ptrCarState[offset++] = carAllRecord.rapidlySubNum  &0x000000FF;
	//���ת��
	ptrCarState[offset++] = (carAllRecord.engineSpeedMax >> 8)  &0x00FF;
	ptrCarState[offset++] = (carAllRecord.engineSpeedMax >> 0)  &0x00FF;
	//��߳���
	ptrCarState[offset++] = (carAllRecord.carSpeedMax >> 8)  &0x00FF;
	ptrCarState[offset++] = (carAllRecord.carSpeedMax >> 0)  &0x00FF;
	//��Ϣ����
	ptrCarState[offset++] = (carAllRecord.messageNum >> 24) &0x000000FF;
	ptrCarState[offset++] = (carAllRecord.messageNum >> 16) &0x000000FF;
	ptrCarState[offset++] = (carAllRecord.messageNum >> 8)  &0x000000FF;
	ptrCarState[offset++] = (carAllRecord.messageNum >> 0)  &0x000000FF;
	//��������
	ptrCarState[offset++] = (carAllRecord.netFlow >> 24) &0x000000FF;
	ptrCarState[offset++] = (carAllRecord.netFlow >> 16) &0x000000FF;
	ptrCarState[offset++] = (carAllRecord.netFlow >> 8)  &0x000000FF;
	ptrCarState[offset++] = (carAllRecord.netFlow >> 0)  &0x000000FF;
	
	OSMutexPend(CDMASendMutex,0,&err);					
	memcpy(&cdmaDataToSend->data[cdmaDataToSend->datLength],ptrCarState,ptrCarState[0]);
	cdmaDataToSend->datLength += ptrCarState[0];
	OSMutexPost(CDMASendMutex);
	
	Mem_free(ptrCarState);	
}



















