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
	ptr = OSQPend(CDMARecieveQ,500,&err);//��Ҫ������ս��յ���Ϣ����
	Mem_free(ptr);
}

void CDMATask(void *pdata)
{
//	uint16_t i = 0;
	uint8_t *ptr = NULL;
	uint8_t err;
	CDMARecieveQ = OSQCreate(&cdmaRecBuf[0],CDMARECBUF_SIZE);  //����CDMA���� ��Ϣ����
	CDMASendQ    = OSQCreate(&cdmaSendBuf[0],CDMASENDBUF_SIZE);//����CDMA���� ��Ϣ����
	
	CDMAPowerOpen_Close();           //����MG2639ģ��
	CDMAConfigInit();                //��ʼ������MG2639
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
//				    &&(ptr[6] == 'R')&&(ptr[7] == 'E')&&(ptr[8] == 'C')&&(ptr[9] == 'V'))//���յ��������·�������
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
static void CDMAPowerOpen_Close(void)//��δ�������������/�ر�CDMA
{
	CDMA_POWER_LOW;
	OSTimeDlyHMSM(0,0,3,500);
	CDMA_POWER_HIGH;
	OSTimeDlyHMSM(0,0,2,0);
}















