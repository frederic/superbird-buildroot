/*
* Copyright (c) 2014 Amlogic, Inc. All rights reserved.
* *
This source code is subject to the terms and conditions defined in the
* file 'LICENSE' which is part of this source code package.
* *
Description:
*/


#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <system/audio.h>
#include <media/AudioSystem.h>
#include <media/AudioParameter.h>

#ifdef LOG
#undef LOG
#endif
#define LOG(x...) printf("[USB_BT] " x)

using namespace android;

struct usb_gain {
    float gain;
    int unmute;
};

struct bt_gain {
    float gain;
    int unmute;
};
/*
static inline float DbToAmpl(float decibels)
{
    if (decibels <= -758) {
        return 0.0f;
    }
    return exp( decibels * 0.115129f); // exp( dB * ln(10) / 20 )
}

static int set_btusb_gain(int id ,float gain)
{
    String8 keyValuePairs = String8("");
    String8 mString = String8("");
    usb_gain ug;
    bt_gain bg;
    char value[64] = {0};
    AudioParameter param = AudioParameter();

    audio_io_handle_t handle = AUDIO_IO_HANDLE_NONE;
    if (id == 0) {
    sscanf(mString.string(), "usb_gain = %f\n ", &ug.gain);
    gain = DbToAmpl(gain);
    ug.gain = gain;
    snprintf(value, 40, "%f", ug.gain);
    param.add(String8("USB_GAIN"), String8(value));
    keyValuePairs = param.toString();
    if (AudioSystem::setParameters(handle, keyValuePairs) != NO_ERROR) {
        LOG("setusbgain: Set gain failed\n");
        return -1;
    }
    param.remove(String8("USB_GAIN"));
  } else if (id == 1) {
     sscanf(mString.string(), "bt_gain = %f\n", &bg.gain);
     gain = DbToAmpl(gain);
     bg.gain = gain;
     snprintf(value, 40, "%f", bg.gain);
     param.add(String8("BT_GAIN"), String8(value));
     keyValuePairs = param.toString();
     if (AudioSystem::setParameters(handle, keyValuePairs) != NO_ERROR) {
        LOG("setbtgain: Set gain failed\n");
        return -1;
    }
    param.remove(String8("BT_GAIN"));
   } else {
      LOG( "unknown id = %d\n", id);
      return -1;
   }
   return 0;
 }
*/
static int set_btusb_mute(int id ,int unmute)
{
    String8 keyValuePairs = String8("");
    String8 mString = String8("");
    usb_gain ug;
    bt_gain bg;
    char value[64] = {0};
    AudioParameter param = AudioParameter();

    audio_io_handle_t handle = AUDIO_IO_HANDLE_NONE;
    if (id == 0) {
    sscanf(mString.string(), "usb_unmute = %d\n ", &ug.unmute);
    ug.unmute = unmute;
    snprintf(value, 40, "%d", ug.unmute);
    param.add(String8("USB_MUTE"), String8(value));
    keyValuePairs = param.toString();
    if (AudioSystem::setParameters(handle, keyValuePairs) != NO_ERROR) {
      LOG("setusbgain: Set unmute failed\n");
      return -1;
    }
    param.remove(String8("USB_MUTE"));
  } else if (id == 1) {
    sscanf(mString.string(), "bt_unmute = %d\n", &bg.unmute);
    bg.unmute = unmute;
    snprintf(value, 40, "%d", bg.unmute);
    param.add(String8("BT_MUTE"), String8(value));
    keyValuePairs = param.toString();
    if (AudioSystem::setParameters(handle, keyValuePairs) != NO_ERROR) {
      LOG("setbtgain: Set unmute failed\n");
      return -1;
    }
    param.remove(String8("BT_MUTE"));
   } else {
     LOG( "unknown id = %d\n", id);
     return -1;
   }
   return 0;
}


int main(int argc, char *argv[])
{
    if (argc != 3) {
        LOG("***************usb bt Gain*******************\n");
        LOG("Usage: %s <id> <gain> <unmute>\n", argv[0]);
        LOG("**************************************************\n\n");
        return -1;
    }
    LOG("*************set usb gain  PARAM***************\n");
    LOG("****id: 0   source***********************\n");
    LOG("**** 0 ->usb   1 -> bt***********************\n");
    LOG("****id: 1   gain *********************** \n");
    LOG("****value: 0 ~ 100\n");

    int id = 0;
    float gain = 1;
    sscanf(argv[1], "%d", &id);
    sscanf(argv[2], "%f", &gain);
    LOG("id = %d gain = %f\n", id,gain);
    //set_btusb_gain(id,gain);
     set_btusb_mute(id,gain);
    return 0;
}
