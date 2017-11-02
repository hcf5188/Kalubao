#include "bsp.h"



//系统时钟滴答（不要改动，非常重要）
void SysTick_Handler(void)
{	
	OSIntEnter();							//进入中断
	OSTimeTick();       					//调用ucos的时钟服务程序               
	OSIntExit();       	 					//触发任务切换软中断
}


