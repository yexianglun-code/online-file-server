######版本##########################################################################################################
	version 4

#完成度
	一期：基本完成。（cd支持路径分析,其他命令不支持）
	
	二期：
			1.密码验证。完成。
			2.日志记录。未做。
			3.断点续传。未做。
			4.mmap映射大文件，进行网络传送。未做。

	三期：基本完成。(包含用户注册)

	四期：未做。

#项目启动方式
	进入FTP_Server/server, make, 用"./ftpserver ../conf/server.conf"启动服务端
	再进入Client/clientx(x为数字，1或2或3)/client, 用"./client [IP] [PORT]"启动客户端

#项目实现方式
	线程池。主线程监听新连接请求，然后转交给子线程，子线程完成账户验证及之后命令操作相关的逻辑业务。