1.编译准备：
  在build_vx.sh中有 AQROOT OUTPUT_DIR OPENCV_ROOT TOOLCHAIN 三个绝对路径，在编译之前换成自己目前使用的路径。
2.编译
  执行：./build_vx.sh
  成功编译之后，会在当前目录下生成一个bin_r 文件夹，里面包含yolofaceint8 和一些.o 文件。需要把yolofaceint8 拷到与build_vx.sh同级的目录。
3.执行
  放在板子上，进入此目录执行： ./yolofaceint8
