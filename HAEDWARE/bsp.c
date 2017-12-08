

#include "bsp.h"

//оƬ��ʼ��������ϵͳʱ�ӡ�оƬ���衢ʱ�ӵδ�ȵ�
void SystemBspInit(void )
{
	uint8_t bitValue;
	BspClockInit();
	SysTickInit();
	
	GlobalVarInit();//ȫ�ֱ�����ʼ��
	
	GPIO_ALL_IN();  //IO������Ϊ����
	
	GPIOLEDInit();
	NVIC_AllConfig();
	
	CDMAUart2Init();
	GPSConfigInit(9600);

	TIM4ConfigInit();
	TIM2ConfigInit();
	RTCConfigureInit();
	
	
	bitValue = GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_0);
	if(bitValue == 1)
	{
		varOperation.USB_NormalMode = 1;
		GPIO_SetBits(GPIOA,GPIO_Pin_15);//ʹ��USB�ӿ�
	}else
	{
		varOperation.USB_NormalMode = 0;				
	}
//	CDMAUart2Init();
//	GPSConfigInit(9600);
//	
//	TIM4ConfigInit();
//	TIM2ConfigInit();
//	RTCConfigureInit();
	
	
	
	
}
//ϵͳʱ�ӵδ��ʼ��
void SysTickInit(void)
{
	SystemCoreClockUpdate();
	SysTick_CLKSourceConfig(SysTick_CLKSource_HCLK);
	SysTick_Config(SystemCoreClock / OS_TICKS_PER_SEC);
}
//ʹ������ʱ��
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
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);	 //ʹ��PB�˿�ʱ��
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
  	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
  	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  	GPIO_Init(GPIOB,&GPIO_InitStructure);
									
	GPIO_InitStructure.GPIO_Pin   =  GPIO_Pin_3 | GPIO_Pin_4 | GPIO_Pin_5; //LED �˿�����
	GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_Out_PP; 		 //�������
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;		 //IO���ٶ�Ϊ50MHz
	GPIO_Init(GPIOB, &GPIO_InitStructure);					 //�����趨������ʼ��GPIOB.5
    
	GPIO_ResetBits(GPIOB,GPIO_Pin_0 | GPIO_Pin_3 | GPIO_Pin_5 | GPIO_Pin_4);		     //��λ��˫����
	
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
	ADC_InitTypeDef  ADC_InitStructure;

	ADC_Cmd(ADC1, DISABLE);
	ADC_DMACmd(ADC1, DISABLE);

	
	//BSP_ADC_DMA_Init();
	
	ADC_InitStructure.ADC_Mode = ADC_Mode_Independent;
	ADC_InitStructure.ADC_ScanConvMode = ENABLE;
	ADC_InitStructure.ADC_ContinuousConvMode = ENABLE;
	ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;
	ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;
	ADC_InitStructure.ADC_NbrOfChannel = 2;
	ADC_Init(ADC1, &ADC_InitStructure);

	ADC_TempSensorVrefintCmd(ENABLE);

	/* ADC1 regular channel8 configuration */ 
	ADC_RegularChannelConfig(ADC1, ADC_Channel_1, 1, ADC_SampleTime_28Cycles5);//ADC_SampleTime_239Cycles5
	ADC_RegularChannelConfig(ADC1, ADC_Channel_16, 2, ADC_SampleTime_28Cycles5);//ADC_SampleTime_239Cycles5

	/* Enable ADC1 */
	ADC_Cmd(ADC1, ENABLE);

	ADC_DMACmd(ADC1, ENABLE);

	/* Enable ADC1 reset calibaration register */   
	ADC_ResetCalibration(ADC1);
	/* Check the end of ADC1 reset calibration register */
	while(ADC_GetResetCalibrationStatus(ADC1));

	/* Start ADC1 calibaration */
	ADC_StartCalibration(ADC1);
	/* Check the end of ADC1 calibration */
	while(ADC_GetCalibrationStatus(ADC1));

	/* Start ADC1 Software Conversion */ 
	ADC_SoftwareStartConvCmd(ADC1, ENABLE);

//#ifdef ADC_USE_AWD
//	
//	/*zx ����ADCģ�⿴�Ź�*/
//	ADC_AnalogWatchdogSingleChannelConfig(ADC1,ADC_Channel_1);
//	ADC_AnalogWatchdogThresholdsConfig(ADC1,1300,900);
//	ADC_AnalogWatchdogCmd(ADC1,ADC_AnalogWatchdog_SingleRegEnable);
//	ADC_ITConfig(ADC1,ADC_IT_AWD,ENABLE);
//#endif

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
	
	//GPRS ģ�鸴λ��ťĬ��L
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12;//GPIOB_12
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOB, &GPIO_InitStructure);
	GPIO_SetBits(GPIOB, GPIO_Pin_12);

	//GPRS ��Դ
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_15;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOB, &GPIO_InitStructure);
	GPIO_SetBits(GPIOB, GPIO_Pin_15);
	
	
	//GPS ��Դ
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8;
  	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
  	GPIO_Init(GPIOC, &GPIO_InitStructure);
	GPIO_ResetBits(GPIOC,GPIO_Pin_8);//Ĭ���ϵ翪��GPS��Դ

	//GPRS Status ״̬
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_13;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(GPIOB, &GPIO_InitStructure);
	
	
	
	USART_InitStructure.USART_BaudRate            = 115200  ; //������Ϊ115200
	USART_InitStructure.USART_WordLength          = USART_WordLength_8b;
	USART_InitStructure.USART_StopBits            = USART_StopBits_1;
	USART_InitStructure.USART_Parity              = USART_Parity_No ;
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_InitStructure.USART_Mode                = USART_Mode_Rx | USART_Mode_Tx;	
	USART_Init(USART2, &USART_InitStructure);
	
	USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);//ʹ�ܴ��ڽ����ж�
	USART_ITConfig(USART2, USART_IT_TC, ENABLE);  //ʹ�ܴ��ڷ����ж�
	USART_Cmd(USART2, ENABLE);                    //ʹ�ܴ���2
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
	
	USART_InitStructure.USART_BaudRate            = 10400  ;		  //KWP2000 ������10400
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
	
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4, ENABLE); //ʱ��ʹ��
	
	//��ʱ��TIM4��ʼ��
	TIM_TimeBaseStructure.TIM_Period = 10;          //��������һ�������¼�װ�����Զ���װ�ؼĴ������ڵ�ֵ	
	TIM_TimeBaseStructure.TIM_Prescaler =7199;      //����������ΪTIMxʱ��Ƶ�ʳ�����Ԥ��Ƶֵ
	TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;      //����ʱ�ӷָ�:TDTS = Tck_tim
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;  //TIM���ϼ���ģʽ
	TIM_TimeBaseInit(TIM4, &TIM_TimeBaseStructure); //����ָ���Ĳ�����ʼ��TIMx��ʱ�������λ
 
	TIM_ITConfig(TIM4,TIM_IT_Update,ENABLE );       //ʹ��ָ����TIM4�ж�,��������ж�	
//	TIM_Cmd(TIM4, ENABLE);  //ʹ��TIMx		
}
void TIM5ConfigInit(void )
{
	TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
	
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM5, ENABLE); //ʱ��ʹ��
	
	//��ʱ��TIM5��ʼ��
	TIM_TimeBaseStructure.TIM_Period = 10;          //��������һ�������¼�װ�����Զ���װ�ؼĴ������ڵ�ֵ	
	TIM_TimeBaseStructure.TIM_Prescaler =7199;      //����������ΪTIMxʱ��Ƶ�ʳ�����Ԥ��Ƶֵ
	TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;      //����ʱ�ӷָ�:TDTS = Tck_tim
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;  //TIM���ϼ���ģʽ
	TIM_TimeBaseInit(TIM5, &TIM_TimeBaseStructure); //����ָ���Ĳ�����ʼ��TIMx��ʱ�������λ
 
	TIM_ITConfig(TIM5,TIM_IT_Update,ENABLE );       //ʹ��ָ����TIM4�ж�,��������ж�	
//	TIM_Cmd(TIM5, ENABLE);  //ʹ��TIMx		
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

	//while(RCC_GetFlagStatus(RCC_FLAG_LSERDY)==RESET);//�ȴ�LSE���������г�񲻶ԣ��ͻ���������

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
void RTC_Time_Adjust(uint32_t value)//RTCʵʱʱ��У��
{
  /* Wait until last write operation on RTC registers has finished */
  RTC_WaitForLastTask();
  /* Change the current time */
  RTC_SetCounter(value);
  /* Wait until last write operation on RTC registers has finished */
  RTC_WaitForLastTask();
}

void WatchDogInit(void )
{

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
	
	//��ʱ��4 �ж����ȼ�NVIC����
	NVIC_InitStructure.NVIC_IRQChannel = TIM4_IRQn; //TIM4�ж�
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;//��ռ���ȼ�0��
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;       //�����ȼ�3��
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE; //IRQͨ����ʹ��
	NVIC_Init(&NVIC_InitStructure);                 //��ʼ��NVIC�Ĵ���
	
	//����3 �жϳ�ʼ��
	NVIC_InitStructure.NVIC_IRQChannel = USART3_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);
	
	//����2 �жϳ�ʼ��
	NVIC_InitStructure.NVIC_IRQChannel = USART2_IRQn;  //Ϊ����2�ж������ж����ȼ�
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 4;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);
	
	NVIC_InitStructure.NVIC_IRQChannel = CAN1_RX1_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 5;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);	
	
	//GPS ���ڳ�ʼ��
	NVIC_InitStructure.NVIC_IRQChannel = UART4_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 6;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);
	
	//��ʱ��2 ����GPS���� �жϳ�ʼ��
	NVIC_InitStructure.NVIC_IRQChannel = TIM2_IRQn; //TIM2�ж�
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;//��ռ���ȼ�2��
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;       //�����ȼ�1��
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE; //IRQͨ����ʹ��
	NVIC_Init(&NVIC_InitStructure);                 //��ʼ��NVIC�Ĵ���
	//��ʱ��5 ����USB���� �жϳ�ʼ��
	NVIC_InitStructure.NVIC_IRQChannel = TIM5_IRQn; //TIM2�ж�
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;//��ռ���ȼ�2��
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;       //�����ȼ�1��
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE; //IRQͨ����ʹ��
	NVIC_Init(&NVIC_InitStructure);                 //��ʼ��NVIC�Ĵ���
}





