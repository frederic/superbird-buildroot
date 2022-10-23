#include <stdio.h>
#include <stdlib.h>
#include <alsa/asoundlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#define VOL_RANG_PRV_SET

#ifdef VOL_RANG_PRV_SET
#define MAX 190
#define MIN 100
#endif
int app_avk_is_pulse(void);
int mixer_alloc(snd_mixer_t **mixerFd, char *card)
{
	int result;

	if ((result = snd_mixer_open( mixerFd, 0)) < 0)
	{
		printf("snd_mixer_open error\n");
		*mixerFd = NULL;
		return 0;
	}
	if ((result = snd_mixer_attach( *mixerFd, card)) < 0)
	{
		printf("snd_mixer_attach error\n");
		snd_mixer_close(*mixerFd);
		*mixerFd = NULL;
		return 0;
	}
	if ((result = snd_mixer_selem_register( *mixerFd, NULL, NULL)) < 0)
	{
		printf("snd_mixer_selem_register error\n");
		snd_mixer_close(*mixerFd);
		*mixerFd = NULL;
		return 0;
	}
	if ((result = snd_mixer_load( *mixerFd)) < 0)
	{
		printf("snd_mixer_load error\n");
		snd_mixer_close(*mixerFd);
		*mixerFd = NULL;
		return 0;
	}

	return 1;
}

snd_mixer_elem_t *find_elem_by_name(snd_mixer_t *mixerFd, char *name)
{
	snd_mixer_elem_t * elem;
	if (mixerFd == NULL) {
		printf("invalid mixerFd\n");
		return 0;
	}

	for (elem = snd_mixer_first_elem(mixerFd); elem; elem = snd_mixer_elem_next(elem))
	{
		if (snd_mixer_elem_get_type(elem) == SND_MIXER_ELEM_SIMPLE &&
				snd_mixer_selem_is_active(elem))
		{
#if 0
			if (strcmp(snd_mixer_selem_get_name(elem), name) == 0)
				return elem;
#endif
			if (strstr(snd_mixer_selem_get_name(elem), "Master"))
				if (strstr(snd_mixer_selem_get_name(elem), "Fine") == 0)
					return elem;
		}
	}
	return 0;
}

long volumeGet_by_elem(snd_mixer_elem_t *elem)
{
	long min, max, vol;

	if (elem == NULL) {
		printf("invalid elem\n");
		return 0;

	}
#ifdef VOL_RANG_PRV_SET
	if (app_avk_is_pulse()) {
		snd_mixer_selem_get_playback_volume_range(elem, &min, &max);
	} else {
		max = MAX;
		min = MIN;
	}
#else
	snd_mixer_selem_get_playback_volume_range(elem, &min, &max);
#endif
	snd_mixer_selem_get_playback_volume(elem, SND_MIXER_SCHN_FRONT_LEFT, &vol);
	printf("volumeget min = %ld, max = %ld, vol = %ld\n", min, max, vol);

	/*use percentage*/
	vol = vol >  max ? max : vol;
	vol = vol <= min ? 0 : (vol - min) * 100 / (max - min);


	printf("current vol = %ld\n", vol);

	return vol;
}


void volumeSet_by_elem(snd_mixer_elem_t *elem, long vol)
{
	long min, max;

	if (elem == NULL) {
		printf("invalid elem\n");
		return ;
	}

#ifdef VOL_RANG_PRV_SET
	if (app_avk_is_pulse()) {
		snd_mixer_selem_get_playback_volume_range(elem, &min, &max);
	} else {
		max = MAX;
		min = MIN;
	}
#else
	snd_mixer_selem_get_playback_volume_range(elem, &min, &max);
#endif
	printf("volumeset min = %ld, max = %ld, vol = %ld\n", min, max, vol);

	/*use percentage*/
	vol = vol >  100 ? 100 : vol;
	vol = vol <= 0 ? min : vol * (max - min) / 100 + min;

	printf("volume actually set: %ld\n", vol);

	snd_mixer_selem_set_playback_volume_all(elem, vol);

}


void volumeUp_by_elem(snd_mixer_elem_t *elem)
{
	long min, max, vol;

	if (elem == NULL) {
		printf("invalid elem\n");
		return ;
	}

#ifdef VOL_RANG_PRV_SET
	if (app_avk_is_pulse()) {
		snd_mixer_selem_get_playback_volume_range(elem, &min, &max);
	} else {
		max = MAX;
		min = MIN;
	}
#else
	snd_mixer_selem_get_playback_volume_range(elem, &min, &max);
#endif
	snd_mixer_selem_get_playback_volume(elem, SND_MIXER_SCHN_FRONT_LEFT, &vol);
	//printf("volumeup min = %ld, max = %ld, vol = %ld\n", min, max, vol);

	if (app_avk_is_pulse()) {
		vol += max/10;
	} else
		vol += 10;

	if (vol > max)
		vol = max;

	if (vol < min)
		vol = min;
	snd_mixer_selem_set_playback_volume_all(elem, vol);
	/*use percentage*/
	if (vol != 0)
		vol = (vol - min) * 100 / (max - min);

	printf("current vol = %ld\n", vol);

}

void volumeDown_by_elem(snd_mixer_elem_t *elem)
{
	long min, max, vol;

	if (elem == NULL) {
		printf("invalid elem\n");
		return ;
	}

#ifdef VOL_RANG_PRV_SET
	if (app_avk_is_pulse()) {
		snd_mixer_selem_get_playback_volume_range(elem, &min, &max);
	} else {
		max = MAX;
		min = MIN;
	}
#else
	snd_mixer_selem_get_playback_volume_range(elem, &min, &max);
#endif
	snd_mixer_selem_get_playback_volume(elem, SND_MIXER_SCHN_FRONT_LEFT, &vol);
	//printf("volumedown min = %ld, max = %ld, vol = %ld\n", min, max, vol);
	if (app_avk_is_pulse())
		vol -= max/10;
	else
		vol -= 10;

	if (vol > max)
		vol = max;

	if (vol < min)
		vol = min;

	snd_mixer_selem_set_playback_volume_all(elem, vol);
	/*use percentage*/
	if (vol != 0)
		vol = (vol - min) * 100 / (max - min);

	printf("current vol = %ld\n", vol);

}

void mixer_dealloc(snd_mixer_t *mixerFd)
{
	snd_mixer_close(mixerFd);
}

