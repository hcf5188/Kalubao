#include "obd.h"



//���ڳ�ʼ����ʱ��CAN����
 CANInformation CANSearchLib[NUMOFCAN]=
{
	{0x18DA10FA,CAN_ID_EXT,CANBAUD_250K},
	{0x18DA10FA,CAN_ID_EXT,CANBAUD_500K},
	{0x7DF     ,CAN_ID_STD,CANBAUD_250K},
	{0x7DF     ,CAN_ID_STD,CANBAUD_500K},
};

//Ҫ�ɼ���PID��Ϣ







