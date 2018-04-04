#include "bsp.h"
#include "includes.h"
#include "apptask.h"
#include "delay.h"

/************************************************************************
*                           LED、蜂鸣器控制任务
*************************************************************************/

uint16_t freCDMALed = LEDFAST;//黄灯  GPRS
void CDMALEDTask(void *pdata)
{
	while(1)
	{
		if(freCDMALed < 50)
			freCDMALed = 50;
		else if(freCDMALed >2000)
			freCDMALed = 2000;
		GPIO_ResetBits(GPIO_LED,LED_GPIO_MOD);
		OSTimeDlyHMSM(0,0,0,freCDMALed);
		GPIO_SetBits(GPIO_LED,LED_GPIO_MOD);
		OSTimeDlyHMSM(0,0,0,freCDMALed);
	}
}
uint16_t freGPSLed = LEDFAST;//绿灯  GPS
void GPSLEDTask(void *pdata)
{
	while(1)
	{
		if(freGPSLed < 50)
			freGPSLed = 50;
		else if(freGPSLed >2000)
			freGPSLed = 2000;
		GPIO_ResetBits(GPIO_LED,LED_GPIO_GPS);
		OSTimeDlyHMSM(0,0,0,freGPSLed);
		GPIO_SetBits(GPIO_LED,LED_GPIO_GPS);
		OSTimeDlyHMSM(0,0,0,freGPSLed);
	}
}
uint16_t freOBDLed = LEDFAST;//红灯  CAN
void OBDLEDTask(void *pdata)
{
	while(1)
	{
		if(freOBDLed < 50)
			freOBDLed = 50;
		else if(freOBDLed >2000)
			freOBDLed = 2000;
		GPIO_ResetBits(GPIO_LED,LED_GPIO_OBD);
		OSTimeDlyHMSM(0,0,0,freOBDLed);
		GPIO_SetBits(GPIO_LED,LED_GPIO_OBD);
		OSTimeDlyHMSM(0,0,0,freOBDLed);
	}
}

static void BSP_BeeperTimerInit(uint16_t ck_value)
{
	TIM_TimeBaseInitTypeDef TIM_BaseInitStructure;
    TIM_OCInitTypeDef TIM_OCInitStructure;

	GPIO_InitTypeDef GPIO_InitStructure;

	uint16_t period=(18000000/ck_value);
	uint16_t period_set=period-1;
	uint16_t pluse=period/2-1;
		
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB  | RCC_APB2Periph_AFIO, ENABLE); 
	GPIO_PinRemapConfig(GPIO_PartialRemap_TIM3, ENABLE);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
  	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
  	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  	GPIO_Init(GPIOB,&GPIO_InitStructure);

	//Timer3CH2 Beeper
	TIM_InternalClockConfig(TIM3);

	TIM_BaseInitStructure.TIM_Prescaler         = 3;//4分频，18M
	TIM_BaseInitStructure.TIM_CounterMode       = TIM_CounterMode_Up;
	TIM_BaseInitStructure.TIM_Period            = period_set;
	TIM_BaseInitStructure.TIM_ClockDivision     = TIM_CKD_DIV1;
	TIM_BaseInitStructure.TIM_RepetitionCounter = 0;
	TIM_TimeBaseInit(TIM3,&TIM_BaseInitStructure);
	TIM_ARRPreloadConfig(TIM3, DISABLE);

	TIM_Cmd(TIM3, ENABLE);  //使能TIMx外设
	
	TIM_OCInitStructure.TIM_OCMode=TIM_OCMode_PWM1;
	TIM_OCInitStructure.TIM_OutputState=TIM_OutputState_Enable;
	TIM_OCInitStructure.TIM_Pulse=pluse;
	TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;
	TIM_OC3Init(TIM3,&TIM_OCInitStructure);
	TIM_CtrlPWMOutputs(TIM3,ENABLE);
}
/* 关闭蜂鸣器 */
void BSP_BeeperTimer_Off(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	TIM_CtrlPWMOutputs(TIM3,DISABLE);
	TIM_Cmd(TIM3,DISABLE);  //使能TIMx外设
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, DISABLE);
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
  	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
  	GPIO_Init(GPIOB,&GPIO_InitStructure);
	GPIO_ResetBits(GPIOB, GPIO_Pin_0);
}

void BeepTask(void *pdata)
{
	uint8_t err;
	TIM1ConfigInit();
	while(1)
	{
		OSSemPend(beepSem,0,&err);//想让蜂鸣器响，发送个信号量即可
		BSP_BeeperTimerInit(1000);//打开蜂鸣器，频率可以设定
		OSTimeDlyHMSM(0,0,0,300);
		BSP_BeeperTimer_Off();    //关闭蜂鸣器
	}
}

void TIM1ConfigInit(void )
{
	TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM1, ENABLE); //时钟使能
	
	//定时器TIM2初始化
	TIM_TimeBaseStructure.TIM_Period = 9999;          //设置在下一个更新事件装入活动的自动重装载寄存器周期的值	
	TIM_TimeBaseStructure.TIM_Prescaler =7199;      //设置用来作为TIMx时钟频率除数的预分频值
	TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;      //设置时钟分割:TDTS = Tck_tim
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;  //TIM向上计数模式
	TIM_TimeBaseStructure.TIM_RepetitionCounter = 0;
	TIM_TimeBaseInit(TIM1, &TIM_TimeBaseStructure); //根据指定的参数初始化TIMx的时间基数单位
 
	TIM_ITConfig(TIM1,TIM_IT_Update,ENABLE );       //使能指定的TIM2中断,允许更新中断
	
	TIM_Cmd(TIM1, ENABLE);  //使能TIMx		
}
u32 timeBase = 0;
void TIM1_UP_IRQHandler(void)
{
	
	TIM_ClearITPendingBit(TIM1, TIM_IT_Update);
	OSIntEnter();	
	timeBase ++;
	OSIntExit();  //中断服务结束，系统进行任务调度
	
}
