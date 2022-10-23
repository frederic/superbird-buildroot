/**
 * @file "snd_convert.c"
 *
 * @brief convert 16/24/32bit, 1/2 channels to 16bit desgnated channels.
 */



snd_pcm_uframes_t
convert_process(const snd_pcm_channel_area_t *areas, snd_pcm_uframes_t offset,
                const snd_pcm_channel_area_t *slave_areas, snd_pcm_uframes_t slave_offset,
                int channels,
                int slave_channels,
                snd_pcm_uframes_t size,
                snd_pcm_format_t format);

snd_pcm_uframes_t
convert_to_s16(const snd_pcm_channel_area_t *areas, snd_pcm_uframes_t offset,
               int in_channels,
               snd_pcm_uframes_t size,
               snd_pcm_format_t format,
               int16_t *buffer);

snd_pcm_uframes_t
convert_to_area(const snd_pcm_channel_area_t *areas, snd_pcm_uframes_t offset,
                int out_channels,
                snd_pcm_uframes_t size,
                snd_pcm_format_t format,
                int16_t *buffer);
