#include "bsp.h"
#include "includes.h"
#include "apptask.h"
#include "delay.h"
#include "obd.h"

uint8_t * pPid[52];

//���ļ����ڶ����һ��

void PIDPtrInit(void)
{
	uint8_t i = 0;
	for(i = 0;i < 11;i++)
	{
		pPid[i]    = Mem_malloc(60);//����PID��Ƶ�ʲɼ������� �ڴ�
		pPid[i][0] = 4;
		pPid[i][1] = 0x60;
		pPid[i][2] = i + 1;
	}
	for(i = 11;i < 50;i++)
	{
		pPid[i]    = Mem_malloc(30);//���ڵ�Ƶ�� PID �ɼ������� �ڴ�
		pPid[i][0] = 4;
		pPid[i][1] = 0x60;
		pPid[i][2] = i + 1;
	}
	pPid[49] = Mem_malloc(30);      //���һ������������ 5016 ������
	pPid[49][0] = 3;
	pPid[49][1] = 0x50;
	pPid[49][2] = 0x21;
	
	pPid[50] = Mem_malloc(30);      //���һ������������ 5016 ������
	pPid[50][0] = 3;
	pPid[50][1] = 0x50;
	pPid[50][2] = 0x16;
	
	pPid[51] = Mem_malloc(60);      //���� GPS ��Ϣ������
	pPid[51][0] = 3;
	pPid[51][1] = 0x50;
	pPid[51][2] = 0x02;
}



























