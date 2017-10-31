

#ifndef __MEMBLOCK_H__
#define __MEMBLOCK_H__

#include "includes.h"
/**********************************************************
*                  �ڴ���س��ýṹ������
***********************************************************/
//��������ֵ
typedef	enum  
{
	OK = 0,
	FAULT,
	OVERFLOW,
	EMPTY
}NORMAL_STATE;

//ѭ�����нṹ
typedef	struct
{
	uint16_t  front;
	uint16_t  rear;
	uint16_t  length;
	uint16_t  count;
	uint8_t	  *data;
}CIR_QUEUE,*pCIR_QUEUE;

//��ջ�ṹ
typedef struct
{
	uint16_t top;
	uint16_t length;
	uint8_t *base;
}STORE, *pSTORE;

//���г�ʼ��
pCIR_QUEUE    Cir_Queue_Init(uint16_t length);
//ɾ������
NORMAL_STATE  CirQ_Delete(pCIR_QUEUE ptr_Q);
//��ն���
NORMAL_STATE  CirQ_Clear(pCIR_QUEUE ptr_Q);
//�õ����б���ֵ������
uint16_t      CirQ_GetLength(pCIR_QUEUE ptr_Q);
//�Ӷ�����ȡ��һ��ֵ
NORMAL_STATE  CirQ_Pop(pCIR_QUEUE ptr_Q,uint8_t *phead);
//�������ѹ��һ��ֵ
NORMAL_STATE  CirQ_OnePush(pCIR_QUEUE ptr_Q,uint8_t dat);
//�������ѹ��ܶ�ֵ
NORMAL_STATE  CirQ_Pushs(pCIR_QUEUE ptr_Q,const uint8_t *pdata,uint16_t length);

pSTORE       Store_Init(uint16_t length);
NORMAL_STATE Store_Delete(pSTORE S);
NORMAL_STATE Store_Clear(pSTORE S);
NORMAL_STATE Store_Pop(pSTORE S,uint8_t *pdat);
uint16_t     Store_Getlength(pSTORE S);
NORMAL_STATE Store_Getdates(pSTORE S,uint8_t *pdate,uint8_t num);
NORMAL_STATE Store_Push(pSTORE S,uint8_t dat);


/**********************************************************
*                  �ڴ�����������
***********************************************************/

#define MEM_10B_ROW		50
#define MEM_10B_COL		12

#define MEM_32B_ROW		40
#define MEM_32B_COL		34

#define MEM_64B_ROW		40
#define MEM_64B_COL		66

#define MEM_128B_ROW	30
#define MEM_128B_COL	130

#define MEM_256B_ROW	20
#define MEM_256B_COL	258

#define MEM_512B_ROW	20
#define MEM_512B_COL	514

#define MEM_1KB_ROW	    6
#define MEM_1KB_COL	    1026

void    *Mem_malloc(uint16_t size);
uint8_t Mem_free(void *ptr);
uint8_t MemBuf_Init(void);






#endif

