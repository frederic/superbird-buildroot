#ifndef _AUDIO_HW_DTV_H_
#define _AUDIO_HW_DTV_H_

enum {
    AUDIO_DTV_PATCH_DECODER_STATE_INIT,
    AUDIO_DTV_PATCH_DECODER_STATE_START,
    AUDIO_DTV_PATCH_DECODER_STATE_RUNING,
    AUDIO_DTV_PATCH_DECODER_STATE_PAUSE,
    AUDIO_DTV_PATCH_DECODER_STATE_RESUME,
    AUDIO_DTV_PATCH_DECODER_RELEASE,
};

enum {
    AUDIO_DTV_PATCH_CMD_NULL,
    AUDIO_DTV_PATCH_CMD_START,
    AUDIO_DTV_PATCH_CMD_STOP,
    AUDIO_DTV_PATCH_CMD_PAUSE,
    AUDIO_DTV_PATCH_CMD_RESUME,
    AUDIO_DTV_PATCH_CMD_NUM,
};

int create_dtv_patch(struct audio_hw_device *dev, audio_devices_t input, audio_devices_t output __unused);
int release_dtv_patch(struct aml_audio_device *dev);
int dtv_patch_add_cmd(int cmd);

#endif