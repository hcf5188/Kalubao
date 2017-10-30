/************************************************************
*公司名称:       宁波GQY视讯股份有限公司
*文件名称: mem_Q.c
*作者姓名: HongCf
*文件内容: 循环队列操作
*编写日期: 2017-4-30
*************************************************************/

#include "memblock.h"


/****************************************************************
*		pCIR_QUEUE Cir_Queue_Init(unsigned int length)
* 描	述 : 初始化循环队列
* 输入参数 : 循环对列的长度
* 返 回 值 : pCIR_QUEUE类型，指向循环队列的指针
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
* 描	述 : 释放循环队列		 		
* 输入参数 : 指向循环队列的指针
* 返 回 值 : 枚举NORMAL_STATE类型。
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
* 描	述 : 清空循环队列		 		
* 输入参数 : 指向循环队列的指针
* 返 回 值 : 枚举NORMAL_STATE类型。
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
* 描	述 : 得到此循环队列保存值得数	 		
* 输入参数 : 指向循环队列的指针
* 返 回 值 : 保存值的数量count。
****************************************************************/
unsigned short int CirQ_GetLength(pCIR_QUEUE ptr_Q)
{
	if(ptr_Q->data == NULL) return 0;
	
	return ptr_Q->count;
}
/****************************************************************
*   NORMAL_STATE CirQ_Pop(pCIR_QUEUE ptr_Q,unsigned char *phead)
* 描	述 : 从队列中弹出一个的值--先进先出 		
* 输入参数 : 指向循环队列的指针，弹向的一个值地址
* 返 回 值 : 枚举NORMAL_STATE类型。
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
* 描	述 : 向队列中压入一个值		
* 输入参数 : 指向循环队列的指针，要压入的值
* 返 回 值 : 枚举NORMAL_STATE类型。
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
* 描	述 : 向队列中压入length个值		
* 输入参数 : 指向循环队列的指针，指向源地址的指针，要压入值的个数
* 返 回 值 : 枚举NORMAL_STATE类型。
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




















