#include "bsp.h"
#include "obd.h"









void KLineFastInit(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	
	//TX IO Output
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_KWP_TX;
  	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
  	GPIO_Init(GPIOB, &GPIO_InitStructure);

	//T Idle ����300ms
	GPIO_SetBits(GPIOB,GPIO_Pin_KWP_TX);
	OSTimeDlyHMSM(0,0,0,500);

	// ����25ms
	GPIO_ResetBits(GPIOB,GPIO_Pin_KWP_TX);
	OSTimeDlyHMSM(0,0,0,25);

	//����25ms
	GPIO_SetBits(GPIOB,GPIO_Pin_KWP_TX);
	OSTimeDlyHMSM(0,0,0,25);
	//USART3  TX תΪ�շ���
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_KWP_TX;
  	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
  	GPIO_Init(GPIOB, &GPIO_InitStructure);
}

void USART3_IRQHandler(void)
{
	uint8_t tmp;
	if(USART_GetITStatus(USART3, USART_IT_RXNE) != RESET)//���յ�����
  	{
		/* Read one byte from the receive data register */
		tmp=USART_ReceiveData(USART3);

		//���պ�����ݴ�����----�����ع�

		USART_ClearITPendingBit(USART3, USART_IT_RXNE) ;

	}
	else if(USART_GetITStatus(USART3, USART_IT_TC) != RESET)//�������
	{
//		if(g_usart_data[u_index].tx_last!=g_usart_data[u_index].tx_used)
//		{
//			tmp=g_usart_data[u_index].tx_buf[g_usart_data[u_index].tx_used++];
//			g_usart_data[u_index].tx_used&=0x01ff;
			USART_SendData(USART3, tmp);
//		}else
//		{
//			 g_usart_data[u_index].flag_txstop=1;
//		}

		USART_ClearITPendingBit(USART3, USART_IT_TC) ;
	}
	else if(USART_GetITStatus(USART3, USART_IT_TXE) != RESET)//��
	{
//		if(g_usart_data[u_index].tx_last!=g_usart_data[u_index].tx_used)
//		{
//			tmp=g_usart_data[u_index].tx_buf[g_usart_data[u_index].tx_used++];
//			g_usart_data[u_index].tx_used&=0x01ff;
			USART_SendData(USART3, tmp);
//		}
//		else
//		{
//			 g_usart_data[u_index].flag_txstop=1;
//		}
		USART_ITConfig(USART3, USART_IT_TXE, DISABLE);
		USART_ClearITPendingBit(USART3, USART_IT_TXE) ;
	}
}







