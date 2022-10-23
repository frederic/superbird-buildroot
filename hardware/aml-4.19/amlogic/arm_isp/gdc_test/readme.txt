1.ion_gdc_test use ion dev to alloc buffer
2.cma_gdc_test use ioctl to alloc dma buffer and then map to use floor
3.sensor_config include sensor config file
4.gdc_tool inlcude gdc tool
5.gdc_image include gdc test example

cmds are same:

./gdc_test -i gdc/frame_1080p_s_3954/input-nv12.bin -c gdc/frame_1080p_s_3954/frame_1080p_s.bin -w 1920x1080-1920x1080 -f 1

./cma_gdc_test -i gdc/frame_1080p_s_3954/input-nv12.bin -c gdc/frame_1080p_s_3954/frame_1080p_s.bin -w 1920x1080-1920x1080 -f 1

-i: input image file
-c: gdc config file
-w: input image size and output image size
-f: input image format, input image format and output image format are same


thanks!
