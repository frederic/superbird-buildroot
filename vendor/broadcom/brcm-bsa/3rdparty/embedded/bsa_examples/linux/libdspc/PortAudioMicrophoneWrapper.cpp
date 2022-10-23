/*
 * PortAudioMicrophoneWrapper.cpp
 *
 * Copyright (c) 2017 Amazon.com, Inc. or its affiliates. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License").
 * You may not use this file except in compliance with the License.
 * A copy of the License is located at
 *
 *     http://aws.amazon.com/apache2.0/
 *
 * or in the "license" file accompanying this file. This file is distributed
 * on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
 * express or implied. See the License for the specific language governing
 * permissions and limitations under the License.
 */
#include <vector>
#include <atomic>
#include <string>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include "PortAudioMicrophoneWrapper.h"

#include <alsa/asoundlib.h>
#include "awelib.h"
#include "AMLogic_VUI_Solution_v6a_VoiceOnly_releaseSimple.h"
#include "TcpIO2.h"
using namespace std;

#define REC_DEVICE_NAME "hw:0,3"
static const int NUM_INPUT_CHANNELS = 8;
static const int NUM_OUTPUT_CHANNELS = 0;
static const double SAMPLE_RATE = 48000;
static const unsigned long PREFERRED_SAMPLES_PER_CALLBACK_FOR_DSP = 768;
static const unsigned long PREFERRED_SAMPLES_PER_CALLBACK_FOR_ALSA = 128;;
static const unsigned long RING_BUFFER_SIZE = PREFERRED_SAMPLES_PER_CALLBACK_FOR_DSP * NUM_INPUT_CHANNELS * 30;
static const unsigned long RING_BUFFER_SIZE_ALSA = PREFERRED_SAMPLES_PER_CALLBACK_FOR_ALSA*30;

#define DUMP_DATA 0
#define nullptr NULL

int prev_state  = 0;
#if DUMP_DATA
FILE* fp_input ;
FILE* fp_output;
//FILE *fp_data;
#endif

int loopCount = 0;

UINT32 inCount = 0;
UINT32 outCount = 0;
UINT32 in_samps = 0;
UINT32 in_chans = 0;
UINT32 out_samps = 0;
UINT32 out_chans = 0;

CAWELib *pAwelib;

std::vector<int16_t> audioData_SDS;

std::vector<int> out_samples;
std::vector<int> in_samples;
std::vector<int> ring_buffer;
int16_t *ring_buffer_alsa;
volatile int rp = 0;
volatile int wp = 0;
int *rp_alsa;
int *wp_alsa;
bool pcm_read_status = false;
PortAudioMicrophoneWrapper* wrapper;
#define SAMPLE_RATE 48000
#define WRITE_DEVICE_NAME "dmixer"
#define READ_FRAME 768
#define BUFFER_SIZE (SAMPLE_RATE/2)
#define PERIOD_SIZE (BUFFER_SIZE/4)

#define DSP_DEBUG 0
#define DSP_SOCKET 0

#if DSP_DEBUG == 1
#define WRITE_UNIT (READ_FRAME * 2)
#define RING_PCM_SIZE (BUFFER_SIZE*2)

int ring_pcm_buffer[RING_PCM_SIZE];
int write_pcm_buffer[WRITE_UNIT];
volatile int rp_pcm = 0 ;
volatile int wp_pcm = 0;
#endif

#if DSP_SOCKET == 1
/** The socket listener. */
static CTcpIO2 *ptcpIO = 0;
static UINT32 s_pktLen = 0;
static UINT32 s_partial = 0;
static UINT32 commandBuf[MAX_COMMAND_BUFFER_LEN];
/**
 * @brief Handle AWE server messages.
 * @param [in] pAwe				the library instance
 */
static void NotifyFunction(void *pAwe, int count)
{
    CAWELib *pAwelib = (CAWELib *)pAwe;
    SReceiveMessage *pCurrentReplyMsg = ptcpIO->BinaryGetMessage();
    const int msglen = pCurrentReplyMsg->size();

    if (msglen != 0) {
        // This is a binary message.
        UINT32 *txPacket_Host = (UINT32 *)(pCurrentReplyMsg->GetData());
        if (txPacket_Host) {
            UINT32 first = txPacket_Host[0];
            UINT32 len = (first >> 16);

            if (s_pktLen) {
                len = s_pktLen;
            }

            if (len > MAX_COMMAND_BUFFER_LEN) {
                printf("count=%d msglen=%d\n", count, msglen);
                printf("NotifyFunction: packet 0x%08x len %d too big (max %d)\n", first,
                       len, MAX_COMMAND_BUFFER_LEN);
                exit(1);
            }
            if (len * sizeof(int) > s_partial + msglen) {
                // Paxket is not complete, copy partial words.
                // printf("Partial message bytes=%d len=%d s_partial=%d\n",
                //	          msglen, len, s_partial);
                memcpy(((char *)commandBuf) + s_partial, txPacket_Host, msglen);
                s_partial += msglen;
                s_pktLen = len;
                return;
            }

            memcpy(((char *)commandBuf) + s_partial, txPacket_Host, msglen);
            s_partial = 0;
            s_pktLen = 0;

            // AWE processes the message.
            int ret = pAwelib->SendCommand(commandBuf, MAX_COMMAND_BUFFER_LEN);
            if (ret < 0) {
                printf("NotifyFunction: SendCommand failed with %d\n", ret);
                exit(1);
            }

            // Verify sane AWE reply.
            len = commandBuf[0] >> 16;
            if (len > MAX_COMMAND_BUFFER_LEN) {
                printf("NotifyFunction: reply packet len %d too big (max %d)\n",
                       len, MAX_COMMAND_BUFFER_LEN);
                exit(1);
            }

            ret = ptcpIO->SendBinaryMessage("", -1, (BYTE *)commandBuf, len * sizeof(UINT32));
            if (ret < 0) {
                printf("NotifyFunction: SendBinaryMessage failed with %d\n", ret);
                exit(1);
            }
        } else {
            printf("NotifyFunction: impossible no message pointer\n");
            exit(1);
        }
    } else {
        printf("NotifyFunction: illegal zero klength message\n");
        exit(1);
    }
}
#endif

#if DSP_DEBUG == 1
void fill_write_buffer(int* input)
{
    int space = rp_pcm - wp_pcm;
    if (space <= 0 ) space = RING_PCM_SIZE + space;
    if (space < READ_FRAME*2 ) {
        printf(" OVERFLOW IN fill_write_buffer wp_pcm %d , rp_pcm %d \n" , wp_pcm , rp_pcm);
    }

    for (int i = 0 ; i < READ_FRAME*2 ;  i = i+2) {
        ring_pcm_buffer[wp_pcm] = input[1];
        wp_pcm++;
        if (wp_pcm == RING_PCM_SIZE)
            wp_pcm = 0;
        ring_pcm_buffer[wp_pcm] = 0;
        wp_pcm++;
        if (wp_pcm == RING_PCM_SIZE)
            wp_pcm = 0;

        input += 2;
    }
}

void fill_write_buffer1(int* input)
{
    int space = rp_pcm - wp_pcm;
    if (space <= 0 ) space = RING_PCM_SIZE + space;

    if (space < READ_FRAME*2 ) {
        printf(" OVERFLOW IN fill_write_buffer1 wp_pcm %d , rp_pcm %d \n" , wp_pcm , rp_pcm);
    }

    for (int i = 0 ; i < READ_FRAME*8 ;  i = i + 8) {
        ring_pcm_buffer[wp_pcm] = input[0];
        wp_pcm++;

        if (wp_pcm == RING_PCM_SIZE)
            wp_pcm = 0;
        ring_pcm_buffer[wp_pcm] = input[1];
        wp_pcm++;
        if (wp_pcm == RING_PCM_SIZE)
            wp_pcm = 0;

        input = input + 8;
    }
}

void PortAudioMicrophoneWrapper::do_debug_pcm_write()
{
    printf("inside thread do_debug_pcm_write \n");
    snd_pcm_hw_params_t *params;
    snd_pcm_t *handle;
    int err;
    int dir;
    unsigned int sampleRate = SAMPLE_RATE;
    snd_pcm_uframes_t periodSize = PERIOD_SIZE;
    snd_pcm_uframes_t bufferSize = BUFFER_SIZE;
    int level;
    int first =0;

    err = snd_pcm_open(&handle, WRITE_DEVICE_NAME, SND_PCM_STREAM_PLAYBACK, 0);
    if (err) {
        printf( "Unable to open PCM device: \n");
        goto error;
    }

    snd_pcm_hw_params_alloca(&params);

    snd_pcm_hw_params_any(handle, params);

    err = snd_pcm_hw_params_set_access(handle, params, SND_PCM_ACCESS_RW_INTERLEAVED);
    if (err) {
        printf("Error setting interleaved mode\n");
        goto error;
    }

    err = snd_pcm_hw_params_set_format(handle, params, SND_PCM_FORMAT_S32_LE);
    if (err) {
        printf("Error setting format: %s\n", snd_strerror(err));
        goto error;
    }

    err = snd_pcm_hw_params_set_channels(handle, params, 2);
    if (err) {
        printf( "Error setting channels: %s\n", snd_strerror(err));
        goto error;
    }

    err = snd_pcm_hw_params_set_buffer_size_near(handle, params, &bufferSize);
    if (err) {
        printf("Error setting buffer size (%ld): %s\n", bufferSize, snd_strerror(err));
        goto error;
    }

    err = snd_pcm_hw_params_set_period_size_near(handle, params, &periodSize, 0);
    if (err) {
        printf("Error setting period time (%ld): %s\n", periodSize, snd_strerror(err));
        goto error;
    }

    err = snd_pcm_hw_params_set_rate_near(handle, params, &sampleRate, &dir);
    if (err) {
        printf("Error setting sampling rate (%d): %s\n", sampleRate, snd_strerror(err));
        goto error;
    }

    snd_pcm_uframes_t final_buffer;
    err = snd_pcm_hw_params_get_buffer_size(params, &final_buffer);
    printf(" final buffer size %ld \n" , final_buffer);

    snd_pcm_uframes_t final_period;
    err = snd_pcm_hw_params_get_period_size(params, &final_period, &dir);
    printf(" final period size %ld \n" , final_period);

    /* Write the parameters to the driver */
    err = snd_pcm_hw_params(handle, params);
    if (err < 0) {
        printf( "Unable to set HW parameters: %s\n", snd_strerror(err));
        goto error;
    }

    printf(" open write device is successful \n");

    while (1) {
        level = wp_pcm - rp_pcm;
        if (level < 0 ) level = RING_PCM_SIZE + level;

        if (first == 0 && level < RING_PCM_SIZE*3/4 ) {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            continue;
        } else
            first = 1;

        if (level >= WRITE_UNIT) {
            for (int i = 0 ; i < WRITE_UNIT ; i++) {
                write_pcm_buffer[i] = ring_pcm_buffer[rp_pcm];
                rp_pcm++;
                if (rp_pcm == RING_PCM_SIZE)
                    rp_pcm = 0;
            }

            err = snd_pcm_writei(handle, &write_pcm_buffer[0], WRITE_UNIT/2);
            if (err == -EPIPE)
                printf( "Underrun occurred from write: %d\n", err);
            if (err < 0) {
                err = snd_pcm_recover(handle, err, 0);
                std::this_thread::sleep_for(std::chrono::milliseconds(20));
                // Still an error, need to exit.
                if (err < 0) {
                    printf( "Error occured while writing: %s\n", snd_strerror(err));
                    goto error;
                }
            }
        }
    }

error:
    if (handle) snd_pcm_close(handle);
}
#endif

void setup_DSP() {
    UINT32 wireId1 = 1;
    UINT32 wireId2 = 2;
    UINT32 layoutId = 0;
    int error = 0;

    // Create AWE.
    pAwelib = AWELibraryFactory();
    error = pAwelib->Create("libtest", 1e9f, 1e7f);
    if (error < 0) {
        printf("[DSP] Create AWE failed with %d\n", error);
        delete pAwelib;
        exit(1);
    }
    printf("[DSP Using library '%s'\n", pAwelib->GetLibraryVersion());

    const char* file = "AMLogic_VUI_Solution_v6a_VoiceOnly_release.awb";
    // Only load a layout if asked.
    error = pAwelib->LoadAwbFile(file);
    if (error < 0) {
        printf("[DSP] LoadAwbFile failed with %d(%s)\n", error, file);
        delete pAwelib;
        exit(1);
    }

    UINT32 in_complex = 0;
    UINT32 out_complex = 0;
    error = pAwelib->PinProps(in_samps, in_chans, in_complex, wireId1,
    out_samps, out_chans, out_complex, wireId2);

    if (error < 0) {
        printf("PinProps failed with %d\n", error);
        delete pAwelib;
        exit(1);
    }

    if (error > 0) {
        // We have a layout.
        if (in_complex) {
            in_chans *= 2;
        }

        if (out_complex) {
            out_chans *= 2;
        }
        printf("[DSP] AWB layout In: %d samples, %d chans ID=%d; out: %d samples, %d chans ID=%d\n",
             in_samps, in_chans, wireId1, out_samps, out_chans, wireId2);
        inCount = in_samps * in_chans;
        outCount = out_samps * out_chans;
    } else {
        // We have no layout.
        printf("[DSP] AWB No layout\n");
    }

    // Now we know the IDs, set them.
    pAwelib->SetLayoutAddresses(wireId1, wireId2, layoutId);

    in_samples.resize(inCount);
    out_samples.resize(outCount);
    ring_buffer.resize(RING_BUFFER_SIZE);

#if DSP_SOCKET == 1
    // Listen for AWE server on 15002.
    UINT32 listenPort = 15002;
    ptcpIO = new CTcpIO2();
    ptcpIO->SetNotifyFunction(pAwelib, NotifyFunction);
    HRESULT hr = ptcpIO->CreateBinaryListenerSocket(listenPort);
    if (FAILED(hr)) {
        printf("Could not open socket on %d failed with %d\n",
               listenPort, hr);
        delete ptcpIO;
        delete pAwelib;
        exit(1);
    }
#endif
}

std::unique_ptr<PortAudioMicrophoneWrapper> PortAudioMicrophoneWrapper::create() {
    std::unique_ptr<PortAudioMicrophoneWrapper> portAudioMicrophoneWrapper(
        new PortAudioMicrophoneWrapper()
    );


    if (!portAudioMicrophoneWrapper->initialize()) {
        printf("Failed to initialize PortAudioMicrophoneWrapper");
        return nullptr;
    }

#if DUMP_DATA
    fp_input = fopen("input.bin" , "wb");
    fp_output = fopen("output.bin" , "wb");
//    fp_data = fopen("/tmp/data.bin", "wb");
#endif

    return portAudioMicrophoneWrapper;
}

PortAudioMicrophoneWrapper::PortAudioMicrophoneWrapper() {

#if DSP_DEBUG == 1
    debug_pcm_write = std::thread(&PortAudioMicrophoneWrapper::do_debug_pcm_write ,this);
#endif

}

PortAudioMicrophoneWrapper::~PortAudioMicrophoneWrapper() {
}

bool PortAudioMicrophoneWrapper::initialize() {
    setup_DSP();

    audioData_SDS.resize(PREFERRED_SAMPLES_PER_CALLBACK_FOR_ALSA);

    return true;
}

bool PortAudioMicrophoneWrapper::thread_run() {

    pcm_read_thread = std::thread (&PortAudioMicrophoneWrapper::do_pcm_read ,this);

    return true;
}

bool PortAudioMicrophoneWrapper::pcm_read_start() {
	pcm_read_status = true;
	return true;
}

bool PortAudioMicrophoneWrapper::pcm_read_stop() {
	pcm_read_status = false;
	return true;
}
//in : 32bit , 48K , 1 ch (outsamples from DSP )
//out : 16bit , 16K , 1 ch ( every 3rd sample ) ( audioData_SDS )
void convert_DSP(int* input , short *output , int frame_num)
{
    for (int i = 0 ; i < frame_num  ; i += 3) {
        *output = *input >> 14;
        output++;
        input += 3;
    }
}

//in : 32bit , 48K , 2 ch (outsamples from DSP )
//out : 16bit , 16K , 1 ch ( every 6th sample ) ( audioData_SDS )
//      or 16bit, 8K ,2ch
void convert_DSP_2ch(int* input , short *output , int frame_num)
{
    for (int i = 0 ; i < frame_num*2  ; i += 6) {
        *output = *input >> 16;
        output++;
        input += 6;
    }
}
//in : 32bit , 48K , 2 ch (outsamples from DSP )
//out : 16bit , 8K , 1 ch ( every 6th sample ) ( audioData_SDS )
void convert_DSP_1ch(int* input , short *output , int frame_num)
{
    for (int i = 0 ; i < frame_num*2; i += 12) {
        *output = *input >> 16;
        output++;
        input += 12;
    }
}
// in : 32bit , 48K , 8 channels
// out : 16bit , 48K , 1 channel
int convert(int *input, short *output, int frame_num)
{
    int i;
    for (i = 0; i < frame_num; i++) {
        output[i] = input[8*i]   >> 14;
    }
    return 0;
}

// input : 8 ch , 0,1 feedback , 2,3,4,5 are data , 6,7 is empty , 32 bit
// output : 10 ch , 0,1,2,3,4,5 : data , 6,7 not used , 8,9 : feedback , 32 bit
int convert1(int *input, int *output, int frame_num)
{
    int i;
    memset(output , 0 , frame_num*10);
    for (i = 0; i < frame_num*8; i = i+8) {
        output[0] = input [0];
        output[1] = input [1];
        output[2] = input [2];
        output[3] = input [3];
        output[4] = input [4];
        output[5] = input [5];
        output[8] = input [6];
        output[9] = input [7];

        input += 8;
        output += 10;
    }

    return 0;
}

void do_dsp_processing_fn(int* in_samples , int* out_samples , int inCount , int outCount , int& result , Sample* startIndex,
                             Sample* endIndex ) {

    int error = 0;
    Sample retVal;

   // printf(" do_dsp_processing_fn \n");
#if 0
    memset(&in_samples[0], 0, inCount * sizeof(short));

    //convert to 10 channels , 32 bit input
    for (UINT32 i = 0 ; i < PREFERRED_SAMPLES_PER_CALLBACK_FOR_DSP; i++) {
        in_samples[i*10] = record_bufffer[i] << 16;
    }
#endif
    loopCount += PREFERRED_SAMPLES_PER_CALLBACK_FOR_DSP;
    if (loopCount > 16000 * 8) {
      //  printf("detection loop heart beat ...\n");
        loopCount = 0;
    }

    if (pAwelib->IsStarted()) {
        error = pAwelib->PumpAudio(&in_samples[0], &out_samples[0], inCount, outCount);
    } else {
        memset(&out_samples[0] , 0 , outCount*4);
    }

#if DUMP_DATA
    //printf(" post PumpAudio \n");
    fwrite(&in_samples[0] , PREFERRED_SAMPLES_PER_CALLBACK_FOR_DSP*10 , sizeof(int) , fp_input);
    fflush(fp_input);
#endif

#if DSP_DEBUG == 1
    fill_write_buffer(&out_samples[0]);
#endif
    if (error < 0) {
        printf("[DSP pump of audio failed with %d.\n", error);
        delete pAwelib;
        exit(1);
    }

    error = pAwelib->FetchValues(MAKE_ADDRESS(AWE_isTriggered_ID, AWE_isTriggered_value_OFFSET), 1,
                        &retVal,0);
    result = retVal.iVal;

    error = pAwelib->FetchValues(MAKE_ADDRESS(AWE_startIndex_ID, AWE_startIndex_value_OFFSET), 1,
                        startIndex , 0);
    //printf(" error startindex %d \n" , error);
    error = pAwelib->FetchValues(MAKE_ADDRESS(AWE_endIndex_ID, AWE_endIndex_value_OFFSET), 1,
                        endIndex , 0);

}

void PortAudioMicrophoneWrapper::do_dsp_processing() {
	int result;
	//Sample DSP_state;
	Sample startIndex[AWE_startIndex_value_SIZE];
	Sample endIndex[AWE_endIndex_value_SIZE];
	uint64_t total_samps = 0;
	wrapper = static_cast<PortAudioMicrophoneWrapper*>(this);
	int detection_count = 0;
	printf("do_dsp_processing\n");
	while (1) {
		int level = wp - rp;
		if (level < 0 ) level = RING_BUFFER_SIZE + level;
		//printf("%s level: %d wp: %d  rp:%d  \n" ,__func__, level,wp,rp);
		if (level >= (int)PREFERRED_SAMPLES_PER_CALLBACK_FOR_DSP*NUM_INPUT_CHANNELS) {
			convert1(&ring_buffer[rp] , &in_samples[0], PREFERRED_SAMPLES_PER_CALLBACK_FOR_DSP);
			
			do_dsp_processing_fn(&in_samples[0],&out_samples[0],inCount,outCount, result , &startIndex[0] , &endIndex[0] );

		//	convert_DSP_2ch(&out_samples[0], &audioData_SDS[0], PREFERRED_SAMPLES_PER_CALLBACK_FOR_DSP); //PREFERRED_SAMPLES_PER_CALLBACK_FOR_ALSA = 256
			convert_DSP_1ch(&out_samples[0], &audioData_SDS[0], PREFERRED_SAMPLES_PER_CALLBACK_FOR_DSP); //PREFERRED_SAMPLES_PER_CALLBACK_FOR_ALSA = 128

#if DUMP_DATA
			fwrite(&audioData_SDS[0] , PREFERRED_SAMPLES_PER_CALLBACK_FOR_ALSA*1 , sizeof(short) , fp_output);
			fflush(fp_output);
#endif

			total_samps += audioData_SDS.size();

			if (result == 1) {
				detection_count++;
			//	printf(" wake word detected %d \n" , detection_count);
			}

			rp = rp + (int)PREFERRED_SAMPLES_PER_CALLBACK_FOR_DSP*NUM_INPUT_CHANNELS;
			if (rp > (int)RING_BUFFER_SIZE) {
				printf(" rp out of bound \n");
			}

			if (rp == RING_BUFFER_SIZE)
				rp = 0;

			int space_alsa = *wp_alsa - *rp_alsa;
			if (space_alsa <= 0 )	
				space_alsa = space_alsa + RING_BUFFER_SIZE_ALSA;
			if (space_alsa < (int)PREFERRED_SAMPLES_PER_CALLBACK_FOR_ALSA) {
				printf("ALSA OVERFLOW : space_alsa:%d\n",space_alsa);
			}
			memcpy(&ring_buffer_alsa[*wp_alsa] , &audioData_SDS[0] , PREFERRED_SAMPLES_PER_CALLBACK_FOR_ALSA*2);
			*wp_alsa = *wp_alsa + PREFERRED_SAMPLES_PER_CALLBACK_FOR_ALSA;

			if (*wp_alsa > (int)RING_BUFFER_SIZE_ALSA)
				printf("alsa wp out of bounds \n");

			if (*wp_alsa == RING_BUFFER_SIZE_ALSA)
				*wp_alsa = 0;

		} else {
			std::this_thread::sleep_for(std::chrono::milliseconds(200));
		}
	}
}

void PortAudioMicrophoneWrapper::do_pcm_read() {
	printf(" do pcm read \n");

	dsp_process = std::thread (&PortAudioMicrophoneWrapper::do_dsp_processing ,this);

	// alsa_process = std::thread (&PortAudioMicrophoneWrapper::do_alsa_output_processing ,this);

	snd_pcm_t *handle = NULL ;
	snd_pcm_hw_params_t *params;
	int err;
	int dir;
	int buffer[READ_FRAME * 8 * 4];
	unsigned int sampleRate = SAMPLE_RATE;
	snd_pcm_uframes_t periodSize = PERIOD_SIZE;
	snd_pcm_uframes_t bufferSize = BUFFER_SIZE;
	snd_pcm_uframes_t final_buffer;
	snd_pcm_uframes_t final_period;
	while (1) {
		if(pcm_read_status == true) {
			if(handle == NULL) {
				err = snd_pcm_open(&handle, REC_DEVICE_NAME, SND_PCM_STREAM_CAPTURE, 0);
				if (err) {
					printf( "Unable to open PCM device: \n");
					goto error;
				}

				snd_pcm_hw_params_alloca(&params);

				snd_pcm_hw_params_any(handle, params);

				err = snd_pcm_hw_params_set_access(handle, params, SND_PCM_ACCESS_RW_INTERLEAVED);
				if (err) {
					printf("Error setting interleaved mode\n");
					goto error;
				}

				err = snd_pcm_hw_params_set_format(handle, params, SND_PCM_FORMAT_S32_LE);
				if (err) {
					printf("Error setting format: %s\n", snd_strerror(err));
					goto error;
				}

				err = snd_pcm_hw_params_set_channels(handle, params, 8);
				if (err) {
					printf( "Error setting channels: %s\n", snd_strerror(err));
					goto error;
				}

				err = snd_pcm_hw_params_set_buffer_size_near(handle, params, &bufferSize);
				if (err) {
					printf("Error setting buffer size (%ld): %s\n", bufferSize, snd_strerror(err));
					goto error;
				}

				err = snd_pcm_hw_params_set_period_size_near(handle, params, &periodSize, 0);
				if (err) {
					printf("Error setting period time (%ld): %s\n", periodSize, snd_strerror(err));
					goto error;
				}

				err = snd_pcm_hw_params_set_rate_near(handle, params, &sampleRate, &dir);
				if (err) {
					printf("Error setting sampling rate (%d): %s\n", sampleRate, snd_strerror(err));
					goto error;
				}

				err = snd_pcm_hw_params_get_buffer_size(params, &final_buffer);
				printf(" final buffer size %ld \n" , final_buffer);

				err = snd_pcm_hw_params_get_period_size(params, &final_period, &dir);
				printf(" final period size %ld \n" , final_period);

				/* Write the parameters to the driver */
				err = snd_pcm_hw_params(handle, params);
				if (err < 0) {
					printf( "Unable to set HW parameters: %s\n", snd_strerror(err));
					goto error;
				}

				printf(" ready to read \n" );
			} 
			err = snd_pcm_readi(handle, buffer, READ_FRAME);
			if (err == -EPIPE) printf( "Overrun occurred: %d\n", err);

			if (err < 0) {
				err = snd_pcm_recover(handle, err, 0);
				// Still an error, need to exit.
				if (err < 0)
				{
					printf( "Error occured while recording: %s\n", snd_strerror(err));
					goto error;
				}
			}

			int space = wp - rp;
			if (space <= 0 ) space = space + RING_BUFFER_SIZE;
			if (space < (int)READ_FRAME*NUM_INPUT_CHANNELS) {
				printf(" OVERFLOW \n");
			}

			memcpy(&ring_buffer[wp] , (int*)buffer , READ_FRAME*4*NUM_INPUT_CHANNELS);

			wp = wp + READ_FRAME*NUM_INPUT_CHANNELS;

			if (wp > (int)RING_BUFFER_SIZE)
				printf(" wp out of bounds \n");

			if (wp == RING_BUFFER_SIZE)
				wp = 0;
		} else {
			std::this_thread::sleep_for(std::chrono::milliseconds(200));
			if (handle) {
				snd_pcm_close(handle);
				handle = NULL;
			}

		}

	}
error:
    if (handle) snd_pcm_close(handle);
}

int PortAudioMicrophoneWrapper::do_dspc_data_buffer_alloc(int16_t *p_ring_buffer, int *p_wp_alsa,int *p_rp_alsa)
{
	printf("do_dspc_data_buffer_alloc\n");
	ring_buffer_alsa = p_ring_buffer;
	wp_alsa = p_wp_alsa;
	rp_alsa = p_rp_alsa;
	return 0;
}

void PortAudioMicrophoneWrapper::do_alsa_output_processing()
{
#if 0
	printf("do_alsa_output_processing\n");
	while(1) {
		int level = *wp_alsa - *rp_alsa;
		if (level < 0 ) level = RING_BUFFER_SIZE_ALSA + level;
//		printf("%s level: %d \n" ,__func__, level);
		if (level >= (int)PREFERRED_SAMPLES_PER_CALLBACK_FOR_ALSA) {
			fwrite(&ring_buffer_alsa[*rp_alsa] , PREFERRED_SAMPLES_PER_CALLBACK_FOR_ALSA, sizeof(short) , fp_data);
			fflush(fp_data);
			*rp_alsa = *rp_alsa + (int)PREFERRED_SAMPLES_PER_CALLBACK_FOR_ALSA;
			if (*rp_alsa > (int)RING_BUFFER_SIZE_ALSA) {
				printf("alsa rp out of bound \n");
			}

			if (*rp_alsa == RING_BUFFER_SIZE_ALSA)
				*rp_alsa = 0;	
		} else {
			std::this_thread::sleep_for(std::chrono::milliseconds(200));
		}
	}
#endif
}
