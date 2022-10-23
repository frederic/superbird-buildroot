1.编译
进入slt_test目录,直接make,如果有问题,修改toolchain到你对应的toolchain
CC=/opt/gcc-linaro-aarch64-linux-gnu-4.9-2014.09_linux/bin/aarch64-linux-gnu-gcc
CROSS_COMPILE=/opt/gcc-linaro-aarch64-linux-gnu-4.9-2014.09_linux/bin/aarch64-linux-gnu-

2.测试参数
生成的isp_slt,执行./isp_slt即可运行
后面可选参数：
(1)模糊对比的容许误差范围:-y 配置pixel 每个通道的允许difference 范围,如果大于会报错(默认是3)
(2)测试帧范围:  -N  执行到从第N帧(N,n需要同时选配,且n < N,默认21-30frame); -n  从第n帧执行(N,n需要同时选配,且n < N,默认21-30frame);

3. 测试方式:
(1) 测试sensor 为IMX290.
(2) 设置sensor 输出test pattern, 通过MIPI CSI -> MIPI adapter-> ISP 输出结果.
(3) 测试程序连续采集10帧数据, 进行md5 得出golden reference, 然后和其他非md5一致图像进行模糊对比.
(4) 输出10帧数据的校验结果, 得出ISP整个SLT case的测试结果. 全部帧校验正确输出pass, 否则fail.

4.测试结果分析
choose frame_21 as base, same frames = 10
选择21帧作为golden reference,计算得到相同的MD5的帧数为10帧
(1).测试通过log: ISP slt test success!
(2).测试失败log: ISP slt test fail!
	此时还会有log：WARNING: frame_x: pixel diff max:y larger than allow z打印
	其中frame_x表示:fail的帧, y表示:实际pixel difference, z表示:模糊对比的容许误差范围 

