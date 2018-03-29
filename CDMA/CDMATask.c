#include "apptask.h"

/*************************   CDMA��������������   *****************************/
void CDMASendCmd(const uint8_t sendDat[],char* compString,uint16_t sendLength);
/*************************   MG2639���õ� ��AT��ָ��     ***********************/
const uint8_t atCmd[]      = "AT\r";                //AT ����ָ��
const uint8_t ate0Cmd[]    = "ATE0\r";              //��ֹ��������
const uint8_t at_ZDSLEEP[] = "AT+ZDSLEEP=0\r";      //���˯��ģʽ  0 - ��ֹ����ģʽ   1 - ����

const uint8_t athCmd[]  = "ATH\r";                  //�Ҷϵ绰
const uint8_t at_CMGF[] = "AT+CMGF=1\r";            //���Ž���Ϊ�ı���ʽ
const uint8_t at_CNMI[] = "AT+CNMI=3,2,0,0,0\r";    //���Ž���ֱ����ʾ������

const uint8_t at_GSN[]  = "AT+GSN\r";               //��ѯIMEI��
const uint8_t at_CPIN[] = "AT+CPIN?\r";             //��ѯ��ǰCPIN״̬

const uint8_t at_SetZpNum[]   = "AT+ZPNUM=\"CMMTM\",\"\",\"\"\r";//����APN���û���������
const uint8_t at_ZPNUM[]      = "AT+ZPNUM=#777\r";  //����APN���û���������
const uint8_t at_ZPPPSTATUS[] = "AT+ZPPPSTATUS\r";  //��ѯ GPRS ��·״̬
const uint8_t at_CSQ[]        = "AT+CSQ\n\r";       //�źż������
const uint8_t at_ICCID[]      = "AT+ZGETICCID\r";   //"AT + GETICCID\r";
const uint8_t at_ZPPPOPEN[]   = "AT+ZPPPOPEN\r";    //�� PPP ��· Ϊ�˿ڼ�����׼��

const uint8_t at_ZIPSETUP[] = "AT+ZIPSETUP=0,%s,%d\r";//����TCP����  todo:��Ҫ���ɿ����õ� %s  %d
const uint8_t at_Check[]    = "AT+ZIPPSTATUS=0\r";  //��ѯ��ǰTCP����״̬
const uint8_t at_TCPClose[] = "AT+ZIPCLOSE=0\r";    //�ر�ͨ����Ϊ0��TCP����
const char at_TCPSend[]     = "AT+ZIPSEND=0,%d\r";  //�����Ӻ�Ϊ 0 �ĵ�ַ���� %d ������   ���� >  �������ݣ��ɹ��󷵻� +ZIPSEND��OK OK
											  
const uint8_t at_GetIP[]    = "AT+ZIPGETIP\r";      //��ȡ����IP		

const uint8_t at_ZDNS[]     = "AT+ZDNSGETIP=\"tcp.51gonggui.com\"\r";//������������ȡIP��ַ

extern uint16_t  freCDMALed;
static uint8_t CDMAReceDeal(uint8_t* ptrRece,char* ptr2);

void CDMATask(void *pdata)
{
	uint8_t *pCDMARece = NULL;
	_CDMADataToSend *pCDMASend = NULL;
	uint8_t err;
	uint8_t err2;
	char sendCmd[30];
	uint16_t sendlen = 0;

	CDMAConfigInit();                //��ʼ������MG2639
	LoginDataSend();                 //���͵�¼����
	varOperation.isDataFlow = 0;     //����������
	
	while(1)//todo:�������Ͽ� TCP ���ӡ�OTA�����������ļ��·�����¼
	{
		OSSemPost(sendMsg); //û�з�������
		
receCDMA:	
		pCDMASend = OSQPend(CDMASendQ,0,&err);
		
		OSSemAccept(sendMsg); //���ڷ������� ���ĵ�����ʱ���ź���
		
		sendlen = sprintf(sendCmd,at_TCPSend,pCDMASend->datLength);
		CDMASendDatas((uint8_t *)sendCmd,sendlen); //֪ͨCDMAҪͨ���ĸ�ͨ�����Ͷ��ٸ��ֽڵ�����
		
		pCDMARece = OSQPend(CDMARecieveQ,150,&err);
		if(err == OS_ERR_NONE)
		{
			err = CDMAReceDeal(pCDMARece,"DISCONNECTED");
			err2 = CDMAReceDeal(pCDMARece,"ERROR");
			
			if((err == 0)||(err2 == 0))         //TCP�Ͽ����ӣ���Ҫ��������
			{
				Mem_free(pCDMARece);            //�ͷ�ռ�õ��ڴ��
				Mem_free(pCDMASend->data);
				Mem_free(pCDMASend);
				
				varOperation.isDataFlow = 1;    //������δ����
				freCDMALed = LEDFAST;           //CDMAС�ƿ���
				CDMAPowerOpen_Close(CDMA_CLOSE);//�ر�CDMA��Դ
				
				CDMAConfigInit();               //��������CDMA
				varOperation.isDataFlow = 0;    //����������
				carAllRecord.cdmaReStart++;     //��¼CDMA��������
				LoginDataSend();                //������Ҫ��¼����
				goto receCDMA;
			}
			err = CDMAReceDeal(pCDMARece,">");
			Mem_free(pCDMARece);
			if(err != 0)
			{
				CDMASendDatas((uint8_t *)sendCmd,sendlen);  //֪ͨCDMAҪͨ���ĸ�ͨ�����Ͷ��ٸ��ֽڵ�����
				pCDMARece = OSQPend(CDMARecieveQ,150,&err); 
				Mem_free(pCDMARece);
			}
		}
		else
		{
			Mem_free(pCDMARece);            //�ͷ�ռ�õ��ڴ��
			Mem_free(pCDMASend->data);
			Mem_free(pCDMASend);
		
			varOperation.isDataFlow = 1;    //������δ����
			freCDMALed = LEDFAST;           // CDMA С�ƿ���
			CDMAPowerOpen_Close(CDMA_CLOSE);//�ر� CDMA ��Դ
			
			CDMAConfigInit();               //�������� CDMA
			varOperation.isDataFlow = 0;    //����������
			carAllRecord.cdmaReStart++;     //��¼ CDMA ��������
			LoginDataSend();                //������Ҫ��¼����
			goto receCDMA;
		}
		//ʵ��Ҫ���͵�����
		CDMASendDatas((uint8_t *)pCDMASend->data,pCDMASend->datLength);
		pCDMARece = OSQPend(CDMARecieveQ,150,&err);  //���ͳɹ�
		Mem_free(pCDMARece);                         //SEND OK
		
	    carAllRecord.netFlow += pCDMASend->datLength;//�����������ֽڣ�
		
		Mem_free(pCDMASend->data);
		Mem_free(pCDMASend);
		
		carAllRecord.messageNum ++;                  //���͵���Ϣ����
		carAllRecord.cdmaReStart = 0;                //���Ϸ��������豸������
	}
}
void CDMAPowerOpen_Close(uint8_t flag)//��δ�������������/�ر�CDMA
{
	uint8_t err;
	//�������ź�����ȷ�������͹ر�
	OSMutexPend(CDMAPowerMutex,0,&err);
	
	if(((flag == CDMA_CLOSE)&&(varOperation.isCDMAStart % 2 == 0))||((flag == CDMA_OPEN)&&(varOperation.isCDMAStart%2 == 1)))
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

//�����Լ��������ݵĹ����У���CDMA���ص�״̬��Ϣ���ַ��������д��� 0 ��������
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
void CDMASendCmd(const uint8_t sendDat[],char* compString,uint16_t sendLength)
{
	uint8_t err;
	uint8_t count = 0;
	uint8_t *ptrCDMACfg;
	ptrCDMACfg = OSQPend(CDMARecieveQ,2,&err);//��������ģ���Զ��ظ��ġ�+CPIN:READY��
	Mem_free(ptrCDMACfg);
	do{
		count ++;
		CDMASendDatas(sendDat,sendLength);
		ptrCDMACfg = OSQPend(CDMARecieveQ,500,&err);
		if(err != OS_ERR_NONE)        //���ճ�ʱ
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
	OSTimeDlyHMSM(0,0,0,100);
	ptrCDMACfg = OSQPend(CDMARecieveQ,2,&err);//��������ģ���Զ��ظ��ġ�+CPIN:READY��
	Mem_free(ptrCDMACfg);
}

static void CDMAReadIMEI_ICCID(const uint8_t at_Get[],uint8_t cmdLength,uint8_t datSave[],uint8_t datLength)
{
	uint8_t err;
	uint8_t i = 0,j=0;
	uint8_t *ptrCDMACfg;
	do{
		CDMASendDatas(at_Get,cmdLength);
		ptrCDMACfg = OSQPend(CDMARecieveQ,500,&err);//todo:��� +CPIN  �ж�
		if(err == OS_ERR_NONE)
		{
			while( (ptrCDMACfg[i] < 0x30)|| (ptrCDMACfg[i]>0x39))
			{
				i++;
				if(i>15)
				{
					err = 2;
					break;
				}
			}
			for(j=0;j<datLength;j++)
			{
				if((ptrCDMACfg[i+j] >= 0x30)&&(ptrCDMACfg[i+j] <= 0x39))
					continue;
				else 
					break;
			}
			if(j == datLength)
				memcpy(datSave,&ptrCDMACfg[i],datLength);
			Mem_free(ptrCDMACfg);
		}
	}while(err != OS_ERR_NONE);
	ptrCDMACfg = OSQPend(CDMARecieveQ,2,&err);//��������ģ���Զ��ظ��ġ�+CPIN:READY��
	Mem_free(ptrCDMACfg);
}
void GetIpAddr(const uint8_t at_Get[],uint8_t cmdLength,uint8_t datSave[],uint8_t datLength )//���������������������IP��ַ
{
	uint8_t err;
	uint8_t i = 0,j = 0;
	uint8_t *ptrCDMACfg;
	do{
		CDMASendDatas(at_Get,cmdLength);
		ptrCDMACfg = OSQPend(CDMARecieveQ,500,&err);//todo:��� +CPIN  �ж�
		if(err == OS_ERR_NONE)
		{
			while( (ptrCDMACfg[i] < 0x30)|| (ptrCDMACfg[i] > 0x39))
			{
				i++;
				if(i>15)
				{
					err = 2;
					break;
				}
			}
			for(j=0;j<datLength;j++)
			{
				if(((ptrCDMACfg[i+j]>=0x30)&&(ptrCDMACfg[i+j]<=0x39))||(ptrCDMACfg[i+j] == '.'))
					continue;
				else 
					break;
			}
			memcpy(datSave,&ptrCDMACfg[i],j);
			Mem_free(ptrCDMACfg);
		}
	}while(err != OS_ERR_NONE);
	ptrCDMACfg = OSQPend(CDMARecieveQ,2,&err);//��������ģ���Զ��ظ��ġ�+CPIN:READY��
	Mem_free(ptrCDMACfg);
}
void CDMAConfigInit(void )
{
	char sendCmd[45];
	uint8_t sendlen = 0;
	static uint8_t tt = 0; 
	CDMAPowerOpen_Close(CDMA_OPEN);  //���� MG2639 ģ��
	
	CDMASendCmd(atCmd,"OK",sizeof(atCmd));
	CDMASendCmd(ate0Cmd,"OK",sizeof(ate0Cmd));
	CDMASendCmd(at_ZDSLEEP,"OK",sizeof(at_ZDSLEEP));
	CDMASendCmd(at_CSQ,"+CSQ:",sizeof(at_CSQ));

	CDMASendCmd(at_CNMI,"OK",sizeof(at_CNMI));
	if(tt == 0)                                                             //�����ظ���ȡ IMEI ��
	{
		CDMAReadIMEI_ICCID(at_GSN,sizeof(at_GSN),varOperation.imei,15);     //��ȡ IMEI ��
		tt = 1;
	}
//	CDMAReadIMEI_ICCID(at_ICCID,sizeof(at_ICCID),varOperation.iccID,20);    //��ȡSIM���� ICCID ��
	
	CDMASendCmd(at_SetZpNum,"OK",sizeof(at_SetZpNum));
	CDMASendCmd(at_ZPPPSTATUS,"+ZPPPSTATUS:",sizeof(at_ZPPPSTATUS));
	
	CDMASendCmd(at_ZPPPOPEN,"+ZPPPOPEN:CONNECTED",sizeof(at_ZPPPOPEN));
#if (BEN_S_C == 2)
	GetIpAddr(at_ZDNS,sizeof(at_ZDNS),varOperation.ipAddr,15);              //������������ȡ�������� IP ��ַ
#endif
	sendlen = sprintf(sendCmd,(const char*)at_ZIPSETUP,varOperation.ipAddr,varOperation.ipPotr);// TCP ����
	CDMASendCmd((uint8_t *)sendCmd,"+ZIPSETUP:CONNECTED",sendlen);

	freCDMALed = LEDSLOW;           //�������ӳɹ�
}








