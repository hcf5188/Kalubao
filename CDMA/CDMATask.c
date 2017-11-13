#include "apptask.h"



/*************************   CDMA��������������   *****************************/
static void CDMAPowerOpen_Close(void);//��δ�������������/�ر�CDMA
static void CDMAConfigInit(void );    //��ʼ������CDMA

/*************************   MG2639���õ� ��AT��ָ��     ***********************/
const uint8_t atCmd[]      = "AT\r";                //AT ����ָ��
const uint8_t ate0Cmd[]    = "ATE0\r";              //��ֹ��������
const uint8_t at_ZDSLEEP[] = "AT+ZDSLEEP=0\r";      //���˯��ģʽ  0-��ֹ����ģʽ   1-����

const uint8_t athCmd[]  = "ATH\r";                  //�Ҷϵ绰
const uint8_t at_CMGF[] = "AT+CMGF=1\r";            //���Ž���Ϊ�ı���ʽ
const uint8_t at_CNMI[] = "AT+CNMI=3,2,0,0,0\r";    //���Ž���ֱ����ʾ������

const uint8_t at_GSN[]  = "AT+GSN\r";               //��ѯIMEI��
const uint8_t at_CPIN[] = "AT+CPIN?\r";             //��ѯ��ǰCPIN״̬

const uint8_t at_SetZpNum[]   = "AT+ZPNUM=\"CMMTM\",\"\",\"\"\r";//����APN���û���������
const uint8_t at_ZPNUM[]      = "AT+ZPNUM=#777\r";  //����APN���û���������
const uint8_t at_ZPPPSTATUS[] = "AT+ZPPPSTATUS\r";  //��ѯGPRS��·״̬
const uint8_t at_CSQ[]        = "AT+CSQ\n\r";       //�źż������
const uint8_t at_ICCID[]      = "AT+ZGETICCID\r";   //"AT+GETICCID\r";
const uint8_t at_ZPPPOPEN[]   = "AT+ZPPPOPEN\r";    //��PPP��·  Ϊ�˿ڼ�����׼��

const uint8_t at_ZIPSETUP[] = "AT+ZIPSETUP=0,116.228.88.101,29999\r";//����TCP����  todo:��Ҫ���ɿ����õ� %s   %d
const uint8_t at_Check[]    = "AT+ZIPPSTATUS=0\r";  //��ѯ��ǰTCP����״̬
const uint8_t at_TCPClose[] = "AT+ZIPCLOSE=0\r";    //�ر�ͨ����Ϊ0��TCP����
const char at_TCPSend[]     = "AT+ZIPSEND=0,%d\r";  //�����Ӻ�Ϊ0�ĵ�ַ����%d������   ����>  �������ݣ��ɹ��󷵻� +ZIPSEND��OK OK
// todo:��Ҫ�����͵ĳ��ȿ����ã�%d
												  
const uint8_t at_GetIP[]    = "AT+ZIPGETIP\r";      //��ȡ����IP										  
/***********************    CDMA���շ��Ͷ��е��������    ***********************/
#define CDMARECBUF_SIZE  10         //������Ϣ���б�����Ϣ�������
void *cdmaRecBuf[CDMARECBUF_SIZE];  //���ڴ��ָ�������ָ��
OS_EVENT *CDMARecieveQ;             //ָ��CDMA������Ϣ���е�ָ��

#define CDMASENDBUF_SIZE  5         //������Ϣ���б�����Ϣ�������
void *cdmaSendBuf[CDMASENDBUF_SIZE];//���ڴ��ָ�������ָ��
OS_EVENT *CDMASendQ;  

extern uint16_t  freCDMALed;
static uint8_t CDMAReceDeal(uint8_t* ptrRece,char* ptr2);

void CDMATask(void *pdata)
{
	uint8_t *pCDMARece = NULL;
	_CDMADataToSend *pCDMASend = NULL;
//	char *p1;
	uint8_t err;
	char sendCmd[30];
	uint16_t sendlen = 0;
	
	CDMARecieveQ = OSQCreate(&cdmaRecBuf[0],CDMARECBUF_SIZE);  //����CDMA���� ��Ϣ����
	CDMASendQ    = OSQCreate(&cdmaSendBuf[0],CDMASENDBUF_SIZE);//����CDMA���� ��Ϣ����
	
	CDMAPowerOpen_Close();           //����MG2639ģ��
	
	CDMAConfigInit();                //��ʼ������MG2639
	
	while(1)//todo:�������Ͽ�TCP���ӡ�OTA�����������ļ��·�����¼
	{
		pCDMASend = OSQPend(CDMASendQ,0,&err);
		
		sendlen = sprintf(sendCmd,at_TCPSend,pCDMASend->datLength);

		CDMASendDatas((uint8_t *)sendCmd,sendlen); //֪ͨCDMAҪͨ���ĸ�ͨ�����Ͷ��ٸ��ֽڵ�����
		pCDMARece = OSQPend(CDMARecieveQ,1500,&err);
		if(err == OS_ERR_NONE)
		{
			err = CDMAReceDeal(pCDMARece,">");
			Mem_free(pCDMARece);
			if(err != 0)
			{
				CDMASendDatas((uint8_t *)sendCmd,sendlen); //֪ͨCDMAҪͨ���ĸ�ͨ�����Ͷ��ٸ��ֽڵ�����
				pCDMARece = OSQPend(CDMARecieveQ,1500,&err);
				Mem_free(pCDMARece);
			}
		}

		CDMASendDatas((uint8_t *)pCDMASend->data,pCDMASend->datLength);//ʵ��Ҫ���͵�����
		pCDMARece = OSQPend(CDMARecieveQ,1500,&err);       //���ͳɹ�
		Mem_free(pCDMARece);
	
		Mem_free(pCDMASend);
	}
}
static void CDMAPowerOpen_Close(void)//��δ�������������/�ر�CDMA
{
	CDMA_POWER_LOW;
	OSTimeDlyHMSM(0,0,3,500);
	CDMA_POWER_HIGH;
	OSTimeDlyHMSM(0,0,2,0);
}
//�����Լ��������ݵĹ����У���CDMA���ص�״̬��Ϣ���д��� 0 ��������
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
//���ͳ�ʼ������ָ��
static void CDMASendCmd(const uint8_t sendDat[],char* compString,uint16_t sendLength)
{
	uint8_t err;
	uint8_t *ptrCDMACfg;
	ptrCDMACfg = OSQPend(CDMARecieveQ,2,&err);//��������ģ���Զ��ظ��ġ�+CPIN:READY��
	Mem_free(ptrCDMACfg);
	do{
		CDMASendDatas(sendDat,sendLength);
		ptrCDMACfg = OSQPend(CDMARecieveQ,1500,&err);
		if(err != OS_ERR_NONE)        //���ճ�ʱ
		{
			CDMASendDatas(sendDat,sendLength);
			ptrCDMACfg = OSQPend(CDMARecieveQ,1500,&err);
		}
		else
		{
			err = CDMAReceDeal(ptrCDMACfg,compString);
			Mem_free(ptrCDMACfg);
		}
	}while(err != 0);
//	ptrCDMACfg = OSQPend(CDMARecieveQ,2,&err);//��������ģ���Զ��ظ��ġ�+CPIN:READY��
//	Mem_free(ptrCDMACfg);
}
extern _SystemInformation sysAllData;//ϵͳȫ�ֱ�����Ϣ
static void CDMAReadIMEI(void )
{
	uint8_t err;
	uint8_t i = 0;
	uint8_t *ptrCDMACfg;
	do{
		CDMASendDatas(at_GSN,sizeof(at_GSN));
		ptrCDMACfg = OSQPend(CDMARecieveQ,1500,&err);
		if(err == OS_ERR_NONE)
		{
			while( (ptrCDMACfg[i]<0x30)|| (ptrCDMACfg[i]>0x39))
			{
				i++;
				if(i>10)
				{
					err = 2;
					break;
				}
			}
			memcpy(sysAllData.imei,&ptrCDMACfg[i],15);
			Mem_free(ptrCDMACfg);
		}
	}while(err != OS_ERR_NONE);
	ptrCDMACfg = OSQPend(CDMARecieveQ,2,&err);//��������ģ���Զ��ظ��ġ�+CPIN:READY��
	Mem_free(ptrCDMACfg);
}
static void CDMAConfigInit(void )
{
	CDMASendCmd(atCmd,"OK",sizeof(atCmd));
	CDMASendCmd(ate0Cmd,"OK",sizeof(ate0Cmd));
	CDMASendCmd(at_ZDSLEEP,"OK",sizeof(at_ZDSLEEP));
	CDMASendCmd(at_CSQ,"+CSQ:",sizeof(at_CSQ));
	CDMASendCmd(at_ICCID,"OK",sizeof(at_ICCID));
	CDMASendCmd(at_CNMI,"OK",sizeof(at_CNMI));
	
	CDMAReadIMEI();//��ȡIMEI��
	
	CDMASendCmd(at_SetZpNum,"OK",sizeof(at_SetZpNum));
	CDMASendCmd(at_ZPPPSTATUS,"+ZPPPSTATUS:",sizeof(at_ZPPPSTATUS));
	CDMASendCmd(at_ZPPPOPEN,"+ZPPPOPEN:CONNECTED",sizeof(at_ZPPPOPEN));
	CDMASendCmd(at_ZIPSETUP,"+ZIPSETUP:CONNECTED",sizeof(at_ZIPSETUP));

	freCDMALed = 300;//�������ӳɹ�
	OSTimeDlyHMSM(0,0,4,0);
}












