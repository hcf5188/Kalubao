#include "bsp.h"
#include "includes.h"
#include "apptask.h"
#include "delay.h"
#include "obd.h"

uint8_t * pPid[102];

//���ļ����ڶ����һ��

void PIDPtrInit(void)
{
	uint8_t i = 0;
	for(i = 0;i < 100;i++)
	{
		pPid[i]    = Mem_malloc(8);//����PID��Ƶ�ʲɼ������� �ڴ�
		pPid[i][0] = i + 1;
		pPid[i][1] = 0 ;
		pPid[i][2] = i + 1;
	}

	pPid[100]   = Mem_malloc(30);  //��������ظ������������
	pPid[100][0] = 3;
	pPid[100][1] = 0x50;
	pPid[100][2] = 0x21;

	pPid[101] = Mem_malloc(60);      //���� GPS ��Ϣ������
	pPid[101][0] = 3;
	pPid[101][1] = 0x50;
	pPid[101][2] = 0x02;
}



























