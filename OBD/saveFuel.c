/*******************************************************************
*                               节油代码
*
*
********************************************************************/

#include "apptask.h"
#include "bsp.h"
#include "obd.h"
#include "saveFuel.h"
  
#define Eng_nRaw           (RCVData0[4]*256 + RCVData0[3])    //发动机转速
#define Accped_rRaw         RCVData1[1]                       //踏板
#define CoEng_trqRaw       (RCVData2[5]*256 + RCVData2[4])    //发动机输出扭矩
#define EngPrt_trqLimRaw   (RCVData3[1]*256 + RCVData3[0])    //外特性
#define CoEng_TrqFrcRaw     RCVData4[0]                       //摩擦扭拒
#define EC1_PackNum         RCVData5[0]                       //Pack number
#define Moffrm_trqMaxRaw   (RCVData5[7]*256 + RCVData5[6])    //最大扭拒
#define RateSpeedRaw       (RCVData5[5]*256 + RCVData5[4])    //Rate speed
#define BAM_EC1             RCVData6                          //
#define Curr_trqRaw         RCVData0[2]                       //当前扭拒
#define DrvDem_trqRaw       RCVData0[1]                       //驾驶员需求扭拒

#define Acc_st             (RCVData2[7]*256 + RCVData2[6])    //巡航状态       什么是巡航状态？
//------------------------------------------------------------------------------------
#define Highload 3
#define Midload  2
#define Lowload  1
 
#define Accpet_tiPT1          0.9
//#define CoEng_tiPT1rDrvLoad   0.7   //原始值
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
u8 RCVData[16];      //接收数据帧 
u8 RCVData0[16];     //接收数据帧
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
const u8 DataLenth;  //数据长度，常量(注意：CAN 每一帧的数据为0~8个字节)

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
long int TrqlimLow,TrqlimMid;   //扭矩限制计算
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
//节油任务
 void SaveFuleTask(void *pdata)
{
	Load_stCurr   = Highload;      // 当前正处    轻、中、重载 的模式
	Load_stTarget = Midload;       // 设置当前于  轻、中、重载 模式的记录变量
	trqmax        = 225;
	TrqLoad       = 225;           // 第一次装载值
	while(1)
	{
		OSTimeDlyHMSM(0,0,0,1);	
		SystemTime10ms  ++;
		SystemTime50ms  ++;
		SystemTime100ms ++;
		SystemTime500ms ++;
		SystemTime1s ++;

		//每毫秒都会进行数据检查
		if(SystemTime10ms >= 5)
		{
			SystemTime10ms = 0;
			if(fCanOK) 
			{
				GearDetect();  // 输入信号的获取
				TrqLim();      // 根据发动机转速，得出中载、轻载状态下额定限制值
/*############################################################################### 
						载荷状态变化时, 扭拒限制的斜率变化
################################################################################*/
				if(Load_stCurr == Highload)              //当前是重载模式  此处是标志
				{
					if(Load_stTarget == Midload)         //手机设定是中档模式   此处是模式标志
					{
						if(TrqLoad > TrqlimMid)          //这个就是拿真实值进行比较了
							TrqLoad = TrqLoad - 0.08;    //   8/s
						else 
						{
							Load_stCurr = Load_stTarget; //达到预期状态了，赋值、相应标志
							TrqLoad = TrqlimMid;
						}
					} 
					else if(Load_stTarget == Lowload)    //应该是轻载模式
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
				ConstLimitation();       //稳油功能
				UpdateTxDataAMT();       //限制按照最小的值来   需求扭矩 跟 设定扭矩还有稳油
//				if(senTime == 0)
//				{
					OBD_CAN_SendData(0x0C000021,CAN_ID_EXT,trq); //todo 发送节油，等待运行
//					senTime = 1;
//				}
//				else
//					senTime = 0;
				
			}
		}
		if(SystemTime50ms >= 25)              //稳油退出 斜率   50ms + 1
		{
			SystemTime50ms = 0;
			if(fCanOK) 
			{
				if(CL_stOutflag == 1) 
				{
					if(trqmax < 225)    //退出节油模式  最大值要自增
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
		if(SystemTime100ms >= 50)	 //计算 20 秒的平均扭矩，3秒的平均油门
		{
			SystemTime100ms = 0;
			numCAN=0;
			if(fCanOK)
			{
				CoEng_rloadCal();    //平均负荷率计算, 平均转速计算, 平均油门计算
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

void GearDetect(void)  //10ms     输入信号的获取 
{
	Eng_nAvg       = Eng_nRaw / 8;                     //发动机转速 / 8
	Eng_nAvgFlt    = SignalFlter(Eng_nAvg,Eng_nAvgFltOld,0.5);//
	Eng_nAvgFltOld = Eng_nAvgFlt;
	Eng_nAvgFltInt = (int)Eng_nAvgFlt;
	Curr_trq       = Curr_trqRaw;         //内部扭拒 当前扭矩
	if(BAM_EC1[0]==0x20 && BAM_EC1[1] == 0x27 && BAM_EC1[2] == 0x00)
	{	  
		if(EC1_PackNum == 3)//这个变量没找到赋值的地方啊
		{
			Moffrm_trqMax = Moffrm_trqMaxRaw;   //最大扭矩
		} 
		else if(EC1_PackNum == 1)
		{
			RateSpeed = RateSpeedRaw/8;
		}
	}    
	Coeng_trqInrCurr = Curr_trq * Moffrm_trqMax / 100;   //赋值之后怎么没有后续的操作呢？没用
	EngPrt_trqLim  = EngPrt_trqLimRaw / 10;          //外特性 / 10
	CoEng_trq      = CoEng_trqRaw / 10;                //发动机的输出扭矩 / 10
	CoEng_trqint   = (signed int) CoEng_trq;		     //取整
	Accped_r       = Accped_rRaw * 0.4;                //踏板 * 0.4
	Accped_rint    = (int)Accped_r;	                 //对踏板运算结果取整
	Accped_rFlt    = SignalFlter(Accped_r,Accped_rFltOld,Accpet_tiPT1);//（踏板运算，上次踏板，0.9）
	Accped_rFltOld = Accped_rFlt;                    //将旧值覆盖，（踏板）
	Accped_rFltint = (int)Accped_rFlt;               //取整
	CoEng_TrqFrc   = CoEng_TrqFrcRaw - 125;	         //摩擦扭矩  运算
	CoEng_rDrvLoad = DrvDem_trqRaw   - 125;          //驾驶员需求扭矩 运算
	CoEng_rDrvLoadFlt    = SignalFlter((float)CoEng_rDrvLoad,CoEng_rDrvLoadFltOld,CoEng_tiPT1rDrvLoad);//（需求扭矩，上次需求，0.7）
	CoEng_rDrvLoadFltOld = CoEng_rDrvLoadFlt;                                   //旧的覆盖新的
	CoEng_trqDrv10Load   = CoEng_rDrv10Load*Moffrm_trqMax / 100;            	//需求扭矩、最大扭矩、100
	EngPrt_trqNet        = EngPrt_trqLim - CoEng_TrqFrc * Moffrm_trqMax / 100;	//赋值之后无操作啊  外特性 - 摩擦扭矩计算*最大扭矩运算/100
}

void CoEng_rloadCal(void)  //100ms   平均负荷率计算, 平均转速计算, 平均油门计算, 100ms * 200 = 20s
{
    static u8  i, j;  
//----------------------------------------------------------------------------------------   
	for(i=0;i<199;i++)
	{
		CoEng_rtrqInr100ms[i]=CoEng_rtrqInr100ms[i+1];
	}
	CoEng_rtrqInr100ms[199] = Curr_trq;    //当前扭矩            
	Coeng_rtrqInrSum = 0;
	for(j=0;j<200;j++)
	{
		Coeng_rtrqInrSum = Coeng_rtrqInrSum+CoEng_rtrqInr100ms[j];
	}
	CoEng_rtrqInrAvg = Coeng_rtrqInrSum / 200;  // 20s 平均负荷计算%
	
//-----------------------------------------------------------------------------------------	
	for(i=0;i<100;i++)
	{
		CoEng_rDrvLoad100ms[i] = CoEng_rDrvLoad100ms[i+1];
	}
	CoEng_rDrvLoad100ms[99] = CoEng_rDrvLoad;
	Coeng_rDrvLoadSum = 0;                                           // 10s 驾驶员平均需求扭矩  %
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
	for(j=0;j<200;j++)                                            // 20s 发动机平均转速
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
	Accped_rSum=0;                                                //3s平均油门踏板
	for(j = 0;j < 30;j ++)
	{
		Accped_rSum=Accped_rSum+Accped_r100ms[j];
	}
	Accped_r3s = Accped_rSum/30;
//------------------------------------------------------------------------------------------		
}
extern const uint16_t  TrqlimM[2][15];
extern const uint16_t TrqlimL[2][15];
//根据发动机转速，查表得出轻载、中载的扭矩限制值，
void TrqLim(void)          //扭矩限制计算
{   
	static u8  i,inow; 

	for(i=0;i<14;i++)
	{
		if(Eng_nAvgFltInt >= TrqlimM[0][i] && Eng_nAvgFltInt < TrqlimM[0][i+1]) //根据发动机转速来判断
			inow = i;
	}
	TrqlimMid = calu(TrqlimM[0][inow],TrqlimM[0][inow+1],TrqlimM[1][inow],TrqlimM[1][inow+1],Eng_nAvgFltInt);
       //限制值  中载
	for(i=0;i<14;i++)
	{
		if(Eng_nAvgFltInt >= TrqlimL[0][i] && Eng_nAvgFltInt < TrqlimL[0][i+1])
			inow = i;
	}//轻载 如果是 225   ECU 会减掉 125 ，意思是 ECU 只在  125 - 225 之间浮动运行
	TrqlimLow = calu(TrqlimL[0][inow],TrqlimL[0][inow+1],TrqlimL[1][inow],TrqlimL[1][inow+1],Eng_nAvgFltInt);  	
}

#define TimerAPP0Reset() {TimerAPPCnt0 = 0;APP_T0 = 0;} //reset APP timer0 

//稳油功能  主要是用来计算trqmax的
void ConstLimitation(void)
{
	static u16 TimerAPPCnt0 ;
	static u8  APP_T0;

	if(TimerAPPCnt0 < 65530)
		TimerAPPCnt0 ++;
	if(TimerAPPCnt0 >= 500)
		APP_T0 = 1; 

	rdev = 20;mrdev = -20;

	App_dev = Accped_rint - Accped_r3s;  //油门开度 - 3s 的平均油门开度

	if(App_rFreeze == 0) //油门开度值冻结   在冻结状态 - !0  不在冻结状态 = 0
	{ 	
		if((Load_stCurr == Midload || Load_stCurr == Lowload) && //设置在 中载 或者 轻载 模式下
			App_dev >= mrdev && App_dev <= rdev &&   //油门的波动范围  正负 20%
			Eng_nAvg > 1000 && 				         //发动机转速 > 1000
			Accped_rint > 10 && Accped_rint < 85 &&  //油门开度   > 10%  < 85%
		    Accped_r3s > 10)                         // 3 秒的平均开度 > 10%
		{
			if((APP_T0 == 1)&&(CL_stOutflag == 0))   //计时保持 5 秒
			{
				trqmax = CoEng_rtrqInrAvg + 1; // trqmax - 稳油的扭矩限制   20s 平均负荷计算
				TimerAPP0Reset();              // 定时器复位就是清  APP_T0
				App_rFreeze = Accped_rint;     // 记住当前的油门开度
			}
		}
		else 
			TimerAPP0Reset();                  //退出稳油
	}
	else if(Load_stCurr == Highload            //当前在重载
		||Accped_r <= 10||Accped_r >= 85       //松掉油门 或者 踩到底
		||App_dev <- 25 ||App_dev  >  15       //当前油门开度 - 3s平均开度
		||(Accped_rint - App_rFreeze) > 20 || (Accped_rint - App_rFreeze) <- 25    //当前踏板开度与冻结值进行比较
		||(Accped_r3s - App_rFreeze) > 15)     //3秒踏板平均开度与冻结值比较 
	{
		CL_stOutflag = 1; 					   //取消扭矩限制
		App_rFreeze  = 0;                      //等待
		TimerAPP0Reset();                      //重新计时 
	}
	else 
		TimerAPP0Reset();	 
}

/*=====================================================================================
功  能: CAN通讯故障检测                                               
参  数: ErrDetecSW:故障检测开关, ErrDebT:设定时间( unit:10ms ), Signal: 接收到CAN信号标志                                                             
返  回:                                     
说  明:超过设定时间 ErrDebT 没有接收到 CAN 数据时故障灯点亮
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
功  能:					Can 发送数据更新
参  数:
返  回:
说  明:per 10ms
======================================================================== */
//按照低的来限制
void UpdateTxDataAMT(void)
{
	//todo：详细讲讲这个代码
    if(Acc_st==1)     //稳油的会算出扭矩限制，查表一个扭矩限制，最终要取小输送给ECU
		trq[3] = 225;
    else
																			{
         if(DRVDEM_TRQFLT_ENABLE)    //始终是使能的
         {
            if(Load_stCurr == Midload || Load_stCurr == Lowload)//中载或者轻载
            {
               TrqDrvFlt=(u8)CoEng_rDrvLoadFlt + 125; //需求扭矩
            } 
			else TrqDrvFlt = 225;
         } 
		 else 
			 TrqDrvFlt = 225;  		//需求扭矩值比较小的话，就把需求扭矩关掉
         if(TrqLoad < trqmax)  		//限制小于最大值。
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

