#ifndef ALSA_VOLUME
#define ALSA_VOLUME
#include <stdio.h>
#include <stdlib.h>
#include <alsa/asoundlib.h>

int mixer_alloc(snd_mixer_t *mixerFd, char *card);
snd_mixer_elem_t *find_elem_by_idex(snd_mixer_t *mixerFd, unsigned int id);
snd_mixer_elem_t *find_elem_by_name(snd_mixer_t *mixerFd, char *name);
void volumeSet_by_elem(snd_mixer_elem_t *elem, unsigned int vol);
void volumeUp_by_elem(snd_mixer_elem_t *elem);
void volumeDown_by_elem(snd_mixer_elem_t *elem);
#endif
