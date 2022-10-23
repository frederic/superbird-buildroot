/**
 * @file "snd_convert.c"
 *
 * @brief convert 16/24/32bit, 1/2 channels to 16bit desgnated channels.
 */

#include<string.h>
/* Alsa Filter Type Plugin Header */
#include <alsa/asoundlib.h>
#include <alsa/pcm_external.h>



static inline void *snd_pcm_channel_area_addr(const snd_pcm_channel_area_t *area, snd_pcm_uframes_t offset)
{
    unsigned int bitofs = area->first + area->step * offset;
    assert(bitofs % 8 == 0);
    return (char *) area->addr + bitofs / 8;
}

int snd_pcm_area_convert(const snd_pcm_channel_area_t *dst_area, snd_pcm_uframes_t dst_offset,
                         const snd_pcm_channel_area_t *src_area, snd_pcm_uframes_t src_offset,
                         unsigned int samples, snd_pcm_format_t format)
{
    char *dst;
    char *src;
    int src_step, dst_step;
    src_step = src_area->step / 8;
    dst_step = dst_area->step / 8;

    if (src_area->step == 16 && dst_area->step == 16) {
        size_t bytes = samples * 16 / 8;
        samples -= bytes * 8 / 16;
        memcpy(dst, src, bytes);
        if (samples == 0)
            return 0;
    }
    src = snd_pcm_channel_area_addr(src_area, src_offset);
    dst = snd_pcm_channel_area_addr(dst_area, dst_offset);

    if (format == SND_PCM_FORMAT_S16) {
        while (samples-- > 0) {
            *(int16_t*)dst = *(int16_t*)src;
            src += src_step;
            dst += dst_step;
        }
    } else if (format == SND_PCM_FORMAT_S32) {
        src += 2;
        while (samples-- > 0) {
            *(int16_t*)dst = *(int16_t*)src;
            src += src_step;
            dst += dst_step;
        }
    } else if (format == SND_PCM_FORMAT_S24) {
        src += 1;
        while (samples-- > 0) {
            *(int16_t*)dst = *(int16_t*)src;
            src += src_step;
            dst += dst_step;
        }
    }
    return 0;
}

int snd_pcm_convert_to_s16(const int16_t *dst_buffer, int dst_step,
                           const snd_pcm_channel_area_t *src_area, snd_pcm_uframes_t src_offset,
                           unsigned int samples, snd_pcm_format_t format)
{
    char *dst;
    char *src;
    int src_step;
    src_step = src_area->step / 8;

    src = snd_pcm_channel_area_addr(src_area, src_offset);
    dst = dst_buffer;

    if (format == SND_PCM_FORMAT_S16) {
        while (samples-- > 0) {
            *(int16_t*)dst = *(int16_t*)src;
            src += src_step;
            dst += dst_step;
        }
    } else if (format == SND_PCM_FORMAT_S32) {
        src += 2;
        while (samples-- > 0) {
            *(int16_t*)dst = *(int16_t*)src;
            src += src_step;
            dst += dst_step;
        }
    } else if (format == SND_PCM_FORMAT_S24) {
        src += 1;
        while (samples-- > 0) {
            *(int16_t*)dst = *(int16_t*)src;
            src += src_step;
            dst += dst_step;
        }
    }
    return 0;
}

int snd_pcm_convert_to_area(const int16_t *src_buffer, int src_step,
                            const snd_pcm_channel_area_t *dst_area, snd_pcm_uframes_t dst_offset,
                            unsigned int samples, snd_pcm_format_t format)
{
    char *dst;
    char *src;
    int dst_step;
    dst_step = dst_area->step / 8;

    dst = snd_pcm_channel_area_addr(dst_area, dst_offset);
    src = src_buffer;

    if (format == SND_PCM_FORMAT_S16) {
        while (samples-- > 0) {
            *(int16_t*)dst = *(int16_t*)src;
            src += src_step;
            dst += dst_step;
        }
    } else if (format == SND_PCM_FORMAT_S32) {
        src += 2;
        while (samples-- > 0) {
            *(int16_t*)dst = *(int16_t*)src;
            src += src_step;
            dst += dst_step;
        }
    } else if (format == SND_PCM_FORMAT_S24) {
        src += 1;
        while (samples-- > 0) {
            *(int16_t*)dst = *(int16_t*)src;
            src += src_step;
            dst += dst_step;
        }
    }
    return 0;
}

snd_pcm_uframes_t
convert_process(const snd_pcm_channel_area_t *areas, snd_pcm_uframes_t offset,
                const snd_pcm_channel_area_t *slave_areas, snd_pcm_uframes_t slave_offset,
                int in_channels,
                int out_channels,
                snd_pcm_uframes_t size,
                snd_pcm_format_t format)
{
    unsigned int dst_channel;
    const snd_pcm_channel_area_t *dst_area;
    const snd_pcm_channel_area_t *src_area;
    dst_area = slave_areas;
    src_area = areas;

    for (dst_channel = 0; dst_channel < out_channels; dst_channel++) {
        if (dst_channel >= in_channels) {
            snd_pcm_area_silence(&dst_area[dst_channel], slave_offset, size, SND_PCM_FORMAT_S16);
        } else {
            snd_pcm_area_convert(&dst_area[dst_channel], slave_offset, &src_area[dst_channel], offset, size, format);
        }
    }
    return size;
}



snd_pcm_uframes_t
convert_to_s16(const snd_pcm_channel_area_t *areas, snd_pcm_uframes_t offset,
               int in_channels,
               snd_pcm_uframes_t size,
               snd_pcm_format_t format,
               int16_t *buffer)
{
    unsigned int dst_channel, dst_step;
    const snd_pcm_channel_area_t *src_area;
    src_area = areas;
    dst_step = 16 * in_channels / 8;

    for (dst_channel = 0; dst_channel < in_channels; dst_channel++) {
        snd_pcm_convert_to_s16(&buffer[dst_channel], dst_step, &src_area[dst_channel], offset, size, format);
    }
    return size;
}

snd_pcm_uframes_t
convert_to_area(const snd_pcm_channel_area_t *areas, snd_pcm_uframes_t offset,
                int out_channels,
                snd_pcm_uframes_t size,
                snd_pcm_format_t format,
                int16_t *buffer)
{
    unsigned int dst_channel, src_step;
    const snd_pcm_channel_area_t *dst_area;
    dst_area = areas;
    src_step = 16 * out_channels / 8;

    for (dst_channel = 0; dst_channel < out_channels; dst_channel++) {
        snd_pcm_convert_to_area(&buffer[dst_channel], src_step, &dst_area[dst_channel], offset, size, format);
    }
    return size;
}

