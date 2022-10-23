1.编译
进入slt_8M目录,直接make,如果有问题,修改toolchain到你对应的toolchain
CC=/opt/gcc-linaro-aarch64-linux-gnu-4.9-2014.09_linux/bin/aarch64-linux-gnu-gcc
CROSS_COMPILE=/opt/gcc-linaro-aarch64-linux-gnu-4.9-2014.09_linux/bin/aarch64-linux-gnu-

2.测试参数
生成的isp_slt,执行./isp_slt即可运行
后面可选参数：
-t x:  x=0 单测fr; x=3测fr+ds1; x=4测fr+ds1+ds2

3. 测试方式:
(1) 测试sensor 为OS08A10. fr出4k, ds1&ds2 均出640*480.
(2) 设置sensor 输出test pattern, 通过MIPI CSI -> MIPI adapter-> ISP 输出结果.
(3) 测试程序连续采集10帧数据, 与预设的golden md5 reference 进行比较，如果都一样则success,不一样就fail.
(4) 输出10帧数据的校验结果, 得出ISP整个SLT case的测试结果. 全部帧校验正确输出pass, 否则fail.

4.测试结果分析
(1).测试通过log: Stream[0] slt test success!
(2).测试失败log: Stream[0] slt test fail!

