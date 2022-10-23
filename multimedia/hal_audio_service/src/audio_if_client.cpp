#include <thread>
#include <mutex>
#include <cstdlib>

#include <hardware/hardware.h>
#include <hardware/audio.h>
#include <pthread.h>
#include <system/audio.h>
#include <cutils/log.h>

#include <grpc/grpc.h>
#include <grpcpp/channel.h>
#include <grpcpp/client_context.h>
#include <grpcpp/create_channel.h>
#include <grpcpp/security/credentials.h>

#include "audio_if_client.h"
#include "audio_client.h"

//#define TRACE_ENTRY() printf("[Client:%s] enter\n", __func__)
#define TRACE_ENTRY()

static AudioClient *client;
static int inited = 0;
static std::mutex client_mutex;

static uint32_t stream_get_sample_rate(const struct audio_stream *stream)
{
  TRACE_ENTRY();
  return client->stream_get_sample_rate(stream);
}

static size_t stream_get_buffer_size(const struct audio_stream *stream)
{
  TRACE_ENTRY();
  return client->stream_get_buffer_size(stream);
}

static audio_channel_mask_t stream_get_channels(const struct audio_stream *stream)
{
  TRACE_ENTRY();
  return client->stream_get_channels(stream);
}

static audio_format_t stream_get_format(const struct audio_stream *stream)
{
  TRACE_ENTRY();
  return client->stream_get_format(stream);
}

static int stream_standby(struct audio_stream *stream)
{
  TRACE_ENTRY();
  return client->stream_standby(stream);
}

static int stream_dump(const struct audio_stream *stream, int fd)
{
  TRACE_ENTRY();
  return client->stream_dump(stream, fd);
}

static audio_devices_t stream_get_device(const struct audio_stream *stream)
{
  TRACE_ENTRY();
  return client->stream_get_device(stream);
}

static int stream_set_parameters(struct audio_stream *stream, const char *kv_pairs)
{
  TRACE_ENTRY();
  return client->stream_set_parameters(stream, kv_pairs);
}

static char * stream_get_parameters(const struct audio_stream *stream,
                             const char *keys)
{
  TRACE_ENTRY();
  return client->stream_get_parameters(stream, keys);
}

static int stream_in_set_gain(struct audio_stream_in *stream, float gain)
{
  TRACE_ENTRY();
  return client->stream_in_set_gain(stream, gain);
}

static ssize_t stream_in_read(struct audio_stream_in *stream, void* buffer,
                    size_t bytes)
{
  TRACE_ENTRY();
  return client->stream_in_read(stream, buffer, bytes);
}

static uint32_t stream_in_get_input_frames_lost(struct audio_stream_in *stream)
{
  TRACE_ENTRY();
  return client->stream_in_get_input_frames_lost(stream);
}

static int stream_in_get_capture_position(const struct audio_stream_in *stream,
                                int64_t *frames, int64_t *time)
{
  TRACE_ENTRY();
  return client->stream_in_get_capture_position(stream, frames, time);
}

static uint32_t stream_out_get_latency(const struct audio_stream_out *stream)
{
  TRACE_ENTRY();
  return client->stream_out_get_latency(stream);
}

static int stream_out_set_volume(struct audio_stream_out *stream, float left, float right)
{
  TRACE_ENTRY();
  return client->stream_out_set_volume(stream, left, right);
}

static ssize_t stream_out_write(struct audio_stream_out *stream, const void* buffer,
                     size_t bytes)
{
  TRACE_ENTRY();
  return client->stream_out_write(stream, buffer, bytes);
}

static int stream_out_get_render_position(const struct audio_stream_out *stream,
                               uint32_t *dsp_frames)
{
  TRACE_ENTRY();
  return client->stream_out_get_render_position(stream, dsp_frames);
}

static int stream_out_get_next_write_timestamp(const struct audio_stream_out *stream,
                                    int64_t *timestamp)
{
  TRACE_ENTRY();
  return client->stream_out_get_next_write_timestamp(stream, timestamp);
}

static int stream_out_pause(struct audio_stream_out* stream)
{
  TRACE_ENTRY();
  return client->stream_out_pause(stream);
}

static int stream_out_resume(struct audio_stream_out* stream)
{
  TRACE_ENTRY();
  return client->stream_out_resume(stream);
}

static int stream_out_flush(struct audio_stream_out* stream)
{
  TRACE_ENTRY();
  return client->stream_out_flush(stream);
}

static int stream_out_get_presentation_position(const struct audio_stream_out *stream,
                               uint64_t *frames, struct timespec *timestamp)
{
  TRACE_ENTRY();
  return client->stream_out_get_presentation_position(stream, frames, timestamp);
}

struct audio_stream_in stream_in_template = {
  .common = {
    .get_sample_rate = stream_get_sample_rate,
    .set_sample_rate = NULL,
    .get_buffer_size = stream_get_buffer_size,
    .get_channels = stream_get_channels,
    .get_format = stream_get_format,
    .set_format = NULL,
    .standby = stream_standby,
    .dump = stream_dump,
    .get_device = stream_get_device,
    .set_device = NULL,
    .set_parameters = stream_set_parameters,
    .get_parameters = stream_get_parameters,
    .add_audio_effect = NULL,
    .remove_audio_effect = NULL
  },
  .set_gain = stream_in_set_gain,
  .read = stream_in_read,
  .get_input_frames_lost = stream_in_get_input_frames_lost,
  .get_capture_position = stream_in_get_capture_position,
  .start = NULL,
  .stop = NULL,
  .create_mmap_buffer = NULL,
  .get_mmap_position = NULL,
  .get_active_microphones = NULL,
  .update_sink_metadata = NULL
};

struct audio_stream_out stream_out_template = {
  .common = {
    .get_sample_rate = stream_get_sample_rate,
    .set_sample_rate = NULL,
    .get_buffer_size = stream_get_buffer_size,
    .get_channels = stream_get_channels,
    .get_format = stream_get_format,
    .set_format = NULL,
    .standby = stream_standby,
    .dump = stream_dump,
    .get_device = stream_get_device,
    .set_device = NULL,
    .set_parameters = stream_set_parameters,
    .get_parameters = stream_get_parameters,
    .add_audio_effect = NULL,
    .remove_audio_effect = NULL
  },
  .get_latency = stream_out_get_latency,
  .set_volume = stream_out_set_volume,
  .write = stream_out_write,
  .get_render_position = stream_out_get_render_position,
  .get_next_write_timestamp = stream_out_get_next_write_timestamp,
  .set_callback = NULL,
  .pause = stream_out_pause,
  .resume = stream_out_resume,
  .drain = NULL,
  .flush = stream_out_flush,
  .get_presentation_position = stream_out_get_presentation_position,
  .start = NULL,
  .stop = NULL,
  .create_mmap_buffer = NULL,
  .get_mmap_position = NULL,
  .update_source_metadata = NULL
};

static int Device_common_close(struct hw_device_t* device)
{
  // Server side audio hal device is constructed with
  // server's life span. Skip Device_common_close for client
  // The method is only needed when server really wants to exit.
  // return client->Device_common_close(device);
  return 0;
}

static int Device_init_check(const struct audio_hw_device *dev)
{
  TRACE_ENTRY();
  return client->Device_init_check(dev);
}

static int Device_set_voice_volume(struct audio_hw_device *dev, float volume)
{
  TRACE_ENTRY();
  return client->Device_set_voice_volume(dev, volume);
}

static int Device_set_master_volume(struct audio_hw_device *dev, float volume)
{
  TRACE_ENTRY();
  return client->Device_set_master_volume(dev, volume);
}

static int Device_get_master_volume(struct audio_hw_device *dev, float *volume)
{
  TRACE_ENTRY();
  return client->Device_get_master_volume(dev, volume);
}

static int Device_set_mode(struct audio_hw_device *dev, audio_mode_t mode)
{
  TRACE_ENTRY();
  return client->Device_set_mode(dev, mode);
}

static int Device_set_mic_mute(struct audio_hw_device *dev, bool state)
{
  TRACE_ENTRY();
  return client->Device_set_mic_mute(dev, state);
}

static int Device_get_mic_mute(const struct audio_hw_device *dev, bool *state)
{
  TRACE_ENTRY();
  return client->Device_get_mic_mute(dev, state);
}

static int Device_set_parameters(struct audio_hw_device *dev, const char *kv_pairs)
{
  TRACE_ENTRY();
  return client->Device_set_parameters(dev, kv_pairs);
}

static char * Device_get_parameters(const struct audio_hw_device *dev,
                             const char *keys)
{
  TRACE_ENTRY();
  return client->Device_get_parameters(dev, keys);
}

static size_t Device_get_input_buffer_size(const struct audio_hw_device *dev,
                                    const struct audio_config *config)
{
  TRACE_ENTRY();
  return client->Device_get_input_buffer_size(dev, config);
}

static int Device_open_output_stream(struct audio_hw_device *dev,
                              audio_io_handle_t handle,
                              audio_devices_t devices,
                              audio_output_flags_t flags,
                              struct audio_config *config,
                              struct audio_stream_out **stream_out,
                              const char *address)
{
  TRACE_ENTRY();

  int r;
  audio_stream_out_client_t *stream_out_client = (audio_stream_out_client_t *)calloc(1, sizeof(audio_stream_out_client_t));
  if (!stream_out_client) return -ENOMEM;

  r = client->Device_open_output_stream(dev, handle, devices, flags, config, stream_out_client, address);
  if (r) {
    free(stream_out);
    return r;
  }

  stream_out_client->stream_out = stream_out_template;
  *stream_out = &(stream_out_client->stream_out);
  return r;
}

static void Device_close_output_stream(struct audio_hw_device *dev,
                                struct audio_stream_out* stream_out)
{
  TRACE_ENTRY();

  client->Device_close_output_stream(dev, stream_out);
  free(audio_stream_out_to_client(stream_out));
}

static int Device_open_input_stream(struct audio_hw_device *dev,
                             audio_io_handle_t handle,
                             audio_devices_t devices,
                             struct audio_config *config,
                             struct audio_stream_in **stream_in,
                             audio_input_flags_t flags,
                             const char *address,
                             audio_source_t source)
{
  TRACE_ENTRY();

  int r;
  audio_stream_in_client_t *stream_in_client = (audio_stream_in_client_t *)calloc(1, sizeof(audio_stream_in_client_t));
  if (!stream_in_client) return -ENOMEM;

  r = client->Device_open_input_stream(dev, handle, devices, config, stream_in_client, flags, address, source);
  if (r) {
    free(stream_in_client);
    return r;
  }

  stream_in_client->stream_in = stream_in_template;
  *stream_in = &(stream_in_client->stream_in);
  return r;
}

static void Device_close_input_stream(struct audio_hw_device *dev,
                               struct audio_stream_in *stream_in)
{
  TRACE_ENTRY();

  client->Device_close_input_stream(dev, stream_in);
  free(audio_stream_in_to_client(stream_in));
}

static int Device_dump(const struct audio_hw_device *dev, int fd)
{
  TRACE_ENTRY();

  return client->Device_dump(dev, fd);
}

static int Device_set_master_mute(struct audio_hw_device *dev, bool mute)
{
  TRACE_ENTRY();

  return client->Device_set_master_mute(dev, mute);
}

static int Device_get_master_mute(struct audio_hw_device *dev, bool *mute)
{
  TRACE_ENTRY();

  return client->Device_get_master_mute(dev, mute);
}

static int Device_create_audio_patch(struct audio_hw_device *dev,
                               unsigned int num_sources,
                               const struct audio_port_config *sources,
                               unsigned int num_sinks,
                               const struct audio_port_config *sinks,
                               audio_patch_handle_t *handle)
{
  TRACE_ENTRY();

  return client->Device_create_audio_patch(dev, num_sources, sources, num_sinks, sinks, handle);
}

static int Device_release_audio_patch(struct audio_hw_device *dev,
                               audio_patch_handle_t handle)
{
  TRACE_ENTRY();

  return client->Device_release_audio_patch(dev, handle);
}

static int Device_set_audio_port_config(struct audio_hw_device *dev,
                         const struct audio_port_config *config)
{
  TRACE_ENTRY();

  return client->Device_set_audio_port_config(dev, config);
}

static struct hw_module_t hw_module = {
  .tag = HARDWARE_MODULE_TAG,
  .module_api_version = AUDIO_MODULE_API_VERSION_0_1,
  .hal_api_version = HARDWARE_HAL_API_VERSION,
  .id = AUDIO_HARDWARE_MODULE_ID,
  .name = "aml audio HW HAL",
  .author = "amlogic, Corp.",
};

static audio_hw_device_t device = {
  .common = {
    .tag = HARDWARE_DEVICE_TAG,
    .version = AUDIO_DEVICE_API_VERSION_3_0,
    .module = &hw_module,
    .reserved = {0},
    .close = Device_common_close,
  },
  .get_supported_devices = NULL,
  .init_check = Device_init_check,
  .set_voice_volume = Device_set_voice_volume,
  .set_master_volume = Device_set_master_volume,
  .get_master_volume = Device_get_master_volume,
  .set_mode = Device_set_mode,
  .set_mic_mute = Device_set_mic_mute,
  .get_mic_mute = Device_get_mic_mute,
  .set_parameters = Device_set_parameters,
  .get_parameters = Device_get_parameters,
  .get_input_buffer_size = Device_get_input_buffer_size,
  .open_output_stream = Device_open_output_stream,
  .close_output_stream = Device_close_output_stream,
  .open_input_stream = Device_open_input_stream,
  .close_input_stream = Device_close_input_stream,
  .get_microphones = NULL,
  .dump = Device_dump,
  .set_master_mute = Device_set_master_mute,
  .get_master_mute = Device_get_master_mute,
  .create_audio_patch = Device_create_audio_patch,
  .release_audio_patch = Device_release_audio_patch,
  .get_audio_port = NULL,
  .set_audio_port_config = Device_set_audio_port_config,
};

extern "C" {

int audio_hw_load_interface(audio_hw_device_t **dev)
{
  TRACE_ENTRY();
  printf("PID = %d, inited = %d\n", ::getpid(), inited);

  std::lock_guard<std::mutex> lock(client_mutex);

  if (inited++ > 0) {
    *dev = &device;
    return 0;
  }

  client = new AudioClient(
    grpc::CreateChannel("localhost:50051",
                        grpc::InsecureChannelCredentials()));

  *dev = &device;
  inited = 1;

  return 0;
}

void audio_hw_unload_interface(audio_hw_device_t *dev)
{
  TRACE_ENTRY();

  std::lock_guard<std::mutex> lock(client_mutex);

  if (--inited == 0) {
    delete client;
    client = nullptr;
  }
}

}
