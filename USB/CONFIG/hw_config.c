#include "includes.h"
#include "usb_lib.h"
#include "usb_prop.h"
#include "usb_desc.h"
#include "usb_istr.h"
#include "hw_config.h"
#include "usb_pwr.h" 
#include "usart.h"  
#include "string.h"	
#include "stdarg.h"		 
#include "stdio.h"	
//////////////////////////////////////////////////////////////////////////////////	 
//������ֻ��ѧϰʹ�ã�δ��������ɣ��������������κ���;
//ALIENTEKս��STM32������V3
//USB-hw_config ����	   
//����ԭ��@ALIENTEK
//������̳:www.openedv.com
//��������:2015/1/28
//�汾��V1.0
//��Ȩ���У�����ؾ���
//Copyright(C) ������������ӿƼ����޹�˾ 2009-2019
//All rights reserved									  
////////////////////////////////////////////////////////////////////////////////// 
 

u8  USART_PRINTF_Buffer[USB_USART_REC_LEN];	//usb_printf���ͻ�����

//�����ƴ���1�������ݵķ���,������USB���⴮�ڽ��յ�������.
u8 USB_USART_RX_BUF[USB_USART_REC_LEN]; 	//���ջ���,���USART_REC_LEN���ֽ�.
//����״̬
//bit15��	������ɱ�־
//bit14��	���յ�0x0d
//bit13~0��	���յ�����Ч�ֽ���Ŀ
u16 USB_USART_RX_STA=0;       				//����״̬���	 

extern LINE_CODING linecoding;							//USB���⴮��������Ϣ


extern pCIR_QUEUE sendUSB_Q;     //ָ�� CDMA ���ڷ��Ͷ���  ��ָ��  USB����
extern pSTORE     receUSB_S;     //ָ�� CDMA ���ڽ������ݶѵ�ָ��
/////////////////////////////////////////////////////////////////////////////////
//��USB����ͨ�ò��ִ���,ST����USB����,�˲��ִ��붼���Թ���.
//�˲��ִ���һ�㲻��Ҫ�޸�!

//USB�����жϷ�����
void USBWakeUp_IRQHandler(void) 
{
	EXTI_ClearITPendingBit(EXTI_Line18);//���USB�����жϹ���λ
} 

//USB�жϴ�����
void USB_LP_CAN1_RX0_IRQHandler(void) 
{
	USB_Istr();
} 

//USBʱ�����ú���,USBclk=48Mhz@HCLK=72Mhz
void Set_USBClock(void)
{
	RCC_USBCLKConfig(RCC_USBCLKSource_PLLCLK_1Div5);//USBclk=PLLclk/1.5=48Mhz	
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USB, ENABLE);	 //USBʱ��ʹ��		 
} 

//USB����͹���ģʽ
//��USB����suspendģʽʱ,MCU����͹���ģʽ
//��������ӵ͹��Ĵ���(�����ʱ�ӵ�)
void Enter_LowPowerMode(void)
{
// 	printf("usb enter low power mode\r\n");
	bDeviceState=SUSPENDED;
} 

//USB�˳��͹���ģʽ
//�û��������������ش���(������������ʱ�ӵ�)
void Leave_LowPowerMode(void)
{
	DEVICE_INFO *pInfo=&Device_Info;
//	printf("leave low power mode\r\n"); 
	if (pInfo->Current_Configuration!=0)bDeviceState=CONFIGURED; 
	else bDeviceState = ATTACHED; 
} 

//USB�ж�����
void USB_Interrupts_Config(void)
{ 
	NVIC_InitTypeDef NVIC_InitStructure;
	EXTI_InitTypeDef EXTI_InitStructure;

 
	/* Configure the EXTI line 18 connected internally to the USB IP */
	EXTI_ClearITPendingBit(EXTI_Line18);
											  //  ������18�ϵ��ж�
	EXTI_InitStructure.EXTI_Line = EXTI_Line18; // USB resume from suspend mode
	EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising;	//line 18���¼��������ش���
	EXTI_InitStructure.EXTI_LineCmd = ENABLE;
	EXTI_Init(&EXTI_InitStructure); 	 

	/* Enable the USB interrupt */
	NVIC_InitStructure.NVIC_IRQChannel = USB_LP_CAN1_RX0_IRQn;	//��2�����ȼ���֮ 
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);
	
	/* Enable the USB Wake-up interrupt */
	NVIC_InitStructure.NVIC_IRQChannel = USBWakeUp_IRQn;   //��2�����ȼ����	
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
	NVIC_Init(&NVIC_InitStructure);   
}	

//USB�ӿ�����(����1.5K��������,ս��V3����Ҫ����,������)
//NewState:DISABLE,������
//         ENABLE,����
void USB_Cable_Config (FunctionalState NewState)
{ 
//	if (NewState!=DISABLE)printf("usb pull up enable\r\n"); 
//	else printf("usb pull up disable\r\n"); 
}

//USBʹ������/����
//enable:0,�Ͽ�
//       1,��������	   
void USB_Port_Set(u8 enable)
{
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA,ENABLE);    //ʹ��PORTAʱ��		 
	if(enable)_SetCNTR(_GetCNTR()&(~(1<<1)));//�˳��ϵ�ģʽ
	else
	{	  
		_SetCNTR(_GetCNTR()|(1<<1));  // �ϵ�ģʽ
		GPIOA->CRH&=0XFFF00FFF;
		GPIOA->CRH|=0X00033000;
		PAout(12)=0;	    		  
	}
}  

//��ȡSTM32��ΨһID
//����USB������Ϣ
void Get_SerialNum(void)
{
	u32 Device_Serial0, Device_Serial1, Device_Serial2;
	Device_Serial0 = *(u32*)(0x1FFFF7E8);
	Device_Serial1 = *(u32*)(0x1FFFF7EC);
	Device_Serial2 = *(u32*)(0x1FFFF7F0);
	Device_Serial0 += Device_Serial2;
	if (Device_Serial0 != 0)
	{
		IntToUnicode (Device_Serial0,&Virtual_Com_Port_StringSerial[2] , 8);
		IntToUnicode (Device_Serial1,&Virtual_Com_Port_StringSerial[18], 4);
	}
} 

//��32λ��ֵת����unicode.
//value,Ҫת����ֵ(32bit)
//pbuf:�洢��ַ
//len:Ҫת���ĳ���
void IntToUnicode (u32 value , u8 *pbuf , u8 len)
{
	u8 idx = 0;
	for( idx = 0 ; idx < len ; idx ++)
	{
		if( ((value >> 28)) < 0xA )
		{
			pbuf[ 2* idx] = (value >> 28) + '0';
		}
		else
		{
			pbuf[2* idx] = (value >> 28) + 'A' - 10; 
		} 
		value = value << 4; 
		pbuf[ 2* idx + 1] = 0;
	}
}
/////////////////////////////////////////////////////////////////////////////////
 
//USB COM�ڵ�������Ϣ,ͨ���˺�����ӡ����. 

 
//�����USB���⴮�ڽ��յ�������
//databuffer:���ݻ�����
//Nb_bytes:���յ����ֽ���.
uint8_t usbRxTime= 0;
uint16_t usbReceLen = 0;
void USB_To_USART_Send_Data(u8* data_buffer, u8 Nb_bytes)
{ 
	u8 i;
	for(i=0;i<Nb_bytes;i++)
	{  
		Store_Push(receUSB_S,data_buffer[i]);
	} 
	usbRxTime = 1;
	TIM_SetCounter(TIM5,0); //��ռ�����
	TIM_Cmd(TIM5, ENABLE);  //ʹ��TIMx	
} 

extern OS_EVENT *USBRecieveQ;//����ӵ��Զ˽��յ���USB����

void TIM5_IRQHandler(void)   //CDMA���ճ�ʱ����ʱ���ж�
{
	uint8_t *ptrRece;
	OSIntEnter();//ϵͳ�����жϷ������
	if (TIM_GetITStatus(TIM5, TIM_IT_Update) != RESET)  //���TIM5�����жϷ������
	{
		TIM_ClearITPendingBit(TIM5, TIM_IT_Update  );  //���TIMx�����жϱ�־ 
		usbRxTime ++;
		if(usbRxTime > 2)
		{
			usbRxTime = 0;
			usbReceLen = Store_Getlength(receUSB_S);
			if(usbReceLen>2 )
			{
				ptrRece = Mem_malloc(usbReceLen);
				if(ptrRece != NULL)//�ڴ������ɹ�
				{
					Store_Getdates(receUSB_S,ptrRece,usbReceLen);
					if(OSQPost(USBRecieveQ,ptrRece) != OS_ERR_NONE)//���Ͳ��ɹ���Ҫ�ͷ��ڴ��
					{
						Mem_free(ptrRece);
						Store_Clear(receUSB_S);//�������ν��յ�����
					}
				}
				else
					Store_Clear(receUSB_S);    //�������ν��յ�����
			}
			else
				Store_Clear(receUSB_S);//���յ����ݳ���<=2 ��Ϊ��Ч���ݣ�û����ô�̵Ļظ�
				
			TIM_Cmd(TIM5, DISABLE);
		}
	}
	OSIntExit();  //�жϷ��������ϵͳ�����������
}
//����һ���ֽ����ݵ�USB���⴮��
extern SYS_OperationVar  varOperation;
void USB_USART_SendDatas(uint8_t* ptrSend,uint16_t datLen)
{
#if OS_CRITICAL_METHOD == 3u           /* Allocate storage for CPU status register           */
	OS_CPU_SR  cpu_sr = 0u;
#endif
	
	if(datLen < 1 || datLen >1020)
		return ;
	OS_ENTER_CRITICAL();
	
	if(CirQ_Pushs(sendUSB_Q,ptrSend,datLen) != OK)
	{
		OS_EXIT_CRITICAL();
	}
	OS_EXIT_CRITICAL();
	
	varOperation.isUSBSendDat = 1;	
}

//usb���⴮��,printf ����
//ȷ��һ�η������ݲ���USB_USART_REC_LEN�ֽ�
void usb_printf(char* fmt,...)  
{  
	u16 datLen;
	va_list ap;
#if OS_CRITICAL_METHOD == 3u           /* Allocate storage for CPU status register           */
	OS_CPU_SR  cpu_sr = 0u;
#endif
	va_start(ap,fmt);
	vsprintf((char*)USART_PRINTF_Buffer,fmt,ap);
	va_end(ap);
	
	datLen=strlen((const char*)USART_PRINTF_Buffer);//�˴η������ݵĳ���
	
	OS_ENTER_CRITICAL();
	
	CirQ_OnePush(sendUSB_Q,0x10); //������Ϣ���
	
	if(CirQ_Pushs(sendUSB_Q,USART_PRINTF_Buffer,datLen) != OK)
	{
		OS_EXIT_CRITICAL();
	}
	OS_EXIT_CRITICAL();
	varOperation.isUSBSendDat = 1;//֪ͨUSB�������ݷ���
	
} 






















