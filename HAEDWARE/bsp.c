

#include "bsp.h"


//芯片初始化，包括系统时钟、芯片外设、时钟滴答等等
void SystemBspInit(void )
{
	SysTickInit();
	BspClockInit();
	GPIOLEDInit();

	CAN1Config();
	
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
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2   \
							|RCC_APB1Periph_USART3 \
							|RCC_APB1Periph_UART4  \
							|RCC_APB1Periph_I2C1   \
							|RCC_APB1Periph_TIM2   \
							|RCC_APB1Periph_TIM3, ENABLE);

  	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1
							|RCC_APB2Periph_GPIOA 
							|RCC_APB2Periph_GPIOB 
							|RCC_APB2Periph_GPIOC
							|RCC_APB2Periph_AFIO
							|RCC_APB2Periph_SPI1
							|RCC_APB2Periph_ADC1, ENABLE);
}

void GPIOLEDInit(void)
{
	GPIO_InitTypeDef  GPIO_InitStructure;
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);	 //使能PB端口时钟
	
//  禁用JTAG接口，采用SWD接口进行调试（恢复PB3、PB4的IO功能）
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
	GPIO_PinRemapConfig(GPIO_Remap_SWJ_JTAGDisable , ENABLE);
//	GPIO_PinRemapConfig(GPIO_Remap_SWJ_Disable, ENABLE);

	GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_3 | GPIO_Pin_4 | GPIO_Pin_5; //LED 端口配置
	GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_Out_PP; 		 //推挽输出
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;		 //IO口速度为50MHz
	GPIO_Init(GPIOB, &GPIO_InitStructure);					 //根据设定参数初始化GPIOB.5

	GPIO_ResetBits(GPIOB,GPIO_Pin_3 | GPIO_Pin_5|GPIO_Pin_4);		     //复位，双灯亮
}



void KLineInit(void )
{
	GPIO_InitTypeDef  GPIO_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
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
	
	NVIC_InitStructure.NVIC_IRQChannel = USART3_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);
}




