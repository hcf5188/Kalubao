#include "apptask.h"



/*************************   CDMA服务函数声明部分   *****************************/
static void CDMAPowerOpen_Close(void);//这段代码是用来启动/关闭CDMA
static void CDMAConfigInit(void );    //初始化配置CDMA

#define CDMARECBUF_SIZE  10       //接收消息队列保存消息的最大量
void *cdmaRecBuf[CDMARECBUF_SIZE];//用于存放指向邮箱的指针
OS_EVENT *CDMARecieveQ;           //指向CDMA接收消息队列的指针

#define CDMASENDBUF_SIZE  100       //发送消息队列保存消息的最大量
void *cdmaSendBuf[CDMASENDBUF_SIZE];//用于存放指向邮箱的指针
OS_EVENT *CDMASendQ;                //指向CDMA发送消息队列的指针

/*************************   MG2639常用的 “AT”指令     ***********************/
const uint8_t atCmd[]   = "AT\r";                //AT 测试指令
const uint8_t ate0Cmd[] = "ATE0\r";              //禁止返回命令
const uint8_t at_ZDSLEEP[]="AT+ZDSLEEP=0\r";     //深度睡眠模式  0-禁止休眠模式   1-允许

const uint8_t athCmd[]  = "ATH\r";               //挂断电话
const uint8_t at_CMGF[] = "AT+CMGF=1\r";         //短信接收为文本格式
const uint8_t at_CNMI[] = "AT+CNMI=3,2,0,0,0\r"; //短信接收直接显示不保存

const uint8_t at_GSN[]  = "AT+GSN\r";            //查询IMEI号
const uint8_t at_CPIN[] = "AT+CPIN?\r";          //查询当前CPIN状态

const uint8_t at_SetZpNum[]="AT+ZPNUM=\"CMMTM\",\"\",\"\"\r";//设置APN，用户名，密码
const uint8_t at_ZPNUM[] = "AT+ZPNUM=#777\r";     //设置APN，用户名，密码
const uint8_t at_GPRSLink[] = "AT+ZPPPSTATUS\r";  //查询GPRS链路状态
const uint8_t at_CSQ[]="AT+CSQ\n\r";              //信号检测命令
const uint8_t at_ICCID[]="AT+ZGETICCID\r";        //"AT+GETICCID\r";
const uint8_t at_ZPPPOPEN[] ="AT+ZPPPOPEN\r";      //打开PPP链路  为端口监听做准备

const uint8_t at_TCPLink[]  = "AT+ZIPSETUP=0,116.62.195.99,9527\r";//建立TCP连接
const uint8_t at_Check[]    = "AT+ZIPPSTATUS=0\r";//查询当前TCP连接状态
const uint8_t at_TCPClose[] = "AT+ZIPCLOSE=0\r";  //关闭通道号为0的TCP连接
const uint8_t at_TCPSend[]  = "AT+ZIPSEND=0,35\r";//向连接号为0的地址发送%d个数据
                                                  //返回>  输入数据，成功后返回 +ZIPSEND：OK OK
const uint8_t at_GetIP[]    = "AT+ZIPGETIP\r";												  
 uint8_t sendDatas[39]={0x7E,0x00,0x1D,
						   0x39,0x39,0x39,0x39,0x39,0x39,0x39,0x39,0x39,0x39,0x39,0x39,0x39,0x39,0x39,0x39,
						   0x80,0x00,0x01,0x00,
							0x00,0x02,0x00,0x00,
							0x05,0x80,0x00,0x00,0x00,
							0xa8,0x6b,0x7E
							};	
uint8_t ReceDatas[100]; 
												  
static void CDMAConfigInit(void )
{
	uint8_t err;
	uint8_t *ptr = NULL;

	CDMASendDatas(atCmd,sizeof(atCmd));
	ptr = OSQPend(CDMARecieveQ,1500,&err);
	Mem_free(ptr);//AT\r\r\nOK\r\n
	OSTimeDlyHMSM(0,0,0,500);
	
	CDMASendDatas(ate0Cmd,sizeof(ate0Cmd));
	ptr = OSQPend(CDMARecieveQ,1500,&err);
	Mem_free(ptr);//ATE0\r\r\nOK\r\n
	OSTimeDlyHMSM(0,0,0,500);
	
	CDMASendDatas(at_ZDSLEEP,sizeof(at_ZDSLEEP));
	ptr = OSQPend(CDMARecieveQ,1500,&err);
	Mem_free(ptr);//\r\nOK\r\n\nOK\r\n
	OSTimeDlyHMSM(0,0,0,500);
	
	CDMASendDatas(at_CSQ,sizeof(at_CSQ));
	ptr = OSQPend(CDMARecieveQ,1500,&err);
	Mem_free(ptr);//\r\n+CSQ:
	OSTimeDlyHMSM(0,0,0,500);
	
	CDMASendDatas(at_GSN,sizeof(at_GSN));
	ptr = OSQPend(CDMARecieveQ,1500,&err);
	Mem_free(ptr);//\r\n863086032502879\r\n\r\nOK\r\n
	OSTimeDlyHMSM(0,0,0,500);
	
	CDMASendDatas(at_ICCID,sizeof(at_ICCID));
	ptr = OSQPend(CDMARecieveQ,1500,&err);
	Mem_free(ptr);//\r\n+ZGETICCID:
	OSTimeDlyHMSM(0,0,0,500);
	
	CDMASendDatas(at_CNMI,sizeof(at_CNMI));
	ptr = OSQPend(CDMARecieveQ,1500,&err);
	Mem_free(ptr);//\r\nOK\r      
	OSTimeDlyHMSM(0,0,0,500);
	
	CDMASendDatas(at_SetZpNum,sizeof(at_SetZpNum));
	ptr = OSQPend(CDMARecieveQ,1500,&err);
	Mem_free(ptr);//\r\nOK\r\n\nOK\r\n
	OSTimeDlyHMSM(0,0,0,500);
	
	CDMASendDatas(at_GPRSLink,sizeof(at_GPRSLink));
	ptr = OSQPend(CDMARecieveQ,1500,&err);
	Mem_free(ptr);//\r\n+ZPPPSTATUS:DISCONNECTED\r\n\r\nOK
	OSTimeDlyHMSM(0,0,0,500);
	
	CDMASendDatas(at_ZPPPOPEN,sizeof(at_ZPPPOPEN));
	ptr = OSQPend(CDMARecieveQ,1500,&err);
	Mem_free(ptr);//\r\n+ZPPPOPEN:CONNECTED\r\n\r\nOK\r\n
	OSTimeDlyHMSM(0,0,0,500);
	
	CDMASendDatas(at_TCPLink,sizeof(at_TCPLink));
	ptr = OSQPend(CDMARecieveQ,1500,&err);
	Mem_free(ptr);
	OSTimeDlyHMSM(0,0,0,500);
	ptr = OSQPend(CDMARecieveQ,500,&err);
	Mem_free(ptr);
	ptr = OSQPend(CDMARecieveQ,500,&err);
	Mem_free(ptr);
	ptr = OSQPend(CDMARecieveQ,500,&err);//主要用来清空接收的消息队列
	Mem_free(ptr);
}

void CDMATask(void *pdata)
{
//	uint16_t i = 0;
	uint8_t *ptr = NULL;
	uint8_t err;
	CDMARecieveQ = OSQCreate(&cdmaRecBuf[0],CDMARECBUF_SIZE);  //建立CDMA接收 消息队列
	CDMASendQ    = OSQCreate(&cdmaSendBuf[0],CDMASENDBUF_SIZE);//建立CDMA发送 消息队列
	
	CDMAPowerOpen_Close();           //启动MG2639模块
	CDMAConfigInit();                //初始化配置MG2639
	OSTimeDlyHMSM(0,0,2,0);
	while(1)
	{

		OSTimeDlyHMSM(0,0,0,20);
		CDMASendDatas(at_TCPSend,sizeof(at_TCPSend));
		ptr = OSQPend(CDMARecieveQ,500,&err);
		Mem_free(ptr);
		
		CDMASendDatas(sendDatas,35);
		ptr = OSQPend(CDMARecieveQ,500,&err);
		Mem_free(ptr);
		
		
		
		
		
		
//		OSTimeDlyHMSM(0,0,0,20);
//		do
//		{
//			ptr = OSQPend(CDMARecieveQ,50,&err);
//			if(err == OS_ERR_NONE)
//			{
//				if((ptr[2] == '+')&&(ptr[3] == 'Z')&&(ptr[4] == 'I')&&(ptr[5] == 'P')
//				    &&(ptr[6] == 'R')&&(ptr[7] == 'E')&&(ptr[8] == 'C')&&(ptr[9] == 'V'))//接收到服务器下发的数据
//				{ 
//					for(i=0;i<60;i++)
//					{
//						ReceDatas[i] = ptr[i];
//					}
//				}
//				Mem_free(ptr);
//			}
//				
//			
//		}while(err == OS_ERR_NONE);
//		CDMASendDatas(atCmd,sizeof(atCmd));
	}
}
static void CDMAPowerOpen_Close(void)//这段代码是用来启动/关闭CDMA
{
	CDMA_POWER_LOW;
	OSTimeDlyHMSM(0,0,3,500);
	CDMA_POWER_HIGH;
	OSTimeDlyHMSM(0,0,2,0);
}















