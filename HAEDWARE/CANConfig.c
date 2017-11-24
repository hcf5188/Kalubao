
#include "bsp.h"
#include "obd.h"

extern SYS_OperationVar  varOperation;

void CAN1Config(void)
{
	GPIO_InitTypeDef  GPIO_InitStructure;
	CAN_InitTypeDef   CAN_InitStructure;
	
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_CAN1, ENABLE);//使能CAN1时钟	
	CAN_DeInit(CAN1);
	CAN_StructInit(&CAN_InitStructure);
	
	
	//PB14  CANC
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_14;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
  	GPIO_Init(GPIOB, &GPIO_InitStructure);
	GPIO_ResetBits(GPIOB, GPIO_Pin_14);//使能CAN芯片 VP230
	
	//CAN1_RX   -  PB8;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_Init(GPIOB, &GPIO_InitStructure);
	//CAN1_TX   -  PB9;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOB, &GPIO_InitStructure);

	GPIO_PinRemapConfig(GPIO_Remap1_CAN1 , ENABLE);
	if((varOperation.canBaud == 8) ||(varOperation.canBaud == 9))
		CAN1_BaudSet(CANBAUD_250K);           //设置CAN波特率为250K
	else if((varOperation.canBaud == 6) ||(varOperation.canBaud == 7))
		CAN1_BaudSet(CANBAUD_500K);           //设置CAN波特率为500K
	CAN1_SetFilter(varOperation.canRxId,varOperation.canIdType);     //设置滤波器  //潍柴：0x18DAFB10   渣土车：0x18DAFA00

//	CAN1_BaudSet(CANBAUD_500K);           //设置CAN波特率为500K
//	CAN1_SetFilter(0x7E8,CAN_ID_STD);     //设置滤波器
	
//	CAN1_ClearFilter();                   //清除滤波器
	
	CAN_ITConfig(CAN1,CAN_IT_FMP1,ENABLE);				//FIFO1消息挂号中断允许.	
	
	
}
//CAN1 波特率设置
void CAN1_BaudSet(CANBAUD_Enum baud)
{
	CAN_InitTypeDef         CAN_InitStructure;
	
	
	CAN_InitStructure.CAN_TTCM = DISABLE;
	CAN_InitStructure.CAN_ABOM = DISABLE;
	CAN_InitStructure.CAN_AWUM = DISABLE;
	CAN_InitStructure.CAN_NART = DISABLE;
	CAN_InitStructure.CAN_RFLM = DISABLE;
	CAN_InitStructure.CAN_TXFP = DISABLE;
	CAN_InitStructure.CAN_Mode = CAN_Mode_Normal;//正常模式

	CAN_InitStructure.CAN_SJW = CAN_SJW_1tq;
	CAN_InitStructure.CAN_BS1 = CAN_BS1_9tq;
	CAN_InitStructure.CAN_BS2 = CAN_BS2_8tq;
	switch(baud)
	{
		case CANBAUD_100K:break;
		case CANBAUD_250K:CAN_InitStructure.CAN_Prescaler = 8;break;
		case CANBAUD_500K:CAN_InitStructure.CAN_Prescaler = 4;break;
		case CANBAUD_1M  :CAN_InitStructure.CAN_Prescaler = 2;break;
		default:break;
	}
	CAN_Init(CAN1, &CAN_InitStructure);
}
//清除滤波器
void CAN1_ClearFilter(void)
{
	CAN_FilterInitTypeDef   CAN_FilterInitStructure;
	
	CAN_FilterInitStructure.CAN_FilterNumber=1;	//过滤器1
	CAN_FilterInitStructure.CAN_FilterMode=CAN_FilterMode_IdMask; 	//屏蔽位模式
	CAN_FilterInitStructure.CAN_FilterScale=CAN_FilterScale_32bit; 	//32位宽 
	
	CAN_FilterInitStructure.CAN_FilterIdHigh=0x0000;	//32位ID
	CAN_FilterInitStructure.CAN_FilterIdLow=0x0000;
	CAN_FilterInitStructure.CAN_FilterMaskIdHigh=0x0000;//32位MASK
	CAN_FilterInitStructure.CAN_FilterMaskIdLow=0x0000;
	CAN_FilterInitStructure.CAN_FilterFIFOAssignment=CAN_Filter_FIFO1;//过滤器1关联到FIFO1
	
	CAN_FilterInitStructure.CAN_FilterActivation=ENABLE;//激活过滤器1
	CAN_FIFORelease(CAN1,CAN_FIFO1);
	CAN_FilterInit(&CAN_FilterInitStructure);			//滤波器初始化
}
//设置滤波器
void CAN1_SetFilter(uint32_t canId,uint32_t canIde)
{
	CAN_FilterInitTypeDef  CAN_FilterInitStructure;

	CAN_FilterInitStructure.CAN_FilterNumber = 0;
	CAN_FilterInitStructure.CAN_FilterMode = CAN_FilterMode_IdMask;
	CAN_FilterInitStructure.CAN_FilterScale = CAN_FilterScale_32bit;
	if(canIde == CAN_ID_STD)
	{
		CAN_FilterInitStructure.CAN_FilterIdHigh   = (((u32)canId<<21)&0xffff0000)>>16;
		CAN_FilterInitStructure.CAN_FilterIdLow   = (((u32)canId<<21)|CAN_ID_STD)&0xffff;
		CAN_FilterInitStructure.CAN_FilterMaskIdHigh  = 0xFE00;
		CAN_FilterInitStructure.CAN_FilterMaskIdLow = 0xFFFC;
	}else
	{
		CAN_FilterInitStructure.CAN_FilterIdHigh   = (((u32)canId<<3)&0xffff0000)>>16;
		CAN_FilterInitStructure.CAN_FilterIdLow   = (((u32)canId<<3)|CAN_ID_EXT)&0xffff;
		CAN_FilterInitStructure.CAN_FilterMaskIdHigh  = 0xFFFF;
		CAN_FilterInitStructure.CAN_FilterMaskIdLow = 0xFFC0;
	}
	CAN_FilterInitStructure.CAN_FilterFIFOAssignment = CAN_FIFO1;//0
	CAN_FilterInitStructure.CAN_FilterActivation = ENABLE;
	CAN_FilterInit(&CAN_FilterInitStructure);
}











