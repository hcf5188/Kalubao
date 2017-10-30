/************************************************************
*公司名称:       宁波GQY视讯股份有限公司
*文件名称: mem_Store.c
*作者姓名: HongCf
*文件内容: 数据堆操作
*编写日期: 2017-4-30
*************************************************************/

#include "memblock.h"

/****************************************************************
*		pSTORE Store_Init(unsigned int length)
* 描	述 : 初始化堆栈		 		
* 输入参数 : 堆栈的长度
* 返 回 值 : pSTORE类型，指向堆栈的指针
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
* 描	述 : 删除堆栈		 		
* 输入参数 : 指向堆栈的指针
* 返 回 值 : NORMAL_STATE类型，操作是否成功状态
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
* 描	述 : 清空堆栈内容		 		
* 输入参数 : 指向堆栈的指针
* 返 回 值 : NORMAL_STATE类型，操作是否成功状态
****************************************************************/
NORMAL_STATE Store_Clear(pSTORE S)
{
	if(S->base == NULL)	return FAULT;
	S->top = 0;
	return OK;
}
/****************************************************************
*		NORMAL_STATE Store_Push(pSTORE S,unsigned char dat)
* 描	述 : 向堆栈中压入一个数据	 		
* 输入参数 : 指向堆栈的指针，要压入的数据
* 返 回 值 : NORMAL_STATE类型，操作是否成功状态
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
* 描	述 : 弹出堆栈中的一个数据	（后进先出） 		
* 输入参数 : 指向堆栈的指针，要弹出的地址
* 返 回 值 : NORMAL_STATE类型，操作是否成功状态
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
* 描	述 : 得到堆栈保存数据的长度		
* 输入参数 : 指向堆栈的指针
* 返 回 值 : unsigned int类型，数据的长度
****************************************************************/
unsigned int Store_Getlength(pSTORE S)
{
	if(S->base == NULL) return 0;
	
	return S->top;
}
/****************************************************************
*		NORMAL_STATE Store_Getdates(pSTORE S,uint8_t *pdate,uint8_t num)
* 描	述 : 从堆栈中得到num个数据	
* 输入参数 : 指向堆栈的指针，指向目的地指针，数据数目
* 返 回 值 : NORMAL_STATE类型，操作是否成功状态
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







