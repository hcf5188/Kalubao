����˵���ĵ���


һ��TCP����ָ�
1��AT+ZIPSETUP=<N>,<IP>,<M>    -  ��ָ��������󶨵�TCP��������������
  ʾ����AT+ZIPSETUP=1,61.144.216.219,2332
		+ZIPSETUP:CONNECTED
		OK
2��AT+ZIPSEND=port,length<CR>  -  ��ָ���������ӵ�Ŀ�������  port-TCP������ͨ����
  ʾ����AT+ZIPSEND=1,10
		>abcdefghij
		+ZIPSEND:OK
		OK
3��AT+ZPPPSTATUS               -  ��ָ�����ڲ�ѯGPRS��·״̬
   ʾ����AT+ZPPPSTATUS
		+ZPPPSTATUS: ESTABLISHED
		OK
   ���ߣ�AT+ZPPPSTATUS
		+ZPPPSTATUS: DISCONNECTED
		OK
4��AT+ZIPCLOSE=<N>             -  �ر�TCP��·  N-ͨ���ţ�ͬ����1�е�N
   ʾ����AT+ZIPCLOSE=1
		+ZIPCLOSE:OK
		OK
5��AT+ZIPSTATUS=<N>            -  ��ѯ��ǰTCP����״̬
   ʾ����AT+ZIPSTATUS=1
		+ZIPSTATUS: ESTABLISHED
		OK
   ˵����ESTABLISHED�� TCP�Ѿ�����
		 DISCONNECTED�� TCP �Ѿ��ر�
6��+ZIPRECV:N,LEN,<DATA>       -  ��ʾ�ӵ�ǰ������·���յ�����
   ʾ����+ZIPRECV:1,5,abcde
   ˵����<N>��  TCP ���ӵ�ͨ����
		<LEN>�� �������ݳ���
		<DATA>�����յ�����


������������ע������
		1�����еİ汾���������������һ�£���Ȼģ���һֱ�������������е�#define SOFTVersion     0x10001001    //����̼��汾��
	�е�SOFTVersion ��ֵҪ���������ֵһ�¡�
	    2����Ҫ����ʱ����Ҫ���Flash����ʱ��������ܹ����������������в������������������û�����ٲ��𵽷�������


�����������⣺

       1���޸ĳ�����������ſ���   --   �ڴ�й©��û�м�ʱ�ͷ��ڴ��
       
       2���������ţ���ⲻ���ź�     --    ����Ϊ����������ģʽ������ģʽ�׳�����

	   3��LogReport������������д��  OSMutexPend(CDMASendMutex,0,&err);���棬
��Ȼ���ϱ������������ݻ�һֱ�ȴ��Լ�ռ�õĻ������ź���  CDMASendMutex

	   4��������CAN��ʱ�����CAN1_SetFilter(varOperation.canRxId ,CAN_ID_EXT);�󣬿���varOperation.canTest = 0;
	   ���CAN1_ClearFilter();���򲻿���varOperation.canTest = 0;��ΪJ1939���� canRecieveQ ����Ϣ����ɽ���������
	   
	   5������������ʱ��Ҫȷ���������ļ������ӵ���������������
	   
	   6����ַ����
	   0x08007800  -  ����OTA������ز���
	   0x08008000  -  �������������еĵ�ַ
	   0x08030000  -  OTA���������̼������ڴˣ�������Sboot�Ὣ�˴��Ĵ��븲����������
	   0x08060000  -  ��ǿ���� ���ͣ���ECU��ʼ�����������ڴˣ������ǽ��ͺ��ֵ����ǿ���ֵ
	   0x08060800  -  CANͨѶ���������ڴ�
	   0x08061000  -  ECU�ڶ������ļ������ڴ�
	   0x08063000  -  ECU PID�����ļ������ڴˣ�����6K�ĵ�ַ����0x08063000 - 0x08064800֮�� 
					˵����ÿ��2K�õ���1870���ֽڱ���110��PIDָ��