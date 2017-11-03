#include "includes.h"
#include "bsp.h"


extern pCIR_QUEUE sendCDMA_Q;//ָ�� CDMA ���ڷ��Ͷ���  ��ָ��
extern pSTORE     receCDMA_S;//ָ�� CDMA ���ڽ������ݶѵ�ָ��

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



uint8_t byteRece = 0;  //���ڴ��ڽ��յ��ֽ�
uint8_t byteSend = 0;  //���ڴ��ڷ��͵��ֽ� 

void USART2_IRQHandler(void)
{
	OSIntEnter();//ϵͳ�����жϷ������
	
	if(USART_GetITStatus(USART2, USART_IT_RXNE) != RESET)//���յ�����
  	{
    	
    	byteRece = USART_ReceiveData(USART2);
		Store_Push(receCDMA_S,byteRece);//todo:��ʱ����ʱ���  �Լ�
		                              //�ж����ݰ����� ֪ͨ��������������
		
		USART_ClearITPendingBit(USART2, USART_IT_RXNE) ;
	}
	else if(USART_GetITStatus(USART2, USART_IT_TC) != RESET)//�������  ��λ�Ĵ�����
	{
		if(CirQ_GetLength(sendCDMA_Q) > 0)
		{
			CirQ_Pop(sendCDMA_Q,&byteSend);
			USART_SendData(USART2, byteSend);
		}
		else
		{
			//todo:֪ͨ��������ɣ����Է�����һ��������
		}
		USART_ClearITPendingBit(USART2, USART_IT_TC) ;
	}
	else if(USART_GetITStatus(USART2, USART_IT_TXE) != RESET)//��  ���ͼĴ�����
	{
		if(CirQ_GetLength(sendCDMA_Q) > 0)
		{
			CirQ_Pop(sendCDMA_Q,&byteSend);
			USART_SendData(USART2, byteSend);
		}
		else
		{
			 //todo:֪ͨ��������ɣ����Է�����һ��������
		}
		USART_ITConfig(USART2, USART_IT_TXE, DISABLE);
		USART_ClearITPendingBit(USART2, USART_IT_TXE) ;
	}
	
	
	OSIntExit();  //�жϷ��������ϵͳ�����������
}
















