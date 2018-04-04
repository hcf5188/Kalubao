/*******************************************************************
*                               ���ʹ���
*
*
********************************************************************/

#include "apptask.h"
#include "bsp.h"
#include "obd.h"
#include "saveFuel.h"
  
#define Eng_nRaw           (RCVData0[4]*256 + RCVData0[3])    //������ת��
#define Accped_rRaw         RCVData1[1]                       //̤��
#define CoEng_trqRaw       (RCVData2[5]*256 + RCVData2[4])    //���������Ť��
#define EngPrt_trqLimRaw   (RCVData3[1]*256 + RCVData3[0])    //������
#define CoEng_TrqFrcRaw     RCVData4[0]                       //Ħ��Ť��
#define EC1_PackNum         RCVData5[0]                       //Pack number
#define Moffrm_trqMaxRaw   (RCVData5[7]*256 + RCVData5[6])    //���Ť��
#define RateSpeedRaw       (RCVData5[5]*256 + RCVData5[4])    //Rate speed
#define BAM_EC1             RCVData6                          //
#define Curr_trqRaw         RCVData0[2]                       //��ǰŤ��
#define DrvDem_trqRaw       RCVData0[1]                       //��ʻԱ����Ť��

#define Acc_st             (RCVData2[7]*256 + RCVData2[6])    //Ѳ��״̬       ʲô��Ѳ��״̬��
//------------------------------------------------------------------------------------
#define Highload 3
#define Midload  2
#define Lowload  1
 
#define Accpet_tiPT1          0.9
//#define CoEng_tiPT1rDrvLoad   0.7   //ԭʼֵ
#define CoEng_tiPT1rDrvLoad   0.5
#define EEPROM_DATA_NUM_MAX   3
#define DRVDEM_TRQFLT_ENABLE  1
//-------------------------------------------------------------------------------------

u16	  SystemTime250us,SystemTime1ms,SystemTime10ms,SystemTime50ms,SystemTime100ms,SystemTime500ms,SystemTime1s,SystemTime3s;
u8	  B_TimeOut,CL_stOutflag;
u8    app1, app2, app3, numapp,appold1,appold2,appold3,appold4,appold5,appold6;
u8    CANerr_flg,CANerr_state,CANr1,CANr2,CANr3,CANr4,CANr5,CANr6,CANr7;
int   App_rFreeze, App_rquit;
float Moffrm_trqMax,EngPrt_trqLim,CoEng_TrqFrc,CoEng_trq,EngPrt_trqNet;
u16   trqmax;
u8    Load_stCurr,Load_stTarget,iout,jout;
int   App_dev;
signed char rdev,mrdev;

u8    Curr_trq,CAN7flag,CAN17flag;
int   numCAN,flagCAN;
float trqmaxfloat;

//CAN
u8 RCVData[16];      //��������֡ 
u8 RCVData0[16];     //��������֡
u8 RCVData1[16]; 
u8 RCVData2[16]; 
u8 RCVData3[16];
u8 RCVData4[16];
u8 RCVData5[16];
u8 RCVData6[16]; 
u8 tx_send1[8];
u8 trq[8];
u8 fReceivedData,fReceivedData17,fReceivedData7;
u8 fCAN7OK,fCAN17OK,fCANBUSOK,CANBUSNUM;
const u8 DataLenth;  //���ݳ��ȣ�����(ע�⣺CAN ÿһ֡������Ϊ0~8���ֽ�)

u8 CANbusClr;

float CoEng_r100msLoad[200];
int   Eng_nAvg100ms[200];
u8    CoEng_rtrqInr100ms[200];
u8    CoEng_rDrvLoad100ms[100];
long  Eng_nAvgSum,Coeng_rtrqInrSum,Coeng_rDrvLoadSum;
int   Eng_nAvg20,Coeng_trqInrCurr,CoEng_rtrqInrAvg;
u8    CoEng_rDrvLoad;
int   Accped_r100ms[30];
long  Accped_rSum;
int   Accped_r3s;
long int TrqlimLow,TrqlimMid;   //Ť�����Ƽ���
float TrqLoad;
int   CoEng_trqDrv10Load;

u8    fCanOK,n;
u8    nop;

// u8 trq2, trq3,trq31,trq31RunUp,trq32,trq4,Vss;
 u8 swt,swt1;

 float Eng_nAvg,Eng_nAvgFlt, Eng_nAvgFltOld,Accped_r,Accped_rFlt,Accped_rFltOld,RateSpeed,CoEng_rCurLoad,CoEng_rAvg20Load,CoEng_rAvg3Load,CoEng_rDrv10Load;
 float CoEng_rDrvLoadFlt,CoEng_rDrvLoadFltOld;
 float CoEng_rSumLoad,CoEng_rSum3Load;
 u16   Eng_nAvgplus,Eng_nAvgFltInt;
 int   CoEng_trqint,Accped_rint,Accped_rFltint;
 u8    Num_ConstLim,Num_LowLoad,Num_MidLoad,TrqDrvFlt;
 
 
 void GearDetect(void);
 void CoEng_rloadCal(void);
 void TrqLim(void);
 void ConstLimitation(void);
 void UpdateTxDataAMT(void);
uint8_t senTime = 0;
//��������
 void SaveFuleTask(void *pdata)
{
	Load_stCurr   = Highload;      // ��ǰ����    �ᡢ�С����� ��ģʽ
	Load_stTarget = Midload;       // ���õ�ǰ��  �ᡢ�С����� ģʽ�ļ�¼����
	trqmax        = 225;
	TrqLoad       = 225;           // ��һ��װ��ֵ
	while(1)
	{
		OSTimeDlyHMSM(0,0,0,1);	
		SystemTime10ms  ++;
		SystemTime50ms  ++;
		SystemTime100ms ++;
		SystemTime500ms ++;
		SystemTime1s ++;

		//ÿ���붼��������ݼ��
		if(SystemTime10ms >= 5)
		{
			SystemTime10ms = 0;
			if(fCanOK) 
			{
				GearDetect();  // �����źŵĻ�ȡ
				TrqLim();      // ���ݷ�����ת�٣��ó����ء�����״̬�¶����ֵ
/*############################################################################### 
						�غ�״̬�仯ʱ, Ť�����Ƶ�б�ʱ仯
################################################################################*/
				if(Load_stCurr == Highload)              //��ǰ������ģʽ  �˴��Ǳ�־
				{
					if(Load_stTarget == Midload)         //�ֻ��趨���е�ģʽ   �˴���ģʽ��־
					{
						if(TrqLoad > TrqlimMid)          //�����������ʵֵ���бȽ���
							TrqLoad = TrqLoad - 0.08;    //   8/s
						else 
						{
							Load_stCurr = Load_stTarget; //�ﵽԤ��״̬�ˣ���ֵ����Ӧ��־
							TrqLoad = TrqlimMid;
						}
					} 
					else if(Load_stTarget == Lowload)    //Ӧ��������ģʽ
					{
						if(TrqLoad > TrqlimLow)
							TrqLoad = TrqLoad - 0.08;    //    8/s
						else 
						{
							Load_stCurr = Load_stTarget;
							TrqLoad = TrqlimLow;
						}
					} 
					else TrqLoad = 225;     
				} 
				else if(Load_stCurr == Midload)
				{
					if(Load_stTarget == Highload)
					{
						if(TrqLoad < 225) 
							TrqLoad = TrqLoad + 0.08; //8/s
						else 
						{
							Load_stCurr = Load_stTarget;
							TrqLoad = 225;
						}
					} 
					else if(Load_stTarget == Lowload)
					{
						if(TrqLoad > TrqlimLow) 
							TrqLoad = TrqLoad - 0.08; //8/s
						else 
						{
							Load_stCurr = Load_stTarget;
							TrqLoad     = TrqlimLow;
						}
					} 
					else TrqLoad = TrqlimMid; 
				}
				else if(Load_stCurr  == Lowload)
				{
					if(Load_stTarget == Highload)
					{
						if(TrqLoad < 225) 
							TrqLoad = TrqLoad + 0.08; //8/s
						else
						{
							Load_stCurr = Load_stTarget;
							TrqLoad     = 225;
						}
					} 
					else if(Load_stTarget == Midload)
					{
						if(TrqLoad<TrqlimMid) 
							TrqLoad = TrqLoad + 0.08; //  8/s
						else 
						{
							Load_stCurr = Load_stTarget;
							TrqLoad = TrqlimMid;
						}
					} 
					else TrqLoad = TrqlimLow; 
				}            
				ConstLimitation();       //���͹���
				UpdateTxDataAMT();       //���ư�����С��ֵ��   ����Ť�� �� �趨Ť�ػ�������
//				if(senTime == 0)
//				{
					OBD_CAN_SendData(0x0C000021,CAN_ID_EXT,trq); //todo ���ͽ��ͣ��ȴ�����
//					senTime = 1;
//				}
//				else
//					senTime = 0;
				
			}
		}
		if(SystemTime50ms >= 25)              //�����˳� б��   50ms + 1
		{
			SystemTime50ms = 0;
			if(fCanOK) 
			{
				if(CL_stOutflag == 1) 
				{
					if(trqmax < 225)    //�˳�����ģʽ  ���ֵҪ����
					{
						trqmax ++;
					} 
					else
					{
						CL_stOutflag = 0;
					}
				}
			}
		}
		if(SystemTime100ms >= 50)	 //���� 20 ���ƽ��Ť�أ�3���ƽ������
		{
			SystemTime100ms = 0;
			numCAN=0;
			if(fCanOK)
			{
				CoEng_rloadCal();    //ƽ�������ʼ���, ƽ��ת�ټ���, ƽ�����ż���
//##############################################################
				CANerr_flg = 1;
			}
			else 
				CANerr_flg = 0;
		} 
		if(SystemTime500ms >= 500)
		{
			SystemTime500ms = 0;
		}
	}
}

void SaveJieYouData(CanRxMsg* CAN1_RxMsg)
{
	switch(CAN1_RxMsg->ExtId)
	{
		default:break;
	} 
}

long int calu(int x1, int x2, int y1, int y2, int x)
{
	long int y;
	y = ((long)(y2-y1)*x+(long)y1*x2-(long)x1*y2)/(x2-x1);
	return(y);
}
float SignalFlter(float x1, float yold, float ti) 
{
    float y;
    y=(x1 - yold)*(0.01 / ti) + yold;
    return( y );
}

void GearDetect(void)  //10ms     �����źŵĻ�ȡ 
{
	Eng_nAvg       = Eng_nRaw / 8;                     //������ת�� / 8
	Eng_nAvgFlt    = SignalFlter(Eng_nAvg,Eng_nAvgFltOld,0.5);//
	Eng_nAvgFltOld = Eng_nAvgFlt;
	Eng_nAvgFltInt = (int)Eng_nAvgFlt;
	Curr_trq       = Curr_trqRaw;         //�ڲ�Ť�� ��ǰŤ��
	if(BAM_EC1[0]==0x20 && BAM_EC1[1] == 0x27 && BAM_EC1[2] == 0x00)
	{	  
		if(EC1_PackNum == 3)//�������û�ҵ���ֵ�ĵط���
		{
			Moffrm_trqMax = Moffrm_trqMaxRaw;   //���Ť��
		} 
		else if(EC1_PackNum == 1)
		{
			RateSpeed = RateSpeedRaw/8;
		}
	}    
	Coeng_trqInrCurr = Curr_trq * Moffrm_trqMax / 100;   //��ֵ֮����ôû�к����Ĳ����أ�û��
	EngPrt_trqLim  = EngPrt_trqLimRaw / 10;          //������ / 10
	CoEng_trq      = CoEng_trqRaw / 10;                //�����������Ť�� / 10
	CoEng_trqint   = (signed int) CoEng_trq;		     //ȡ��
	Accped_r       = Accped_rRaw * 0.4;                //̤�� * 0.4
	Accped_rint    = (int)Accped_r;	                 //��̤��������ȡ��
	Accped_rFlt    = SignalFlter(Accped_r,Accped_rFltOld,Accpet_tiPT1);//��̤�����㣬�ϴ�̤�壬0.9��
	Accped_rFltOld = Accped_rFlt;                    //����ֵ���ǣ���̤�壩
	Accped_rFltint = (int)Accped_rFlt;               //ȡ��
	CoEng_TrqFrc   = CoEng_TrqFrcRaw - 125;	         //Ħ��Ť��  ����
	CoEng_rDrvLoad = DrvDem_trqRaw   - 125;          //��ʻԱ����Ť�� ����
	CoEng_rDrvLoadFlt    = SignalFlter((float)CoEng_rDrvLoad,CoEng_rDrvLoadFltOld,CoEng_tiPT1rDrvLoad);//������Ť�أ��ϴ�����0.7��
	CoEng_rDrvLoadFltOld = CoEng_rDrvLoadFlt;                                   //�ɵĸ����µ�
	CoEng_trqDrv10Load   = CoEng_rDrv10Load*Moffrm_trqMax / 100;            	//����Ť�ء����Ť�ء�100
	EngPrt_trqNet        = EngPrt_trqLim - CoEng_TrqFrc * Moffrm_trqMax / 100;	//��ֵ֮���޲�����  ������ - Ħ��Ť�ؼ���*���Ť������/100
}

void CoEng_rloadCal(void)  //100ms   ƽ�������ʼ���, ƽ��ת�ټ���, ƽ�����ż���, 100ms * 200 = 20s
{
    static u8  i, j;  
//----------------------------------------------------------------------------------------   
	for(i=0;i<199;i++)
	{
		CoEng_rtrqInr100ms[i]=CoEng_rtrqInr100ms[i+1];
	}
	CoEng_rtrqInr100ms[199] = Curr_trq;    //��ǰŤ��            
	Coeng_rtrqInrSum = 0;
	for(j=0;j<200;j++)
	{
		Coeng_rtrqInrSum = Coeng_rtrqInrSum+CoEng_rtrqInr100ms[j];
	}
	CoEng_rtrqInrAvg = Coeng_rtrqInrSum / 200;  // 20s ƽ�����ɼ���%
	
//-----------------------------------------------------------------------------------------	
	for(i=0;i<100;i++)
	{
		CoEng_rDrvLoad100ms[i] = CoEng_rDrvLoad100ms[i+1];
	}
	CoEng_rDrvLoad100ms[99] = CoEng_rDrvLoad;
	Coeng_rDrvLoadSum = 0;                                           // 10s ��ʻԱƽ������Ť��  %
	for(j = 0;j < 100;j++)
	{
		Coeng_rDrvLoadSum = Coeng_rDrvLoadSum + CoEng_rDrvLoad100ms[j];
	}
	CoEng_rDrv10Load = Coeng_rDrvLoadSum / 100;	
	
//------------------------------------------------------------------------------------------	
	for(i=0;i<199;i++)
	{
		Eng_nAvg100ms[i]=Eng_nAvg100ms[i+1];
	}
	Eng_nAvg100ms[199]=Eng_nAvg;
	Eng_nAvgSum=0;
	for(j=0;j<200;j++)                                            // 20s ������ƽ��ת��
	{
		Eng_nAvgSum = Eng_nAvgSum+Eng_nAvg100ms[j];
	}
	Eng_nAvg20=Eng_nAvgSum/200;
//------------------------------------------------------------------------------------------	
	for(i=0;i<29;i++)
	{
		Accped_r100ms[i] = Accped_r100ms[i+1];
	}
	Accped_r100ms[29] = Accped_rint;
	Accped_rSum=0;                                                //3sƽ������̤��
	for(j = 0;j < 30;j ++)
	{
		Accped_rSum=Accped_rSum+Accped_r100ms[j];
	}
	Accped_r3s = Accped_rSum/30;
//------------------------------------------------------------------------------------------		
}
extern const uint16_t  TrqlimM[2][15];
extern const uint16_t TrqlimL[2][15];
//���ݷ�����ת�٣����ó����ء����ص�Ť������ֵ��
void TrqLim(void)          //Ť�����Ƽ���
{   
	static u8  i,inow; 

	for(i=0;i<14;i++)
	{
		if(Eng_nAvgFltInt >= TrqlimM[0][i] && Eng_nAvgFltInt < TrqlimM[0][i+1]) //���ݷ�����ת�����ж�
			inow = i;
	}
	TrqlimMid = calu(TrqlimM[0][inow],TrqlimM[0][inow+1],TrqlimM[1][inow],TrqlimM[1][inow+1],Eng_nAvgFltInt);
       //����ֵ  ����
	for(i=0;i<14;i++)
	{
		if(Eng_nAvgFltInt >= TrqlimL[0][i] && Eng_nAvgFltInt < TrqlimL[0][i+1])
			inow = i;
	}//���� ����� 225   ECU ����� 125 ����˼�� ECU ֻ��  125 - 225 ֮�両������
	TrqlimLow = calu(TrqlimL[0][inow],TrqlimL[0][inow+1],TrqlimL[1][inow],TrqlimL[1][inow+1],Eng_nAvgFltInt);  	
}

#define TimerAPP0Reset() {TimerAPPCnt0 = 0;APP_T0 = 0;} //reset APP timer0 

//���͹���  ��Ҫ����������trqmax��
void ConstLimitation(void)
{
	static u16 TimerAPPCnt0 ;
	static u8  APP_T0;

	if(TimerAPPCnt0 < 65530)
		TimerAPPCnt0 ++;
	if(TimerAPPCnt0 >= 500)
		APP_T0 = 1; 

	rdev = 20;mrdev = -20;

	App_dev = Accped_rint - Accped_r3s;  //���ſ��� - 3s ��ƽ�����ſ���

	if(App_rFreeze == 0) //���ſ���ֵ����   �ڶ���״̬ - !0  ���ڶ���״̬ = 0
	{ 	
		if((Load_stCurr == Midload || Load_stCurr == Lowload) && //������ ���� ���� ���� ģʽ��
			App_dev >= mrdev && App_dev <= rdev &&   //���ŵĲ�����Χ  ���� 20%
			Eng_nAvg > 1000 && 				         //������ת�� > 1000
			Accped_rint > 10 && Accped_rint < 85 &&  //���ſ���   > 10%  < 85%
		    Accped_r3s > 10)                         // 3 ���ƽ������ > 10%
		{
			if((APP_T0 == 1)&&(CL_stOutflag == 0))   //��ʱ���� 5 ��
			{
				trqmax = CoEng_rtrqInrAvg + 1; // trqmax - ���͵�Ť������   20s ƽ�����ɼ���
				TimerAPP0Reset();              // ��ʱ����λ������  APP_T0
				App_rFreeze = Accped_rint;     // ��ס��ǰ�����ſ���
			}
		}
		else 
			TimerAPP0Reset();                  //�˳�����
	}
	else if(Load_stCurr == Highload            //��ǰ������
		||Accped_r <= 10||Accped_r >= 85       //�ɵ����� ���� �ȵ���
		||App_dev <- 25 ||App_dev  >  15       //��ǰ���ſ��� - 3sƽ������
		||(Accped_rint - App_rFreeze) > 20 || (Accped_rint - App_rFreeze) <- 25    //��ǰ̤�忪���붳��ֵ���бȽ�
		||(Accped_r3s - App_rFreeze) > 15)     //3��̤��ƽ�������붳��ֵ�Ƚ� 
	{
		CL_stOutflag = 1; 					   //ȡ��Ť������
		App_rFreeze  = 0;                      //�ȴ�
		TimerAPP0Reset();                      //���¼�ʱ 
	}
	else 
		TimerAPP0Reset();	 
}

/*=====================================================================================
��  ��: CANͨѶ���ϼ��                                               
��  ��: ErrDetecSW:���ϼ�⿪��, ErrDebT:�趨ʱ��( unit:10ms ), Signal: ���յ�CAN�źű�־                                                             
��  ��:                                     
˵  ��:�����趨ʱ�� ErrDebT û�н��յ� CAN ����ʱ���ϵƵ���
       per 1ms
======================================================================================*/
u8 CanErrDetec(u16 ErrDebT,u8 Signal)
{
   static u16 CanErrCnt = 150;
    
   if(CanErrCnt < 65530)CanErrCnt++;
   if(Signal==1 && CANr1==1 && CANr2==1 && CANr3==1 && CANr4==1 && CANr5==1 && CANr6==1){
     CanErrCnt = 0; 
   }
   if(CanErrCnt >= ErrDebT)
	   return 0;
   else 
	   return 1;

}
u8 CanErrDetec7(u16 ErrDebT,u8 Signal)
{
	static u16 CanErrCnt = 150;

	if(CanErrCnt < 65530)
		CanErrCnt++;
	if(Signal==1 && CANr1==1 && CANr2==1 && CANr3==1 && CANr4==1 && CANr5==1 && CANr6==1)
		CanErrCnt = 0;
	     
	if(CanErrCnt >= ErrDebT)
		return 0;
	else 
		return 1;
}
u8 CanErrDetec17(u16 ErrDebT,u8 Signal)
{
	static u16 CanErrCnt = 150;

	if(CanErrCnt < 65530)
		CanErrCnt ++;
	if(Signal == 1 && CANr1 == 1 && CANr2 == 1 && CANr3 == 1 && CANr4 == 1 && CANr5==1 && CANr6 == 1){
		CanErrCnt = 0; 
	}     
	if(CanErrCnt >= ErrDebT)
		return 0;
	else 
		return 1;
}

/* ========================================================================
��  ��:					Can �������ݸ���
��  ��:
��  ��:
˵  ��:per 10ms
======================================================================== */
//���յ͵�������
void UpdateTxDataAMT(void)
{
	//todo����ϸ�����������
    if(Acc_st==1)     //���͵Ļ����Ť�����ƣ����һ��Ť�����ƣ�����ҪȡС���͸�ECU
		trq[3] = 225;
    else
																			{
         if(DRVDEM_TRQFLT_ENABLE)    //ʼ����ʹ�ܵ�
         {
            if(Load_stCurr == Midload || Load_stCurr == Lowload)//���ػ�������
            {
               TrqDrvFlt=(u8)CoEng_rDrvLoadFlt + 125; //����Ť��
            } 
			else TrqDrvFlt = 225;
         } 
		 else 
			 TrqDrvFlt = 225;  		//����Ť��ֵ�Ƚ�С�Ļ����Ͱ�����Ť�عص�
         if(TrqLoad < trqmax)  		//����С�����ֵ��
         {
            if(TrqLoad < TrqDrvFlt) 
				trq[3] = TrqLoad;
            else 
				trq[3] = TrqDrvFlt;
         }
         else
         {
            if(trqmax < TrqDrvFlt) 
				trq[3] = trqmax;
            else 
				trq[3] = TrqDrvFlt;
         }
		 trq[0] = 35;
		 trq[1] = 0xFF;
		 trq[2] = 0xFF;
    }
	 trq[0] = 35;
	 trq[1] = 0xFF;
	 trq[2] = 0xFF;
}

