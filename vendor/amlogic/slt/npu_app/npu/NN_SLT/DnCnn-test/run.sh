#! /bin/sh
path=$(cd `dirname $0`;pwd)
cd $path
export VSI_NN_LOG_LEVEL=0
export VNN_NOT_PRINT_TIME=0
export VNN_LOOP_TIME=10
./dncnn DnCNN.export.data inp_2.tensor
