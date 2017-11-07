#include "apptask.h"



/*************************   CDMA��������������   *****************************/
static void CDMAPowerOpen_Close(void);//��δ�������������/�ر�CDMA
static void CDMAConfigInit(void );    //��ʼ������CDMA

#define CDMARECBUF_SIZE  10       //������Ϣ���б�����Ϣ�������
void *cdmaRecBuf[CDMARECBUF_SIZE];//���ڴ��ָ�������ָ��
OS_EVENT *CDMARecieveQ;           //ָ��CDMA������Ϣ���е�ָ��

#define CDMASENDBUF_SIZE  100       //������Ϣ���б�����Ϣ�������
void *cdmaSendBuf[CDMASENDBUF_SIZE];//���ڴ��ָ�������ָ��
OS_EVENT *CDMASendQ;                //ָ��CDMA������Ϣ���е�ָ��

/*************************   MG2639���õ� ��AT��ָ��     ***********************/
const uint8_t atCmd[]   = "AT\r";                //AT ����ָ��
const uint8_t ate0Cmd[] = "ATE0\r";              //��ֹ��������
const uint8_t at_ZDSLEEP[]="AT+ZDSLEEP=0\r";     //���˯��ģʽ  0-��ֹ����ģʽ   1-����

const uint8_t athCmd[]  = "ATH\r";               //�Ҷϵ绰
const uint8_t at_CMGF[] = "AT+CMGF=1\r";         //���Ž���Ϊ�ı���ʽ
const uint8_t at_CNMI[] = "AT+CNMI=3,2,0,0,0\r"; //���Ž���ֱ����ʾ������

const uint8_t at_GSN[]  = "AT+GSN\r";            //��ѯIMEI��
const uint8_t at_CPIN[] = "AT+CPIN?\r";          //��ѯ��ǰCPIN״̬

const uint8_t at_SetZpNum[]="AT+ZPNUM=\"CMMTM\",\"\",\"\"\r";//����APN���û���������
const uint8_t at_ZPNUM[] = "AT+ZPNUM=#777\r";     //����APN���û���������
const uint8_t at_GPRSLink[] = "AT+ZPPPSTATUS\r";  //��ѯGPRS��·״̬
const uint8_t at_CSQ[]="AT+CSQ\n\r";              //�źż������
const uint8_t at_ICCID[]="AT+ZGETICCID\r";        //"AT+GETICCID\r";
const uint8_t at_ZPPPOPEN[] ="AT+ZPPPOPEN\r";      //��PPP��·  Ϊ�˿ڼ�����׼��

const uint8_t at_TCPLink[]  = "AT+ZIPSETUP=0,116.62.195.99,9527\r";//����TCP����
const uint8_t at_Check[]    = "AT+ZIPPSTATUS=0\r";//��ѯ��ǰTCP����״̬
const uint8_t at_TCPClose[] = "AT+ZIPCLOSE=0\r";  //�ر�ͨ����Ϊ0��TCP����
const uint8_t at_TCPSend[]  = "AT+ZIPSEND=0,35\r";//�����Ӻ�Ϊ0�ĵ�ַ����%d������
                                                  //����>  �������ݣ��ɹ��󷵻� +ZIPSEND��OK OK
												  
const uint8_t at_TCPSend1[]  = "AT+ZIPSEND=0,33\r";//�����Ӻ�Ϊ0�ĵ�ַ����%d������
const uint8_t at_GetIP[]    = "AT+ZIPGETIP\r";												  
 uint8_t sendDatas[39]={0x7E,0x00,0x1D,
						0x39,0x39,0x39,0x39,0x39,0x39,0x39,0x39,0x39,0x39,0x39,0x39,0x39,0x39,0x39,0x39,
						0x80,0x00,0x00,0x00,
						0x00,0x02,0x00,0x00,
						0x05,0x80,0x00,0x00,0x00,
						0x43,0x65,0x7E};	
uint8_t ReceDatas[100]; 
static void CDMAConfigInit(void )
{
	uint8_t err;
	uint8_t *ptrCDMACfg = NULL;

	CDMASendDatas(atCmd,sizeof(atCmd));
	ptrCDMACfg = OSQPend(CDMARecieveQ,1500,&err);
	Mem_free(ptrCDMACfg);//AT\r\r\nOK\r\n
	OSTimeDlyHMSM(0,0,0,500);
	
	CDMASendDatas(ate0Cmd,sizeof(ate0Cmd));
	ptrCDMACfg = OSQPend(CDMARecieveQ,1500,&err);
	Mem_free(ptrCDMACfg);//ATE0\r\r\nOK\r\n
	OSTimeDlyHMSM(0,0,0,500);
	
	CDMASendDatas(at_CPIN,sizeof(at_CPIN));
	ptrCDMACfg = OSQPend(CDMARecieveQ,1500,&err);
	Mem_free(ptrCDMACfg);//\r\nOK\r\n\nOK\r\n
	OSTimeDlyHMSM(0,0,0,500);
	
	CDMASendDatas(at_ZDSLEEP,sizeof(at_ZDSLEEP));
	ptrCDMACfg = OSQPend(CDMARecieveQ,1500,&err);
	Mem_free(ptrCDMACfg);//\r\nOK\r\n\nOK\r\n
	OSTimeDlyHMSM(0,0,0,500);
	
	CDMASendDatas(at_CSQ,sizeof(at_CSQ));
	ptrCDMACfg = OSQPend(CDMARecieveQ,1500,&err);
	Mem_free(ptrCDMACfg);//\r\n+CSQ:
	OSTimeDlyHMSM(0,0,0,500);
	
	CDMASendDatas(at_GSN,sizeof(at_GSN));
	ptrCDMACfg = OSQPend(CDMARecieveQ,1500,&err);
	Mem_free(ptrCDMACfg);//\r\n863086032502879\r\n\r\nOK\r\n
	OSTimeDlyHMSM(0,0,0,500);
	
	CDMASendDatas(at_ICCID,sizeof(at_ICCID));
	ptrCDMACfg = OSQPend(CDMARecieveQ,1500,&err);
	Mem_free(ptrCDMACfg);//\r\n+ZGETICCID:
	OSTimeDlyHMSM(0,0,0,500);
	
//	CDMASendDatas(at_CNMI,sizeof(at_CNMI));
//	ptrCDMACfg = OSQPend(CDMARecieveQ,1500,&err);
//	Mem_free(ptrCDMACfg);//\r\nOK\r      
//	OSTimeDlyHMSM(0,0,0,500);
//	
//	CDMASendDatas(at_SetZpNum,sizeof(at_SetZpNum));
//	ptrCDMACfg = OSQPend(CDMARecieveQ,1500,&err);
//	Mem_free(ptrCDMACfg);//\r\nOK\r\n\nOK\r\n
//	OSTimeDlyHMSM(0,0,5,500);
//	
//	CDMASendDatas(at_GPRSLink,sizeof(at_GPRSLink));
//	ptrCDMACfg = OSQPend(CDMARecieveQ,1500,&err);
//	Mem_free(ptrCDMACfg);//\r\n+ZPPPSTATUS:DISCONNECTED\r\n\r\nOK
//	OSTimeDlyHMSM(0,0,3,500);
//	
//	CDMASendDatas(at_ZPPPOPEN,sizeof(at_ZPPPOPEN));
//	ptrCDMACfg = OSQPend(CDMARecieveQ,1500,&err);
//	Mem_free(ptrCDMACfg);//\r\n+ZPPPOPEN:CONNECTED\r\n\r\nOK\r\n
//	OSTimeDlyHMSM(0,0,0,500);
//	
//	CDMASendDatas(at_TCPLink,sizeof(at_TCPLink));
//	ptrCDMACfg = OSQPend(CDMARecieveQ,1500,&err);
//	Mem_free(ptrCDMACfg);
//	OSTimeDlyHMSM(0,0,0,500);
//	ptrCDMACfg = OSQPend(CDMARecieveQ,500,&err);
//	Mem_free(ptrCDMACfg);
//	ptrCDMACfg = OSQPend(CDMARecieveQ,500,&err);
//	Mem_free(ptrCDMACfg);
//	ptrCDMACfg = OSQPend(CDMARecieveQ,500,&err);//��Ҫ������ս��յ���Ϣ����
//	Mem_free(ptrCDMACfg);
}

void CDMATask(void *pdata)
{
//	uint16_t crc = 0;
//	uint8_t *ptrCDMARece = NULL;
//	uint8_t err;
	CDMARecieveQ = OSQCreate(&cdmaRecBuf[0],CDMARECBUF_SIZE);  //����CDMA���� ��Ϣ����
	CDMASendQ    = OSQCreate(&cdmaSendBuf[0],CDMASENDBUF_SIZE);//����CDMA���� ��Ϣ����
	
	CDMAPowerOpen_Close();           //����MG2639ģ��
	CDMAConfigInit();                //��ʼ������MG2639
	OSTimeDlyHMSM(0,0,2,0);
	while(1)
	{
		OSTimeDlyHMSM(0,0,1,20);
		
//		if(sendDatas[2] == 29)
//		{
//			CDMASendDatas(at_TCPSend,sizeof(at_TCPSend));
//			ptrCDMARece = OSQPend(CDMARecieveQ,1000,&err);
//			Mem_free(ptrCDMARece);
//			crc = CRC_Compute16(&sendDatas[1],31);
//			sendDatas[32] = (crc>>8)&0xff;
//			sendDatas[33] = (crc)&0xff;
//			CDMASendDatas(sendDatas,35);
//			sendDatas[2] = 27;
//			sendDatas[27] = 0x03;
//			sendDatas[32] = 0x7E;
//		}
//		else if(sendDatas[2] == 27)
//		{
//			CDMASendDatas(at_TCPSend1,sizeof(at_TCPSend1));
//			ptrCDMARece = OSQPend(CDMARecieveQ,1000,&err);
//			Mem_free(ptrCDMARece);
//			sendDatas[29]++;
//			crc = CRC_Compute16(&sendDatas[1],29);
//			
//			sendDatas[30] = (crc>>8)&0xff;
//			sendDatas[31] = (crc)&0xff;
//			
//			CDMASendDatas(sendDatas,33);

//		}
//		ptrCDMARece = OSQPend(CDMARecieveQ,3000,&err);
//		Mem_free(ptrCDMARece);
//		ptrCDMARece = OSQPend(CDMARecieveQ,3000,&err);
//		Mem_free(ptrCDMARece);
//		ptrCDMARece = OSQPend(CDMARecieveQ,3000,&err);
//		Mem_free(ptrCDMARece);
//		
//		sendDatas[22]++;
//		sendDatas[25]++;
//		
	}
}
static void CDMAPowerOpen_Close(void)//��δ�������������/�ر�CDMA
{
	CDMA_POWER_LOW;
	OSTimeDlyHMSM(0,0,3,500);
	CDMA_POWER_HIGH;
	OSTimeDlyHMSM(0,0,2,0);
}















