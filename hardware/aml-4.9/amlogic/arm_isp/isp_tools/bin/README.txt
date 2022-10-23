1.code compile
		open control_tool/act-server/makefile
		change at the front:
			GCC = aarch64-linux-gnu-gcc
			GXX = aarch64-linux-gnu-g++
			GST = aarch64-linux-gnu-strip
		
		then make, bin file act-server is at dir control_tool/act-server/build

2.environment
	1)file prepare
		on linux
			act-server
			control_tool/act-client/content
			preset file chardev-sw.set
		
		note(act-server,chardev-sw.set,content these three must in one dir)
	
	2)pc prepare
		arm_isp/driver/control_tool_files/IV009-SW-Control.xml

3.connection
	1)on linux
		1.connect the network
		2.insmod all isp ko
		3.config the ip(if need,like: ifconfig eth0 10.18.29.119 up)
		4.run isp testapp backend
		5.run ./act-server --preset=chardev-sw.set
		6.if set successfully, it shows in serial port
				Control Tool 3.5.1 of 2018-01-10 15:58:59 +0000 (built: Jun 29 2018 15:00:13)
				(C) COPYRIGHT 2013 - 2018 ARM Limited or its affiliates.
				00:21:10.177   INFO > HTTPServer: please open http://10.18.29.119:8000/ in firefox or chrome
				00:21:15.714   INFO > BusManager: driver chardev has been successfully initialized in sync mode for fw-channel
				00:21:15.714   INFO > CharDeviceDriver: successfully opened device /dev/ac_isp
				00:21:15.714   INFO > CommandManager: access manager configured successfully
				00:21:15.714   INFO > CommandManager: hw access type: stream
				00:21:15.715   INFO > CommandManager: fw access type: stream
	
	2)on pc
		1.after all the set on linux, open chrome/firefox,input ip address
		http://10.18.29.119:8000/
		2.if all the settings are set, the webpage will note you to load xml, then choose IV009-SW-Control.xml
		3.you will see the isp tool window

thus,isp tool environment build complete.

		


1.code编译
	打开control_tool/act-server/makefile
	在最前面添加:
	GCC = aarch64-linux-gnu-gcc
	GXX = aarch64-linux-gnu-g++
	GST = aarch64-linux-gnu-strip
	在control_tool/act-server/目录直接make，可执行文件act-server生成在目录control_tool/act-server/build

2.环境搭建
	1)文件准备
		板子端：
			act-server
			control_tool/act-client/content目录
			附件中提供的chardev-sw.set
			dts在ethmac里面注掉两个pinctl，status改成ok，编译生成dtb
			(注意事项:act-server,chardev-sw.set,content目录需要放在buildroot同一个目录)

		pc端：
			arm_isp/driver/control_tool_files/IV009-SW-Control.xml
	
	2)连接
		板子端
			1.连接网线
			2.使用之前改好的dtb,起来后执行./insmod.sh
			3.配置ip地址ifconfig eth0 10.18.29.119 up(需要预先知道ip地址)
			4.后台让testapp显示
			5.运行./act-server --preset=chardev-sw.set
			6.正常情况下串口应该打印:
				 Control Tool 3.5.1 of 2018-01-10 15:58:59 +0000 (built: Jun 29 2018 15:00:13)
				(C) COPYRIGHT 2013 - 2018 ARM Limited or its affiliates.
				00:21:10.177   INFO > HTTPServer: please open http://10.18.29.119:8000/ in firefox or chrome
				00:21:15.714   INFO > BusManager: driver chardev has been successfully initialized in sync mode for fw-channel
				00:21:15.714   INFO > CharDeviceDriver: successfully opened device /dev/ac_isp
				00:21:15.714   INFO > CommandManager: access manager configured successfully
				00:21:15.714   INFO > CommandManager: hw access type: stream
				00:21:15.715   INFO > CommandManager: fw access type: stream
		
		pc端
			1.在板子端执行完成以后，在chrome/firefox浏览器登录http://10.18.29.119:8000/
			2.正常登录后，会有一个load xml的界面，选择IV009-SW-Control.xml
			3.网页自动load到tool调试界面

至此,isp tool搭建完成
