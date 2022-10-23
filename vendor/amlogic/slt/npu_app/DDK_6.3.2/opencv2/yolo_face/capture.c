/*
 * Apical(ARM) V4L2 test application 2016
 *
 * This is debugging purpose SW tool running on JUNO.
 */

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <sys/ioctl.h>
#include <linux/videodev2.h>
#include <math.h>

#include "common.h"
#include "isp_metadata.h"
#include "capture.h"
#include "capture_dng.h"
#include "logs.h"

#define SD_HEADER_SIZE          0x100
#define SD_META_SIZE            0x1000

static int32_t fill_header(int stream_type, char *header, frame_t *pframe, int plane) {
    char *buf = header;
    int convnum = 0;

#if ARM_V4L2_TEST_HAS_META
    if(stream_type == ARM_V4L2_TEST_STREAM_META) {
        ERR("[T%d] Error, metadata doesn't have header.\n", stream_type);
        return -1;
    }
#endif

    switch (stream_type) {
    case ARM_V4L2_TEST_STREAM_FR:
        // Todo: for now it's RGB only
        sprintf(buf,"P6\n");                buf += strlen(buf);
        if(pframe->pixelformat==V4L2_PIX_FMT_NV12){
        	sprintf(buf,"#YUV header\n");       buf += strlen(buf);
        }else{
        	sprintf(buf,"#RGB header\n");       buf += strlen(buf);
        }
        convnum = 1023;
        break;
#if ARM_V4L2_TEST_HAS_RAW
    case ARM_V4L2_TEST_STREAM_RAW:
        // Todo: for now it's RGB only
        sprintf(buf,"P5\n");                buf += strlen(buf);
        sprintf(buf,"#RAW header\n");       buf += strlen(buf);
        switch (pframe->bit_depth[plane])
        {
            case 8:
                convnum = 255;
                break;
            case 10:
                convnum = 1023;
                break;
            case 12:
                convnum = 4095;
                break;
            case 14:
                convnum = 16383;
                break;
            case 16:
                convnum = 65535;
                break;
            default:
                ERR("[T%d] Error, invalid sensor bits bit_depth:%d plane:%d !\n", stream_type,pframe->bit_depth[plane],plane);
                return -1;
        }
        break;
#endif
    default:
        ERR("[T%d] Error, invalid stream type !\n", stream_type);
        return -1;
    }

    sprintf(buf,"%d", pframe->width[plane]);       buf += strlen(buf);
    sprintf(buf," ");                       buf += strlen(buf);
    sprintf(buf,"%d", pframe->height[plane]);      buf += strlen(buf);
    sprintf(buf," ");                       buf += strlen(buf);

    sprintf(buf,"%d",convnum);              buf += strlen(buf);
    sprintf(buf,"\n");                      buf += strlen(buf);

    return (int32_t)(buf - header);
}


static int store_data_to_file( int stream_type, uint32_t frame_id, struct tm *tm,
                                char * filename, char * postfix, char * ext,
                                const char * header, uint32_t header_size,
                                const char * buf, uint32_t buf_size ) {
    FILE *bfp;

    MSG("[Stream-%d] Capturing started.\n", stream_type);

    if (filename == NULL || postfix == NULL) {
        ERR("[Stream-%d] NULL string ! (filename = %p, postfix = %p)\n", stream_type, filename, postfix);
        return -1;
    }
#if 1
    sprintf(filename, "IMG%06d_%04d%02d%02d_%02d%02d%02d_%s.%s",
            frame_id, tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec, postfix, ext);
#else /* Removed postfix to make it compatible to IQ tools */
    sprintf(filename, "IMG%06d_%04d%02d%02d_%02d%02d%02d.%s",
            frame_id, tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec, ext);
#endif
    MSG("[Stream-%d]   - filename: %s, frame_id = %d\n", stream_type, filename, frame_id);

    if (buf == NULL || buf_size == 0) {
        ERR("[Stream-%d] invalid parameter ! (buf = %p, buf_size = %d)\n", stream_type, buf, buf_size);
        return -1;
    }

    MSG("[Stream-%d]   - header = %p, header_size = %d\n", stream_type, header, header_size);
    MSG("[Stream-%d]   - buffer = %p, buf_size = %d\n", stream_type, buf, buf_size);

    bfp = fopen(filename, "wb");
    if (bfp) {
        if (header != NULL && header_size > 0) {
            fwrite(header, sizeof(uint8_t), header_size, bfp);
        }
        fwrite(buf, sizeof(uint8_t), buf_size, bfp);
        fflush(bfp);
        fclose(bfp) ;
    }
    MSG("[Stream-%d] Capture data done.\n", stream_type);

    return 0;
}

static FILE * prepare_frm_file(int stream_type, uint32_t frame_id, struct tm *tm,
        char * filename, char * postfix, char * ext,
		frame_t *pframe){
	FILE *bfp;
	MSG("[Stream-%d] FRM Capturing started %d %d %d.\n", stream_type,pframe->width[0],pframe->height[0],pframe->bytes_per_line[0]);

	if (filename == NULL || postfix == NULL) {
		ERR("[Stream-%d] NULL string ! (filename = %p, postfix = %p)\n", stream_type, filename, postfix);
		return NULL;
	}

	sprintf(filename, "IMG%06d_%04d%02d%02d_%02d%02d%02d_%s.%s",
		frame_id, tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec, postfix, ext);

	bfp = fopen(filename, "wb");

	if(bfp){
		fprintf(bfp,"#ISP1.width.height.depth.type.layers.");
		fprintf(bfp,"%d.", pframe->width[0]);
		fprintf(bfp,"%d.", pframe->height[0]);
		fprintf(bfp,"%d.",pframe->bit_depth[0]);
		fprintf(bfp,"%d.",(pframe->bytes_per_line[0]/pframe->width[0])*8); //type or allignment in bits
		fprintf(bfp,"%d.", pframe->num_planes);
		fflush(bfp);
	}

	return bfp;

}

static int write_frm_file(FILE * frm_file,const char * buf, uint32_t buf_size){
	if (buf == NULL || buf_size == 0 || frm_file==NULL) {
		ERR("[File-%p] invalid parameter ! (buf = %p, buf_size = %d)\n", frm_file, buf, buf_size);
		return -1;
	}
	if (frm_file) {
		fwrite(buf, sizeof(uint8_t), buf_size, frm_file);
		fflush(frm_file);
	}
	return 0;
}

static void finish_frm_file(FILE * frm_file){
	if (frm_file) {
		fflush(frm_file);
		fclose(frm_file);
	}
}

static void release_frame_pack(capture_module_t * cap_mod, int index) {
    int rc, i;

    for (i = 0; i < ARM_V4L2_TEST_STREAM_MAX; i++) {
        if(cap_mod->frame_pack_queue[index].frame_flag & ID_TO_FLAG(i)) {
            rc = ioctl (cap_mod->frame_pack_queue[index].frame_data[i].vfd,
                        VIDIOC_QBUF,
                        &cap_mod->frame_pack_queue[index].frame_data[i].vbuf);
            if (rc < 0) {
                ERR("queue buf error on frame id %d (type %d).\n",
                        cap_mod->frame_pack_queue[index].frame_id, i);
            }
            if(cap_mod->frame_pack_queue[index].frame_data[i].vbuf.type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE)
            	free(cap_mod->frame_pack_queue[index].frame_data[i].vbuf.m.planes);
            cap_mod->frame_pack_queue[index].frame_flag &= ~ID_TO_FLAG(i);
        }
    }
}

static void process_capture_legacy(capture_module_t * cap_mod, int index, int frm) {
    int                     frame_id = cap_mod->frame_pack_queue[index].frame_id;
    frame_t                 * pframe;

    time_t                  t = time(NULL);
    struct tm               tm = *localtime(&t);

    uint8_t                 * buf = NULL;
    uint32_t                buf_size = 0;
    char                    * header = NULL;
    static uint32_t         header_size = 0;
#if ARM_V4L2_TEST_HAS_META
    firmware_metadata_t     * meta = NULL;
    char                    * metadata_buf = NULL;
    uint32_t                metadata_buf_size = 0;
#endif
    int j;
    FILE * frm_file;

    MSG("Doing legacy capture for all streams.\n");

    /* allocate buffer */
    header = (char *)malloc(SD_HEADER_SIZE);

    /* do fr capture */
    pframe = &cap_mod->frame_pack_queue[index].frame_data[ARM_V4L2_TEST_STREAM_FR];

    if(frm==1){
    	char postfix[8];
    	sprintf(postfix, "fr %d",j);
    	frm_file=prepare_frm_file(ARM_V4L2_TEST_STREAM_FR, frame_id, &tm,
				(cap_mod->last_capture_filename[ARM_V4L2_TEST_STREAM_FR]),
				postfix, "FRM",pframe);
    }

    //mutiplane support
    for(j=0;j<pframe->num_planes;j++){
		buf = pframe->paddr[j];
		if(pframe->vbuf.type==V4L2_BUF_TYPE_VIDEO_CAPTURE)
			buf_size = pframe->vbuf.bytesused;
		else if(pframe->vbuf.type==V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE)
			buf_size = pframe->vbuf.m.planes[j].bytesused;
		MSG("[fr]  buffer = %p, buf_size = %d\n",  buf, buf_size);
		if(frm==0){
			header_size = fill_header(ARM_V4L2_TEST_STREAM_FR, header, pframe,j);
			char postfix[8];
			sprintf(postfix, "fr-%d",j);
			char pformat[4];
			if(pframe->pixelformat==V4L2_PIX_FMT_NV12)
				sprintf(pformat, "YUV");
			else
				sprintf(pformat, "RGB");
			//MSG("[fr]  buffer = %p, buf_size = %d\n",  buf, buf_size);
			store_data_to_file(ARM_V4L2_TEST_STREAM_FR, frame_id, &tm,
								(cap_mod->last_capture_filename[ARM_V4L2_TEST_STREAM_FR]),
								postfix, pformat, header, header_size, buf, buf_size);
		}else{
			write_frm_file(frm_file,buf,buf_size);
		}
		cap_mod->last_buf_size = buf_size;
		cap_mod->last_header_size = header_size;
    }

    if(frm==1){
    	finish_frm_file(frm_file);
    }

#if ARM_V4L2_TEST_HAS_META
    /* do meta capture */
    metadata_buf = (char *)malloc(SD_META_SIZE);
    meta = (firmware_metadata_t *)cap_mod->frame_pack_queue[index].frame_data[ARM_V4L2_TEST_STREAM_META].paddr[0];

    metadata_buf_size = fill_meta_buf(metadata_buf, meta);
    MSG("[meta]  buffer = %p, buf_size = %d\n",  metadata_buf, metadata_buf_size);
    store_data_to_file(ARM_V4L2_TEST_STREAM_META, frame_id, &tm,
                        cap_mod->last_capture_filename[ARM_V4L2_TEST_STREAM_META],
                        "meta", "TXT", NULL, 0, metadata_buf, metadata_buf_size);
    free(metadata_buf);

    cap_mod->last_metadata_buf_size = metadata_buf_size;
#endif

#if ARM_V4L2_TEST_HAS_RAW
    /* do raw capture */
    pframe = &cap_mod->frame_pack_queue[index].frame_data[ARM_V4L2_TEST_STREAM_RAW];

    if(frm==1){
		char postfix[8];
		sprintf(postfix, "raw",j);
		frm_file=prepare_frm_file(ARM_V4L2_TEST_STREAM_RAW, frame_id, &tm,
				(cap_mod->last_capture_filename[ARM_V4L2_TEST_STREAM_RAW]),
				postfix, "FRM",pframe);
	}
    //mutiplane support
    for(j=0;j<pframe->num_planes;j++){
		buf = pframe->paddr[j];
		if(pframe->vbuf.type==V4L2_BUF_TYPE_VIDEO_CAPTURE)
			buf_size = pframe->vbuf.bytesused;
		else if(pframe->vbuf.type==V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE)
			buf_size = pframe->vbuf.m.planes[j].bytesused;

		MSG("[raw]  buffer = %p, buf_size = %d\n",  buf, buf_size);
	//#if CONVERT_RAW_4_TO_2 // Hardcoded to convert 2byte raw to 4byte apical-raw
		if(frm==0){
			header_size = fill_header(ARM_V4L2_TEST_STREAM_RAW, header, pframe,j);
			uint32_t buf_padded_size = pframe->width[j] * 4 * pframe->height[j];
			uint8_t * buf_padded = malloc(buf_padded_size);
			{
				int i, offset_d, offset_s;
				for(i = 0; i < pframe->height[j]; i++) {
					offset_d = i * pframe->width[j] * 4;
					offset_s = i * pframe->bytes_per_line[j];
					memcpy(buf_padded + offset_d, buf + offset_s, pframe->width[j] * 2);
				}
			}
			buf = buf_padded;
			buf_size = buf_padded_size;
		//}
	//#endif
			char postfix[8];
			sprintf(postfix, (frm?"frm-%d":"raw-%d"),j);
			store_data_to_file(ARM_V4L2_TEST_STREAM_RAW, frame_id, &tm,
								(cap_mod->last_capture_filename[ARM_V4L2_TEST_STREAM_RAW]),
								postfix,(frm?"FRM":"RAW"), header, header_size, buf, buf_size);
	//#if CONVERT_RAW_4_TO_2 // Hardcoded to convert 2byte raw to 4byte apical-raw
			free(buf_padded);
    	}else{
    		write_frm_file(frm_file,buf,buf_size);
    	}
	//#endif
    }
    if(frm==1){
		finish_frm_file(frm_file);
	}
#endif

    free(header);
}

int enqueue_buffer(capture_module_t *cap_mod, uint32_t stream_type,
                    frame_t *newframe, capture_type_t try_capture) {
    uint32_t frame_id = newframe->vbuf.sequence;
    uint32_t index = frame_id % FRAME_PACK_QUEUE_SIZE;
    uint32_t flag = ID_TO_FLAG(stream_type);
    int captured = 0;

    if (stream_type > ARM_V4L2_TEST_STREAM_MAX) {
        ERR("Error, invalid stream_type %d\n", stream_type);
    }

    pthread_mutex_lock(&cap_mod->vmutex);

    /* check if incoming frame_id is new to queue */
    if(cap_mod->frame_pack_queue[index].frame_id != frame_id) {
        // qbuf oldest buffers and set flag to 0
    	DBG("queue id: frame_pack_queue frame id %d (frameid %d).\n",cap_mod->frame_pack_queue[index].frame_id,frame_id);
        release_frame_pack(cap_mod, index);
    }


    /* check if frame_pack already has buffer for this stream_type */
    if (cap_mod->frame_pack_queue[index].frame_flag & flag) {
        int rc;
        ERR("[T#%d] Error, redundant frame id %d !\n",stream_type, frame_id);
        rc = ioctl (cap_mod->frame_pack_queue[index].frame_data[stream_type].vfd,
                    VIDIOC_QBUF,
                    &cap_mod->frame_pack_queue[index].frame_data[stream_type].vbuf);
        if (rc < 0) {
            ERR("queue buf error on frame id %d (type %d).\n",
                    cap_mod->frame_pack_queue[index].frame_id, stream_type);
        }
        if(cap_mod->frame_pack_queue[index].frame_data[stream_type].vbuf.type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE)
        	free(cap_mod->frame_pack_queue[index].frame_data[stream_type].vbuf.m.planes);
    }

    /* add buffer to frame_pack and set flag for the stream_type */
    cap_mod->frame_pack_queue[index].frame_id = frame_id;
    cap_mod->frame_pack_queue[index].frame_data[stream_type] = *newframe;
    cap_mod->frame_pack_queue[index].frame_flag |= flag;
    DBG("[T#%d] new frame_id:%d index:%d flag:0x%x\n",stream_type,frame_id,index,cap_mod->frame_pack_queue[index].frame_flag);
    /*if(stream_type==ARM_V4L2_TEST_STREAM_RAW){
    	printf("frame_id:%d newframe->bit_depth[0]:%d\n",frame_id,newframe->bit_depth[0]);
    }*/

    /* try capture if buffer matched */
    if (try_capture != V4L2_TEST_CAPTURE_NONE &&
            cap_mod->frame_pack_queue[index].frame_flag == STREAM_FLAG_CAPTURE) {
        switch(try_capture) {
        case V4L2_TEST_CAPTURE_LEGACY:
        case V4L2_TEST_CAPTURE_FRM:
            process_capture_legacy(cap_mod, index,(try_capture==V4L2_TEST_CAPTURE_FRM)?1:0);
            release_frame_pack(cap_mod, index);
            captured = 1;
            break; 
        default:
            ERR("Capture type is not supported now !");
            break;
        }
    }

    pthread_mutex_unlock(&cap_mod->vmutex);

    return captured;
}

void init_capture_module(capture_module_t * cap_mod) {
    int i;

    /* init queue */
    for (i = 0; i < FRAME_PACK_QUEUE_SIZE; i++) {
        cap_mod->frame_pack_queue[i].frame_id = -1;
        cap_mod->frame_pack_queue[i].frame_flag = 0;
    }

    /* init mutex */
    pthread_mutex_init(&cap_mod->vmutex, NULL);

    /* init capture history */
    cap_mod->capture_performed = 0;
    cap_mod->last_buf_size = 0;
    cap_mod->last_header_size = 0;
#if ARM_V4L2_TEST_HAS_META
    cap_mod->last_metadata_buf_size = 0;
#endif
}

void release_capture_module_stream(capture_module_t * cap_mod, int stream_type) {
    uint32_t flag = ID_TO_FLAG(stream_type);
    int i;

    pthread_mutex_lock(&cap_mod->vmutex);
    /* release all buffers for stream_type */
    for (i = 0; i < FRAME_PACK_QUEUE_SIZE; i++) {
        if (cap_mod->frame_pack_queue[i].frame_flag & flag) {
            int rc;
            rc = ioctl (cap_mod->frame_pack_queue[i].frame_data[stream_type].vfd,
                        VIDIOC_QBUF,
                        &cap_mod->frame_pack_queue[i].frame_data[stream_type].vbuf);
            if (rc < 0) {
                ERR("queue buf error on frame id %d (type %d).\n",
                        cap_mod->frame_pack_queue[i].frame_id, stream_type);
            }
            if(cap_mod->frame_pack_queue[i].frame_data[stream_type].vbuf.type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE)
            	free(cap_mod->frame_pack_queue[i].frame_data[stream_type].vbuf.m.planes);
            cap_mod->frame_pack_queue[i].frame_flag &= ~flag;
        }
    }

    pthread_mutex_unlock(&cap_mod->vmutex);
}
