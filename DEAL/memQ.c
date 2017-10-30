/************************************************************
*��˾����:       ����GQY��Ѷ�ɷ����޹�˾
*�ļ�����: mem_Q.c
*��������: HongCf
*�ļ�����: ѭ�����в���
*��д����: 2017-4-30
*************************************************************/

#include "memblock.h"


/****************************************************************
*		pCIR_QUEUE Cir_Queue_Init(unsigned int length)
* ��	�� : ��ʼ��ѭ������
* ������� : ѭ�����еĳ���
* �� �� ֵ : pCIR_QUEUE���ͣ�ָ��ѭ�����е�ָ��
****************************************************************/
pCIR_QUEUE Cir_Queue_Init(unsigned int length)
{
	pCIR_QUEUE ptr_Q = NULL;
	
	if(length < 2)
		return NULL;
	if((ptr_Q = (pCIR_QUEUE)Mem_malloc(sizeof(CIR_QUEUE))) != NULL)
	{
		if((ptr_Q->data = (uint8_t *)Mem_malloc(length)) != NULL)
		{
			ptr_Q->front  = 0;
			ptr_Q->rear   = 0;
			ptr_Q->length = length;
			return ptr_Q;
		}
		else 
			Mem_free(ptr_Q);
	}
	return NULL;
}
/****************************************************************
*		NORMAL_STATE  CirQ_Delete(pCIR_QUEUE ptr_Q)
* ��	�� : �ͷ�ѭ������		 		
* ������� : ָ��ѭ�����е�ָ��
* �� �� ֵ : ö��NORMAL_STATE���͡�
****************************************************************/
NORMAL_STATE  CirQ_Delete(pCIR_QUEUE ptr_Q)
{
	if(ptr_Q->data == NULL)
		return FAULT;
	
	if(Mem_free(ptr_Q->data) != 0)
		return FAULT;
	
	if(Mem_free(ptr_Q) != 0)
		return FAULT;
	return OK;
}
/****************************************************************
*		NORMAL_STATE  CirQ_Clear(pCIR_QUEUE ptr_Q)
* ��	�� : ���ѭ������		 		
* ������� : ָ��ѭ�����е�ָ��
* �� �� ֵ : ö��NORMAL_STATE���͡�
****************************************************************/
NORMAL_STATE  CirQ_Clear(pCIR_QUEUE ptr_Q)
{
	if(ptr_Q->data == NULL)  return FAULT;
	
	ptr_Q->front  = 0;
	ptr_Q->rear   = 0;
	ptr_Q->count  = 0;
	
	return OK;
}
/****************************************************************
*		unsigned short int CirQ_GetLength(pCIR_QUEUE ptr_Q)
* ��	�� : �õ���ѭ�����б���ֵ����	 		
* ������� : ָ��ѭ�����е�ָ��
* �� �� ֵ : ����ֵ������count��
****************************************************************/
unsigned short int CirQ_GetLength(pCIR_QUEUE ptr_Q)
{
	if(ptr_Q->data == NULL) return 0;
	
	return ptr_Q->count;
}
/****************************************************************
*   NORMAL_STATE CirQ_Pop(pCIR_QUEUE ptr_Q,unsigned char *phead)
* ��	�� : �Ӷ����е���һ����ֵ--�Ƚ��ȳ� 		
* ������� : ָ��ѭ�����е�ָ�룬�����һ��ֵ��ַ
* �� �� ֵ : ö��NORMAL_STATE���͡�
****************************************************************/
NORMAL_STATE CirQ_Pop(pCIR_QUEUE ptr_Q,unsigned char *phead)
{
	if(ptr_Q->data == NULL)  return FAULT;
	if(ptr_Q->count == 0)    return EMPTY;
	
	*phead = ptr_Q->data[ptr_Q->front];
	ptr_Q->front ++;
	if(ptr_Q->front == ptr_Q->length)
		ptr_Q->front = 0;
	ptr_Q->count --;
	return OK;
}
/****************************************************************
*   NORMAL_STATE CirQ_OnePush(pCIR_QUEUE ptr_Q,unsigned char dat)
* ��	�� : �������ѹ��һ��ֵ		
* ������� : ָ��ѭ�����е�ָ�룬Ҫѹ���ֵ
* �� �� ֵ : ö��NORMAL_STATE���͡�
****************************************************************/
NORMAL_STATE CirQ_OnePush(pCIR_QUEUE ptr_Q,unsigned char dat)
{
	if(ptr_Q->data == NULL)           return FAULT;
	if(ptr_Q->count == ptr_Q->length) return OVERFLOW;
	
	ptr_Q->data[ptr_Q->rear] = dat;
	ptr_Q->rear ++;
	if(ptr_Q->rear == ptr_Q->length)
		ptr_Q->rear = 0;
	
	ptr_Q->count ++;
	
	return OK;
}
/****************************************************************
*   NORMAL_STATE CirQ_Pushs(pCIR_QUEUE ptr_Q,const unsigned char *pdata,unsigned char length)
* ��	�� : �������ѹ��length��ֵ		
* ������� : ָ��ѭ�����е�ָ�룬ָ��Դ��ַ��ָ�룬Ҫѹ��ֵ�ĸ���
* �� �� ֵ : ö��NORMAL_STATE���͡�
****************************************************************/
NORMAL_STATE CirQ_Pushs(pCIR_QUEUE ptr_Q,const unsigned char *pdata,unsigned char length)
{
	if(ptr_Q->data == NULL) return FAULT;
	if(length == 0)         return FAULT;
	else if(length == 1)         
		return CirQ_OnePush(ptr_Q,*pdata);
	if(ptr_Q->count + length > ptr_Q->length)
		return OVERFLOW;
	if(ptr_Q->rear + length > ptr_Q->length)
	{
		memcpy(ptr_Q->data + ptr_Q->rear,pdata,ptr_Q->length - ptr_Q->rear);
		memcpy(ptr_Q->data,pdata + ptr_Q->length - ptr_Q->rear,length + ptr_Q->rear - ptr_Q->length);
		ptr_Q->rear = length + ptr_Q->rear - ptr_Q->length;
	}
	else
	{
		memcpy(ptr_Q->data + ptr_Q->rear,pdata,length);
		ptr_Q->rear = length + ptr_Q->rear;
	}
	ptr_Q->count = ptr_Q->count + length;
	
	return OK;
}




















