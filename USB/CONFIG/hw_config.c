#include "includes.h"
#include "usb_lib.h"
#include "usb_prop.h"
#include "usb_desc.h"
#include "usb_istr.h"
#include "hw_config.h"
#include "usb_pwr.h" 
#include "usart.h"  
#include "string.h"	
#include "stdarg.h"		 
#include "stdio.h"	
//////////////////////////////////////////////////////////////////////////////////	 
//本程序只供学习使用，未经作者许可，不得用于其它任何用途
//ALIENTEK战舰STM32开发板V3
//USB-hw_config 代码	   
//正点原子@ALIENTEK
//技术论坛:www.openedv.com
//创建日期:2015/1/28
//版本：V1.0
//版权所有，盗版必究。
//Copyright(C) 广州市星翼电子科技有限公司 2009-2019
//All rights reserved									  
////////////////////////////////////////////////////////////////////////////////// 
 

u8  USART_PRINTF_Buffer[USB_USART_REC_LEN];	//usb_printf发送缓冲区

//用类似串口1接收数据的方法,来处理USB虚拟串口接收到的数据.
u8 USB_USART_RX_BUF[USB_USART_REC_LEN]; 	//接收缓冲,最大USART_REC_LEN个字节.
//接收状态
//bit15，	接收完成标志
//bit14，	接收到0x0d
//bit13~0，	接收到的有效字节数目
u16 USB_USART_RX_STA=0;       				//接收状态标记	 

extern LINE_CODING linecoding;							//USB虚拟串口配置信息


extern pCIR_QUEUE sendUSB_Q;     //指向 CDMA 串口发送队列  的指针  USB借用
extern pSTORE     receUSB_S;     //指向 CDMA 串口接收数据堆的指针
/////////////////////////////////////////////////////////////////////////////////
//各USB例程通用部分代码,ST各各USB例程,此部分代码都可以共用.
//此部分代码一般不需要修改!

//USB唤醒中断服务函数
void USBWakeUp_IRQHandler(void) 
{
	EXTI_ClearITPendingBit(EXTI_Line18);//清除USB唤醒中断挂起位
} 

//USB中断处理函数
void USB_LP_CAN1_RX0_IRQHandler(void) 
{
	USB_Istr();
} 

//USB时钟配置函数,USBclk=48Mhz@HCLK=72Mhz
void Set_USBClock(void)
{
	RCC_USBCLKConfig(RCC_USBCLKSource_PLLCLK_1Div5);//USBclk=PLLclk/1.5=48Mhz	
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USB, ENABLE);	 //USB时钟使能		 
} 

//USB进入低功耗模式
//当USB进入suspend模式时,MCU进入低功耗模式
//需自行添加低功耗代码(比如关时钟等)
void Enter_LowPowerMode(void)
{
// 	printf("usb enter low power mode\r\n");
	bDeviceState=SUSPENDED;
} 

//USB退出低功耗模式
//用户可以自行添加相关代码(比如重新配置时钟等)
void Leave_LowPowerMode(void)
{
	DEVICE_INFO *pInfo=&Device_Info;
//	printf("leave low power mode\r\n"); 
	if (pInfo->Current_Configuration!=0)bDeviceState=CONFIGURED; 
	else bDeviceState = ATTACHED; 
} 

//USB中断配置
void USB_Interrupts_Config(void)
{ 
	NVIC_InitTypeDef NVIC_InitStructure;
	EXTI_InitTypeDef EXTI_InitStructure;

 
	/* Configure the EXTI line 18 connected internally to the USB IP */
	EXTI_ClearITPendingBit(EXTI_Line18);
											  //  开启线18上的中断
	EXTI_InitStructure.EXTI_Line = EXTI_Line18; // USB resume from suspend mode
	EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising;	//line 18上事件上升降沿触发
	EXTI_InitStructure.EXTI_LineCmd = ENABLE;
	EXTI_Init(&EXTI_InitStructure); 	 

	/* Enable the USB interrupt */
	NVIC_InitStructure.NVIC_IRQChannel = USB_LP_CAN1_RX0_IRQn;	//组2，优先级次之 
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);
	
	/* Enable the USB Wake-up interrupt */
	NVIC_InitStructure.NVIC_IRQChannel = USBWakeUp_IRQn;   //组2，优先级最高	
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
	NVIC_Init(&NVIC_InitStructure);   
}	

//USB接口配置(配置1.5K上拉电阻,战舰V3不需要配置,恒上拉)
//NewState:DISABLE,不上拉
//         ENABLE,上拉
void USB_Cable_Config (FunctionalState NewState)
{ 
//	if (NewState!=DISABLE)printf("usb pull up enable\r\n"); 
//	else printf("usb pull up disable\r\n"); 
}

//USB使能连接/断线
//enable:0,断开
//       1,允许连接	   
void USB_Port_Set(u8 enable)
{
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA,ENABLE);    //使能PORTA时钟		 
	if(enable)_SetCNTR(_GetCNTR()&(~(1<<1)));//退出断电模式
	else
	{	  
		_SetCNTR(_GetCNTR()|(1<<1));  // 断电模式
		GPIOA->CRH&=0XFFF00FFF;
		GPIOA->CRH|=0X00033000;
		PAout(12)=0;	    		  
	}
}  

//获取STM32的唯一ID
//用于USB配置信息
void Get_SerialNum(void)
{
	u32 Device_Serial0, Device_Serial1, Device_Serial2;
	Device_Serial0 = *(u32*)(0x1FFFF7E8);
	Device_Serial1 = *(u32*)(0x1FFFF7EC);
	Device_Serial2 = *(u32*)(0x1FFFF7F0);
	Device_Serial0 += Device_Serial2;
	if (Device_Serial0 != 0)
	{
		IntToUnicode (Device_Serial0,&Virtual_Com_Port_StringSerial[2] , 8);
		IntToUnicode (Device_Serial1,&Virtual_Com_Port_StringSerial[18], 4);
	}
} 

//将32位的值转换成unicode.
//value,要转换的值(32bit)
//pbuf:存储地址
//len:要转换的长度
void IntToUnicode (u32 value , u8 *pbuf , u8 len)
{
	u8 idx = 0;
	for( idx = 0 ; idx < len ; idx ++)
	{
		if( ((value >> 28)) < 0xA )
		{
			pbuf[ 2* idx] = (value >> 28) + '0';
		}
		else
		{
			pbuf[2* idx] = (value >> 28) + 'A' - 10; 
		} 
		value = value << 4; 
		pbuf[ 2* idx + 1] = 0;
	}
}
/////////////////////////////////////////////////////////////////////////////////
 
//USB COM口的配置信息,通过此函数打印出来. 

 
//处理从USB虚拟串口接收到的数据
//databuffer:数据缓存区
//Nb_bytes:接收到的字节数.
uint8_t usbRxTime= 0;
uint16_t usbReceLen = 0;
void USB_To_USART_Send_Data(u8* data_buffer, u8 Nb_bytes)
{ 
	u8 i;
	for(i=0;i<Nb_bytes;i++)
	{  
		Store_Push(receUSB_S,data_buffer[i]);
	} 
	usbRxTime = 1;
	TIM_SetCounter(TIM5,0); //清空计数器
	TIM_Cmd(TIM5, ENABLE);  //使能TIMx	
} 

extern OS_EVENT *USBRecieveQ;//保存从电脑端接收到的USB数据

void TIM5_IRQHandler(void)   //CDMA接收超时处理定时器中断
{
	uint8_t *ptrRece;
	OSIntEnter();//系统进入中断服务程序
	if (TIM_GetITStatus(TIM5, TIM_IT_Update) != RESET)  //检查TIM5更新中断发生与否
	{
		TIM_ClearITPendingBit(TIM5, TIM_IT_Update  );  //清除TIMx更新中断标志 
		usbRxTime ++;
		if(usbRxTime > 2)
		{
			usbRxTime = 0;
			usbReceLen = Store_Getlength(receUSB_S);
			if(usbReceLen>2 )
			{
				ptrRece = Mem_malloc(usbReceLen);
				if(ptrRece != NULL)//内存块申请成功
				{
					Store_Getdates(receUSB_S,ptrRece,usbReceLen);
					if(OSQPost(USBRecieveQ,ptrRece) != OS_ERR_NONE)//推送不成功需要释放内存块
					{
						Mem_free(ptrRece);
						Store_Clear(receUSB_S);//舍弃本次接收的数据
					}
				}
				else
					Store_Clear(receUSB_S);    //舍弃本次接收的数据
			}
			else
				Store_Clear(receUSB_S);//接收的数据长度<=2 视为无效数据，没有这么短的回复
				
			TIM_Cmd(TIM5, DISABLE);
		}
	}
	OSIntExit();  //中断服务结束，系统进行任务调度
}
//发送一个字节数据到USB虚拟串口
extern SYS_OperationVar  varOperation;
void USB_USART_SendDatas(uint8_t* ptrSend,uint16_t datLen)
{
#if OS_CRITICAL_METHOD == 3u           /* Allocate storage for CPU status register           */
	OS_CPU_SR  cpu_sr = 0u;
#endif
	
	if(datLen < 1 || datLen >1020)
		return ;
	OS_ENTER_CRITICAL();
	
	if(CirQ_Pushs(sendUSB_Q,ptrSend,datLen) != OK)
	{
		OS_EXIT_CRITICAL();
	}
	OS_EXIT_CRITICAL();
	
	varOperation.isUSBSendDat = 1;	
}

//usb虚拟串口,printf 函数
//确保一次发送数据不超USB_USART_REC_LEN字节
void usb_printf(char* fmt,...)  
{  
	u16 datLen;
	va_list ap;
#if OS_CRITICAL_METHOD == 3u           /* Allocate storage for CPU status register           */
	OS_CPU_SR  cpu_sr = 0u;
#endif
	va_start(ap,fmt);
	vsprintf((char*)USART_PRINTF_Buffer,fmt,ap);
	va_end(ap);
	
	datLen=strlen((const char*)USART_PRINTF_Buffer);//此次发送数据的长度
	
	OS_ENTER_CRITICAL();
	
	CirQ_OnePush(sendUSB_Q,0x10); //调试信息输出
	
	if(CirQ_Pushs(sendUSB_Q,USART_PRINTF_Buffer,datLen) != OK)
	{
		OS_EXIT_CRITICAL();
	}
	OS_EXIT_CRITICAL();
	varOperation.isUSBSendDat = 1;//通知USB进行数据发送
	
} 






















