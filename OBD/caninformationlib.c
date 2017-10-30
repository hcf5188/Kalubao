#include "obd.h"



//用于初始化的时候CAN测试
 CANInformation CANSearchLib[NUMOFCAN]=
{
	{0x18DA10FA,CAN_ID_EXT,CANBAUD_250K},
	{0x18DA10FA,CAN_ID_EXT,CANBAUD_500K},
	{0x7DF     ,CAN_ID_STD,CANBAUD_250K},
	{0x7DF     ,CAN_ID_STD,CANBAUD_500K},
};

//要采集的PID信息







