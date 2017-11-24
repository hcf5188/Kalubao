/************************************************************
*��˾����:       �Ϻ����������Ƽ�
*�ļ�����: api_mem.c
*��������: HongCf
*�ļ�����: �ڴ����(�ڴ����롢�ͷš���ʼ��)
*��д����: 2017-10-23
*************************************************************/
#include "memblock.h"

OS_MEM	*pMemBuf_16B;
OS_MEM	*pMemBuf_32B;
OS_MEM  *pMemBuf_64B;
OS_MEM	*pMemBuf_128B;
OS_MEM	*pMemBuf_256B;
OS_MEM	*pMemBuf_512B;
OS_MEM	*pMemBuf_1KB;
           
uint8_t MemBuf_16B[MEM_16B_ROW][MEM_16B_COL];
uint8_t MemBuf_32B[MEM_32B_ROW][MEM_32B_COL];
uint8_t MemBuf_64B[MEM_64B_ROW][MEM_64B_COL];
uint8_t MemBuf_128B[MEM_128B_ROW][MEM_128B_COL];
uint8_t MemBuf_256B[MEM_256B_ROW][MEM_256B_COL];
uint8_t MemBuf_512B[MEM_512B_ROW][MEM_512B_COL];
uint8_t MemBuf_1KB[MEM_1KB_ROW][MEM_1KB_COL];

MEM_Check allMemState;

/****************************************************************
*			void	*Mem_malloc(uint16_t size)
* ��	�� : ��ȡһ���ڴ���׵�ַ			 		
* ������� : ��Ҫ�ڴ��ĳ���
* �� �� ֵ : void * ,�ڴ���2���ֽڵ�ַ
****************************************************************/
void *Mem_malloc(uint16_t size)
{
	uint8_t err;
	uint16_t *ptr;
	
	if(size < 1)
		return NULL;
	else if(size <= MEM_16B_COL - 2)
	{
		if((ptr = OSMemGet(pMemBuf_16B,&err)) != NULL)
		{
			*ptr = MEM_16B_COL;
			allMemState.memUsedNum1 ++;
			allMemState.memUsedMax1 = allMemState.memUsedMax1>allMemState.memUsedNum1?allMemState.memUsedMax1:allMemState.memUsedNum1;
			return ptr + 1;
		}
		else 
			return NULL;
	}else if(size <= MEM_32B_COL - 2)
	{
		if((ptr = OSMemGet(pMemBuf_32B,&err)) != NULL)
		{
			*ptr = MEM_32B_COL;
			allMemState.memUsedNum2 ++;
			allMemState.memUsedMax2 = allMemState.memUsedMax2>allMemState.memUsedNum2?allMemState.memUsedMax2:allMemState.memUsedNum2;
			return ptr + 1;
		}
		else 
			return NULL;
	}else if(size <= MEM_64B_COL - 2)
	{
		if((ptr = OSMemGet(pMemBuf_64B,&err)) != NULL)
		{
			*ptr = MEM_64B_COL;
			allMemState.memUsedNum3 ++;
			allMemState.memUsedMax3 = allMemState.memUsedMax3>allMemState.memUsedNum3?allMemState.memUsedMax3:allMemState.memUsedNum3;
			return ptr + 1;
		}
		else 
			return NULL;
	}else if(size <= MEM_128B_COL - 2)
	{
		if((ptr = OSMemGet(pMemBuf_128B,&err)) != NULL)
		{
			*ptr = MEM_128B_COL;
			allMemState.memUsedNum4 ++;
			allMemState.memUsedMax4 = allMemState.memUsedMax4>allMemState.memUsedNum4?allMemState.memUsedMax4:allMemState.memUsedNum4;
			return ptr + 1;
		}
		else 
			return NULL;
	}else if(size <= MEM_256B_COL - 2)
	{
		if((ptr = OSMemGet(pMemBuf_256B,&err)) != NULL)
		{
			*ptr = MEM_256B_COL;
			allMemState.memUsedNum5 ++;
			allMemState.memUsedMax5 = allMemState.memUsedMax5>allMemState.memUsedNum5?allMemState.memUsedMax5:allMemState.memUsedNum5;
			return ptr + 1;
		}
		else 
			return NULL;
	}else if(size <= MEM_512B_COL - 2)
	{
		if((ptr = OSMemGet(pMemBuf_512B,&err)) != NULL)
		{
			*ptr = MEM_512B_COL;
			allMemState.memUsedNum6 ++;
			allMemState.memUsedMax6 = allMemState.memUsedMax6>allMemState.memUsedNum6?allMemState.memUsedMax6:allMemState.memUsedNum6;
			return ptr + 1;
		}
		else 
			return NULL;
	}else if(size <= MEM_1KB_COL - 2)
	{
		if((ptr = OSMemGet(pMemBuf_1KB,&err)) != NULL)
		{
			*ptr = MEM_1KB_COL;
			allMemState.memUsedNum7 ++;
			allMemState.memUsedMax7 = allMemState.memUsedMax7>allMemState.memUsedNum7?allMemState.memUsedMax7:allMemState.memUsedNum7;
			return ptr + 1;
		}
		else 
			return NULL;
	}
	return NULL;
}
/****************************************************************
*			uint8_t Mem_free(void *ptr)
* ��	�� : �ͷ�һ���ڴ��			 		
* ������� : ָ����ڴ���ָ��
* �� �� ֵ : uint8_t ,������Ϣ
****************************************************************/
uint8_t Mem_free(void *ptr)
{
	uint16_t *ptr_free = ptr;
	ptr = NULL;
	if(ptr_free == NULL)
		return 1;
	
	ptr_free = ptr_free - 1;
	switch(*ptr_free)
	{
		case MEM_16B_COL: 
			allMemState.memUsedNum1 --;
			memset(ptr_free,0,MEM_16B_COL);
			return OSMemPut(pMemBuf_16B,ptr_free);
		case MEM_32B_COL: 
			allMemState.memUsedNum2 --;
			memset(ptr_free,0,MEM_32B_COL);
			return OSMemPut(pMemBuf_32B,ptr_free);
		case MEM_64B_COL: 
			allMemState.memUsedNum3 --;
			memset(ptr_free,0,MEM_64B_COL);
			return OSMemPut(pMemBuf_64B,ptr_free);
		case MEM_128B_COL: 
			allMemState.memUsedNum4 --;
			memset(ptr_free,0,MEM_128B_COL);
			return OSMemPut(pMemBuf_128B,ptr_free);
		case MEM_256B_COL: 
			allMemState.memUsedNum5 --;
			memset(ptr_free,0,MEM_256B_COL);
			return OSMemPut(pMemBuf_256B,ptr_free);
		case MEM_512B_COL: 
			allMemState.memUsedNum6 --;
			memset(ptr_free,0,MEM_512B_COL);
			return OSMemPut(pMemBuf_512B,ptr_free);
		case MEM_1KB_COL: 
			allMemState.memUsedNum7 --;
			memset(ptr_free,0,MEM_1KB_COL);
			return OSMemPut(pMemBuf_1KB,ptr_free);
		default :
			return 255;
	}
}
/****************************************************************
*			uint8_t MemBuf_Init(void)
* ��	�� : ��ʼ���ڴ����		 		
* ������� : ��
* �� �� ֵ : uint8_t��������
****************************************************************/
uint8_t MemBuf_Init(void)
{
	uint8_t err;
	
	pMemBuf_16B = OSMemCreate(MemBuf_16B,MEM_16B_ROW,MEM_16B_COL,&err);
	if(err != OS_ERR_NONE)
		return err;
	
	pMemBuf_32B = OSMemCreate(MemBuf_32B,MEM_32B_ROW,MEM_32B_COL,&err);
	if(err != OS_ERR_NONE)
		return err;
	
	pMemBuf_64B = OSMemCreate(MemBuf_64B,MEM_64B_ROW,MEM_64B_COL,&err);
	if(err != OS_ERR_NONE)
		return err;
	
	pMemBuf_128B = OSMemCreate(MemBuf_128B,MEM_128B_ROW,MEM_128B_COL,&err);
	if(err != OS_ERR_NONE)
		return err;
	
	pMemBuf_256B = OSMemCreate(MemBuf_256B,MEM_256B_ROW,MEM_256B_COL,&err);
	if(err != OS_ERR_NONE)
		return err;
	
	pMemBuf_512B = OSMemCreate(MemBuf_512B,MEM_512B_ROW,MEM_512B_COL,&err);
	if(err != OS_ERR_NONE)
		return err;
	
	pMemBuf_1KB = OSMemCreate(MemBuf_1KB,MEM_1KB_ROW,MEM_1KB_COL,&err);
	if(err != OS_ERR_NONE)
		return err;
		
	allMemState.memUsedMax1 = 0;
	allMemState.memUsedNum1 = 0;
	allMemState.memUsedMax2 = 0;
	allMemState.memUsedNum2 = 0;
	allMemState.memUsedMax3 = 0;
	allMemState.memUsedNum3 = 0;
	allMemState.memUsedMax4 = 0;
	allMemState.memUsedNum4 = 0;
	allMemState.memUsedMax5 = 0;
	allMemState.memUsedNum5 = 0;
	allMemState.memUsedMax6 = 0;
	allMemState.memUsedNum6 = 0;
	allMemState.memUsedMax7 = 0;
	allMemState.memUsedNum7 = 0;
	
	return err;
}












