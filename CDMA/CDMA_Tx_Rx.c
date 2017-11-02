#include "includes.h"
#include "bsp.h"




uint8_t byteRec = 0;  //用于串口接收的字节

void USART2_IRQHandler(void)
{
	OSIntEnter();//系统进入中断服务程序
	
	
	if(USART_GetITStatus(USART2, USART_IT_RXNE) != RESET)//接收到数据
  	{
    	/* Read one byte from the receive data register */
    	byteRec=USART_ReceiveData(USART2);
//		g_usart_data[u_index].rx_buf[g_usart_data[u_index].rx_last++]=tmp;
//		g_usart_data[u_index].rx_last&=0x01ff;
		USART_ClearITPendingBit(USART2, USART_IT_RXNE) ;
	}
	else if(USART_GetITStatus(USART2, USART_IT_TC) != RESET)//发送完毕
	{
//		if(g_usart_data[u_index].tx_last!=g_usart_data[u_index].tx_used)
//		{
//			tmp=g_usart_data[u_index].tx_buf[g_usart_data[u_index].tx_used++];
//			g_usart_data[u_index].tx_used&=0x01ff;
//			USART_SendData(USART2, tmp);
//		}else
//		{
//			 g_usart_data[u_index].flag_txstop=1;
//		}
		USART_ClearITPendingBit(USART2, USART_IT_TC) ;
	}
	else if(USART_GetITStatus(USART2, USART_IT_TXE) != RESET)//空
	{
//		if(g_usart_data[u_index].tx_last!=g_usart_data[u_index].tx_used)
//		{
//			tmp=g_usart_data[u_index].tx_buf[g_usart_data[u_index].tx_used++];
//			g_usart_data[u_index].tx_used&=0x01ff;
//			USART_SendData(USART2, tmp);
//		}
//		else
//		{
//			 g_usart_data[u_index].flag_txstop=1;
//		}
		USART_ITConfig(USART2, USART_IT_TXE, DISABLE);
		USART_ClearITPendingBit(USART2, USART_IT_TXE) ;
	}
	
	
	OSIntExit();  //中断服务结束，系统进行任务调度
}
















