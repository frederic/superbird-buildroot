Compile steps:
1.cd encoder/bjunion_enc, CXX=aarch64-linux-gnu-g++ make
2.cd encoder, CXX=aarch64-linux-gnu-g++ make

The above two steps will compile and generate executable file testApi.

Test steps:
1.copy testApi to /data/
2.chmod +x /data/testApi
3.copy 640x480-nv21.yuv  to /data/
4./data/testApi  /data/640x480-nv21.yuv  /data/output.h264 640 480 10 30 2000000 20 0


The above fourth step will generate the encoding file output.h264. 

Here is the detailed parameter description.
/data/testApi  srcfile  outfile  width  height  gop  framerate  bitrate  num  fmt

Parameter	Description
srcfile	        source yuv data url
outfile	        encoded h264 stream url 
width	        source yuv width
height	        source yuv height
gop	        I frame refresh interval(N=0: only one I frame; N>0: one I frame per N frames)
framerate	framerate(fps)
bitrate	        bitrate(bps)
num	        encode frame count
fmt	        source yuv format( 0:nv21, 1:yv12, 2:rgb888)
