demo程序支持四个模型(yoloface/yolov2/yolov3/facenet)的基本流程演示。简单demo程序

yoloface/yolov2/yolov3：
分别对应如下命令：
./detect_demo 0 1080p.bmp
./detect_demo 1 1080p.bmp
./detect_demo 2 1080p.bmp 
结果会保存成：output.bmp

facenet（人脸识别检测）
执行命令：
1、生成人脸对应数据，保存到数据库中
./detect_demo 11 1080p.bmp  0 --->使用yoloface人脸检测后，crop图进行人脸数据库数据生成，并保持到emb.db中。
当前会保存两张bmp文件，face.bmp和face_160.bmp，即检测出的人脸crop后的保存图，以及resize成160x160后保存的图。
当前emb.db数据库里面有三个数据，deng.liu/jian.cao 以及1080p.bmp图片对应的数据。

如果当前执行了该命令的话，进行人脸检测时候，代码里面需要改动：（如下两处）
  #define NAME_NUM 3
  string name[NAME_NUM]={"deng.liu","jian.cao", "xxxx.xie"};

1、进行人脸识别
 ./detect_demo 11 1080p.bmp  1 --->使用yoloface人脸检测后，crop图进行人脸识别
当前会保存两张bmp文件，face.bmp和face_160.bmp，即检测出的人脸crop后的保存图，以及resize成160x160后保存的图。
打印识别结果：face detected,your id is 2, name is xxxx.xie。  没有该打印，说明当前未识别出人脸。
log：mindis:0, index=2 提示为当前检测的最小值和下标，当前阈值设置为0.6


