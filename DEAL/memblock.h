

#ifndef __MEMBLOCK_H__
#define __MEMBLOCK_H__

#include "includes.h"
/**********************************************************
*                  ���ýṹ������
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
	unsigned char	*data;
	unsigned char	front;
	unsigned char	rear;
	unsigned short int	length;
	unsigned short int	count;
}CIR_QUEUE,*pCIR_QUEUE;

//��ջ�ṹ
typedef struct
{
	unsigned char *base;
	unsigned short int top;
	unsigned short int length;
}STORE, *pSTORE;



//���г�ʼ��
pCIR_QUEUE Cir_Queue_Init(unsigned int length);
//ɾ������
NORMAL_STATE  CirQ_Delete(pCIR_QUEUE ptr_Q);
//��ն���
NORMAL_STATE  CirQ_Clear(pCIR_QUEUE ptr_Q);
//�õ����б���ֵ������
unsigned short int CirQ_GetLength(pCIR_QUEUE ptr_Q);
//�Ӷ�����ȡ��һ��ֵ
NORMAL_STATE CirQ_Pop(pCIR_QUEUE ptr_Q,unsigned char *phead);
//�������ѹ��һ��ֵ
NORMAL_STATE CirQ_OnePush(pCIR_QUEUE ptr_Q,unsigned char dat);
//�������ѹ��ܶ�ֵ
NORMAL_STATE CirQ_Pushs(pCIR_QUEUE ptr_Q,const unsigned char *pdata,unsigned char length);




pSTORE Store_Init(unsigned int length);
NORMAL_STATE Store_Delete(pSTORE S);
NORMAL_STATE Store_Clear(pSTORE S);
NORMAL_STATE Store_Pop(pSTORE S,unsigned char *pdat);
unsigned int Store_Getlength(pSTORE S);
NORMAL_STATE Store_Getdates(pSTORE S,uint8_t *pdate,uint8_t num);
NORMAL_STATE Store_Push(pSTORE S,unsigned char dat);



/**********************************************************
*                  �ڴ�����������
***********************************************************/

#define MEM_10B_ROW		60
#define MEM_10B_COL		12

#define MEM_32B_ROW		20
#define MEM_32B_COL		34

#define MEM_64B_ROW		20
#define MEM_64B_COL		66

#define MEM_128B_ROW	20
#define MEM_128B_COL	130

#define MEM_256B_ROW	20
#define MEM_256B_COL	258

#define MEM_512B_ROW	10
#define MEM_512B_COL	514

#define MEM_1KB_ROW	    5
#define MEM_1KB_COL	    1026

void    *Mem_malloc(uint16_t size);
uint8_t Mem_free(void *ptr);
uint8_t MemBuf_Init(void);












#endif

