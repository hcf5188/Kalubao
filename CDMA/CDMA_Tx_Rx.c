#include "includes.h"
#include "bsp.h"


extern pCIR_QUEUE sendCDMA_Q;//指向 CDMA 串口发送队列  的指针
extern pSTORE     receCDMA_S;//指向 CDMA 串口接收数据堆的指针

void CDMASendByte(uint8_t dat)
{
#if OS_CRITICAL_METHOD == 3u           /* Allocate storage for CPU status register           */
	OS_CPU_SR  cpu_sr = 0u;
#endif
	if(CirQ_GetLength(sendCDMA_Q)>0)
	{
		OS_ENTER_CRITICAL();
		CirQ_OnePush(sendCDMA_Q,dat);
		OS_EXIT_CRITICAL();
	}
	else
		USART_SendData(USART2, dat);
}
uint8_t CDMASendDatas(const uint8_t* s,uint16_t length)
{
#if OS_CRITICAL_METHOD == 3u           /* Allocate storage for CPU status register           */
	OS_CPU_SR  cpu_sr = 0u;
#endif
//	uint8_t data;
	if(length < 1 || length >1020)
		return 1;
	OS_ENTER_CRITICAL();
	if(CirQ_Pushs(sendCDMA_Q,s,length) != OK)
	{
		OS_EXIT_CRITICAL();
		return 2;
	}
	OS_EXIT_CRITICAL();
	if(CirQ_GetLength(sendCDMA_Q) > 0)
	{
//		OS_ENTER_CRITICAL();
//		CirQ_Pop(sendCDMA_Q,&data);
//		OS_EXIT_CRITICAL();
		
		USART_ITConfig(USART2, USART_IT_TXE, ENABLE);
//		USART_SendData(USART2, data);
	}
	return  0;
	
	
}



uint8_t byteRece = 0;  //用于串口接收的字节
uint8_t byteSend = 0;  //用于串口发送的字节 

void USART2_IRQHandler(void)
{
	OSIntEnter();//系统进入中断服务程序
	
	if(USART_GetITStatus(USART2, USART_IT_RXNE) != RESET)//接收到数据
  	{
    	
    	byteRece = USART_ReceiveData(USART2);
		Store_Push(receCDMA_S,byteRece);//todo:定时器超时相关  以及
		                              //判断数据包结束 通知接收任务接收完成
		
		USART_ClearITPendingBit(USART2, USART_IT_RXNE) ;
	}
	else if(USART_GetITStatus(USART2, USART_IT_TC) != RESET)//发送完毕  移位寄存器空
	{
		if(CirQ_GetLength(sendCDMA_Q) > 0)
		{
			CirQ_Pop(sendCDMA_Q,&byteSend);
			USART_SendData(USART2, byteSend);
		}
		else
		{
			//todo:通知任务发送完成，可以发送下一包数据了
		}
		USART_ClearITPendingBit(USART2, USART_IT_TC) ;
	}
	else if(USART_GetITStatus(USART2, USART_IT_TXE) != RESET)//空  发送寄存器空
	{
		if(CirQ_GetLength(sendCDMA_Q) > 0)
		{
			CirQ_Pop(sendCDMA_Q,&byteSend);
			USART_SendData(USART2, byteSend);
		}
		else
		{
			 //todo:通知任务发送完成，可以发送下一包数据了
		}
		USART_ITConfig(USART2, USART_IT_TXE, DISABLE);
		USART_ClearITPendingBit(USART2, USART_IT_TXE) ;
	}
	
	
	OSIntExit();  //中断服务结束，系统进行任务调度
}
















