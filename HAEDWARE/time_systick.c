#include "bsp.h"



//ϵͳʱ�ӵδ𣨲�Ҫ�Ķ����ǳ���Ҫ��
void SysTick_Handler(void)
{	
	OSIntEnter();							//�����ж�
	OSTimeTick();       					//����ucos��ʱ�ӷ������               
	OSIntExit();       	 					//���������л����ж�
}


