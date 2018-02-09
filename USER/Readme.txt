程序说明文档：


一、TCP连接指令：
1、AT+ZIPSETUP=<N>,<IP>,<M>    -  该指令用于向绑定的TCP服务器发送数据
  示例：AT+ZIPSETUP=1,61.144.216.219,2332
		+ZIPSETUP:CONNECTED
		OK
2、AT+ZIPSEND=port,length<CR>  -  该指令用于连接到目标服务器  port-TCP的连接通道号
  示例：AT+ZIPSEND=1,10
		>abcdefghij
		+ZIPSEND:OK
		OK
3、AT+ZPPPSTATUS               -  该指令用于查询GPRS链路状态
   示例：AT+ZPPPSTATUS
		+ZPPPSTATUS: ESTABLISHED
		OK
   或者：AT+ZPPPSTATUS
		+ZPPPSTATUS: DISCONNECTED
		OK
4、AT+ZIPCLOSE=<N>             -  关闭TCP链路  N-通道号，同命令1中的N
   示例：AT+ZIPCLOSE=1
		+ZIPCLOSE:OK
		OK
5、AT+ZIPSTATUS=<N>            -  查询当前TCP连接状态
   示例：AT+ZIPSTATUS=1
		+ZIPSTATUS: ESTABLISHED
		OK
   说明：ESTABLISHED： TCP已经建立
		 DISCONNECTED： TCP 已经关闭
6、+ZIPRECV:N,LEN,<DATA>       -  提示从当前数据链路接收到数据
   示例：+ZIPRECV:1,5,abcde
   说明：<N>：  TCP 连接的通道号
		<LEN>： 接收数据长度
		<DATA>：接收的数据


二、程序升级注意事项
		1、运行的版本必须与服务器保持一致，不然模块会一直升级，即程序中的#define SOFTVersion     0x10001001    //软件固件版本号
	中的SOFTVersion 的值要与服务器的值一致。
	    2、重要升级时，需要清空Flash，此时如果程序能够运行正常，再在有参数的情况下升级，都没问题再部署到服务器。


三、常见问题：

       1、修改程序后，跑着跑着卡死   --   内存泄漏，没有及时释放内存块
       
       2、输入引脚，检测不到信号     --    配置为上拉、下拉模式，浮空模式易初问题

	   3、LogReport（）函数不能写在  OSMutexPend(CDMASendMutex,0,&err);里面，
不然，上报服务器的数据会一直等待自己占用的互斥型信号量  CDMASendMutex

	   4、在配置CAN的时候，如果CAN1_SetFilter(varOperation.canRxId ,CAN_ID_EXT);后，可以varOperation.canTest = 0;
	   如果CAN1_ClearFilter();，则不可以varOperation.canTest = 0;因为J1939会往 canRecieveQ 推消息，造成解析卡死。
	   
	   5、程序升级的时候，要确定升级的文件，连接的是外网服务器。
	   
	   6、地址管理
	   0x08007800  -  保存OTA升级相关参数
	   0x08008000  -  主程序正常运行的地址
	   0x08030000  -  OTA升级，将固件保存在此，重启后，Sboot会将此处的代码覆盖主程序区
	   0x08060000  -  增强动力 节油，将ECU初始喷油量保存在此，后面是降低后的值、增强后的值
	   0x08060800  -  CAN通讯参数保存在此
	   0x08061000  -  ECU第二配置文件保存在此
	   0x08063000  -  ECU PID配置文件保存在此，连续6K的地址，即0x08063000 - 0x08064800之间 
					说明：每个2K用到了1870个字节保存110个PID指令