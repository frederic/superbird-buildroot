/*
 * Copyright (C) 2019 Amlogic Corporation.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <time.h>
#include <stdlib.h>

#ifndef __USE_GNU
/* Define this to avoid a warning about implicit definition of ppoll.*/
#define __USE_GNU
#endif
#include <poll.h>

#include "aml_audio_timer.h"


uint64_t aml_audio_get_systime(void)
{
    struct timespec ts;
    uint64_t sys_time;

    clock_gettime(CLOCK_MONOTONIC, &ts);

    sys_time = (uint64_t)ts.tv_sec * 1000000LL + (uint64_t)ts.tv_nsec / 1000LL;

    return sys_time;
}

uint64_t aml_audio_get_systime_ns(void)
{
    struct timespec ts;
    uint64_t sys_time;

    clock_gettime(CLOCK_MONOTONIC, &ts);

    sys_time = (uint64_t)ts.tv_sec * 1000*1000*1000LL + (uint64_t)ts.tv_nsec;

    return sys_time;
}


int aml_audio_sleep(uint64_t us)
{
    int ret = -1;
    struct timespec ts;
    if (us == 0) {
        return 0;
    }

    ts.tv_sec = (long)us / 1000000ULL;
    ts.tv_nsec = (long)(us - ts.tv_sec) * 1000;

    ret = ppoll(NULL, 0, &ts, NULL);
    return ret;
}




