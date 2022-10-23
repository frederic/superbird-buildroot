×××××××××××××××编译yoloface×××××××××××××××××
0. 必须保证工程在编译时menuconfig中打开了opencv（Target package/Libraries/Graphics/opencv-2.4）的开关

1. 编译
   运行build_vx.sh脚本：./build_vx.sh {bildroot的根目录} {工程名称}
   
   buildroot的根目录:代表与bootloader buildroot hardware output等文件夹所在的目录
   工程名称：代表所编译的工程，目前所用的工程为mesong12b_skt_release

2. 注意事项
   编译成功后会在当前目录下生成一个bin_r文件夹，文件夹里面包含yolofaceint8执行文件和若干对象文件（*.o）

×××××××××××××××执行yoloface×××××××××××××××××
0.程序需要yolofaceint8、dynamic_fixed_point-8.export.data两个文件

1.运行时保证有camera

2.运行
	命令：./yolofaceint8  ./dynamic_fixed_point-8.export.data


