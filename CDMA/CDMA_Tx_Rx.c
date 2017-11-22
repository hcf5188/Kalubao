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
		USART_ITConfig(USART2, USART_IT_TXE, ENABLE);
	}
	return  0;
}

uint8_t byteRece = 0;  //用于串口接收的字节
uint8_t byteSend = 0;  //用于串口发送的字节 
uint8_t *dkfs;
static uint16_t rxTimeOut = 0;//1-接收超时   0 - 正在接收
NORMAL_STATE rxtxState;
void USART2_IRQHandler(void)
{
	OSIntEnter();//系统进入中断服务程序
	dkfs = receCDMA_S->base;
	if(USART_GetITStatus(USART2, USART_IT_RXNE) != RESET)//接收到数据
  	{
    	byteRece = USART_ReceiveData(USART2);
	
		rxtxState = Store_Push(receCDMA_S,byteRece);   
		if(rxtxState == OK)
		{
			rxTimeOut = 1;
			TIM_SetCounter(TIM4,0); //清空计数器
			TIM_Cmd(TIM4, ENABLE);  //使能TIMx
		}
		USART_ClearITPendingBit(USART2, USART_IT_RXNE) ;
	}
	else if(USART_GetITStatus(USART2, USART_IT_TC) != RESET)//发送完毕  移位寄存器空
	{
		if(CirQ_GetLength(sendCDMA_Q) > 0)
		{
			rxtxState = CirQ_Pop(sendCDMA_Q,&byteSend);
			if(rxtxState == OK)
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
			rxtxState = CirQ_Pop(sendCDMA_Q,&byteSend);
			if(rxtxState == OK)
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


extern OS_EVENT *ZIPRecv_Q;             //指向RECV消息队列的指针
extern OS_EVENT *CDMARecieveQ;
uint8_t *ptrRece;
uint16_t receDatalen= 0;
char* pCmp = NULL;
uint8_t adf[3000];
uint16_t offadf = 0;
void TIM4_IRQHandler(void)   //CDMA接收超时处理定时器中断
{
	OSIntEnter();//系统进入中断服务程序
	if (TIM_GetITStatus(TIM4, TIM_IT_Update) != RESET)  //检查TIM4更新中断发生与否
	{
		TIM_ClearITPendingBit(TIM4, TIM_IT_Update  );  //清除TIMx更新中断标志 
		rxTimeOut ++;
		if(rxTimeOut > 2)
		{
			rxTimeOut = 0;
			receDatalen = Store_Getlength(receCDMA_S);
			if(receDatalen>2 )
			{
				ptrRece = Mem_malloc(receDatalen);
				if(ptrRece != NULL)//内存块申请成功
				{
					Store_Getdates(receCDMA_S,ptrRece,receDatalen);
					pCmp = strstr((const char*)ptrRece,"ZIPRECV");  //如果接收到服务器主动下发的消息则发送给专门处理函数的消息队列
					if(pCmp == NULL)
					{
						if(OSQPost(CDMARecieveQ,ptrRece) != OS_ERR_NONE)//推送不成功需要释放内存块
						{
							Mem_free(ptrRece);
							Store_Clear(receCDMA_S);//舍弃本次接收的数据
						}
					}
					else
					{
						memcpy(adf,ptrRece,receDatalen);
						offadf += receDatalen;
						if(OSQPost(ZIPRecv_Q,ptrRece) != OS_ERR_NONE)// 推送给专门处理服务器下发的数据的消息队列
						{
							Mem_free(ptrRece);
							Store_Clear(receCDMA_S);//舍弃本次接收的数据
						}
					}
				}
				else
					Store_Clear(receCDMA_S);    //舍弃本次接收的数据
			}
			else
				Store_Clear(receCDMA_S);//接收的数据长度<=2 视为无效数据，没有这么短的回复
				
			TIM_Cmd(TIM4, DISABLE);
		}
	}
	OSIntExit();  //中断服务结束，系统进行任务调度
}













