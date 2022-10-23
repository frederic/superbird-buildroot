#ifndef AUDIO_CLIENT_H
#define AUDIO_CLIENT_H

#include <boost/interprocess/managed_shared_memory.hpp>
#include <cstdlib>
#include <iomanip>
#include <unistd.h>

#include <hardware/hardware.h>
#include <hardware/audio.h>
#include "CircularBuffer.h"

#include <grpc/grpc.h>
#include <grpcpp/channel.h>
#include <grpcpp/client_context.h>
#include <grpcpp/create_channel.h>
#include <grpcpp/security/credentials.h>
#include "audio_service.grpc.pb.h"

using grpc::Channel;
using grpc::ClientContext;
using grpc::ClientReader;
using grpc::ClientReaderWriter;
using grpc::ClientWriter;
using grpc::Status;
using google::protobuf::Empty;

typedef struct audio_stream_client {
  char name[32];
  struct audio_stream stream;
} audio_stream_client_t;

typedef struct audio_stream_out_client {
  char name[32];
  struct audio_stream_out stream_out;
} audio_stream_out_client_t;

typedef struct audio_stream_in_client {
  char name[32];
  struct audio_stream_in stream_in;
} audio_stream_in_client_t;

template< class T, class M >
static inline constexpr ptrdiff_t offset_of( const M T::*member ) {
  return reinterpret_cast< ptrdiff_t >( &( reinterpret_cast< T* >( 0 )->*member ) );
}

template< class T, class M >
static inline T* container_of( const M *ptr, const M T::*member ) {
  return reinterpret_cast< T* >( reinterpret_cast< intptr_t >( ptr ) - offset_of( member ) );
}

inline audio_stream_in_client_t* audio_stream_in_to_client(const audio_stream_in *p)
{
  return container_of(p, &audio_stream_in_client::stream_in);
}

inline audio_stream_out_client_t* audio_stream_out_to_client(const audio_stream_out *p)
{
  return container_of(p, &audio_stream_out_client::stream_out);
}

inline audio_stream_client_t* audio_stream_to_client(const audio_stream *p)
{
  return container_of(p, &audio_stream_client::stream);
}

using namespace audio_service;

class AudioClient {
  public:
    AudioClient(std::shared_ptr<Channel> channel)
      : stub_(AudioService::NewStub(channel)) {
        shm_ = std::make_unique<managed_shared_memory>(open_only, "AudioServiceShmem");
      }

    // Device methods
    int Device_common_close(struct hw_device_t* device);
    int Device_init_check(const struct audio_hw_device *dev);
    int Device_set_voice_volume(struct audio_hw_device *dev, float volume);
    int Device_set_master_volume(struct audio_hw_device *dev, float volume);
    int Device_get_master_volume(struct audio_hw_device *dev, float *volume);
    int Device_set_mode(struct audio_hw_device *dev, audio_mode_t mode);
    int Device_set_mic_mute(struct audio_hw_device *dev, bool state);
    int Device_get_mic_mute(const struct audio_hw_device *dev, bool *state);
    int Device_set_parameters(struct audio_hw_device *dev, const char *kv_pairs);
    char * Device_get_parameters(const struct audio_hw_device *dev, const char *keys);
    size_t Device_get_input_buffer_size(const struct audio_hw_device *dev,
                                        const struct audio_config *config);
    int Device_open_output_stream(struct audio_hw_device *dev,
                                  audio_io_handle_t handle,
                                  audio_devices_t devices,
                                  audio_output_flags_t flags,
                                  struct audio_config *config,
                                  audio_stream_out_client_t *stream_out,
                                  const char *address);
    void Device_close_output_stream(struct audio_hw_device *dev,
                                    struct audio_stream_out* stream_out);
    int Device_open_input_stream(struct audio_hw_device *dev,
                                 audio_io_handle_t handle,
                                 audio_devices_t devices,
                                 struct audio_config *config,
                                 audio_stream_in_client_t *stream_in,
                                 audio_input_flags_t flags,
                                 const char *address,
                                 audio_source_t source);
    void Device_close_input_stream(struct audio_hw_device *dev,
                                   struct audio_stream_in *stream_in);
    int Device_dump(const struct audio_hw_device *dev, int fd);
    int Device_set_master_mute(struct audio_hw_device *dev, bool mute);
    int Device_get_master_mute(struct audio_hw_device *dev, bool *mute);
    int Device_create_audio_patch(struct audio_hw_device *dev,
                                  unsigned int num_sources,
                                  const struct audio_port_config *sources,
                                  unsigned int num_sinks,
                                  const struct audio_port_config *sinks,
                                  audio_patch_handle_t *handle);
    int Device_release_audio_patch(struct audio_hw_device *dev,
                                   audio_patch_handle_t handle);
    int Device_set_audio_port_config(struct audio_hw_device *dev,
                                     const struct audio_port_config *config);

    // stream in methods
    int stream_in_set_gain(struct audio_stream_in *stream, float gain);
    ssize_t stream_in_read(struct audio_stream_in *stream, void* buffer,
                           size_t bytes);
    uint32_t stream_in_get_input_frames_lost(struct audio_stream_in *stream);
    int stream_in_get_capture_position(const struct audio_stream_in *stream,
                                       int64_t *frames, int64_t *time);

    // stream_out methods
    uint32_t stream_out_get_latency(const struct audio_stream_out *stream);
    int stream_out_set_volume(struct audio_stream_out *stream, float left, float right);
    ssize_t stream_out_write(struct audio_stream_out *stream, const void* buffer,
                     size_t bytes);
    int stream_out_get_render_position(const struct audio_stream_out *stream,
                      uint32_t *dsp_frames);
    int stream_out_get_next_write_timestamp(const struct audio_stream_out *stream,
                      int64_t *timestamp);
    int stream_out_pause(struct audio_stream_out* stream);
    int stream_out_resume(struct audio_stream_out* stream);
    int stream_out_flush(struct audio_stream_out* stream);
    int stream_out_get_presentation_position(const struct audio_stream_out *stream,
                      uint64_t *frames, struct timespec *timestamp);

    // stream common methods
    uint32_t stream_get_sample_rate(const struct audio_stream *stream);
    size_t stream_get_buffer_size(const struct audio_stream *stream);
    audio_channel_mask_t stream_get_channels(const struct audio_stream *stream);
    audio_format_t stream_get_format(const struct audio_stream *stream);
    int stream_standby(struct audio_stream *stream);
    int stream_dump(const struct audio_stream *stream, int fd);
    audio_devices_t stream_get_device(const struct audio_stream *stream);
    int stream_set_parameters(struct audio_stream *stream, const char *kv_pairs);
    char * stream_get_parameters(const struct audio_stream *stream,
                      const char *keys);

    const int kSharedBufferSize = 256 * 1024;

  private:
    std::string new_stream_name(char *name, size_t size) {
      int pid = ::getpid();
      int seq = (stream_seq_++);
      printf("pid=%d seq=%d\n", pid, seq);
      snprintf(name, size, "%d-%d", pid, seq);
      printf("name = %s\n", name);
      return std::string(name);
    }

    std::unique_ptr<AudioService::Stub> stub_;
    std::unique_ptr<managed_shared_memory> shm_;
    static std::atomic_int stream_seq_;
};

#endif // AUDIO_CLIENT_H

