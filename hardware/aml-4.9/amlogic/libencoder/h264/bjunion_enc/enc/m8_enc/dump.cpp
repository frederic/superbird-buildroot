//#define LOG_NDEBUG 0
#define LOG_TAG "M8VENCDUMP"
#include <utils/Log.h>
#include <cutils/properties.h>

#include "dump.h"

static dump_struct_t dstruct;

unsigned dump_init(int enc_width, int enc_height)
{
    char prop[PROPERTY_VALUE_MAX];
    unsigned value = 0;
    memset(prop,0,sizeof(prop));
    if (property_get("hw.encoder.enable.dump", prop, NULL) > 0) {
        sscanf(prop,"%x",&value);
    }
    memset(&dstruct, 0x0, sizeof(dstruct));
    if ((value & DUMP_REFERENCE) == DUMP_REFERENCE) {
        if ((dstruct.ref_fp = fopen("/data/reference_buffer.yuv","w")) == NULL) {
            ALOGE("Open dump reference file failed");
            value = value & (~DUMP_REFERENCE);
        }else{
            int size =  enc_width * enc_height;
            if ((dstruct.little_endian_buffer = (unsigned char *)malloc(size*3/2)) == NULL) {
                ALOGE("malloc buffer for little endian buffer failed");
                fclose(dstruct.ref_fp);
                dstruct.ref_fp = NULL;
                value = value & (~DUMP_REFERENCE);
            }else{
                if ((dstruct.convert_buffer = (unsigned char *)malloc(size/2)) == NULL) {
                    ALOGE("mallco convert buffer failed");
                    fclose(dstruct.ref_fp);
                    free(dstruct.little_endian_buffer);
                    dstruct.ref_fp = NULL;
                    dstruct.little_endian_buffer = NULL;
                    value = value & (~DUMP_REFERENCE);
                }
            }
        }
    }

    if ((value & DUMP_ENCODE_DATA) == DUMP_ENCODE_DATA) {
        ALOGV("enable dump encode data");
        dstruct.start_frm_count = -1;
        dstruct.end_frm_count = -1;
    }
    dstruct.mode = value;
    ALOGD("encoder dump value:%x",value);
    return value;
}


void dump_encode_data(m8venc_drv_t* p, int type, int size)
{
    const char *mode = "a";
    const char *filename = "";
    int start_frm_count = dstruct.start_frm_count;
    int end_frm_count = dstruct.end_frm_count;
    int frm_count = p->total_encode_frame;
    uint8* ptr = NULL;
    FILE *file = NULL;
    if ((dstruct.mode & DUMP_ENCODE_DATA) != DUMP_ENCODE_DATA)
        return;
    if (type == ENCODER_BUFFER_QP) {
        filename = "/data/encoder_qp_info.dat";
        ptr = (uint8*)(p->qp_info.addr);
    } else if (type == ENCODER_BUFFER_INPUT) {
        filename = "/data/encoder_ie_me.dat";
        ptr = (uint8*)(p->input_buf.addr);
    }
    file = fopen(filename, "r");
    if (file == NULL) {
        mode = "w";
    } else {
        fclose(file);
    }

    file = fopen(filename, mode);

    if (file) {
        int i,j;
        if (((start_frm_count == -1 || frm_count >= start_frm_count))&&
          ((end_frm_count == -1 || frm_count <= end_frm_count))) {
            if (type == ENCODER_BUFFER_QP) {
                fprintf(file, "%d==============================\n", frm_count);
            } else {
                fprintf(file, "%d==============================\n", frm_count);
            }
            for (i=0;i<size;) {
                if (type == ENCODER_BUFFER_QP) {
                    for (j=7;j>=0;j--) {
                        fprintf(file, "%02x", ptr[i+j]);
                    }
                    i+=8;
                } else {
                    for (j=0;j<4;j++) {
                        fprintf(file, "%02x%02x", ptr[i+1],ptr[i]);
                        i+=2;
                    }
                }
                fprintf(file, "\n");
            }
        }
        fclose(file);
    }
    return;
}


void dump_reference_buffer(m8venc_drv_t *p)
{
    unsigned char *little_endian_buffer = dstruct.little_endian_buffer;
    unsigned char *convert_buffer = dstruct.convert_buffer;
    unsigned char *y, *uv;
    int i, t;
    unsigned offset;
    int size =  p->src.pix_height*p->src.pix_width;
    if ((dstruct.mode & DUMP_REFERENCE) != DUMP_REFERENCE)
        return;
    if (dstruct.ref_fp != NULL) {
        y = (unsigned char *)p->ref_info.y;
        uv =  (unsigned char *)p->ref_info.uv;

        if (little_endian_buffer != NULL && convert_buffer != NULL) {
            for (i = 0; i < size; i+=8)
                for (t= 0; t < 8; t++)
                    little_endian_buffer[i + t] = y[i + 7 - t];
            for (i = 0; i< size/2; i+=8)
                for (t = 0; t < 8; t++)
                    little_endian_buffer[size + i + t] = uv[i + 7 - t];
            for (i = 0,t = 0; i < size / 2; i += 2) {
                convert_buffer[t] = little_endian_buffer[i + size];
                convert_buffer[size / 4 + t] = little_endian_buffer[size + i+1];
                t++;
            }
            fwrite(little_endian_buffer,size ,1, dstruct.ref_fp);
            fwrite(convert_buffer, size/2,1,dstruct.ref_fp);
        }
    }
    return;
}

void dump_mv_bits(m8venc_drv_t *p)
{
    char filename[60];
    FILE *fdump;
    if ((dstruct.mode & DUMP_MV_BITS) != DUMP_MV_BITS)
        return;
    memset(filename,0x0,60);
    sprintf(filename,"%s%d","/data/encoder_inter_bits_info.dat", p->total_encode_frame);
    fdump = fopen(filename,"w");
    if (fdump != NULL) {
        fwrite(p->inter_bits_info.addr,p->inter_bits_info.size,1,fdump);
        fclose(fdump);
    }
    memset(filename,0x0,60);
    sprintf(filename,"%s%d","/data/encoder_inter_mv_info.dat", p->total_encode_frame);
    fdump = fopen(filename,"w");
    if (fdump != NULL) {
        fwrite(p->inter_mv_info.addr,p->inter_mv_info.size,1,fdump);
        fclose(fdump);
    }
    return;
}

void dump_ie_bits(m8venc_drv_t *p)
{
    char filename[60];
    FILE *fdump;
    if ((dstruct.mode & DUMP_IE_BITS) != DUMP_IE_BITS)
        return;
    memset(filename,0x0,60);
    sprintf(filename,"%s%d","/data/encoder_intra_bits_info.dat", p->total_encode_frame);
    fdump = fopen(filename,"w");
    if (fdump != NULL) {
        fwrite(p->intra_bits_info.addr,p->intra_bits_info.size,1,fdump);
        fclose(fdump);
    }
    memset(filename,0x0,60);
    sprintf(filename,"%s%d","/data/encoder_intra_premode.dat", p->total_encode_frame);
    fdump = fopen(filename,"w");
    if (fdump != NULL) {
        fwrite(p->intra_pred_info.addr,p->intra_pred_info.size,1,fdump);
        fclose(fdump);
    }
    return;
}

void dump_release(void)
{
    if (dstruct.ref_fp != NULL)
        fclose(dstruct.ref_fp);
    dstruct.ref_fp = NULL;

    if (dstruct.little_endian_buffer != NULL)
        free(dstruct.little_endian_buffer);
    dstruct.little_endian_buffer = NULL;

    if (dstruct.convert_buffer != NULL)
        free(dstruct.convert_buffer);

    dstruct.convert_buffer = NULL;
    memset(&dstruct, 0x0, sizeof(dstruct));
    return;
}
