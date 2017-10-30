

#ifndef __MEMBLOCK_H__
#define __MEMBLOCK_H__

#include "includes.h"
/**********************************************************
*                  常用结构体声明
***********************************************************/
//常见返回值
typedef	enum  
{
	OK = 0,
	FAULT,
	OVERFLOW,
	EMPTY
}NORMAL_STATE;

//循环队列结构
typedef	struct
{
	unsigned char	*data;
	unsigned char	front;
	unsigned char	rear;
	unsigned short int	length;
	unsigned short int	count;
}CIR_QUEUE,*pCIR_QUEUE;

//堆栈结构
typedef struct
{
	unsigned char *base;
	unsigned short int top;
	unsigned short int length;
}STORE, *pSTORE;



//队列初始化
pCIR_QUEUE Cir_Queue_Init(unsigned int length);
//删除队列
NORMAL_STATE  CirQ_Delete(pCIR_QUEUE ptr_Q);
//清空队列
NORMAL_STATE  CirQ_Clear(pCIR_QUEUE ptr_Q);
//得到队列保存值的数量
unsigned short int CirQ_GetLength(pCIR_QUEUE ptr_Q);
//从队列中取出一个值
NORMAL_STATE CirQ_Pop(pCIR_QUEUE ptr_Q,unsigned char *phead);
//向队列中压入一个值
NORMAL_STATE CirQ_OnePush(pCIR_QUEUE ptr_Q,unsigned char dat);
//向队列中压入很多值
NORMAL_STATE CirQ_Pushs(pCIR_QUEUE ptr_Q,const unsigned char *pdata,unsigned char length);




pSTORE Store_Init(unsigned int length);
NORMAL_STATE Store_Delete(pSTORE S);
NORMAL_STATE Store_Clear(pSTORE S);
NORMAL_STATE Store_Pop(pSTORE S,unsigned char *pdat);
unsigned int Store_Getlength(pSTORE S);
NORMAL_STATE Store_Getdates(pSTORE S,uint8_t *pdate,uint8_t num);
NORMAL_STATE Store_Push(pSTORE S,unsigned char dat);



/**********************************************************
*                  内存控制相关声明
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

