#include "apptask.h"



/*************************   CDMA服务函数声明部分   *****************************/
void CDMASendCmd(const uint8_t sendDat[],char* compString,uint16_t sendLength);
/*************************   MG2639常用的 “AT”指令     ***********************/
const uint8_t atCmd[]      = "AT\r";                //AT 测试指令
const uint8_t ate0Cmd[]    = "ATE0\r";              //禁止返回命令
const uint8_t at_ZDSLEEP[] = "AT+ZDSLEEP=0\r";      //深度睡眠模式  0-禁止休眠模式   1-允许

const uint8_t athCmd[]  = "ATH\r";                  //挂断电话
const uint8_t at_CMGF[] = "AT+CMGF=1\r";            //短信接收为文本格式
const uint8_t at_CNMI[] = "AT+CNMI=3,2,0,0,0\r";    //短信接收直接显示不保存

const uint8_t at_GSN[]  = "AT+GSN\r";               //查询IMEI号
const uint8_t at_CPIN[] = "AT+CPIN?\r";             //查询当前CPIN状态

const uint8_t at_SetZpNum[]   = "AT+ZPNUM=\"CMMTM\",\"\",\"\"\r";//设置APN，用户名，密码
const uint8_t at_ZPNUM[]      = "AT+ZPNUM=#777\r";  //设置APN，用户名，密码
const uint8_t at_ZPPPSTATUS[] = "AT+ZPPPSTATUS\r";  //查询GPRS链路状态
const uint8_t at_CSQ[]        = "AT+CSQ\n\r";       //信号检测命令
const uint8_t at_ICCID[]      = "AT+ZGETICCID\r";   //"AT+GETICCID\r";
const uint8_t at_ZPPPOPEN[]   = "AT+ZPPPOPEN\r";    //打开PPP链路  为端口监听做准备

const uint8_t at_ZIPSETUP[] = "AT+ZIPSETUP=0,%s,%d\r";//建立TCP连接  todo:需要做成可配置的 %s   %d
const uint8_t at_Check[]    = "AT+ZIPPSTATUS=0\r";  //查询当前TCP连接状态
const uint8_t at_TCPClose[] = "AT+ZIPCLOSE=0\r";    //关闭通道号为0的TCP连接
const char at_TCPSend[]     = "AT+ZIPSEND=0,%d\r";  //向连接号为0的地址发送%d个数据   返回>  输入数据，成功后返回 +ZIPSEND：OK OK
// todo:需要将发送的长度可配置，%d
												  
const uint8_t at_GetIP[]    = "AT+ZIPGETIP\r";      //获取本地IP										  


extern uint16_t  freCDMALed;
static uint8_t CDMAReceDeal(uint8_t* ptrRece,char* ptr2);

void CDMATask(void *pdata)
{
	uint8_t *pCDMARece = NULL;
	_CDMADataToSend *pCDMASend = NULL;
	uint8_t err;
	char sendCmd[30];
	uint16_t sendlen = 0;

	CDMAConfigInit();                //初始化配置MG2639
	LoginDataSend();                 //发送登录报文
	varOperation.isDataFlow = 0;     //开启数据流
	
	while(1)//todo:断网、断开TCP连接、OTA升级、配置文件下发、登录
	{
		OSSemPost(sendMsg); //没有发送数据
		
receCDMA:	
		pCDMASend = OSQPend(CDMASendQ,0,&err);
		
		OSSemAccept(sendMsg); //正在发送数据 消耗掉发送时的信号量
		
		sendlen = sprintf(sendCmd,at_TCPSend,pCDMASend->datLength);
		CDMASendDatas((uint8_t *)sendCmd,sendlen); //通知CDMA要通过哪个通道发送多少个字节的数据
		
		pCDMARece = OSQPend(CDMARecieveQ,150,&err);
		if(err == OS_ERR_NONE)
		{
			err = CDMAReceDeal(pCDMARece,"DISCONNECTED");
			if(err == 0)                        //TCP断开连接，需要重新连接
			{
				Mem_free(pCDMARece);            //释放占用的内存块
				Mem_free(pCDMASend->data);
				Mem_free(pCDMASend);
			
				varOperation.isDataFlow = 1;    //数据流未流动
				freCDMALed = 100;               //CDMA小灯快闪
				CDMAPowerOpen_Close(CDMA_CLOSE);//关闭CDMA电源
				
				CDMAConfigInit();               //重新启动CDMA
				varOperation.isDataFlow = 0;    //开启数据流
				goto receCDMA;
			}
			err = CDMAReceDeal(pCDMARece,">");
			Mem_free(pCDMARece);
			if(err != 0)
			{
				CDMASendDatas((uint8_t *)sendCmd,sendlen);  //通知CDMA要通过哪个通道发送多少个字节的数据
				pCDMARece = OSQPend(CDMARecieveQ,150,&err); 
				Mem_free(pCDMARece);
			}
		}											 //实际要发送的数据
		CDMASendDatas((uint8_t *)pCDMASend->data,pCDMASend->datLength);
		pCDMARece = OSQPend(CDMARecieveQ,150,&err);  //发送成功
		Mem_free(pCDMARece);                         //SEND OK
		
	    carAllRecord.netFlow += pCDMASend->datLength;//网络流量（字节）
		
		Mem_free(pCDMASend->data);
		Mem_free(pCDMASend);
		
		carAllRecord.messageNum ++;                  //发送的消息条数
	}
}
void CDMAPowerOpen_Close(uint8_t flag)//这段代码是用来启动/关闭CDMA
{
	uint8_t err;
	//互斥型信号量，确保开启和关闭
	OSMutexPend(CDMAPowerMutex,0,&err);
	
	if(((flag == CDMA_CLOSE)&&(varOperation.isCDMAStart%2 == 0))||((flag == CDMA_OPEN)&&(varOperation.isCDMAStart%2 == 1)))
	{
		varOperation.isCDMAStart++;
		CDMA_POWER_LOW;
		OSTimeDlyHMSM(0,0,4,500);
		CDMA_POWER_HIGH;
		if(varOperation.isCDMAStart%2 == 1)
			OSTimeDlyHMSM(0,0,6,0);
		else
			OSTimeDlyHMSM(0,0,12,0);
	}
	OSMutexPost(CDMAPowerMutex);
}
//配置以及发送数据的过程中，对CDMA返回的状态信息进行处理 0 接收正常
static uint8_t CDMAReceDeal(uint8_t* ptrRece,char* ptr2)
{
	char* p1 = NULL;
	p1 = strstr((const char*)ptrRece,"+CPIN");
	if(p1 != NULL)
		return 1;
	p1 = strstr((const char*)ptrRece,ptr2);
	if(p1 == NULL)
		return 1;
	return 0;
}
//发送初始化配置指令
void CDMASendCmd(const uint8_t sendDat[],char* compString,uint16_t sendLength)
{
	uint8_t err;
	uint8_t count = 0;
	uint8_t *ptrCDMACfg;
	ptrCDMACfg = OSQPend(CDMARecieveQ,2,&err);//用以消耗模块自动回复的“+CPIN:READY”
	Mem_free(ptrCDMACfg);
	do{
		count ++;
		CDMASendDatas(sendDat,sendLength);
		ptrCDMACfg = OSQPend(CDMARecieveQ,500,&err);
		if(err != OS_ERR_NONE)        //接收超时
		{
			CDMASendDatas(sendDat,sendLength);
			ptrCDMACfg = OSQPend(CDMARecieveQ,500,&err);
		}
		else
		{
			err = CDMAReceDeal(ptrCDMACfg,compString);
			Mem_free(ptrCDMACfg);
		}
	}while((err != 0) && (count < 200));
//	ptrCDMACfg = OSQPend(CDMARecieveQ,2,&err);//用以消耗模块自动回复的“+CPIN:READY”
//	Mem_free(ptrCDMACfg);
}

static void CDMAReadIMEI_ICCID(const uint8_t at_Get[],uint8_t cmdLength,uint8_t datSave[],uint8_t datLength )
{
	uint8_t err;
	uint8_t i = 0;
	uint8_t *ptrCDMACfg;
	do{
		CDMASendDatas(at_Get,cmdLength);
		ptrCDMACfg = OSQPend(CDMARecieveQ,500,&err);//todo:添加 +CPIN  判断
		if(err == OS_ERR_NONE)
		{
			while( (ptrCDMACfg[i]<0x30)|| (ptrCDMACfg[i]>0x39))
			{
				i++;
				if(i>15)
				{
					err = 2;
					break;
				}
			}
			memcpy(datSave,&ptrCDMACfg[i],datLength);
			Mem_free(ptrCDMACfg);
		}
	}while(err != OS_ERR_NONE);
	ptrCDMACfg = OSQPend(CDMARecieveQ,2,&err);//用以消耗模块自动回复的“+CPIN:READY”
	Mem_free(ptrCDMACfg);
}
void CDMAConfigInit(void )
{
	char sendCmd[45];
	uint8_t sendlen = 0;
	CDMAPowerOpen_Close(CDMA_OPEN);  //启动MG2639模块
	
	CDMASendCmd(atCmd,"OK",sizeof(atCmd));
	CDMASendCmd(ate0Cmd,"OK",sizeof(ate0Cmd));
	CDMASendCmd(at_ZDSLEEP,"OK",sizeof(at_ZDSLEEP));
	CDMASendCmd(at_CSQ,"+CSQ:",sizeof(at_CSQ));

	CDMASendCmd(at_CNMI,"OK",sizeof(at_CNMI));
	
	CDMAReadIMEI_ICCID(at_GSN,sizeof(at_GSN),varOperation.imei,15);     //读取IMEI号
	CDMAReadIMEI_ICCID(at_ICCID,sizeof(at_ICCID),varOperation.iccID,20);//读取SIM卡的ICCID号
	
	CDMASendCmd(at_SetZpNum,"OK",sizeof(at_SetZpNum));
	CDMASendCmd(at_ZPPPSTATUS,"+ZPPPSTATUS:",sizeof(at_ZPPPSTATUS));
	CDMASendCmd(at_ZPPPOPEN,"+ZPPPOPEN:CONNECTED",sizeof(at_ZPPPOPEN));
	
	sendlen = sprintf(sendCmd,(const char*)at_ZIPSETUP,varOperation.ipAddr,varOperation.ipPotr);//TCP连接
	CDMASendCmd((uint8_t *)sendCmd,"+ZIPSETUP:CONNECTED",sendlen);

	freCDMALed = 500;           //网络连接成功
	
}












