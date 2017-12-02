#include "apptask.h"
#include "obd.h"
#include "bsp.h"

uint8_t a = 0;
void CANBaudCheckTask (void *pdata)
{
	
	while(1)
	{
		CAN1_BaudSet(CANBAUD_1M);
		a =1;
		OSTimeDlyHMSM(0,0,2,0);
		
		CAN1_BaudSet(CANBAUD_500K);
		a=2;
		OSTimeDlyHMSM(0,0,2,0);
		
		CAN1_BaudSet(CANBAUD_250K);
		a=3;
		OSTimeDlyHMSM(0,0,2,0);
	}
}































































////判断是否进行波特率切换和超时退出
//void set_change_state_erred(uint8 canid)
//{
//	LPC_CAN_TypeDef* pCan;

//	pCan = CAN_GetPointer(canid);

//	//主动错误或者总线上检测到错误时，表示数据发送失败
//	//"主动错误"应定义为：CANxGSR中的ES位为1，并且发送出错计数器>＝128
//	//总线错误定义为:CANxICR中的BEI位为1，即是检测到总线错误中断被置位
//	if(((pCan->GSR & CAN_GSR_ES)&&((pCan->GSR & 0xff000000) >=128))	|| (pCan->ICR & CAN_ICR_BEI))
//	{  
//		//遍历一次波特率表，如果没有协商到说明没有波特率与之匹配
//		//强制设置波特率为最大值1000K
//		//同时立即退出波特率协商，不必等待超时
//		if(try_num[canid] == 16)
//		{
//			try_num[canid]  = 0;
//			CAN_setting[canid].baudrate  = set_baudrate[try_num[canid]];
//			auto_csd_start(canid);
//			baudrate_change_state[canid] = 0;
//			return ;
//		}
//		//波特率相同不必重复检测
//		if(CAN_setting[canid].baudrate ==set_baudrate[try_num[canid]])
//		{
//			try_num[canid]++;
//		}
//		//切换到位速率表中下一个
//		CAN_setting[canid].baudrate = set_baudrate[try_num[canid]];
//		auto_csd_start(canid);
//		send_data_state[canid] = 0;
//		try_num[canid]++;
//	}
//	//设置超时时间1s
//	//如果时间到还没有出现错误，则证明波特率正确，
//	//则停止本次波特率的协商
//	else if( baudrate_time_out[canid] >= 1000)
//	{
//		baudrate_change_state[canid] = 0;
//	}
//}
// 
// 
////每次发送一帧测试数据
//void send_test_data(uint8 canid)
//{
//	if(send_data_state[canid] == 0)
//	{
//		my_can_write(&CAN_port[canid],(tCAN_frame *)test_arry,1); 
//		baudrate_time_out[canid] = 1;
//		send_data_state[canid] = 1;
//	}  
//}
////主函数调用
//void auto_baudrate_set(void)
//{
//	uint8 i;
//	for(i=0;i<MAX_CAN_NUM;i++)
//	{
//		if(baudrate_change_state[i]== 1)
//		{
//			send_test_data(i);
//			set_change_state_erred(i);
//		}
//	}
//}

////自动波特率检测时，设置波特率初值，与网页配置无关
//void auto_baudrate_init(void)
//{
//	uint8 i;
//	//初始化两路CAN的波特率均为1000K
//	//这个值与网页配置无关，并不保存在flash中
//	for(i=0;i<MAX_CAN_NUM;i++)
//	{
//		CAN_setting[i].baudrate = set_baudrate[0];   //1000K
//	}
//}
//void get_pin_state(void)
//{
//	uint8 i;
//	//获取引脚状态，当为低时，启动波特率协商
//	if(GPIO_ReadValue(UP_APP_PORT)&UP_APP_MASK)
//	{
//		if(pin_state != 0)
//		{
//			pin_state = 0;
//		}
//		return ;
//	}
//	//添加pin_state的状态获得，主要是用于当客户设备的波特率改变时
//	//可以通过控制引脚的上拉和下拉来随时启动波特率改变

//	//主要方式为:当客户设备波特率设置好之后，拉低引脚来启动波特率改变
//	//第一次协商好之后，如果客户设备的波特率改变，则需要将引脚拉高，然后再拉低
//	//来进行波特率的改变
//	else
//	{
//		if(pin_state == 0)
//		{
//			for(i=0;i<MAX_CAN_NUM;i++)
//			{    
//				try_num[i] = 0;
//				send_data_state[i] = 0;
//				baudrate_change_state[i] = 1;
//				auto_csd_start(i);
//			}
//			pin_state = 1;
//		}
//		else
//			return ;
//	}
//}
















