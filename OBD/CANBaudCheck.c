#include "apptask.h"
#include "obd.h"
#include "bsp.h"

uint8_t a = 0;
void CANBaudCheckTask (void *pdata)
{
	
	while(1)
	{
		CAN1_BaudSet(CANBAUD_1M);
		a =1;
		OSTimeDlyHMSM(0,0,2,0);
		
		CAN1_BaudSet(CANBAUD_500K);
		a=2;
		OSTimeDlyHMSM(0,0,2,0);
		
		CAN1_BaudSet(CANBAUD_250K);
		a=3;
		OSTimeDlyHMSM(0,0,2,0);
	}
}































































////�ж��Ƿ���в������л��ͳ�ʱ�˳�
//void set_change_state_erred(uint8 canid)
//{
//	LPC_CAN_TypeDef* pCan;

//	pCan = CAN_GetPointer(canid);

//	//����������������ϼ�⵽����ʱ����ʾ���ݷ���ʧ��
//	//"��������"Ӧ����Ϊ��CANxGSR�е�ESλΪ1�����ҷ��ͳ��������>��128
//	//���ߴ�����Ϊ:CANxICR�е�BEIλΪ1�����Ǽ�⵽���ߴ����жϱ���λ
//	if(((pCan->GSR & CAN_GSR_ES)&&((pCan->GSR & 0xff000000) >=128))	|| (pCan->ICR & CAN_ICR_BEI))
//	{  
//		//����һ�β����ʱ����û��Э�̵�˵��û�в�������֮ƥ��
//		//ǿ�����ò�����Ϊ���ֵ1000K
//		//ͬʱ�����˳�������Э�̣����صȴ���ʱ
//		if(try_num[canid] == 16)
//		{
//			try_num[canid]  = 0;
//			CAN_setting[canid].baudrate  = set_baudrate[try_num[canid]];
//			auto_csd_start(canid);
//			baudrate_change_state[canid] = 0;
//			return ;
//		}
//		//��������ͬ�����ظ����
//		if(CAN_setting[canid].baudrate ==set_baudrate[try_num[canid]])
//		{
//			try_num[canid]++;
//		}
//		//�л���λ���ʱ�����һ��
//		CAN_setting[canid].baudrate = set_baudrate[try_num[canid]];
//		auto_csd_start(canid);
//		send_data_state[canid] = 0;
//		try_num[canid]++;
//	}
//	//���ó�ʱʱ��1s
//	//���ʱ�䵽��û�г��ִ�����֤����������ȷ��
//	//��ֹͣ���β����ʵ�Э��
//	else if( baudrate_time_out[canid] >= 1000)
//	{
//		baudrate_change_state[canid] = 0;
//	}
//}
// 
// 
////ÿ�η���һ֡��������
//void send_test_data(uint8 canid)
//{
//	if(send_data_state[canid] == 0)
//	{
//		my_can_write(&CAN_port[canid],(tCAN_frame *)test_arry,1); 
//		baudrate_time_out[canid] = 1;
//		send_data_state[canid] = 1;
//	}  
//}
////����������
//void auto_baudrate_set(void)
//{
//	uint8 i;
//	for(i=0;i<MAX_CAN_NUM;i++)
//	{
//		if(baudrate_change_state[i]== 1)
//		{
//			send_test_data(i);
//			set_change_state_erred(i);
//		}
//	}
//}

////�Զ������ʼ��ʱ�����ò����ʳ�ֵ������ҳ�����޹�
//void auto_baudrate_init(void)
//{
//	uint8 i;
//	//��ʼ����·CAN�Ĳ����ʾ�Ϊ1000K
//	//���ֵ����ҳ�����޹أ�����������flash��
//	for(i=0;i<MAX_CAN_NUM;i++)
//	{
//		CAN_setting[i].baudrate = set_baudrate[0];   //1000K
//	}
//}
//void get_pin_state(void)
//{
//	uint8 i;
//	//��ȡ����״̬����Ϊ��ʱ������������Э��
//	if(GPIO_ReadValue(UP_APP_PORT)&UP_APP_MASK)
//	{
//		if(pin_state != 0)
//		{
//			pin_state = 0;
//		}
//		return ;
//	}
//	//���pin_state��״̬��ã���Ҫ�����ڵ��ͻ��豸�Ĳ����ʸı�ʱ
//	//����ͨ���������ŵ���������������ʱ���������ʸı�

//	//��Ҫ��ʽΪ:���ͻ��豸���������ú�֮���������������������ʸı�
//	//��һ��Э�̺�֮������ͻ��豸�Ĳ����ʸı䣬����Ҫ���������ߣ�Ȼ��������
//	//�����в����ʵĸı�
//	else
//	{
//		if(pin_state == 0)
//		{
//			for(i=0;i<MAX_CAN_NUM;i++)
//			{    
//				try_num[i] = 0;
//				send_data_state[i] = 0;
//				baudrate_change_state[i] = 1;
//				auto_csd_start(i);
//			}
//			pin_state = 1;
//		}
//		else
//			return ;
//	}
//}
















