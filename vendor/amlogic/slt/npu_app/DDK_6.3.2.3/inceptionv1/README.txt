×××××××××××××××编译DNCNN×××××××××××××××××
1. 编译
   运行build_vx.sh脚本：./build_vx.sh {buildroot的根目录} {工程名称}
   
   buildroot的根目录:代表与bootloader buildroot hardware output等文件夹所在的目录
   工程名称：代表所编译的工程，目前所用的工程为mesong12b_skt_release
   
2. 注意事项
   编译成功后会在当前目录下生成一个bin_r文件夹，文件夹里面包含inceptionv1执行文件和若干对象文件（*.o）,需把inceptionv1拷贝到和run.sh同一目录下
×××××××××××××××执行DNCNN×××××××××××××××××
1.运行run.sh
  命令：./run.sh 






 


