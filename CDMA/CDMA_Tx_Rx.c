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
uint8_t *dkfs;
static uint16_t rxTimeOut = 0;//1-���ճ�ʱ   0 - ���ڽ���
NORMAL_STATE rxtxState;
void USART2_IRQHandler(void)
{
	OSIntEnter();//ϵͳ�����жϷ������
	dkfs = receCDMA_S->base;
	if(USART_GetITStatus(USART2, USART_IT_RXNE) != RESET)//���յ�����
  	{
    	byteRece = USART_ReceiveData(USART2);
	
		rxtxState = Store_Push(receCDMA_S,byteRece);   
		if(rxtxState == OK)
		{
			rxTimeOut = 1;
			TIM_Cmd(TIM3, ENABLE);  //ʹ��TIMx
		}
		USART_ClearITPendingBit(USART2, USART_IT_RXNE) ;
	}
	else if(USART_GetITStatus(USART2, USART_IT_TC) != RESET)//�������  ��λ�Ĵ�����
	{
		if(CirQ_GetLength(sendCDMA_Q) > 0)
		{
			rxtxState = CirQ_Pop(sendCDMA_Q,&byteSend);
			if(rxtxState == OK)
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
			rxtxState = CirQ_Pop(sendCDMA_Q,&byteSend);
			if(rxtxState == OK)
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
extern OS_EVENT *CDMARecieveQ;
uint8_t *ptrRece;
uint16_t receDatalen= 0;
void TIM3_IRQHandler(void)   //TIM3�ж�
{
	if (TIM_GetITStatus(TIM3, TIM_IT_Update) != RESET)  //���TIM3�����жϷ������
	{
		TIM_ClearITPendingBit(TIM3, TIM_IT_Update  );  //���TIMx�����жϱ�־ 
		rxTimeOut ++;
		if(rxTimeOut > 10)
		{
			rxTimeOut = 0;
			receDatalen = Store_Getlength(receCDMA_S);
			if(receDatalen>2 )
			{
				ptrRece = Mem_malloc(receDatalen);
				Store_Getdates(receCDMA_S,ptrRece,receDatalen);
				OSQPost(CDMARecieveQ,ptrRece);
			}
			else
				Store_Clear(receCDMA_S);
			
			TIM_Cmd(TIM3, DISABLE);
		}
	
	}
}













