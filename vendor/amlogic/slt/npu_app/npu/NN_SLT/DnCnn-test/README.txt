
××××××××××××××× buildroot编译DNCNN ×××××××××××××××××

1. 编译
   运行build_vx.sh脚本：./build_vx.sh {bildroot的根目录} {工程名称}
   
   buildroot的根目录:代表与bootloader buildroot hardware output等文件夹所在的目录
   工程名称：代表所编译的工程，目前所用的工程为mesong12b_skt_release

2. 注意事项
   编译成功后会在当前目录下生成一个bin_r文件夹，文件夹里面包含dncnn执行文件和若干对象文件（*.o）,需把dncnn拷贝到和run.sh同一目录下
×××××××××××××××执行DNCNN×××××××××××××××××
0.程序必须保证dncnn、DnCNN.export.data、input.tensor、output.bin、input.bin、run.sh六个文件在同一目录下才能运行

1.运行run.sh
  命令：./run.sh  

2. run.sh 中运行的是dncnn程序，需要输入四个参数：./dncnn DnCNN.export.data inp_2.tensor 
  dncnn：代表可执行程序
  DnCNN.export.data：程序的第一个参数，表示网络的参数
  input.tensor：程序的第二个参数，是网络的输入

3. 如果DNCnn完整的运行返回值为0，若未能完全运行，则返回值为1

××××××××××××××× android编译DNCNN ×××××××××××××××××

1. 把整个文件夹放到android 工程中（例如：vendor/amlogic/common/**）
2. 在根目录下编译nn slt模块
    mmm vendor//amlogic/common/**
3. 编译出bin 文件在：out/target/product/XX/vendor/bin/dncnn

×××××××××××××××版本×××××××××××××××××
acuity-5，DDK-6.3.3.4



 


