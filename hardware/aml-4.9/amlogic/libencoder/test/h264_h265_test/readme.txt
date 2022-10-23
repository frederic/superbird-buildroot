How to compile?
1. make && make install
2. run encoder_test



Design Idea:

//H.265
Feed yuv to h.265 encoder,in this case, encode one frame each time, totally two times.
then make md5sum for encoded es streams and compare the md5sum value.
if these two md5sum values are equal to each other, hadware encoder module works well, otherwise, something wrong
may appear.

//H.264
Feed yuv to h.264 encoder,in this case, encode one frame each time, totally two times.
then make md5sum for encoded es streams and compare the md5sum value.
if these two md5sum values are equal to each other, hadware encoder module works well, otherwise, something wrong
may appear.
