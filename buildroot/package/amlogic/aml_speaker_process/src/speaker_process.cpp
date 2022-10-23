/*
 * This file is part of avs.
 * Copyright (c) amlogic 2017
 * All rights reserved.
 * author:renjun.xu@amlogic.com
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software.
 *
*/
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <thread>
#include <vector>
#include <alsa/asoundlib.h>
#include <awelib.h>
#include <TcpIO2.h>
#include <AMLogic_VUI_Solution_Model.h>

#define SAMPLE_RATE 48000
#define REC_DEVICE_NAME "hw:1,1,0"
#define WRITE_DEVICE_NAME "softvol"
#define READ_FRAME 768
#define BUFFER_SIZE (SAMPLE_RATE/2)
#define PERIOD_SIZE (SAMPLE_RATE/4)

#define DSP_SOCKET 0
#define DUMP_DATA 0

#if DUMP_DATA
FILE* fp_input ; FILE* fp_output;
#endif

UINT32 inCount = 0;
UINT32 outCount = 0;
UINT32 in_samps = 0;
UINT32 in_chans = 0;
UINT32 out_samps = 0;
UINT32 out_chans = 0;
CAWELib *pAwelib;
std::vector<int> out_samples;
std::vector<int> in_samples;


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

	if (msglen != 0)
	{
		// This is a binary message.
		UINT32 *txPacket_Host = (UINT32 *)(pCurrentReplyMsg->GetData());
		if (txPacket_Host)
		{
			UINT32 first = txPacket_Host[0];
			UINT32 len = (first >> 16);
			if (s_pktLen)
			{
				len = s_pktLen;
			}
			if (len > MAX_COMMAND_BUFFER_LEN)
			{
				printf("count=%d msglen=%d\n",
					count, msglen);
				printf("NotifyFunction: packet 0x%08x len %d too big (max %d)\n", first,
					len, MAX_COMMAND_BUFFER_LEN);
				exit(1);
			}
			if (len * sizeof(int) > s_partial + msglen)
			{
				// Paxket is not complete, copy partial words.
//				printf("Partial message bytes=%d len=%d s_partial=%d\n",
//					msglen, len, s_partial);
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
			if (ret < 0)
			{
				printf("NotifyFunction: SendCommand failed with %d\n", ret);
				exit(1);
			}

			// Verify sane AWE reply.
			len = commandBuf[0] >> 16;
			if (len > MAX_COMMAND_BUFFER_LEN)
			{
				printf("NotifyFunction: reply packet len %d too big (max %d)\n",
					len, MAX_COMMAND_BUFFER_LEN);
				exit(1);
			}
			ret = ptcpIO->SendBinaryMessage("", -1, (BYTE *)commandBuf, len * sizeof(UINT32));
			if (ret < 0)
			{
				printf("NotifyFunction: SendBinaryMessage failed with %d\n", ret);
				exit(1);
			}
		}
		else
		{
			printf("NotifyFunction: impossible no message pointer\n");
			exit(1);
		}
	}
	else
	{
		printf("NotifyFunction: illegal zero klength message\n");
		exit(1);
	}
}
#endif

void setup_DSP() {
	UINT32 wireId1 = 1;
	UINT32 wireId2 = 2;
	UINT32 layoutId = 0;
	int error = 0;
	// Create AWE.
	pAwelib = AWELibraryFactory();
	error = pAwelib->CreateEx("spk", 1.418e9f, 1e7f, true, 768, 0, 2, 2);
	if (error < 0)
	{
		printf("[DSP] Create AWE failed with %d\n", error);
		delete pAwelib;
		exit(1);
	}
	printf("[DSP Using library '%s'\n", pAwelib->GetLibraryVersion());

	const char* file = "/usr/bin/speaker_processing.awb";
	// Only load a layout if asked.
	uint32_t pos;
	error = pAwelib->LoadAwbFileUnencrypted(file, &pos);
	if (error < 0)
	{
		printf("[DSP] LoadAwbFile failed with %d\n", error);
		delete pAwelib;
		exit(1);
	}

	{
		UINT32 in_complex = 0;
		UINT32 out_complex = 0;
		error = pAwelib->PinProps(in_samps, in_chans, in_complex, wireId1,
			out_samps, out_chans, out_complex, wireId2);
		if (error < 0)
		{
			printf("PinProps failed with %d\n", error);
			delete pAwelib;
			exit(1);
		}
		if (error > 0)
		{
			// We have a layout.
			if (in_complex)
			{
				in_chans *= 2;
			}
			if (out_complex)
			{
				out_chans *= 2;
			}
			printf("[DSP] AWB layout In: %d samples, %d chans ID=%d; out: %d samples, %d chans ID=%d\n",
				in_samps, in_chans, wireId1, out_samps, out_chans, wireId2);
			inCount = in_samps * in_chans;
			outCount = out_samps * out_chans;
		}
		else
		{
			// We have no layout.
			printf("[DSP] AWB No layout\n");
		}
	}

	// Now we know the IDs, set them.
	pAwelib->SetLayoutAddresses(wireId1, wireId2, layoutId);

	in_samples.resize(inCount);
        out_samples.resize(outCount);
	//ring_buffer.resize(RING_BUFFER_SIZE);

#if DSP_SOCKET == 1
	// Listen for AWE server on 15002.
	UINT32 listenPort = 15020;
	ptcpIO = new CTcpIO2();
	ptcpIO->SetNotifyFunction(pAwelib, NotifyFunction);
	HRESULT hr = ptcpIO->CreateBinaryListenerSocket(listenPort);
	if (FAILED(hr))
	{
		printf("Could not open socket on %d failed with %d\n",
			listenPort, hr);
		delete ptcpIO;
		delete pAwelib;
		exit(1);
	}
#endif

}


int main()
{
    setup_DSP();
#if DUMP_DATA
    fp_input = fopen("/data/input.bin" , "wb");
    fp_output = fopen("/data/output.bin" , "wb");
#endif
    snd_pcm_t *handle;
    snd_pcm_hw_params_t *params;
    int err;
    int dir;
    int buffer[READ_FRAME * 2];
    int ret = -1;
    unsigned int sampleRate = SAMPLE_RATE;
    snd_pcm_uframes_t periodSize = PERIOD_SIZE;
    snd_pcm_uframes_t bufferSize = BUFFER_SIZE;
    int count = 0;
    int error = 0;

    snd_pcm_hw_params_t *write_params;
    snd_pcm_t *write_handle;
    int write_err;
    int write_dir;
    unsigned int write_sampleRate = SAMPLE_RATE;
    snd_pcm_uframes_t write_periodSize = PERIOD_SIZE;
    snd_pcm_uframes_t write_bufferSize = BUFFER_SIZE;

    err = snd_pcm_open(&handle, REC_DEVICE_NAME, SND_PCM_STREAM_CAPTURE, 0);
    if (err)
    {
        printf( "Unable to open capture PCM device: \n");
        goto error;
    }

    snd_pcm_hw_params_alloca(&params);

    snd_pcm_hw_params_any(handle, params);

    err = snd_pcm_hw_params_set_access(handle, params, SND_PCM_ACCESS_RW_INTERLEAVED);
    if (err)
    {
        printf("Error setting interleaved mode\n");
        goto error;
    }

    err = snd_pcm_hw_params_set_format(handle, params, SND_PCM_FORMAT_S32_LE);
    if (err)
    {
        printf("Error setting format: %s\n", snd_strerror(err));
        goto error;
    }

    err = snd_pcm_hw_params_set_channels(handle, params, 2);
    if (err)
    {
        printf( "Error setting channels: %s\n", snd_strerror(err));
        goto error;
    }

    err = snd_pcm_hw_params_set_buffer_size_near(handle, params, &bufferSize);
    if (err)
    {
        printf("Error setting buffer size (%d): %s\n", bufferSize, snd_strerror(err));
        goto error;
    }

    err = snd_pcm_hw_params_set_period_size_near(handle, params, &periodSize, 0);
    if (err)
    {
        printf("Error setting period time (%d): %s\n", periodSize, snd_strerror(err));
        goto error;
    }

    err = snd_pcm_hw_params_set_rate_near(handle, params, &sampleRate, &dir);
    if (err)
    {
        printf("Error setting sampling rate (%d): %s\n", sampleRate, snd_strerror(err));
        goto error;
    }

    /* Write the parameters to the driver */
    err = snd_pcm_hw_params(handle, params);
    if (err < 0)
    {
        printf( "Unable to set HW parameters: %s\n", snd_strerror(err));
        goto error;
    }
    printf(" open record device done \n");

    write_err = snd_pcm_open(&write_handle, WRITE_DEVICE_NAME, SND_PCM_STREAM_PLAYBACK, 0);

    if (write_err)
    {
        printf( "Unable to open playback PCM device: \n");
        goto error;
    }

    snd_pcm_hw_params_alloca(&write_params);

    snd_pcm_hw_params_any(write_handle, write_params);

    write_err = snd_pcm_hw_params_set_access(write_handle, write_params, SND_PCM_ACCESS_RW_INTERLEAVED);
    if (write_err)
    {
        printf("Error setting interleaved mode\n");
        goto error;
    }

    write_err = snd_pcm_hw_params_set_format(write_handle, write_params, SND_PCM_FORMAT_S32_LE);
    if (write_err)
    {
        printf("Error setting format: %s\n", snd_strerror(write_err));
        goto error;
    }

    write_err = snd_pcm_hw_params_set_channels(write_handle, write_params, 2);
    if (write_err)
    {
        printf( "Error setting channels: %s\n", snd_strerror(write_err));
        goto error;
    }

    write_err = snd_pcm_hw_params_set_buffer_size_near(write_handle, write_params, &write_bufferSize);

    if (write_err)
    {
        printf("Error setting buffer size (%ld): %s\n", write_bufferSize, snd_strerror(write_err));
        goto error;
    }

    write_err = snd_pcm_hw_params_set_period_size_near(write_handle, write_params, &write_periodSize, 0);
    if (write_err)
    {
        printf("Error setting period time (%ld): %s\n", write_periodSize, snd_strerror(write_err));
        goto error;
    }

    write_err = snd_pcm_hw_params_set_rate_near(write_handle, write_params, &write_sampleRate, &write_dir);
    if (write_err)
    {
        printf("Error setting sampling rate (%d): %s\n", write_sampleRate, snd_strerror(write_err));
        goto error;
    }

    snd_pcm_uframes_t write_final_buffer;
    write_err = snd_pcm_hw_params_get_buffer_size(write_params, &write_final_buffer);
    printf(" final buffer size %ld \n" , write_final_buffer);

    snd_pcm_uframes_t write_final_period;
    write_err = snd_pcm_hw_params_get_period_size(write_params, &write_final_period, &write_dir);
    printf(" final period size %ld \n" , write_final_period);

    /* Write the parameters to the driver */
    write_err = snd_pcm_hw_params(write_handle, write_params);
    if (write_err < 0)
    {
        printf( "Unable to set HW parameters: %s\n", snd_strerror(write_err));
        goto error;
    }

    printf(" open write device is successful \n");

    while (1)
    {
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
	if (pAwelib->IsStarted()) {
		error = pAwelib->PumpAudio(&buffer[0], &out_samples[0], inCount, outCount);
	}
	else
		memset(&out_samples[0] , 0 , outCount*4);
#if DUMP_DATA
        fwrite(&buffer[0] , READ_FRAME*2 , sizeof(int) , fp_input);
        fflush(fp_input);
        fwrite(&out_samples[0] , READ_FRAME*2 , sizeof(int) , fp_output);
        fflush(fp_output);
#endif
	err = snd_pcm_writei(write_handle, &out_samples[0], READ_FRAME);
	if (err == -EPIPE) printf( "Underrun occurred from write: %d\n", err);
	if (err < 0) {
		err = snd_pcm_recover(write_handle, err, 0);
		std::this_thread::sleep_for(std::chrono::milliseconds(20));
		if (err < 0)
		{
			printf( "Error occured while writing: %s\n", snd_strerror(err));
			goto error;
		}
	}
    }
    ret = 0;

error:
    if (handle) snd_pcm_close(handle);
    return ret;
}
