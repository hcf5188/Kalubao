/************************************************************
*��˾����:       ����GQY��Ѷ�ɷ����޹�˾
*�ļ�����: mem_Store.c
*��������: HongCf
*�ļ�����: ���ݶѲ���
*��д����: 2017-4-30
*************************************************************/

#include "memblock.h"

/****************************************************************
*		pSTORE Store_Init(unsigned int length)
* ��	�� : ��ʼ����ջ		 		
* ������� : ��ջ�ĳ���
* �� �� ֵ : pSTORE���ͣ�ָ���ջ��ָ��
****************************************************************/
pSTORE Store_Init(unsigned int length)
{
	pSTORE S = NULL;
	if(length < 2)  return NULL;
	if((S = (pSTORE)Mem_malloc(sizeof(STORE))) != NULL)
	{
		if((S->base = (unsigned char *)Mem_malloc(length)) != NULL)
		{
			S->length = length;
			S->top    = 0;
			return S;
		}
		else
			Mem_free(S);
	}
	return NULL;
}
/****************************************************************
*		NORMAL_STATE Store_Delete(pSTORE S)
* ��	�� : ɾ����ջ		 		
* ������� : ָ���ջ��ָ��
* �� �� ֵ : NORMAL_STATE���ͣ������Ƿ�ɹ�״̬
****************************************************************/
NORMAL_STATE Store_Delete(pSTORE S)
{
	if(S->base == NULL)         return FAULT;
	
	if(Mem_free(S->base) != 0)  return FAULT;
	
	if(Mem_free(S) != 0)        return FAULT;
	
	return OK;
}
/****************************************************************
*		NORMAL_STATE Store_Clear(pSTORE S)
* ��	�� : ��ն�ջ����		 		
* ������� : ָ���ջ��ָ��
* �� �� ֵ : NORMAL_STATE���ͣ������Ƿ�ɹ�״̬
****************************************************************/
NORMAL_STATE Store_Clear(pSTORE S)
{
	if(S->base == NULL)	return FAULT;
	S->top = 0;
	return OK;
}
/****************************************************************
*		NORMAL_STATE Store_Push(pSTORE S,unsigned char dat)
* ��	�� : ���ջ��ѹ��һ������	 		
* ������� : ָ���ջ��ָ�룬Ҫѹ�������
* �� �� ֵ : NORMAL_STATE���ͣ������Ƿ�ɹ�״̬
****************************************************************/
NORMAL_STATE Store_Push(pSTORE S,unsigned char dat)
{
	if(S->base == NULL) return FAULT;
	if(S->top  == S->length) return OVERFLOW;
	
	S->base[S->top] = dat;
	S->top ++;
	
	return OK;
}
/****************************************************************
*		NORMAL_STATE Store_Pop(pSTORE S,unsigned char *pdat)
* ��	�� : ������ջ�е�һ������	������ȳ��� 		
* ������� : ָ���ջ��ָ�룬Ҫ�����ĵ�ַ
* �� �� ֵ : NORMAL_STATE���ͣ������Ƿ�ɹ�״̬
****************************************************************/
NORMAL_STATE Store_Pop(pSTORE S,unsigned char *pdat)
{
	if(S->base == NULL) return FAULT;
	if(S->top == 0)    return EMPTY;
	
	S->top --;
	*pdat = S->base[S->top];
	
	return OK;
}
/****************************************************************
*		unsigned int Store_Getlength(pSTORE S)
* ��	�� : �õ���ջ�������ݵĳ���		
* ������� : ָ���ջ��ָ��
* �� �� ֵ : unsigned int���ͣ����ݵĳ���
****************************************************************/
unsigned int Store_Getlength(pSTORE S)
{
	if(S->base == NULL) return 0;
	
	return S->top;
}
/****************************************************************
*		NORMAL_STATE Store_Getdates(pSTORE S,uint8_t *pdate,uint8_t num)
* ��	�� : �Ӷ�ջ�еõ�num������	
* ������� : ָ���ջ��ָ�룬ָ��Ŀ�ĵ�ָ�룬������Ŀ
* �� �� ֵ : NORMAL_STATE���ͣ������Ƿ�ɹ�״̬
****************************************************************/
NORMAL_STATE Store_Getdates(pSTORE S,uint8_t *pdate,uint8_t num)
{
	if(S->base == NULL) return FAULT;
	if(num > S->top)    return FAULT;
	if(pdate == NULL)   return FAULT;
	
	memcpy(pdate,S->base,num);
	S->top = S->top - num;
	memcpy(S->base,S->base + num,S->top);
	
	return OK;
}







