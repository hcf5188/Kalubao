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


������ȡ





