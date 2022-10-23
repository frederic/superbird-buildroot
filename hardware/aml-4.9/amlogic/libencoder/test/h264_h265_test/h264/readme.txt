Compile steps:
1.cd encoder/bjunion_enc, make clean && make ARM=1
2.cd encoder, make clean && make ARM=1

The above two steps will compile and generate executable file testApi.

Test steps:
1.copy testApi to /data/
2.chmod 777 /data/testApi
3.copy 720p-nv12.yuv to /data/

test command:
    case 1: fmt: nv12, num_planes: 1
        ./testApi 720p-nv12.yuv   output.h264 1280 720 0 30 1000000 12 1 3 1
    case 2: fmt: nv12, num_planes: 2
        ./testApi 720p-nv12.yuv   output.h264 1280 720 0 30 1000000 12 1 3 2
    case 3: fmt: yv12, num_planes: 1
        ./testApi 720p-yv12.yuv   output.h264 1280 720 0 30 1000000 12 3 3 1
    case 4: fmt: yv12, num_planes: 3
        ./testApi 720p-yv12.yuv   output.h264 1280 720 0 30 1000000 12 3 3 3
    case 5: fmt: yv12, user ptr
        ./testApi 720p-yv12.yuv   output.h264 1280 720 0 30 1000000 12 3 0 0


The above fourth step will generate the encoding file output.h264. 

Here is the detailed parameter description.
/data/testApi  srcfile  outfile  width  height  gop  framerate  bitrate  num  fmt buf_type num_planes

Parameter	Description
srcfile	        source yuv data url
outfile	        encoded h264 stream url 
width	        source yuv width
height	        source yuv height
gop	        I frame refresh interval(N=0: only one I frame; N>0: one I frame per N frames)
framerate	framerate(fps)
bitrate	        bitrate(bps)
num	        encode frame count
fmt	        source yuv format( 1:nv12, 2:nv21, 3:yv12, 4:rgb888 5:bgr888)
buf_type        input buf type(0: vmalloc, 3:dma_buf)
num_planes      nv12/nv21: 1 or 2. yv12: 1 or 3