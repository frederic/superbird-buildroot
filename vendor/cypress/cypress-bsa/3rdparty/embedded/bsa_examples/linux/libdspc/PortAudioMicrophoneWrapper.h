/*
 * PortAudioMicrophoneWrapper.h
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

#ifndef _PORT_AUDIO_MICROPHONE_WRAPPER_H_
#define _PORT_AUDIO_MICROPHONE_WRAPPER_H_

#include <mutex>
#include <thread>
#include <pthread.h>

/// This acts as a wrapper around PortAudio, a cross-platform open-source audio I/O library.
class PortAudioMicrophoneWrapper {
public:
    /**
     * Creates a @c PortAudioMicrophoneWrapper.
     *
     * @param stream The shared data stream to write to.
     * @return A unique_ptr to a @c PortAudioMicrophoneWrapper if creation was successful and @c nullptr otherwise.
     */ 
    static std::unique_ptr<PortAudioMicrophoneWrapper> create();

    /**
     * Destructor.
     */
    ~PortAudioMicrophoneWrapper();

    std::thread dsp_process;
    std::thread alsa_process;
    void do_dsp_processing();

    std::thread debug_pcm_write;
    void do_debug_pcm_write();

    std::thread pcm_read_thread;
    void do_pcm_read();

    int do_dspc_data_buffer_alloc(int16_t *p_ring_buffer,int *p_wp_alsa, int *p_rp_alsa);

    void do_alsa_output_processing();

    bool thread_run();
    
    bool pcm_read_start();
    
    bool pcm_read_stop();
private:

    /**
     * Constructor.
     * 
     * @param stream The shared data stream to write to.
     */
    PortAudioMicrophoneWrapper();

    // Initializes PortAudio
    bool initialize();

    /**
     * A lock to seralize access to startStreamingMicrophoneData() and stopStreamingMicrophoneData() between different 
     * threads.
     */
    std::mutex m_mutex;
};


#endif // _PORT_AUDIO_MICROPHONE_WRAPPER_H_
