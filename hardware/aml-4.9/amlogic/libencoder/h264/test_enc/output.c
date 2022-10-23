#include "amlenc.h"
#include <stdio.h>
#include <stdlib.h>
//#include <malloc.h>
int main(int argc, const char *argv[])
{
    int width, height , qp, framerate, num;
    char *outputdata;
    if (argc < 7)
    {
        printf("Amlogic AVC Encode API \n");
        printf("usage: output [srcfile] [outfile] [width] [height] [qp] [framerate] [num]\n");
        printf("options :\n");
        printf("         srcfile  : yuv data url in your root fs\n");
        printf("         outfile  : stream url in your root fs\n");
        printf("         width   : width\n");
        printf("         height  : height\n");
        printf("         qp       : QP value\n");
        printf("         framerate :  IDR period \n ");
        printf("         num :  encode frame count \n ");
        return -1;
    }
    else
    {
        printf("%s\n", argv[1]);
        printf("%s\n", argv[2]);
    }
    width =  atoi(argv[3]);
    if ((width < 1) || (width > 1920))
    {
        printf("invalid width \n");
        return -1;
    }
    height = atoi(argv[4]);
    if ((height < 1) || (height > 1080))
    {
        printf("invalid height \n");
        return -1;
    }
    qp  =  atoi(argv[5]);
    framerate  =  atoi(argv[6]);
    num  =  atoi(argv[7]);
    if ((qp < 0) || (qp > 51))
    {
        printf("invalid qp \n");
        return -1;
    }
    if ((framerate < 0) || (framerate > 30))
    {
        printf("invalid framerate \n");
        return -1;
    }
    if (num < 0)
    {
        printf("invalid num \n");
        return -1;
    }
    printf("src url    is %s ;\n", argv[1]);
    printf("out url    is %s ;\n", argv[2]);
    printf("width  is %d ;\n", width);
    printf("height is %d ;\n", height);
    printf("qp  is %d ;\n", qp);
    printf("framerate is %d ;\n", framerate);
    printf("num   is %d ;\n", num);
    StartEncode((char *)argv[1], (char *)argv[2], width, height, qp, framerate, num);
    return 0;
}
