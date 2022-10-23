1、将整个目录拷贝到U盘，并挂载到板子上。（该目录下所有文件均需要）
2、执行： 执行run.sh或者rungdb.sh 即可

run.sh解析：
 export LD_LIBRARY_PATH=`pwd`:$LD_LIBRARY_PATH  设置环境变量
 export LD_LIBRARY_PATH=./opencv3:$LD_LIBRARY_PATH  设置所需的opencv3的几个so链接。如果版本中带有opencv的库，可以不需要
./mtcnn_int8 0									运行程序（参数1：value 1表示带gdc处理，value 0表示不带gdc处理）

