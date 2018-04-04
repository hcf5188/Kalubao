

#include "bsp.h"

//芯片初始化，包括系统时钟、芯片外设、时钟滴答等等
void SystemBspInit(void )
{
	uint8_t bitValue;
	BspClockInit();
	SysTickInit();
	
	GlobalVarInit();//全局变量初始化
	PIDPtrInit();
	CARVarInit();   //与车辆行驶相关结构体的初始化
	
	GPIO_ALL_IN();  //IO口设置为输入
	
	GPIOLEDInit();
	NVIC_AllConfig();
	
	CDMAUart2Init();
	GPSConfigInit(9600);
	
	TIM4ConfigInit();
	TIM2ConfigInit();
//	ADC1Init();
	RTCConfigureInit();
	
	bitValue = GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_0);
	if(bitValue == 1)//USB升级模式
	{
		varOperation.USB_NormalMode = 1;
		GPIO_SetBits(GPIOA,GPIO_Pin_15);//使能USB接口
	}else            //正常工作模式
	{
		varOperation.USB_NormalMode = 0;
		WatchDogInit(7,2000);           //看门狗初始化，系统卡死大概12s后重启 		
	}
//	CDMAUart2Init();
//	GPSConfigInit(9600);
//	
//	TIM4ConfigInit();
//	TIM2ConfigInit();
//	RTCConfigureInit();	
}
//系统时钟滴答初始化
void SysTickInit(void)
{
	SystemCoreClockUpdate();
	SysTick_CLKSourceConfig(SysTick_CLKSource_HCLK);
	SysTick_Config(SystemCoreClock / OS_TICKS_PER_SEC);
}
//使能外设时钟
void BspClockInit(void)
{
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);
	
	//RCC_APB1Periph_CAN1	
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2|RCC_APB1Periph_USART3|RCC_APB1Periph_UART4|RCC_APB1Periph_I2C1|RCC_APB1Periph_TIM2|RCC_APB1Periph_TIM3, ENABLE);

  	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1|RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB |RCC_APB2Periph_GPIOC|
								   RCC_APB2Periph_AFIO|RCC_APB2Periph_SPI1|RCC_APB2Periph_ADC1, ENABLE);

	GPIO_PinRemapConfig(GPIO_Remap_SWJ_JTAGDisable,ENABLE);
//	GPIO_PinRemapConfig(GPIO_Remap_SWJ_Disable, ENABLE);
}

void GPIO_ALL_IN(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;

	/* Enable GPIOD and GPIOE clock */
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB 
	                     | RCC_APB2Periph_GPIOC | RCC_APB2Periph_GPIOD 
	                     | RCC_APB2Periph_GPIOE| RCC_APB2Periph_AFIO, ENABLE);
	  /* PA  */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_All;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	  /* PB  */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_All;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
	GPIO_Init(GPIOB, &GPIO_InitStructure);
	  /* PC  */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_All&(~GPIO_Pin_8);//GPS EN Keep Output
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
	GPIO_Init(GPIOC, &GPIO_InitStructure);
	  /* PD  */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_All;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
	GPIO_Init(GPIOD, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_All;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
	GPIO_Init(GPIOE, &GPIO_InitStructure);
}

void GPIOLEDInit(void)
{
	GPIO_InitTypeDef  GPIO_InitStructure;
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);	 //使能 PB 端口时钟
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
  	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
  	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  	GPIO_Init(GPIOB,&GPIO_InitStructure);
									
	GPIO_InitStructure.GPIO_Pin   =  GPIO_Pin_3 | GPIO_Pin_4 | GPIO_Pin_5; // LED 端口配置
	GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_Out_PP; 		 //推挽输出
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;		 //IO口速度为50MHz
	GPIO_Init(GPIOB, &GPIO_InitStructure);					 //根据设定参数初始化 GPIOB.5
    
	GPIO_ResetBits(GPIOB,GPIO_Pin_0 | GPIO_Pin_3 | GPIO_Pin_5 | GPIO_Pin_4);	//复位，双灯亮
	
	//USB EN
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_15;
  	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
  	GPIO_Init(GPIOA, &GPIO_InitStructure);

	GPIO_ResetBits(GPIOA,GPIO_Pin_15);
	//USB STATUS
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPD;
	GPIO_Init(GPIOC, &GPIO_InitStructure);
}
void ADC1Init(void)
{
	ADC_InitTypeDef ADC_InitStructure; 
	GPIO_InitTypeDef GPIO_InitStructure;

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA |RCC_APB2Periph_ADC1	, ENABLE );	  //使能ADC1通道时钟
 

	RCC_ADCCLKConfig(RCC_PCLK2_Div6);   //设置ADC分频因子6 72M/6=12,ADC最大时间不能超过14M

	//PA1 作为模拟通道输入引脚                         
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;		//模拟输入引脚
	GPIO_Init(GPIOA, &GPIO_InitStructure);	

	ADC_DeInit(ADC1);  //复位ADC1 

	ADC_InitStructure.ADC_Mode = ADC_Mode_Independent;	//ADC工作模式:ADC1和ADC2工作在独立模式
	ADC_InitStructure.ADC_ScanConvMode = DISABLE;	    //模数转换工作在单通道模式
	ADC_InitStructure.ADC_ContinuousConvMode = DISABLE;	//模数转换工作在单次转换模式
	ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;	//转换由软件而不是外部触发启动
	ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;	            //ADC数据右对齐
	ADC_InitStructure.ADC_NbrOfChannel = 1;	                            //顺序进行规则转换的ADC通道的数目
	ADC_Init(ADC1, &ADC_InitStructure);	                                //根据ADC_InitStruct中指定的参数初始化外设ADCx的寄存器   

  
	ADC_Cmd(ADC1, ENABLE);	//使能指定的  ADC1
	
	ADC_ResetCalibration(ADC1);	//使能复位校准  
	 
	while(ADC_GetResetCalibrationStatus(ADC1));	//等待复位校准结束
	
	ADC_StartCalibration(ADC1);	 //开启AD校准
 
	while(ADC_GetCalibrationStatus(ADC1));	 //等待校准结束

}

u16 Get_Adc(u8 ch)   
{
  	//设置指定ADC的规则组通道，一个序列，采样时间
	ADC_RegularChannelConfig(ADC1, ch, 1, ADC_SampleTime_239Cycles5 );	//ADC1,ADC通道,采样时间为239.5周期	  			    
  
	ADC_SoftwareStartConvCmd(ADC1, ENABLE);		//使能指定的ADC1的软件转换启动功能	
	 
	while(!ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC)); //等待转换结束

	return ADC_GetConversionValue(ADC1);	//返回最近一次ADC1规则组的转换结果
}
u16 Get_Adc_Average(u8 ch,u8 times)
{
	u32 temp_val=0;
	u8 t;
	for(t = 0;t < times;t ++)
	{
		temp_val += Get_Adc(ch);
		OSTimeDlyHMSM(0,0,0,5);
	}
	return temp_val / times;
}

void CDMAUart2Init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	
	USART_InitTypeDef USART_InitStructure;
	
	//USART2 RX
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	//USART2 TX
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	
	//GPRS 模块复位按钮默认L
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12;//GPIOB_12
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOB, &GPIO_InitStructure);
	GPIO_SetBits(GPIOB, GPIO_Pin_12);

	//GPRS 电源
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_15;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOB, &GPIO_InitStructure);
	GPIO_SetBits(GPIOB, GPIO_Pin_15);
	
	
	//GPS 电源
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8;
  	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
  	GPIO_Init(GPIOC, &GPIO_InitStructure);
	GPIO_ResetBits(GPIOC,GPIO_Pin_8);//默认上电开启GPS电源

	//GPRS Status 状态
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_13;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(GPIOB, &GPIO_InitStructure);
	
	USART_InitStructure.USART_BaudRate            = 115200  ; //波特率为115200
	USART_InitStructure.USART_WordLength          = USART_WordLength_8b;
	USART_InitStructure.USART_StopBits            = USART_StopBits_1;
	USART_InitStructure.USART_Parity              = USART_Parity_No ;
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_InitStructure.USART_Mode                = USART_Mode_Rx | USART_Mode_Tx;	
	USART_Init(USART2, &USART_InitStructure);
	
	USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);//使能串口接收中断
	USART_ITConfig(USART2, USART_IT_TC, ENABLE);  //使能串口发送中断
	USART_Cmd(USART2, ENABLE);                    //使能串口2
}

void KLineInit(void )
{
	GPIO_InitTypeDef  GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure;
	
	//USART3 RX
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_KWP_RX;
  	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
  	GPIO_Init(GPIOB, &GPIO_InitStructure);
	//USART3 TX
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_KWP_TX;
  	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
  	GPIO_Init(GPIOB, &GPIO_InitStructure);
	
	USART_InitStructure.USART_BaudRate            = 10400  ;		  //KWP2000 波特率10400
	USART_InitStructure.USART_WordLength          = USART_WordLength_8b;
	USART_InitStructure.USART_StopBits            = USART_StopBits_1;
	USART_InitStructure.USART_Parity              = USART_Parity_No ;
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_InitStructure.USART_Mode                = USART_Mode_Rx | USART_Mode_Tx;

	USART_Init(USART3, &USART_InitStructure);
	USART_ITConfig(USART3, USART_IT_RXNE, ENABLE);
	USART_ITConfig(USART3, USART_IT_TC, ENABLE);
	USART_Cmd(USART3, ENABLE); 
}

void TIM4ConfigInit(void )
{
	TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
	
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4, ENABLE); //时钟使能
	
	//定时器TIM4初始化
	TIM_TimeBaseStructure.TIM_Period = 10;          //设置在下一个更新事件装入活动的自动重装载寄存器周期的值	
	TIM_TimeBaseStructure.TIM_Prescaler =7199;      //设置用来作为TIMx时钟频率除数的预分频值
	TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;      //设置时钟分割:TDTS = Tck_tim
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;  //TIM向上计数模式
	TIM_TimeBaseInit(TIM4, &TIM_TimeBaseStructure); //根据指定的参数初始化TIMx的时间基数单位
 
	TIM_ITConfig(TIM4,TIM_IT_Update,ENABLE );       //使能指定的TIM4中断,允许更新中断	
//	TIM_Cmd(TIM4, ENABLE);  //使能TIMx		
}
void TIM5ConfigInit(void )
{
	TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
	
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM5, ENABLE); //时钟使能
	
	//定时器TIM5初始化
	TIM_TimeBaseStructure.TIM_Period = 10;          //设置在下一个更新事件装入活动的自动重装载寄存器周期的值	
	TIM_TimeBaseStructure.TIM_Prescaler =7199;      //设置用来作为TIMx时钟频率除数的预分频值
	TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;      //设置时钟分割:TDTS = Tck_tim
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;  //TIM向上计数模式
	TIM_TimeBaseInit(TIM5, &TIM_TimeBaseStructure); //根据指定的参数初始化TIMx的时间基数单位
 
	TIM_ITConfig(TIM5,TIM_IT_Update,ENABLE );       //使能指定的TIM4中断,允许更新中断	
//	TIM_Cmd(TIM5, ENABLE);  //使能TIMx		
}
void RTCConfigureInit(void)
{
	EXTI_InitTypeDef EXTI_InitStructure;  
 
	/* Enable PWR and BKP clocks */
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR | RCC_APB1Periph_BKP, ENABLE);

	/* Allow access to BKP Domain */
	PWR_BackupAccessCmd(ENABLE);

	/* Reset Backup Domain */
	//BKP_DeInit();

	//BKP_ClearFlag();
	//RCC_LSEConfig(RCC_LSE_ON);
	RCC_LSICmd(ENABLE);

	//while(RCC_GetFlagStatus(RCC_FLAG_LSERDY)==RESET);//等待LSE就绪，如果谐振不对，就会死在这里

	BKP_TamperPinCmd(DISABLE);

	/* Select LSE as RTC Clock Source */
	//RCC_RTCCLKConfig(RCC_RTCCLKSource_LSE);
	//RCC_RTCCLKConfig(RCC_RTCCLKSource_HSE_Div128); //			  RCC_RTCCLKSource_LSE RCC_RTCCLKSource_HSE_Div128
	RCC_RTCCLKConfig(RCC_RTCCLKSource_LSI);

	/* Enable RTC Clock */
	RCC_RTCCLKCmd(ENABLE);

	/* Wait for RTC registers synchronization */
	RTC_WaitForSynchro();


	/* Wait until last write operation on RTC registers has finished */
	RTC_WaitForLastTask();

	/* Enable the RTC Second */
	//RTC_ITConfig(RTC_IT_SEC, ENABLE);

	RTC_ITConfig(RTC_IT_ALR, ENABLE);

	/* Wait until last write operation on RTC registers has finished */
	RTC_WaitForLastTask();

	/* Set RTC prescaler: set RTC period to 1sec */
	//RTC_SetPrescaler(32767); /* RTC period = RTCCLK/RTC_PR = (32.768 KHz)/(32767+1) */
	RTC_SetPrescaler(39999);	  //8M/128=62500 //40k
	//RTC_SetPrescaler(93749);	  //12M/128=93750
	/* Wait until last write operation on RTC registers has finished */
	RTC_WaitForLastTask();
	
	EXTI_ClearITPendingBit(EXTI_Line17);  
	EXTI_InitStructure.EXTI_Line = EXTI_Line17;  
	EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;  
	EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising;  
	EXTI_InitStructure.EXTI_LineCmd = ENABLE;  
	EXTI_Init(&EXTI_InitStructure);  
}
void RTC_Time_Adjust(uint32_t value)//RTC实时时钟校正
{
  /* Wait until last write operation on RTC registers has finished */
  RTC_WaitForLastTask();
  /* Change the current time */
  RTC_SetCounter(value);
  /* Wait until last write operation on RTC registers has finished */
  RTC_WaitForLastTask();
}
//初始化独立看门狗
//prer:分频数:0~7(只有低3位有效!)
//分频因子=4*2^prer.但最大值只能是256!
//rlr:重装载寄存器值:低11位有效.
//时间计算(大概):Tout=((4*2^prer)*rlr)/40 (ms).
void WatchDogInit(uint8_t prer,uint16_t rlr )
{
	IWDG_WriteAccessCmd(IWDG_WriteAccess_Enable);  //使能对寄存器IWDG_PR和IWDG_RLR的写操作
	
	IWDG_SetPrescaler(prer);  //设置IWDG预分频值:设置IWDG预分频值为64
	
	IWDG_SetReload(rlr);  //设置IWDG重装载值
	
	IWDG_ReloadCounter();  //按照IWDG重装载寄存器的值重装载IWDG计数器
	
	IWDG_Enable();  //使能IWDG
}

void NVIC_AllConfig(void )
{
	NVIC_InitTypeDef NVIC_InitStructure; 
	NVIC_SetVectorTable(NVIC_VectTab_FLASH, 0x8000);
	
	/* Enable the RTC Interrupt */
	NVIC_InitStructure.NVIC_IRQChannel = RTCAlarm_IRQn;  
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;  
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;  
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;  
	NVIC_Init(&NVIC_InitStructure);  
	
	//定时器4 中断优先级NVIC设置
	NVIC_InitStructure.NVIC_IRQChannel = TIM4_IRQn; //TIM4中断
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;//先占优先级0级
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;       //从优先级3级
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE; //IRQ通道被使能
	NVIC_Init(&NVIC_InitStructure);                 //初始化NVIC寄存器
	
	//串口3 中断初始化
	NVIC_InitStructure.NVIC_IRQChannel = USART3_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);
	
	//串口2 中断初始化
	NVIC_InitStructure.NVIC_IRQChannel = USART2_IRQn;  //为串口2中断配置中断优先级
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 4;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);
	
	NVIC_InitStructure.NVIC_IRQChannel = CAN1_RX1_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 5;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);	
	
	//GPS 串口初始化
	NVIC_InitStructure.NVIC_IRQChannel = UART4_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 6;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);
	
	//定时器2 辅助GPS接收 中断初始化
	NVIC_InitStructure.NVIC_IRQChannel = TIM2_IRQn; //TIM2中断
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;//先占优先级2级
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;       //从优先级1级
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE; //IRQ通道被使能
	NVIC_Init(&NVIC_InitStructure);                 //初始化NVIC寄存器
	//定时器5 辅助USB接收 中断初始化
	NVIC_InitStructure.NVIC_IRQChannel = TIM5_IRQn; //TIM2中断
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;//先占优先级2级
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;       //从优先级1级
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE; //IRQ通道被使能
	NVIC_Init(&NVIC_InitStructure);                 //初始化NVIC寄存器
	
	NVIC_InitStructure.NVIC_IRQChannel = TIM1_UP_IRQn; //TIM2中断
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;//先占优先级2级
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;       //从优先级1级
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE; //IRQ通道被使能
	NVIC_Init(&NVIC_InitStructure);                 //初始化NVIC寄存器
}





