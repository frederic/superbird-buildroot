/*
 * Copyright (C) 2014-2019 Amlogic, Inc. All rights reserved.
 *
 * All information contained herein is Amlogic confidential.
 *
 * This software is provided to you pursuant to Software License Agreement
 * (SLA) with Amlogic Inc ("Amlogic"). This software may be used
 * only in accordance with the terms of this agreement.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification is strictly prohibited without prior written permission from
 * Amlogic.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef LIBSOUNDTOUCHCWRAP_H

#define LIBSOUNDTOUCHCWRAP_H

#ifdef __cplusplus
extern "C" {
#endif

typedef void * CSoundTouch;
typedef short SAMPLETYPE;
typedef long LONG_SAMPLETYPE;

typedef struct {
  CSoundTouch *(*construct)(void);
  void (*destruct)(CSoundTouch *st);
  /// Sets new rate control value. Normal rate = 1.0, smaller values
  /// represent slower rate, larger faster rates.
  void (*setRate)(CSoundTouch *st, float newRate);

  /// Sets new tempo control value. Normal tempo = 1.0, smaller values
  /// represent slower tempo, larger faster tempo.
  void (*setTempo)(CSoundTouch *st, float newTempo);

  /// Sets new rate control value as a difference in percents compared
  /// to the original rate (-50 .. +100 %)
  void (*setRateChange)(CSoundTouch *st, float newRate);

  /// Sets new tempo control value as a difference in percents compared
  /// to the original tempo (-50 .. +100 %)
  void (*setTempoChange)(CSoundTouch *st, float newTempo);

  /// Sets new pitch control value. Original pitch = 1.0, smaller values
  /// represent lower pitches, larger values higher pitch.
  void (*setPitch)(CSoundTouch *st, float newPitch);

  /// Sets pitch change in octaves compared to the original pitch
  /// (-1.00 .. +1.00)
  void (*setPitchOctaves)(CSoundTouch *st, float newPitch);

  /// Sets pitch change in semi-tones compared to the original pitch
  /// (-12 .. +12)
  void (*setPitchSemiTones)(CSoundTouch *st, int newPitch);

  /// Sets the number of channels, 1 = mono, 2 = stereo
  void (*setChannels)(CSoundTouch *st, uint numChannels);

  /// Sets sample rate.
  void (*setSampleRate)(CSoundTouch *st, uint srate);

  /// Get ratio between input and output audio durations, useful for calculating
  /// processed output duration: if you'll process a stream of N samples, then
  /// you can expect to get out N * getInputOutputSampleRatio() samples.
  ///
  /// This ratio will give accurate target duration ratio for a full audio
  /// track, given that the the whole track is processed with same processing
  /// parameters.
  ///
  /// If this ratio is applied to calculate intermediate offsets inside a
  /// processing stream, then this ratio is approximate and can deviate +- some
  /// tens of milliseconds from ideal offset, yet by end of the audio stream the
  /// duration ratio will become exact.
  ///
  /// Example: if processing with parameters "-tempo=15 -pitch=-3", the function
  /// will return value 0.8695652... Now, if processing an audio stream whose
  /// duration is exactly one million audio samples, then you can expect the
  /// processed output duration  be 0.869565 * 1000000 = 869565 samples.
  double (*getInputOutputSampleRatio)(CSoundTouch *st);

  /// Flushes the last samples from the processing pipeline to the output.
  /// Clears also the internal processing buffers.
  //
  /// Note: This function is meant for extracting the last samples of a sound
  /// stream. This function may introduce additional blank samples in the end
  /// of the sound stream, and thus it's not recommended to call this function
  /// in the middle of a sound stream.
  void (*flush)(CSoundTouch *st);

  /// Adds 'numSamples' pcs of samples from the 'samples' memory position into
  /// the input of the object. Notice that sample rate _has_to_ be set before
  /// calling this function, otherwise throws a runtime_error exception.
  void (*putSamples)(
      CSoundTouch *st,
      const SAMPLETYPE *samples, ///< Pointer to sample buffer.
      uint numSamples            ///< Number of samples in buffer. Notice
                      ///< that in case of stereo-sound a single sample
                      ///< contains data for both channels.
  );

  /// Output samples from beginning of the sample buffer. Copies requested
  /// samples to output buffer and removes them from the sample buffer. If there
  /// are less than 'numsample' samples in the buffer, returns all that
  /// available.
  ///
  /// \return Number of samples returned.
  uint (*receiveSamples)(
      CSoundTouch *st,
      SAMPLETYPE *output, ///< Buffer where to copy output samples.
      uint maxSamples     ///< How many samples to receive at max.
  );

  /// Clears all the samples in the object's output and internal processing
  /// buffers.
  void (*clear)(CSoundTouch *st);

  /// Changes a setting controlling the processing system behaviour. See the
  /// 'SETTING_...' defines for available setting ID's.
  ///
  /// \return 'TRUE' if the setting was succesfully changed
  int (*setSetting)(
      CSoundTouch *st,
      int settingId, ///< Setting ID number. see SETTING_... defines.
      int value      ///< New setting value.
  );

  /// Reads a setting controlling the processing system behaviour. See the
  /// 'SETTING_...' defines for available setting ID's.
  ///
  /// \return the setting value.
  int (*getSetting)(
      CSoundTouch *st,
      int settingId ///< Setting ID number, see SETTING_... defines.
  );

  /// Returns number of samples currently unprocessed.
  uint (*numUnprocessedSamples)(CSoundTouch *st);

  /// Returns number of samples currently available.
  uint (*numSamples)(CSoundTouch * st);

} CSoundTouchSymbols;

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* end of include guard: LIBSOUNDTOUCHCWRAP_H */
