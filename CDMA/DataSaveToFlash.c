#include "includes.h"
#include "bsp.h"
#include "stdint.h"

//�³�������ݸ���
void SoftProgramUpdate(uint32_t wAddr,uint8_t* ptrBuff,uint16_t datLength)
{
	OS_CPU_SR  cpu_sr = 0u;
	uint16_t i = 0;
	if(wAddr > 0x80000)
		return ;
	wAddr += NEW_SOFT_ADDR;
	
	OS_ENTER_CRITICAL();//��ֹ�ж�
	{
		volatile FLASH_Status FLASHStatus = FLASH_COMPLETE;
		volatile uint32_t     w_addr      = wAddr;
		volatile uint32_t*    p_w_data    = (uint32_t*)ptrBuff;	//4�ֽ�ָ��
		volatile uint32_t     w_data      = (uint32_t)(*p_w_data);//ȡ����Ӧ����

		for(i=0;(i<(datLength/4))&&(FLASHStatus == FLASH_COMPLETE);i++) 
		{
			FLASHStatus = FLASH_ProgramWord(w_addr, w_data);
			w_addr = w_addr + 4;
			p_w_data++;
			w_data=(uint32_t)(*p_w_data)  ;	
		}
	}
	OS_EXIT_CRITICAL(); //���ж�
}

void SoftErasePage(uint32_t addr)
{
	OS_CPU_SR  cpu_sr = 0u;
	OS_ENTER_CRITICAL();//��ֹ�ж�
	{
		volatile FLASH_Status FLASHStatus = FLASH_COMPLETE;
		
		if(addr<0x80000)//ƫ�Ƶ�ַ������ʼ��ַ
			addr += NEW_SOFT_ADDR;
		FLASH_Unlock();
		FLASH_ClearFlag(FLASH_FLAG_BSY | FLASH_FLAG_EOP | FLASH_FLAG_PGERR | FLASH_FLAG_WRPRTERR);

		if(FLASHStatus==FLASH_COMPLETE)
			FLASHStatus = FLASH_ErasePage(addr);
	}
	OS_EXIT_CRITICAL(); //���ж�
}

void SbootParameterSaveToFlash(_SystemInformation* parameter)
{
	OS_CPU_SR  cpu_sr = 0u;
	uint16_t i = 0;
	OS_ENTER_CRITICAL();//��ֹ�ж�
	{
		volatile FLASH_Status FLASHStatus = FLASH_COMPLETE;
		volatile uint32_t w_addr = 0x00;
		volatile uint32_t w_data=0x00;
		volatile uint32_t* p_w_data=0x00;
		//int i=0;
		FLASH_Unlock();
		FLASH_ClearFlag(FLASH_FLAG_BSY | FLASH_FLAG_EOP | FLASH_FLAG_PGERR | FLASH_FLAG_WRPRTERR);

		//��ȥ��Ӧҳ
		if(FLASHStatus==FLASH_COMPLETE)
		FLASHStatus = FLASH_ErasePage(SBOOT_UPGREAD_ADDR);
		//д��
		if(FLASHStatus==FLASH_COMPLETE)
		{
			w_addr = (uint32_t)(SBOOT_UPGREAD_ADDR);	
					
			p_w_data=(uint32_t*)(parameter);

			w_data=(uint32_t)(*p_w_data);//ȡ����Ӧ����

			for(i=0;(i<(sizeof(_SystemInformation)/4 + 1))&&(FLASHStatus == FLASH_COMPLETE);i++) //
			{
				FLASHStatus = FLASH_ProgramWord(w_addr, w_data);

				if(FLASHStatus!=FLASH_COMPLETE)
					break;
		 		w_addr = w_addr + 4;
		 		p_w_data++;
		 		w_data=(uint32_t)(*p_w_data) ;	//ȡ������
			}
		}
	}	
	OS_EXIT_CRITICAL(); //���ж�
}

//�³�������ݸ���
void SaveConfigToFlash(uint8_t* ptrBuff,uint16_t datLength)
{
	OS_CPU_SR  cpu_sr = 0u;
	uint16_t i = 0;
	
	FLASH_Unlock();
	FLASH_ClearFlag(FLASH_FLAG_BSY | FLASH_FLAG_EOP | FLASH_FLAG_PGERR | FLASH_FLAG_WRPRTERR);
	FLASH_ErasePage(PIDConfig_ADDR);
	
	OS_ENTER_CRITICAL();//��ֹ�ж�
	{
		volatile FLASH_Status FLASHStatus = FLASH_COMPLETE;
		volatile uint32_t     w_addr      = PIDConfig_ADDR;
		volatile uint32_t*    p_w_data    = (uint32_t*)ptrBuff;	//4�ֽ�ָ��
		volatile uint32_t     w_data      = (uint32_t)(*p_w_data);//ȡ����Ӧ����

		for(i=0;(i<(datLength/4))&&(FLASHStatus == FLASH_COMPLETE);i++) 
		{
			FLASHStatus = FLASH_ProgramWord(w_addr, w_data);
			w_addr = w_addr + 4;
			p_w_data++;
			w_data=(uint32_t)(*p_w_data)  ;	
		}
	}
	OS_EXIT_CRITICAL(); //���ж�
}
//����������ʱ�򣬴�Flash�ж�ȡȫ�ֲ�����

int Flash_ReadDat(uint32_t iAddress, uint8_t *buf, int32_t readLength) 
{
	int i = 0;
	while(i < readLength ) 
	{
		*(buf + i) = *(__IO uint8_t*) iAddress++;
		i++;
	}
	return i;
}




