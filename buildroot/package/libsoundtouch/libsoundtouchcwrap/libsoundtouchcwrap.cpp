#include <soundtouch/STTypes.h>
#include <soundtouch/SoundTouch.h>

#include "libsoundtouchcwrap.h"
using namespace soundtouch;

extern "C" {

#define SOUND_TOUCH_C_FUNC(f) sound_touch_##f

#define SOUND_TOUCH_CPP_TO_C_VOID_0(f)                                         \
  void SOUND_TOUCH_C_FUNC(f)(CSoundTouch * st) { ((SoundTouch *)st)->f(); }

#define SOUND_TOUCH_CPP_TO_C_VOID_1(f, t1)                                     \
  void SOUND_TOUCH_C_FUNC(f)(CSoundTouch * st, t1 _1) {                        \
    ((SoundTouch *)st)->f(_1);                                                 \
  }

#define SOUND_TOUCH_CPP_TO_C_VOID_2(f, t1, t2)                                 \
  void SOUND_TOUCH_C_FUNC(f)(CSoundTouch * st, t1 _1, t2 _2) {                 \
    ((SoundTouch *)st)->f(_1, _2);                                             \
  }

#define SOUND_TOUCH_CPP_TO_C_RET_0(f, ret)                                     \
  ret SOUND_TOUCH_C_FUNC(f)(CSoundTouch * st) {                                \
    return ((SoundTouch *)st)->f();                                            \
  }

#define SOUND_TOUCH_CPP_TO_C_RET_1(f, ret, t1)                                 \
  ret SOUND_TOUCH_C_FUNC(f)(CSoundTouch * st, t1 _1) {                         \
    return ((SoundTouch *)st)->f(_1);                                          \
  }

#define SOUND_TOUCH_CPP_TO_C_RET_2(f, ret, t1, t2)                             \
  ret SOUND_TOUCH_C_FUNC(f)(CSoundTouch * st, t1 _1, t2 _2) {                  \
    return ((SoundTouch *)st)->f(_1, _2);                                      \
  }

CSoundTouch *SOUND_TOUCH_C_FUNC(construct)(void) {
  return (CSoundTouch *)(new SoundTouch());
}

void SOUND_TOUCH_C_FUNC(destruct)(CSoundTouch *st) {
  delete ((SoundTouch *)st);
}

SOUND_TOUCH_CPP_TO_C_VOID_1(setRate, float)
SOUND_TOUCH_CPP_TO_C_VOID_1(setTempo, float)
SOUND_TOUCH_CPP_TO_C_VOID_1(setRateChange, float)
SOUND_TOUCH_CPP_TO_C_VOID_1(setTempoChange, float)
SOUND_TOUCH_CPP_TO_C_VOID_1(setPitch, float)
SOUND_TOUCH_CPP_TO_C_VOID_1(setPitchOctaves, float)
SOUND_TOUCH_CPP_TO_C_VOID_1(setPitchSemiTones, int)
SOUND_TOUCH_CPP_TO_C_VOID_1(setChannels, uint)
SOUND_TOUCH_CPP_TO_C_VOID_1(setSampleRate, uint)
SOUND_TOUCH_CPP_TO_C_RET_0(getInputOutputSampleRatio, double)
SOUND_TOUCH_CPP_TO_C_VOID_0(flush)
SOUND_TOUCH_CPP_TO_C_VOID_2(putSamples, const SAMPLETYPE *, uint)
SOUND_TOUCH_CPP_TO_C_RET_2(receiveSamples, uint, SAMPLETYPE *, uint)
SOUND_TOUCH_CPP_TO_C_VOID_0(clear)
SOUND_TOUCH_CPP_TO_C_RET_2(setSetting, int, int, int)
SOUND_TOUCH_CPP_TO_C_RET_1(getSetting, int, int)
SOUND_TOUCH_CPP_TO_C_RET_0(numUnprocessedSamples, uint)
SOUND_TOUCH_CPP_TO_C_RET_0(numSamples, uint)

CSoundTouchSymbols sound_touch_c_wrapper = {
    SOUND_TOUCH_C_FUNC(construct),
    SOUND_TOUCH_C_FUNC(destruct),
    SOUND_TOUCH_C_FUNC(setRate),
    SOUND_TOUCH_C_FUNC(setTempo),
    SOUND_TOUCH_C_FUNC(setRateChange),
    SOUND_TOUCH_C_FUNC(setTempoChange),
    SOUND_TOUCH_C_FUNC(setPitch),
    SOUND_TOUCH_C_FUNC(setPitchOctaves),
    SOUND_TOUCH_C_FUNC(setPitchSemiTones),
    SOUND_TOUCH_C_FUNC(setChannels),
    SOUND_TOUCH_C_FUNC(setSampleRate),
    SOUND_TOUCH_C_FUNC(getInputOutputSampleRatio),
    SOUND_TOUCH_C_FUNC(flush),
    SOUND_TOUCH_C_FUNC(putSamples),
    SOUND_TOUCH_C_FUNC(receiveSamples),
    SOUND_TOUCH_C_FUNC(clear),
    SOUND_TOUCH_C_FUNC(setSetting),
    SOUND_TOUCH_C_FUNC(getSetting),
    SOUND_TOUCH_C_FUNC(numUnprocessedSamples),
    SOUND_TOUCH_C_FUNC(numSamples)};
}

