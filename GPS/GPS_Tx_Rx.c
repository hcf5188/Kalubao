#include "bsp.h"

extern pCIR_QUEUE sendGPS_Q ;     //指向 GPS 串口发送队列  的指针
extern pSTORE     receGPS_S ;     //指向 GPS 串口接收数据堆的指针

uint8_t GPSSendDatas(const uint8_t* s,uint8_t length)
{
#if OS_CRITICAL_METHOD == 3u           /* Allocate storage for CPU status register           */
	OS_CPU_SR  cpu_sr = 0u;
#endif
//	uint8_t data;
	if(length < 1 || length >250)
		return 1;
	OS_ENTER_CRITICAL();
	if(CirQ_Pushs(sendGPS_Q,s,length) != OK)
	{
		OS_EXIT_CRITICAL();
		return 2;
	}
	OS_EXIT_CRITICAL();
	
	if(CirQ_GetLength(sendGPS_Q) > 0)
		USART_ITConfig(UART4, USART_IT_TXE, ENABLE);

	return  0;
}

uint8_t byteGPSRece  = 0;
uint8_t byteGPSSend  = 0;
uint8_t rxGPSTimeOut = 0;
void UART4_IRQHandler(void)
{
	NORMAL_STATE rxtxState;
	OSIntEnter();//系统进入中断服务程序
	if(USART_GetITStatus(UART4, USART_IT_RXNE) != RESET)//接收到数据
  	{
    	byteGPSRece = USART_ReceiveData(UART4);
	
		rxtxState = Store_Push(receGPS_S,byteGPSRece);   
		if(rxtxState == OK)
		{
			rxGPSTimeOut = 1;
			TIM_SetCounter(TIM2,0); //清空计数器
			TIM_Cmd(TIM2, ENABLE);  //使能TIMx
		}
		USART_ClearITPendingBit(UART4, USART_IT_RXNE) ;
	}
	else if(USART_GetITStatus(UART4, USART_IT_TC) != RESET)//发送完毕  移位寄存器空
	{
		if(CirQ_GetLength(sendGPS_Q) > 0)
		{
			rxtxState = CirQ_Pop(sendGPS_Q,&byteGPSSend);
			if(rxtxState == OK)
				USART_SendData(UART4, byteGPSSend);
		}
		else
		{
			//todo:通知任务发送完成，可以发送下一包数据了
		}
		USART_ClearITPendingBit(UART4, USART_IT_TC) ;
	}
	else if(USART_GetITStatus(UART4, USART_IT_TXE) != RESET)//空  发送寄存器空
	{
		if(CirQ_GetLength(sendGPS_Q) > 0)
		{
			rxtxState = CirQ_Pop(sendGPS_Q,&byteGPSSend);
			if(rxtxState == OK)
				USART_SendData(UART4, byteGPSSend);
		}
		else
		{
			//todo:通知任务发送完成，可以发送下一包数据了
		}
		USART_ITConfig(UART4, USART_IT_TXE, DISABLE);
		USART_ClearITPendingBit(UART4, USART_IT_TXE) ;
	}
	
	OSIntExit();  //中断服务结束，系统进行任务调度
}

extern OS_EVENT* receGPSQ;

static uint16_t receGPSLen = 0;
uint8_t *ptrGPSRece = NULL;
void TIM2_IRQHandler(void)
{
	OSIntEnter();//系统进入中断服务程序
	
	if (TIM_GetITStatus(TIM2, TIM_IT_Update) != RESET)  //检查TIM2更新中断发生与否
	{
		TIM_ClearITPendingBit(TIM2, TIM_IT_Update  );  //清除TIMx更新中断标志 
		rxGPSTimeOut ++;
		if(rxGPSTimeOut > 10)//10ms 接收超时
		{
			rxGPSTimeOut = 0;
			receGPSLen = Store_Getlength(receGPS_S);
			
			if(receGPSLen>2 )
			{
				ptrGPSRece = Mem_malloc(receGPSLen+2);
				if(ptrGPSRece != NULL)
				{
					ptrGPSRece[0] = (receGPSLen>>8)&0xff;
					ptrGPSRece[1] = receGPSLen&0xff;
					Store_Getdates(receGPS_S,&ptrGPSRece[2],receGPSLen);
					if(OSQPost(receGPSQ,ptrGPSRece)!=OS_ERR_NONE)
						Mem_free(ptrGPSRece);
				}
				else
					Store_Clear(receGPS_S);
			}
			else
				Store_Clear(receGPS_S);
			TIM_Cmd(TIM2, DISABLE);
		}
	}
	OSIntExit();  //中断服务结束，系统进行任务调度
}

