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
		1、运行的版本必须与服务器保持一致，不然模块会一直升级，即程序中的#define SOFTVersion             0x10001001    //软件固件版本号
	中的SOFTVersion 的值要与服务器的值一致。


三、常见问题：

       1、修改程序后，跑着跑着卡死   --   内存泄漏，没有及时释放内存块
       
       2、输入引脚，检测不到信号     --    配置为上拉、下拉模式，浮空模式易初问题
