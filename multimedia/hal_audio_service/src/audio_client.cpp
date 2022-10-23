#include <unistd.h>
#include <atomic>
#include <hardware/hardware.h>
#include <hardware/audio.h>

#include "CircularBuffer.h"
#include "audio_client.h"

#define LOG_TAG "audio_client"
#include <cutils/log.h>

//#define DEBUG__

#ifdef DEBUG__
#define TRACE_ENTRY() ALOGI("%s enter\n", __func__)
#define TRACE_EXIT() ALOGI("%s exit\n", __func__)
#else
#define TRACE_ENTRY()
#define TRACE_EXIT()
#endif

Volume MakeVolume(float vol)
{
  Volume v;
  v.set_vol(vol);
  return v;
}

Mode MakeMode(audio_mode_t mode)
{
  Mode m;
  m.set_mode(mode);
  return m;
}

Mute MakeMute(bool state)
{
  Mute m;
  m.set_mute(state);
  return m;
}

Kv_pairs MakeKv_pairs(const char *pairs)
{
  Kv_pairs p;
  p.set_params(std::string(pairs));
  return p;
}

Keys MakeKeys(const char *keys)
{
  Keys k;
  k.set_keys(std::string(keys));
  return k;
}

StreamReadWrite MakeStreamReadWrite(char *name, size_t bytes)
{
  StreamReadWrite rw;
  rw.set_name(std::string(name));
  rw.set_size(bytes);
  return rw;
}

AudioConfig MakeAudioConfig(const struct audio_config *config)
{
  AudioConfig c;
  c.set_sample_rate(config->sample_rate);
  c.set_channel_mask(config->channel_mask);
  c.set_format(config->format);
  c.set_frame_count(config->frame_count);
  return c;
}

Handle MakeHandle(int32_t handle)
{
  Handle h;
  h.set_handle(handle);
  return h;
}

StreamOutSetVolume MakeStreamOutSetVolume(char *name, float left, float right)
{
  StreamOutSetVolume v;
  v.set_name(std::string(name));
  v.set_left(left);
  v.set_right(right);
  return v;
}

OpenOutputStream MakeOpenOutputStream(std::string name,
                                        uint32_t size,
                                        uint32_t handle,
                                        uint32_t devices,
                                        uint32_t flags,
                                        const struct audio_config *config,
                                        const char *address)
{
  OpenOutputStream opt;
  opt.set_name(name);
  opt.set_size(size);
  opt.set_handle(handle);
  opt.set_devices(devices);
  opt.mutable_config()->CopyFrom(MakeAudioConfig(config));
  opt.set_flags(flags);
  opt.set_address(address ? std::string(address) : std::string(""));
  return opt;
}

OpenInputStream MakeOpenInputStream(std::string name,
                                     uint32_t size,
                                     uint32_t handle,
                                     uint32_t devices,
                                     const struct audio_config *config,
                                     uint32_t flags,
                                     const char *address,
                                     uint32_t source)
{
  OpenInputStream opt;
  opt.set_name(name);
  opt.set_size(size);
  opt.set_handle(handle);
  opt.set_devices(devices);
  opt.mutable_config()->CopyFrom(MakeAudioConfig(config));
  opt.set_flags(flags);
  opt.set_address(std::string(address));
  opt.set_source(source);
  return opt;
}

Stream MakeStream(std::string name)
{
  Stream stream;
  stream.set_name(name);
  return stream;
}

AudioPortConfig MakeAudioPortConfig(const struct audio_port_config *config)
{
  AudioPortConfig c;
  c.set_id(config->id);
  c.set_role(config->role);
  c.set_type(config->type);
  c.set_config_mask(config->config_mask);
  c.set_sample_rate(config->sample_rate);
  c.set_channel_mask(config->channel_mask);
  c.set_format(config->format);
  AudioGainConfig *gain = c.mutable_gain();
  gain->set_index(config->gain.index);
  gain->set_mode(config->gain.mode);
  gain->set_channel_mask(config->gain.channel_mask);
  for (int i = 0; i < sizeof(config->gain.values) / sizeof(int); i++) {
    gain->add_values(config->gain.values[i]);
  }
  gain->set_ramp_duration_ms(config->gain.ramp_duration_ms);
  if (config->type == AUDIO_PORT_TYPE_DEVICE) {
    c.mutable_device()->set_hw_module(config->ext.device.hw_module);
    c.mutable_device()->set_type(config->ext.device.type);
    c.mutable_device()->set_address(std::string("null"));
  } else if (config->type == AUDIO_PORT_TYPE_MIX) {
    c.mutable_mix()->set_hw_module(config->ext.mix.hw_module);
    c.mutable_mix()->set_handle(config->ext.mix.handle);
    c.mutable_mix()->set_stream_source(config->ext.mix.usecase.stream);
  } else if (config->type == AUDIO_PORT_TYPE_SESSION) {
    c.mutable_session()->set_session(config->ext.session.session);
  }
  return c;
}

StreamSetParameters MakeStreamSetParameters(char *name, const char *kv_pairs)
{
  StreamSetParameters p;
  p.set_name(std::string(name));
  p.set_kv_pairs(std::string(kv_pairs));
  return p;
}

StreamGetParameters MakeStreamGetParameters(char *name, const char *keys)
{
  StreamGetParameters p;
  p.set_name(std::string(name));
  p.set_keys(std::string(keys));
  return p;
}

std::atomic_int AudioClient::stream_seq_;

int AudioClient::Device_common_close(struct hw_device_t* device)
{
  TRACE_ENTRY();
  ClientContext context;
  StatusReturn r;
  Status status = stub_->Device_common_close(&context, Empty(), &r);
  return r.ret();
}

int AudioClient::Device_init_check(const struct audio_hw_device *dev)
{
  TRACE_ENTRY();
  ClientContext context;
  StatusReturn r;
  Status status = stub_->Device_init_check(&context, Empty(), &r);
  return r.ret();
}

int AudioClient::Device_set_voice_volume(struct audio_hw_device *dev, float volume)
{
  TRACE_ENTRY();
  ClientContext context;
  StatusReturn r;
  Status status = stub_->Device_set_voice_volume(&context, MakeVolume(volume), &r);
  return r.ret();
}

int AudioClient::Device_set_master_volume(struct audio_hw_device *dev, float volume)
{
  TRACE_ENTRY();
  ClientContext context;
  StatusReturn r;
  Status status = stub_->Device_set_master_volume(&context, MakeVolume(volume), &r);
  return r.ret();
}

int AudioClient::Device_get_master_volume(struct audio_hw_device *dev, float *volume)
{
  TRACE_ENTRY();
  ClientContext context;
  StatusReturn r;
  Status status = stub_->Device_get_master_volume(&context, Empty(), &r);
  *volume = r.status_float();
  return r.ret();
}

int AudioClient::Device_set_mode(struct audio_hw_device *dev, audio_mode_t mode)
{
  TRACE_ENTRY();
  ClientContext context;
  StatusReturn r;
  Status status = stub_->Device_set_mode(&context, MakeMode(mode), &r);
  return r.ret();
}

int AudioClient::Device_set_mic_mute(struct audio_hw_device *dev, bool state)
{
  TRACE_ENTRY();
  ClientContext context;
  StatusReturn r;
  Status status = stub_->Device_set_mic_mute(&context, MakeMute(state), &r);
  return r.ret();
}

int AudioClient::Device_get_mic_mute(const struct audio_hw_device *dev, bool *state)
{
  TRACE_ENTRY();
  ClientContext context;
  StatusReturn r;
  Status status = stub_->Device_get_mic_mute(&context, Empty(), &r);
  *state = r.status_bool();
  return r.ret();
}

int AudioClient::Device_set_parameters(struct audio_hw_device *dev, const char *kv_pairs)
{
  TRACE_ENTRY();
  ClientContext context;
  StatusReturn r;
  Status status = stub_->Device_set_parameters(&context, MakeKv_pairs(kv_pairs), &r);
  return r.ret();
}

char * AudioClient::Device_get_parameters(const struct audio_hw_device *dev, const char *keys)
{
  TRACE_ENTRY();
  ClientContext context;
  StatusReturn r;
  Status status = stub_->Device_get_parameters(&context, MakeKeys(keys), &r);
  char *p = (char *)malloc(r.status_string().size() + 1);
  if (p) {
    strcpy(p, r.status_string().c_str());
  }
  return p;
}

size_t AudioClient::Device_get_input_buffer_size(const struct audio_hw_device *dev,
                                const struct audio_config *config)
{
  TRACE_ENTRY();
  ClientContext context;
  StatusReturn r;
  Status status = stub_->Device_get_input_buffer_size(&context, MakeAudioConfig(config), &r);
  return r.ret();
}

int AudioClient::Device_open_output_stream(struct audio_hw_device *dev,
                          audio_io_handle_t handle,
                          audio_devices_t devices,
                          audio_output_flags_t flags,
                          struct audio_config *config,
                          audio_stream_out_client_t *stream_out,
                          const char *address)
{
  TRACE_ENTRY();
  ClientContext context;
  StatusReturn r;
  Status status = stub_->Device_open_output_stream(&context,
      MakeOpenOutputStream(new_stream_name(stream_out->name, sizeof(stream_out->name)),
                      kSharedBufferSize,
                      handle,
                      devices,
                      flags,
                      config,
                      address), &r);
  return r.ret();
}

void AudioClient::Device_close_output_stream(struct audio_hw_device *dev,
                            struct audio_stream_out* stream_out)
{
  TRACE_ENTRY();
  struct audio_stream_out_client *out = audio_stream_out_to_client(stream_out);
  ClientContext context;
  StatusReturn r;
  Status status = stub_->Device_close_output_stream(&context, MakeStream(std::string(out->name)), &r);
  return;
}

int AudioClient::Device_open_input_stream(struct audio_hw_device *dev,
                         audio_io_handle_t handle,
                         audio_devices_t devices,
                         struct audio_config *config,
                         struct audio_stream_in_client *stream_in,
                         audio_input_flags_t flags,
                         const char *address,
                         audio_source_t source)
{
  TRACE_ENTRY();
  ClientContext context;
  StatusReturn r;
  Status status = stub_->Device_open_input_stream(&context,
      MakeOpenInputStream(new_stream_name(stream_in->name, sizeof(stream_in->name)),
                      kSharedBufferSize,
                      handle,
                      devices,
                      config,
                      flags,
                      address,
                      source), &r);
  return r.ret();
}

void AudioClient::Device_close_input_stream(struct audio_hw_device *dev,
                           struct audio_stream_in *stream_in)
{
  TRACE_ENTRY();
  struct audio_stream_in_client *in = audio_stream_in_to_client(stream_in);
  ClientContext context;
  StatusReturn r;
  Status status = stub_->Device_close_input_stream(&context, MakeStream(std::string(in->name)), &r);
}

int AudioClient::Device_dump(const struct audio_hw_device *dev, int fd)
{
  TRACE_ENTRY();
  ClientContext context;
  StatusReturn r;
  Status status = stub_->Device_dump(&context, Empty(), &r);
  write(fd, r.status_string().c_str(), r.status_string().size());
  return r.ret();
}

int AudioClient::Device_set_master_mute(struct audio_hw_device *dev, bool mute)
{
  TRACE_ENTRY();
  ClientContext context;
  StatusReturn r;
  Status status = stub_->Device_set_master_mute(&context, MakeMute(mute), &r);
  return r.ret();
}

int AudioClient::Device_get_master_mute(struct audio_hw_device *dev, bool *mute)
{
  TRACE_ENTRY();
  ClientContext context;
  StatusReturn r;
  Status status = stub_->Device_get_master_mute(&context, Empty(), &r);
  *mute = r.status_bool();
  return r.ret();
}

int AudioClient::Device_create_audio_patch(struct audio_hw_device *dev,
                           unsigned int num_sources,
                           const struct audio_port_config *sources,
                           unsigned int num_sinks,
                           const struct audio_port_config *sinks,
                           audio_patch_handle_t *handle)
{
  TRACE_ENTRY();
  ClientContext context;
  StatusReturn r;
  CreateAudioPatch request;
  const struct audio_port_config *c;

  c = sources;
  for (int i = 0; i < num_sources; i++, c++) {
    AudioPortConfig *config = request.add_sources();
    config->CopyFrom(MakeAudioPortConfig(c));
  }

  c = sinks;
  for (int i = 0; i < num_sinks; i++, c++) {
    AudioPortConfig *config = request.add_sinks();
    config->CopyFrom(MakeAudioPortConfig(c));
  }

  Status status = stub_->Device_create_audio_patch(&context, request, &r);
  *handle = r.status_32();
  return r.ret();
}

int AudioClient::Device_release_audio_patch(struct audio_hw_device *dev,
                           audio_patch_handle_t handle)
{
  TRACE_ENTRY();
  ClientContext context;
  StatusReturn r;
  Status status = stub_->Device_release_audio_patch(&context, MakeHandle(handle), &r);
  return r.ret();
}

int AudioClient::Device_set_audio_port_config(struct audio_hw_device *dev,
                     const struct audio_port_config *config)
{
  TRACE_ENTRY();
  ClientContext context;
  StatusReturn r;
  Status status = stub_->Device_set_audio_port_config(&context, MakeAudioPortConfig(config), &r);
  return r.ret();
}

int AudioClient::stream_in_set_gain(struct audio_stream_in *stream, float gain)
{
  TRACE_ENTRY();
  ClientContext context;
  StatusReturn r;
  StreamGain request;
  char *name = (audio_stream_in_to_client(stream))->name;
  request.set_name(std::string(name));
  request.set_gain(gain);
  Status status = stub_->StreamIn_set_gain(&context, request, &r);
  return r.ret();
}

ssize_t AudioClient::stream_in_read(struct audio_stream_in *stream,
                                    void* buffer,
                                    size_t bytes)
{
  TRACE_ENTRY();
  ClientContext context;
  StatusReturn r;
  char *name = (audio_stream_in_to_client(stream))->name;
  Status status = stub_->StreamIn_read(&context, MakeStreamReadWrite(name, bytes), &r);
  if (r.ret() > 0) {
    CircularBuffer *cb = shm_->find<CircularBuffer>(name).first;
    if (cb) {
      memcpy(buffer, cb->start_ptr(*shm_), r.ret());
    }
  }
  return r.ret();
}

uint32_t AudioClient::stream_in_get_input_frames_lost(struct audio_stream_in *stream)
{
  TRACE_ENTRY();
  ClientContext context;
  StatusReturn r;
  char *name = (audio_stream_in_to_client(stream))->name;
  Status status = stub_->StreamIn_get_input_frames_lost(&context, MakeStream(name), &r);
  return r.ret();
}

int AudioClient::stream_in_get_capture_position(const struct audio_stream_in *stream,
                                       int64_t *frames, int64_t *time)
{
  TRACE_ENTRY();
  ClientContext context;
  GetCapturePositionReturn r;
  char *name = (audio_stream_in_to_client(stream))->name;
  Status status = stub_->StreamIn_get_capture_position(&context, MakeStream(name), &r);
  *frames = r.frames();
  *time = r.time();
  return r.ret();
}

uint32_t AudioClient::stream_out_get_latency(const struct audio_stream_out *stream)
{
  TRACE_ENTRY();
  ClientContext context;
  StatusReturn r;
  char *name = (audio_stream_out_to_client(stream))->name;
  Status status = stub_->StreamOut_get_latency(&context, MakeStream(name), &r);
  return r.ret();
}

int AudioClient::stream_out_set_volume(struct audio_stream_out *stream, float left, float right)
{
  TRACE_ENTRY();
  ClientContext context;
  StatusReturn r;
  char *name = (audio_stream_out_to_client(stream))->name;
  Status status = stub_->StreamOut_set_volume(&context, MakeStreamOutSetVolume(name, left, right), &r);
  return r.ret();
}

ssize_t AudioClient::stream_out_write(struct audio_stream_out *stream, const void* buffer,
                                      size_t bytes)
{
  TRACE_ENTRY();
  ClientContext context;
  StatusReturn r;
  char *name = (audio_stream_out_to_client(stream))->name;
  CircularBuffer *cb = shm_->find<CircularBuffer>(name).first;
  if (cb) {
    memcpy(cb->start_ptr(*shm_), buffer, std::min(bytes, cb->capacity()));
  }
  Status status = stub_->StreamOut_write(&context, MakeStreamReadWrite(name, bytes), &r);
  return r.ret();
}

int AudioClient::stream_out_get_render_position(const struct audio_stream_out *stream,
                                                uint32_t *dsp_frames)
{
  TRACE_ENTRY();
  ClientContext context;
  StatusReturn r;
  char *name = (audio_stream_out_to_client(stream))->name;
  Status status = stub_->StreamOut_get_render_position(&context, MakeStream(name), &r);
  *dsp_frames = r.status_32();
  return r.ret();
}

int AudioClient::stream_out_get_next_write_timestamp(const struct audio_stream_out *stream,
                                                     int64_t *timestamp)
{
  TRACE_ENTRY();
  ClientContext context;
  StatusReturn r;
  char *name = (audio_stream_out_to_client(stream))->name;
  Status status = stub_->StreamOut_get_next_write_timestamp(&context, MakeStream(name), &r);
  *timestamp = r.status_64();
  return r.ret();
}

int AudioClient::stream_out_pause(struct audio_stream_out* stream)
{
  TRACE_ENTRY();
  ClientContext context;
  StatusReturn r;
  char *name = (audio_stream_out_to_client(stream))->name;
  Status status = stub_->StreamOut_pause(&context, MakeStream(name), &r);
  return r.ret();
}

int AudioClient::stream_out_resume(struct audio_stream_out* stream)
{
  TRACE_ENTRY();
  ClientContext context;
  StatusReturn r;
  char *name = (audio_stream_out_to_client(stream))->name;
  Status status = stub_->StreamOut_resume(&context, MakeStream(name), &r);
  return r.ret();
}

int AudioClient::stream_out_flush(struct audio_stream_out* stream)
{
  TRACE_ENTRY();
  ClientContext context;
  StatusReturn r;
  char *name = (audio_stream_out_to_client(stream))->name;
  Status status = stub_->StreamOut_flush(&context, MakeStream(name), &r);
  return r.ret();
}

int AudioClient::stream_out_get_presentation_position(const struct audio_stream_out *stream,
                                                      uint64_t *frames,
                                                      struct timespec *timestamp)
{
  TRACE_ENTRY();
  ClientContext context;
  GetFrameTimestampReturn r;
  char *name = (audio_stream_out_to_client(stream))->name;
  Status status = stub_->StreamOut_get_presentation_position(&context, MakeStream(name), &r);
  *frames = r.frames();
  timestamp->tv_sec = r.timestamp().seconds();
  timestamp->tv_nsec = r.timestamp().nanos();
  return r.ret();
}

uint32_t AudioClient::stream_get_sample_rate(const struct audio_stream *stream)
{
  TRACE_ENTRY();
  ClientContext context;
  StatusReturn r;
  char *name = (audio_stream_to_client(stream))->name;
  Status status = stub_->Stream_get_sample_rate(&context, MakeStream(name), &r);
  return r.ret();
}

size_t AudioClient::stream_get_buffer_size(const struct audio_stream *stream)
{
  TRACE_ENTRY();
  ClientContext context;
  StatusReturn r;
  char *name = (audio_stream_to_client(stream))->name;
  Status status = stub_->Stream_get_buffer_size(&context, MakeStream(name), &r);
  return r.ret();
}

audio_channel_mask_t AudioClient::stream_get_channels(const struct audio_stream *stream)
{
  TRACE_ENTRY();
  ClientContext context;
  StatusReturn r;
  char *name = (audio_stream_to_client(stream))->name;
  Status status = stub_->Stream_get_channels(&context, MakeStream(name), &r);
  return r.ret();
}

audio_format_t AudioClient::stream_get_format(const struct audio_stream *stream)
{
  TRACE_ENTRY();
  ClientContext context;
  StatusReturn r;
  char *name = (audio_stream_to_client(stream))->name;
  Status status = stub_->Stream_get_format(&context, MakeStream(name), &r);
  return (audio_format_t)(r.ret());
}

int AudioClient::stream_standby(struct audio_stream *stream)
{
  TRACE_ENTRY();
  ClientContext context;
  StatusReturn r;
  char *name = (audio_stream_to_client(stream))->name;
  Status status = stub_->Stream_standby(&context, MakeStream(name), &r);
  return r.ret();
}

int AudioClient::stream_dump(const struct audio_stream *stream, int fd)
{
  TRACE_ENTRY();
  //TODO
  return 0;
}

audio_devices_t AudioClient::stream_get_device(const struct audio_stream *stream)
{
  TRACE_ENTRY();
  ClientContext context;
  StatusReturn r;
  char *name = (audio_stream_to_client(stream))->name;
  Status status = stub_->Stream_get_device(&context, MakeStream(name), &r);
  return r.ret();
}

int AudioClient::stream_set_parameters(struct audio_stream *stream, const char *kv_pairs)
{
  TRACE_ENTRY();
  ClientContext context;
  StatusReturn r;
  char *name = (audio_stream_to_client(stream))->name;
  Status status = stub_->Stream_set_parameters(&context, MakeStreamSetParameters(name, kv_pairs), &r);
  return r.ret();
}

char * AudioClient::stream_get_parameters(const struct audio_stream *stream,
                                          const char *keys)
{
  TRACE_ENTRY();
  ClientContext context;
  StatusReturn r;
  char *name = (audio_stream_to_client(stream))->name;
  Status status = stub_->Stream_get_parameters(&context, MakeStreamGetParameters(name, keys), &r);
  char *p = (char *)malloc(r.status_string().size() + 1);
  if (p) {
    strcpy(p, r.status_string().c_str());
  }
  return p;
}
