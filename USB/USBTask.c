#include "bsp.h"
#include "includes.h"
#include "apptask.h"
#include "delay.h"
#include "usb_lib.h"
#include "hw_config.h"
#include "usb_pwr.h"


pCIR_QUEUE sendUSB_Q;     //指向 USB 串口发送队列  的指针  
pSTORE     receUSB_S;     //指向 USB 串口接收数据堆的指针
extern uint8_t updateBuff[2048];       //升级用
uint8_t upda[3][3]={{0x00,0xAA,0xBB},{0x01,0x00,0x00},{0x02,0x4F,0x4B}};

//USB 升级任务
void USBUpdataTask (void *pdata)
{
	uint8_t err;
	uint16_t i;
	uint16_t cmdId = 0;
	
	uint16_t softCRC = 0;
	uint16_t receFrameCRC = 0;
	uint16_t comFrameCRC = 0;
	
	uint16_t frameIndex = 0;
	uint8_t * ptrReceD;
	uint8_t offset = 0;
	uint32_t flashAddr  = 0;
	
	sendUSB_Q = Cir_Queue_Init(500);//USB 串口发送 循环队列 缓冲区
	receUSB_S = Store_Init(500);    //USB 串口接收 数据堆   缓冲区
	
	varOperation.isUSBSendDat = 0;
	
	OSTimeDlyHMSM(0,0,1,800);
	TIM5ConfigInit();   //配置定时器
	USB_Port_Set(0);
	OSTimeDlyHMSM(0,0,0,700);
	USB_Port_Set(1);	//USB再次连接
 	Set_USBClock();   
 	USB_Interrupts_Config();    
 	USB_Init();	    

	while(1)
	{
	as: USB_USART_SendDatas(upda[0],3);//发送握手帧
		ptrReceD = OSQPend(USBRecieveQ,500,&err);
		if(err == OS_ERR_NONE)
		{
			offset = 1;
			varOperation.frameNum = ptrReceD[offset++];
			varOperation.frameNum = (varOperation.frameNum << 8) + ptrReceD[offset++];
			varOperation.newSoftCRC = ptrReceD[offset++];
			varOperation.newSoftCRC = (varOperation.newSoftCRC << 8) + ptrReceD[offset++];
			
			varOperation.newSoftVersion = ptrReceD[offset++];
			varOperation.newSoftVersion = (varOperation.newSoftVersion<<8)+ptrReceD[offset++];
			varOperation.newSoftVersion = (varOperation.newSoftVersion<<8)+ptrReceD[offset++];
			varOperation.newSoftVersion = (varOperation.newSoftVersion<<8)+ptrReceD[offset++];
			receFrameCRC = ptrReceD[offset++];
			receFrameCRC = (receFrameCRC << 8) + ptrReceD[offset++];
			
			comFrameCRC = CRC_Compute16(ptrReceD,ptrReceD[0] - 2);
			Mem_free(ptrReceD);
			if(comFrameCRC != receFrameCRC)
				goto as;
			
			OSTimeDlyHMSM(0,0,0,500);
		}
		else goto as;
		frameIndex = 0;
		for(i=1;i<=varOperation.frameNum;)
		{
			offset = 1;
			upda[1][1] = i/256;
			upda[1][2] = i%256;
			USB_USART_SendDatas(upda[1],3);
			ptrReceD = OSQPend(USBRecieveQ,1500,&err);
			if(err == OS_ERR_NONE)
			{
				cmdId = ptrReceD[offset++];
				cmdId = (cmdId << 8) + ptrReceD[offset++];
				receFrameCRC = ptrReceD[ptrReceD[0] - 2];
				receFrameCRC = (receFrameCRC << 8)+ptrReceD[ptrReceD[0] - 1];
				comFrameCRC = CRC_Compute16(ptrReceD,ptrReceD[0] - 2);
				if((i != cmdId)||(comFrameCRC != receFrameCRC))
				{
					Mem_free(ptrReceD);
					continue;
				}
				softCRC = CRC_ComputeFile(softCRC,&ptrReceD[3],ptrReceD[0] - 5);
				memcpy(&updateBuff[frameIndex*128],&ptrReceD[3],ptrReceD[0] - 5);
				frameIndex++;
				i++;
				if(frameIndex >= 16||(i>varOperation.frameNum))
				{
					frameIndex = 0;
					SoftErasePage(flashAddr);
					SoftProgramUpdate(flashAddr,updateBuff,2048);
					memset(updateBuff,0,2048);
					flashAddr += 0x800;
				}
				Mem_free(ptrReceD);
			}
		}
		if(softCRC != varOperation.newSoftCRC)
			goto as;
		sysUpdateVar.isSoftUpdate = 1;      //告诉Sboot,程序需要升级
		sysUpdateVar.pageNum      = flashAddr/0x800;
		sysUpdateVar.softVersion  = varOperation.newSoftVersion;
			
		SbootParameterSaveToFlash(&sysUpdateVar);//将升级参数保存到Flash中
		USB_USART_SendDatas(upda[2],3);
		
		ptrReceD = OSQPend(USBRecieveQ,500,&err);
		Mem_free(ptrReceD);
		while(1)           //升级成功
		{
			OSTimeDlyHMSM(0,0,2,0);
		}
	}
}







